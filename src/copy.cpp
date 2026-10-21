// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// Ordinary Win32 file operations following core worker patterns: new copies,
// independent recovery copies and explicitly selected existing-file replacements.
#include "precomp.h"
#include "copy.h"
#include "recovery.h"
#include "overwrite.h"
#include "delete.h"
#include "sync.h"
#include <aclapi.h>
#include <objbase.h>
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <cstdio>
#include <limits>

namespace dirsync {
void CopyProgress::ResetControls() noexcept
{
    errors={};awaitingDecision=false;errorSite=ErrorSite::Source;
    pauseRequested=false;paused=false;bytesPerSecond=0;pausedMilliseconds=0;completedWork=0;completedCopyBytes=0;
    throttleTick=GetTickCount64();throttleCopied=0;throttleRate=0;throttleDebt=0;
}
void CopyProgress::CompleteDisplay()
{
    std::lock_guard<std::mutex> lock(displayMutex);
    if(display.finished) return;
    const bool copying=!display.directory && (display.kind==TransferKind::Copy || display.kind==TransferKind::Overwrite || display.kind==TransferKind::Save);
    const auto maximum=(std::numeric_limits<uint64_t>::max)();
    const auto work=copying?std::max(uint64_t(4096),display.total>maximum/3?maximum:display.total*3):uint64_t(4096);
    auto add=[&](std::atomic<uint64_t>& counter,uint64_t amount) {
        auto current=counter.load(std::memory_order_relaxed);
        while(!counter.compare_exchange_weak(current,amount>maximum-current?maximum:current+amount,std::memory_order_relaxed)) {}
    };
    add(completedWork,work);if(copying) add(completedCopyBytes,display.total);
    ++completed;display.finished=true;
}
bool CopyProgress::Checkpoint(const std::atomic<bool>& cancel) noexcept
{
    uint64_t tick=GetTickCount64();
    while(!cancel.load(std::memory_order_relaxed) && pauseRequested.load(std::memory_order_relaxed)) {
        paused=true;
        Sleep(25);
        const auto now=GetTickCount64();pausedMilliseconds.fetch_add(now-tick,std::memory_order_relaxed);tick=now;
    }
    paused=false;
    return !cancel.load(std::memory_order_relaxed);
}
bool CopyProgress::Throttle(const std::atomic<bool>& cancel,uint64_t cumulativeCopied) noexcept
{
    if(!Checkpoint(cancel)) return false;
    auto tick=GetTickCount64()-pausedMilliseconds.load(std::memory_order_relaxed);
    if(!throttleTick || cumulativeCopied<throttleCopied) {
        throttleTick=tick;throttleCopied=0;throttleRate=bytesPerSecond.load(std::memory_order_relaxed);throttleDebt=0;
    }
    bool changed=false;
    auto account=[&](uint64_t now) {
        if(throttleRate) throttleDebt-=double(now-throttleTick)*double(throttleRate)/1000.0;
        throttleTick=now;const auto next=bytesPerSecond.load(std::memory_order_relaxed);
        changed=next!=throttleRate;if(changed)throttleDebt=0;throttleRate=next;
        if(!throttleRate) throttleDebt=0;
        // Carry timer oversleep forward so small copy blocks do not turn
        // e.g. a 50 MiB/s setting into one block per system timer tick. Bound
        // idle credit to 50 ms; verification/other work cannot buy a large burst.
        else throttleDebt=std::max(-double(throttleRate)/20.0,throttleDebt);
    };
    account(tick);
    // A limit reduced during a write cannot undo bytes already written. Like
    // the core limiter, cap this transition's braking to one second; the next
    // read uses the new, smaller block size instead of accumulating minutes.
    if(throttleRate) throttleDebt+=double(changed?std::min(cumulativeCopied-throttleCopied,throttleRate):cumulativeCopied-throttleCopied);
    throttleCopied=cumulativeCopied;
    while(throttleRate && throttleDebt>0) {
        const auto delay=DWORD(std::min(25.0,std::max(1.0,throttleDebt*1000.0/double(throttleRate))));
        Sleep(delay);
        if(!Checkpoint(cancel)) return false;
        account(GetTickCount64()-pausedMilliseconds.load(std::memory_order_relaxed));
    }
    return !cancel.load(std::memory_order_relaxed);
}
namespace {
struct Failure { DWORD error; };
void Need(BOOL ok) { if (!ok) throw Failure{GetLastError()}; }
void Stop(const std::atomic<bool>& cancel) { if (cancel) throw Failure{ERROR_CANCELLED}; }
void Control(const std::atomic<bool>& cancel,CopyProgress& progress) {if(!progress.Checkpoint(cancel)) throw Failure{ERROR_CANCELLED};}
struct File {
    HANDLE h = INVALID_HANDLE_VALUE;
    explicit File(HANDLE value = INVALID_HANDLE_VALUE) : h(value) {}
    ~File() { Close(); }
    void Close() { if (h != INVALID_HANDLE_VALUE) { CloseHandle(h); h = INVALID_HANDLE_VALUE; } }
    File(const File&) = delete; File& operator=(const File&) = delete;
};
constexpr DWORD shares = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
constexpr DWORD selectedAttrs = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
bool Unsupported(DWORD a) { return (a & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_ENCRYPTED |
    FILE_ATTRIBUTE_OFFLINE | 0x00040000 | 0x00400000)) != 0; }
HANDLE Open(const std::wstring& p, DWORD access, DWORD share = shares, DWORD disposition = OPEN_EXISTING)
{ return CreateFileW(ApiPath(p).c_str(), access, share, nullptr, disposition,
    FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr); }
Entry Info(HANDLE h)
{
    BY_HANDLE_FILE_INFORMATION i{}; FILE_BASIC_INFO b{};
    Need(GetFileInformationByHandle(h, &i)); Need(GetFileInformationByHandleEx(h, FileBasicInfo, &b, sizeof(b)));
    Entry e; e.identified = true; e.volume = i.dwVolumeSerialNumber;
    e.id = (uint64_t(i.nFileIndexHigh) << 32) | i.nFileIndexLow;
    e.size = (uint64_t(i.nFileSizeHigh) << 32) | i.nFileSizeLow;
    e.time = (uint64_t(i.ftLastWriteTime.dwHighDateTime) << 32) | i.ftLastWriteTime.dwLowDateTime;
    e.change = uint64_t(b.ChangeTime.QuadPart); e.attributes = i.dwFileAttributes;
    e.directory = (i.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; e.links = i.nNumberOfLinks; return e;
}
bool SameId(const Entry& a, const Entry& b) { return a.identified && b.identified && a.volume == b.volume && a.id == b.id; }
bool Unchanged(const Entry& a, const Entry& b) { return SameId(a,b) && a.size == b.size && a.time == b.time &&
    a.change == b.change && a.attributes == b.attributes && a.links == b.links; }
std::wstring Join(const std::wstring& root, const std::wstring& relative)
{ return root + (root.back() == L'\\' ? L"" : L"\\") + relative; }
bool Relative(const std::wstring& p)
{
    if (p.empty() || p.front() == L'\\' || p.find_first_of(L"/:\0", 0, 3) != std::wstring::npos) return false;
    size_t start = 0;
    while (start < p.size()) {
        size_t end = p.find(L'\\', start); if (end == std::wstring::npos) end = p.size();
        auto part = p.substr(start, end - start);
        if (part.empty() || part == L"." || part == L".." || part.back() == L'.' || part.back() == L' ') return false;
        start = end + 1;
    }
    return p.back() != L'\\';
}
void Absent(const std::wstring& p)
{
    DWORD attr = GetFileAttributesW(ApiPath(p).c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) throw Failure{ERROR_FILE_EXISTS};
    DWORD error = GetLastError();
    if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) throw Failure{error};
}
void Parents(const std::wstring& root, const std::wstring& full, bool create, std::atomic<bool>& cancel)
{
    size_t end = full.find_last_of(L'\\'), base = 3;
    if (full.rfind(L"\\\\",0) == 0) {
        size_t server = full.find(L'\\',2); base = full.find(L'\\',server + 1);
        if (base == std::wstring::npos) throw Failure{ERROR_BAD_PATHNAME};
    }
    size_t n = base;
    for (;;) {
        Stop(cancel); std::wstring p = full.substr(0,n);
        File h(Open(p, FILE_READ_ATTRIBUTES));
        if (h.h == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (!create || n <= root.size() || (error != ERROR_PATH_NOT_FOUND && error != ERROR_FILE_NOT_FOUND)) throw Failure{error};
            if (!CreateDirectoryW(ApiPath(p).c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) throw Failure{GetLastError()};
            h.h = Open(p,FILE_READ_ATTRIBUTES); Need(h.h != INVALID_HANDLE_VALUE);
        }
        Entry e = Info(h.h);
        if (!e.directory || Unsupported(e.attributes)) throw Failure{ERROR_NOT_SUPPORTED};
        if (n >= end) break;
        size_t next = full.find(L'\\',n); if (next == n) next = full.find(L'\\',n + 1);
        n = next == std::wstring::npos || next > end ? end : next;
    }
}
struct Security {
    std::vector<BYTE> bytes;
    explicit Security(const std::wstring& p) {
        DWORD n=0; GetFileSecurityW(ApiPath(p).c_str(),DACL_SECURITY_INFORMATION,nullptr,0,&n);
        if (!n) throw Failure{GetLastError()}; bytes.resize(n);
        Need(GetFileSecurityW(ApiPath(p).c_str(),DACL_SECURITY_INFORMATION,bytes.data(),n,&n));
    }
    void Apply(const std::wstring& p) const {
        SECURITY_DESCRIPTOR_CONTROL c{}; DWORD revision=0;
        Need(GetSecurityDescriptorControl(const_cast<BYTE*>(bytes.data()), &c, &revision));
        Need(SetFileSecurityW(ApiPath(p).c_str(), DACL_SECURITY_INFORMATION |
            ((c & SE_DACL_PROTECTED) ? PROTECTED_DACL_SECURITY_INFORMATION : UNPROTECTED_DACL_SECURITY_INFORMATION),
            const_cast<BYTE*>(bytes.data())));
    }
    bool Matches(const std::wstring& p) const {
        Security b(p); PACL x=nullptr,y=nullptr; BOOL xp=FALSE,yp=FALSE,xd=FALSE,yd=FALSE;
        Need(GetSecurityDescriptorDacl(const_cast<BYTE*>(bytes.data()),&xp,&x,&xd));
        Need(GetSecurityDescriptorDacl(b.bytes.data(),&yp,&y,&yd));
        SECURITY_DESCRIPTOR_CONTROL xc{},yc{}; DWORD revision=0;
        Need(GetSecurityDescriptorControl(const_cast<BYTE*>(bytes.data()),&xc,&revision));
        Need(GetSecurityDescriptorControl(b.bytes.data(),&yc,&revision));
        return xp==yp && (xc & SE_DACL_PROTECTED)==(yc & SE_DACL_PROTECTED) &&
            ((!x && !y) || (x && y && x->AclSize == y->AclSize && memcmp(x,y,x->AclSize)==0));
    }
};
using Streams = std::map<std::wstring,uint64_t>;
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
thread_local std::function<void(CopyPhase,const std::wstring&)> hook;
#endif
Streams StreamsOf(const std::wstring& p)
{
    WIN32_FIND_STREAM_DATA d{}; HANDLE find = FindFirstStreamW(ApiPath(p).c_str(),FindStreamInfoStandard,&d,0);
    if (find == INVALID_HANDLE_VALUE) throw Failure{GetLastError()};
    Streams streams;
    try {
        do { streams.emplace(d.cStreamName,uint64_t(d.StreamSize.QuadPart)); } while (FindNextStreamW(find,&d));
        DWORD error=GetLastError(); if (error != ERROR_HANDLE_EOF) throw Failure{error};
    } catch (...) { FindClose(find); throw; }
    FindClose(find); return streams;
}
void Verify(const std::wstring& source, const std::wstring& stage, std::atomic<bool>& cancel, CopyProgress& progress)
{
    Streams a=StreamsOf(source), b=StreamsOf(stage);
    if (a != b) throw Failure{ERROR_CRC};
    uint64_t verifyTotal=0;for(const auto& stream:a)verifyTotal+=stream.second;
    progress.VerifyDisplay(verifyTotal);
    std::vector<BYTE> x(256*1024), y(x.size());
    for (const auto& stream:a) {
        Control(cancel,progress); auto suffix=stream.first==L"::$DATA" ? L"" : stream.first;
        File in(Open(source+suffix,GENERIC_READ)), out(Open(stage+suffix,GENERIC_READ));
        Need(in.h != INVALID_HANDLE_VALUE && out.h != INVALID_HANDLE_VALUE);
        uint64_t total=0;
        for (;;) {
            Control(cancel,progress); DWORD nx=0,ny=0;
            Need(ReadFile(in.h,x.data(),DWORD(x.size()),&nx,nullptr));
            Need(ReadFile(out.h,y.data(),DWORD(y.size()),&ny,nullptr));
            if (nx!=ny || memcmp(x.data(),y.data(),nx)!=0) throw Failure{ERROR_CRC};
            if (!nx) break; total+=nx; progress.verified.fetch_add(uint64_t(nx)*2,std::memory_order_relaxed);
            progress.VerifiedDisplay(nx);
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
            if (hook) hook(CopyPhase::Verifying,stage);
#endif
        }
        if (total!=stream.second) throw Failure{ERROR_CRC};
    }
    if (StreamsOf(source)!=a || StreamsOf(stage)!=a) throw Failure{ERROR_FILE_INVALID};
}
void Phase(CopyProgress& p, CopyPhase phase, const std::wstring& path) {
    p.phase=phase;
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
    if (hook) hook(phase,path);
#else
    (void)path;
#endif
}
// The host's private worker cannot be called through the SDK. Use the same
// principle here: choose a small buffer BEFORE reading/writing, then pace it.
// Copy every data stream into the already owned stage; publication, metadata,
// destination permissions and full verification remain in CopyOne below.
void CopyStreams(const std::wstring& source,const std::wstring& stage,File& sourceFile,
                 std::atomic<bool>& cancel,CopyProgress& progress,const Entry& owned)
{
    const auto streams=StreamsOf(source);uint64_t total=0,transferred=0;
    for(const auto& stream:streams){if(stream.second>UINT64_MAX-total)throw Failure{ERROR_FILE_TOO_LARGE};total+=stream.second;}
    FILETIME created{},accessed{},modified{};Need(GetFileTime(sourceFile.h,&created,&accessed,&modified));
    const auto base=progress.copied.load();std::vector<BYTE> buffer(1024*1024);
    for(const auto& stream:streams) {
        Control(cancel,progress);const auto suffix=stream.first==L"::$DATA"?L"":stream.first;
        progress.errorSite=ErrorSite::Source;
        File in(Open(source+suffix,GENERIC_READ));Need(in.h!=INVALID_HANDLE_VALUE);
        progress.errorSite=ErrorSite::Target;
        File out(Open(stage+suffix,GENERIC_WRITE,shares,suffix.empty()?OPEN_EXISTING:CREATE_NEW));
        Need(out.h!=INVALID_HANDLE_VALUE);if(!SameId(owned,Info(out.h)))throw Failure{ERROR_FILE_INVALID};
        if(suffix.empty())Need(SetEndOfFile(out.h));
        uint64_t copied=0;
        for(;;) {
            Control(cancel,progress);DWORD read=0;progress.errorSite=ErrorSite::Source;
            Need(ReadFile(in.h,buffer.data(),progress.CopyBlockSize(),&read,nullptr));if(!read)break;
            progress.errorSite=ErrorSite::Target;
            DWORD offset=0;
            while(offset<read){Control(cancel,progress);DWORD written=0;
                const BOOL success=WriteFile(out.h,buffer.data()+offset,read-offset,&written,nullptr);
                const auto error=success?DWORD(ERROR_SUCCESS):GetLastError();
                if(written>read-offset)throw Failure{ERROR_INVALID_DATA};offset+=written;
                copied+=written;transferred+=written;progress.copied=base+transferred;
                progress.CopyDisplay(total,transferred);
                if(!success)throw Failure{error};if(!written)throw Failure{ERROR_WRITE_FAULT};
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
                if(hook)hook(CopyPhase::Copying,stage);
#endif
                if(!progress.Throttle(cancel,base+transferred))throw Failure{ERROR_CANCELLED};
            }
        }
        if(copied!=stream.second)throw Failure{ERROR_FILE_INVALID};
    }
    progress.errorSite=ErrorSite::Target;
    File out(Open(stage,FILE_WRITE_ATTRIBUTES));Need(out.h!=INVALID_HANDLE_VALUE);
    if(!SameId(owned,Info(out.h)))throw Failure{ERROR_FILE_INVALID};
    Need(SetFileTime(out.h,&created,&accessed,&modified));
}
void CopyOne(const std::wstring& sourceRoot, const std::wstring& targetRoot, const Row& row,
             CopyOutcome& outcome, std::atomic<bool>& cancel, CopyProgress& progress,
             const Entry* original=nullptr,const Security* targetSecurity=nullptr,bool recovery=false,Entry* savedEntry=nullptr)
{
    const Entry& expected=row.action==Action::CopyRight ? row.left : row.right;
    const std::wstring source=Join(sourceRoot,row.path), target=Join(targetRoot,row.path);
    progress.BeginDisplay(source,target,recovery?TransferKind::Save:original?TransferKind::Overwrite:TransferKind::Copy,false,expected.size);
    progress.errorSite=ErrorSite::Source;
    Entry owned{}; std::wstring stage;
    try {
        if (!Relative(row.path)) throw Failure{ERROR_INVALID_NAME};
        Control(cancel,progress); Parents(sourceRoot,source,false,cancel);
        progress.errorSite=ErrorSite::Target;Parents(targetRoot,target,false,cancel);
    } catch (const Failure& e) {
        // Missing target descendants may be created, never missing roots/ancestors.
        if (e.error!=ERROR_PATH_NOT_FOUND && e.error!=ERROR_FILE_NOT_FOUND) throw;
        progress.errorSite=ErrorSite::Source;Parents(sourceRoot,source,false,cancel);
        progress.errorSite=ErrorSite::Target;Parents(targetRoot,target,true,cancel);
    }
    progress.errorSite=ErrorSite::Source;
    File in(Open(source,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE)); Need(in.h != INVALID_HANDLE_VALUE);
    if (!Unchanged(expected,Info(in.h))) throw Failure{ERROR_FILE_INVALID};
    auto checkTarget=[&]() {
        if(!original) { Absent(target); return; }
        File current(Open(target,FILE_READ_ATTRIBUTES|READ_CONTROL)); Need(current.h!=INVALID_HANDLE_VALUE);
        if(!Unchanged(*original,Info(current.h)) || SameId(expected,*original) || !targetSecurity || !targetSecurity->Matches(target))
            throw Failure{ERROR_FILE_INVALID};
        if(original->attributes & FILE_ATTRIBUTE_READONLY) throw Failure{ERROR_ACCESS_DENIED};
    };
    progress.errorSite=ErrorSite::Target;checkTarget(); Control(cancel,progress);
    try {
        auto parent=target.substr(0,target.find_last_of(L'\\'));
        for (int tries=0; tries<16; ++tries) {
            GUID guid{}; if (CoCreateGuid(&guid)!=S_OK) throw Failure{ERROR_GEN_FAILURE};
            wchar_t token[48]{}; if (!StringFromGUID2(guid,token,int(std::size(token)))) throw Failure{ERROR_GEN_FAILURE};
            stage=Join(parent,L".dirsync-"+std::wstring(token)+L".tmp");
            File f(Open(stage,GENERIC_WRITE|READ_CONTROL,0,CREATE_NEW));
            if (f.h==INVALID_HANDLE_VALUE) { DWORD error=GetLastError(); if (error==ERROR_FILE_EXISTS) continue; throw Failure{error}; }
            owned=Info(f.h); break;
        }
        if (!owned.identified) throw Failure{ERROR_FILE_EXISTS};
        Security inherited(stage);
        const Security& desired=targetSecurity?*targetSecurity:inherited;
        Phase(progress,CopyPhase::Copying,stage); Control(cancel,progress);
        // Destination is our CREATE_NEW reservation, never the user's target.
        progress.errorSite=ErrorSite::Data;
        CopyStreams(source,stage,in,cancel,progress,owned);
        Control(cancel,progress);progress.errorSite=ErrorSite::Target;
        File staging(Open(stage,GENERIC_READ|FILE_WRITE_ATTRIBUTES)); Need(staging.h!=INVALID_HANDLE_VALUE);
        if (!SameId(owned,Info(staging.h))) throw Failure{ERROR_FILE_INVALID};
        Phase(progress,CopyPhase::Preparing,stage); Control(cancel,progress);
        // A normal Modify grant need not include WRITE_DAC. Avoid rewriting an
        // already correct inherited DACL; restore it only if copying changed it.
        if (!desired.Matches(stage)) desired.Apply(stage);
        FILETIME mtime{DWORD(expected.time),DWORD(expected.time>>32)};
        Need(SetFileTime(staging.h,nullptr,nullptr,&mtime));
        Need(SetFileAttributesW(ApiPath(stage).c_str(),(expected.attributes & selectedAttrs)|FILE_ATTRIBUTE_ARCHIVE));
        Phase(progress,CopyPhase::Verifying,stage); Verify(source,stage,cancel,progress);
        auto prepared=Info(staging.h);
        if (prepared.time!=expected.time || (prepared.attributes & selectedAttrs)!=(expected.attributes & selectedAttrs) || !desired.Matches(stage))
            throw Failure{ERROR_INVALID_DATA};
        Phase(progress,CopyPhase::Publishing,stage); Control(cancel,progress);
        // Pause may have allowed the source or target to change. Repeat the
        // final source check after resuming, alongside the target checks below.
        if (!Unchanged(expected,Info(in.h))) throw Failure{ERROR_FILE_INVALID};
        File currentStage(Open(stage,FILE_READ_ATTRIBUTES));Need(currentStage.h!=INVALID_HANDLE_VALUE);
        if (!Unchanged(prepared,Info(staging.h)) || !Unchanged(prepared,Info(currentStage.h)) || !desired.Matches(stage))
            throw Failure{ERROR_FILE_INVALID};
        Parents(targetRoot,target,false,cancel); checkTarget();
        currentStage.Close();staging.Close();
        // Same-volume replacement changes this name, never the old hardlink's data.
        // There is deliberately no delete/recreate or cross-volume fallback.
        progress.errorSite=ErrorSite::Target;
        Need(MoveFileExW(ApiPath(stage).c_str(),ApiPath(target).c_str(),original?MOVEFILE_REPLACE_EXISTING:0));
        outcome.published=true;
        Phase(progress,CopyPhase::Published,target);
        File published(Open(target,FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES)); Need(published.h!=INVALID_HANDLE_VALUE);
        if (!SameId(owned,Info(published.h))) throw Failure{ERROR_FILE_INVALID};
        Need(SetFileTime(published.h,nullptr,nullptr,&mtime));
        Need(SetFileAttributesW(ApiPath(target).c_str(),(expected.attributes & selectedAttrs)|FILE_ATTRIBUTE_ARCHIVE));
        auto final=Info(published.h);
        if (final.time!=expected.time || (final.attributes & selectedAttrs)!=(expected.attributes & selectedAttrs) || !desired.Matches(target))
            throw Failure{ERROR_INVALID_DATA};
        if(savedEntry)*savedEntry=final;
        outcome.status=CopyStatus::Copied; progress.CompleteDisplay(); return;
    } catch (...) {
        // Only our unpublished staging can be removed, after checking its identity.
        // A published destination is never deleted or rolled back on failure.
        if (owned.identified && !outcome.published) {
            try {
                File f(Open(stage,FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES));
                if (f.h!=INVALID_HANDLE_VALUE) {
                    if (SameId(owned,Info(f.h))) {
                        SetFileAttributesW(ApiPath(stage).c_str(),FILE_ATTRIBUTE_NORMAL); f.Close();
                        if (!DeleteFileW(ApiPath(stage).c_str())) outcome.remainingStage=stage;
                    } else outcome.remainingStage=stage;
                } else if (GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND) outcome.remainingStage=stage;
            } catch (const Failure&) { outcome.remainingStage=stage; }
        }
        throw;
    }
}
} // namespace
bool IsNewFileCopy(const Row& r)
{
    if (r.state==State::Blocked || r.state==State::Conflict) return false;
    const Entry* e=nullptr;
    if (r.action==Action::CopyRight && r.hasLeft && !r.hasRight) e=&r.left;
    if (r.action==Action::CopyLeft && r.hasRight && !r.hasLeft) e=&r.right;
    return e && e->identified && !e->directory && !e->error && !Unsupported(e->attributes) && Relative(r.path);
}
bool IsNewCopy(const Row& r)
{
    if(IsNewFileCopy(r)) return true;
    if(r.state==State::Blocked || r.state==State::Conflict || !Relative(r.path)) return false;
    const Entry* e=nullptr;
    if(r.action==Action::CopyRight && r.hasLeft && !r.hasRight) e=&r.left;
    if(r.action==Action::CopyLeft && r.hasRight && !r.hasLeft) e=&r.right;
    return e && e->identified && e->directory && e->empty && !e->error && !Unsupported(e->attributes);
}
bool IsOriginalToSave(const Row& r)
{
    if (r.state==State::Blocked || r.state==State::Conflict || !r.hasLeft || !r.hasRight ||
        (r.action!=Action::CopyLeft && r.action!=Action::CopyRight) || !Relative(r.path)) return false;
    for (const Entry* e:{&r.left,&r.right})
        if (!e->identified || e->directory || e->error || Unsupported(e->attributes)) return false;
    return true;
}
namespace {
// A journal failure must never be mistaken for a completed recovery record.
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
thread_local std::function<DWORD(const std::wstring&)> recoveryLogHook;
#endif
std::wstring RecoveryCanonical(const std::wstring& path,std::atomic<bool>& cancel)
{
    Parents(path,Join(path,L"root-check"),false,cancel);
    File directory(Open(path,FILE_READ_ATTRIBUTES)); Need(directory.h!=INVALID_HANDLE_VALUE);
    DWORD n=GetFinalPathNameByHandleW(directory.h,nullptr,0,VOLUME_NAME_DOS); Need(n!=0);
    std::vector<wchar_t> buffer(size_t(n)+1);
    DWORD used=GetFinalPathNameByHandleW(directory.h,buffer.data(),DWORD(buffer.size()),VOLUME_NAME_DOS);
    Need(used && used<buffer.size());
    std::wstring result;
    if (!NormalizeRoot(std::wstring(buffer.data(),used),result)) throw Failure{ERROR_INVALID_NAME};
    return result;
}
bool RecoveryWithin(const std::wstring& root,const std::wstring& path)
{
    return path.size()>=root.size() && CompareStringOrdinal(root.c_str(),int(root.size()),path.c_str(),int(root.size()),TRUE)==CSTR_EQUAL &&
        (path.size()==root.size() || root.back()==L'\\' || path[root.size()]==L'\\');
}
void RecoveryDirectory(const std::wstring& path,std::atomic<bool>& cancel)
{
    Stop(cancel);
    if (!CreateDirectoryW(ApiPath(path).c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) throw Failure{GetLastError()};
    Parents(path,Join(path,L"directory-check"),false,cancel);
}
std::wstring RecoveryPair(const std::wstring& left,const std::wstring& right)
{
    auto text=left+L"\n"+right; CharUpperBuffW(&text[0],DWORD(text.size()));
    uint64_t hash=14695981039346656037ULL;
    for (wchar_t c:text) { hash^=uint16_t(c); hash*=1099511628211ULL; }
    wchar_t token[32]{}; swprintf_s(token,L"pair-%016llx",hash); return token;
}
std::wstring RecoveryRun(const std::wstring& pair,std::atomic<bool>& cancel)
{
    for (int i=0;i<16;++i) {
        Stop(cancel); GUID guid{}; if (CoCreateGuid(&guid)!=S_OK) throw Failure{ERROR_GEN_FAILURE};
        wchar_t token[48]{}; if (!StringFromGUID2(guid,token,int(std::size(token)))) throw Failure{ERROR_GEN_FAILURE};
        auto run=Join(pair,L"run-"+std::wstring(token));
        if (CreateDirectoryW(ApiPath(run).c_str(),nullptr)) return run;
        if (GetLastError()!=ERROR_ALREADY_EXISTS) throw Failure{GetLastError()};
    }
    throw Failure{ERROR_FILE_EXISTS};
}
std::wstring Json(const std::wstring& value)
{
    std::wstring out=L"\"";
    for (wchar_t c:value) {
        if (c==L'"' || c==L'\\') { out+=L'\\'; out+=c; }
        else if (c<32) { wchar_t escape[8]{}; swprintf_s(escape,L"\\u%04x",unsigned(c)); out+=escape; }
        else out+=c;
    }
    return out+L"\"";
}
void RecoveryLine(HANDLE journal,const std::wstring& line)
{
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
    if (recoveryLogHook) { DWORD error=recoveryLogHook(line); if (error) throw Failure{error}; }
#endif
    auto text=line+L"\n";
    int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.c_str(),int(text.size()),nullptr,0,nullptr,nullptr);
    if (!n) throw Failure{GetLastError()};
    std::vector<char> bytes(static_cast<size_t>(n));
    Need(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.c_str(),int(text.size()),bytes.data(),n,nullptr,nullptr)!=0);
    DWORD written=0; Need(WriteFile(journal,bytes.data(),DWORD(bytes.size()),&written,nullptr));
    if (written!=bytes.size()) throw Failure{ERROR_WRITE_FAULT};
    Need(FlushFileBuffers(journal));
}
std::wstring OutcomeLine(size_t index,const CopyOutcome& c,const wchar_t* completed=L"saved")
{
    const wchar_t* status=c.status==CopyStatus::Copied?completed:c.status==CopyStatus::Failed?L"failed":
        c.status==CopyStatus::Cancelled?L"cancelled":c.status==CopyStatus::Skipped?L"skipped":
        c.status==CopyStatus::DependencyBlocked?L"dependency_blocked":L"not_started";
    return L"{\"event\":\"result\",\"item\":"+std::to_wstring(index+1)+L",\"status\":"+Json(status)+
        L",\"error\":"+std::to_wstring(c.error)+L",\"published\":"+(c.published?L"true":L"false")+
        L",\"remainingStage\":"+Json(c.remainingStage)+L",\"attempts\":"+std::to_wstring(c.attempts)+
        L",\"lastError\":"+std::to_wstring(c.lastError)+L",\"automaticSkip\":"+(c.automaticSkip?L"true":L"false")+L"}";
}
// Shared per-item boundary. Journal writes deliberately remain outside this
// helper, so their failure can never be answered with Skip or Retry.
template<class Work,class Pending> void ExecuteCopyItem(CopyOutcome& item,TransferKind kind,const std::wstring& source,
    const std::wstring& target,Result& snapshot,std::atomic<bool>& cancel,
    Progress& checking,CopyProgress& progress,bool& cancelled,bool& stale,Work work,Pending pending,
    bool decisions=true,const std::vector<size_t>& removed={})
{
    for(;;) {
        item.error=0;item.status=CopyStatus::NotStarted;++item.attempts;
        try {Control(cancel,progress);work();return;}
        catch(const Failure& failure) {
            item.error=item.lastError=failure.error;
            cancelled=cancel.load() || failure.error==ERROR_CANCELLED || failure.error==ERROR_OPERATION_ABORTED || failure.error==ERROR_REQUEST_ABORTED;
            item.status=cancelled?CopyStatus::Cancelled:CopyStatus::Failed;
            OperationError error{kind,progress.phase.load(),progress.errorSite,failure.error,source,target,item.published,!item.remainingStage.empty()};
            if(cancelled || !decisions || progress.errors.stopOnFirstError || !progress.errors.prompt || !Recoverable(error))return;
            bool automatic=false;ErrorDecision decision=ErrorDecision::Cancel;
            try {decision=progress.errors.Decide(error,automatic);}
            catch(...) {item.error=ERROR_NOT_ENOUGH_MEMORY;return;}
            if(cancel || decision==ErrorDecision::Cancel){cancel=true;cancelled=true;item.status=CopyStatus::Cancelled;return;}
            if(decision==ErrorDecision::Skip){item.status=CopyStatus::Skipped;item.automaticSkip=automatic;}
            auto remaining=pending(decision==ErrorDecision::Retry);
            if(!AdvanceExecutionSnapshot(snapshot,remaining,cancel,checking,removed)) {
                cancelled=cancel.load();stale=!cancelled;item.automaticSkip=false;
                item.status=cancelled?CopyStatus::Cancelled:CopyStatus::Failed;
                item.error=cancelled?ERROR_CANCELLED:ERROR_FILE_INVALID;return;
            }
            if(decision==ErrorDecision::Skip)return;
        }
        catch(...) {item.error=ERROR_NOT_ENOUGH_MEMORY;item.status=CopyStatus::Failed;return;}
    }
}
void DirectoryStreams(const std::wstring& path)
{
    WIN32_FIND_STREAM_DATA data{};
    HANDLE find=FindFirstStreamW(ApiPath(path).c_str(),FindStreamInfoStandard,&data,0);
    if(find==INVALID_HANDLE_VALUE) {
        DWORD error=GetLastError();
        if(error==ERROR_HANDLE_EOF) return; // An ordinary NTFS directory has no data stream.
        throw Failure{error};
    }
    FindClose(find);
    // Directory ADS recovery is not supported. Never discard it, even with
    // Keep originals OFF; surface the unsupported item before any deletion.
    throw Failure{ERROR_NOT_SUPPORTED};
}
void EmptyDirectory(const std::wstring& path)
{
    WIN32_FIND_DATAW data{};
    HANDLE find=FindFirstFileW(ApiPath(Join(path,L"*")).c_str(),&data);
    if(find==INVALID_HANDLE_VALUE) {if(GetLastError()==ERROR_FILE_NOT_FOUND) return;throw Failure{GetLastError()};}
    bool occupied=false;
    do {if(wcscmp(data.cFileName,L".") && wcscmp(data.cFileName,L"..")) {occupied=true;break;}} while(FindNextFileW(find,&data));
    const auto error=GetLastError();FindClose(find);
    if(occupied) throw Failure{ERROR_DIR_NOT_EMPTY};
    if(error!=ERROR_NO_MORE_FILES) throw Failure{error};
}
void CopyEmptyDirectory(const std::wstring& sourceRoot,const std::wstring& targetRoot,const Row& row,
    CopyOutcome& outcome,std::atomic<bool>& cancel,CopyProgress& progress)
{
    const auto& expected=row.action==Action::CopyRight?row.left:row.right;
    const auto source=Join(sourceRoot,row.path),target=Join(targetRoot,row.path);
    progress.BeginDisplay(source,target,TransferKind::Copy,true);
    progress.errorSite=ErrorSite::Source;
    Control(cancel,progress);Parents(sourceRoot,source,false,cancel);
    File original(Open(source,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE));Need(original.h!=INVALID_HANDLE_VALUE);
    auto check=[&]{if(!Unchanged(expected,Info(original.h))) throw Failure{ERROR_FILE_INVALID};EmptyDirectory(source);DirectoryStreams(source);};
    check();progress.errorSite=ErrorSite::Target;Parents(targetRoot,target,true,cancel);Absent(target);
    const auto parent=target.substr(0,target.find_last_of(L'\\'));
    std::wstring stage;Entry owned;
    try {
        for(int attempt=0;attempt<16;++attempt) {
            GUID guid{};if(CoCreateGuid(&guid)!=S_OK) throw Failure{ERROR_GEN_FAILURE};
            wchar_t token[48]{};if(!StringFromGUID2(guid,token,int(std::size(token)))) throw Failure{ERROR_GEN_FAILURE};
            stage=Join(parent,L".dirsync-"+std::wstring(token)+L".tmp");
            if(!CreateDirectoryW(ApiPath(stage).c_str(),nullptr)) {if(GetLastError()==ERROR_ALREADY_EXISTS) continue;throw Failure{GetLastError()};}
            File file(Open(stage,FILE_READ_ATTRIBUTES));Need(file.h!=INVALID_HANDLE_VALUE);owned=Info(file.h);break;
        }
        if(!owned.identified) throw Failure{ERROR_FILE_EXISTS};
        Security inherited(stage);Phase(progress,CopyPhase::Preparing,stage);Control(cancel,progress);
        File file(Open(stage,FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES));Need(file.h!=INVALID_HANDLE_VALUE);
        FILETIME time{DWORD(expected.time),DWORD(expected.time>>32)};
        Need(SetFileTime(file.h,nullptr,nullptr,&time));
        Need(SetFileAttributesW(ApiPath(stage).c_str(),(expected.attributes&selectedAttrs)|FILE_ATTRIBUTE_DIRECTORY));
        Phase(progress,CopyPhase::Verifying,stage);Control(cancel,progress);check();EmptyDirectory(stage);
        const auto prepared=Info(file.h);
        if(!SameId(owned,prepared) || prepared.time!=expected.time ||
            (prepared.attributes&selectedAttrs)!=(expected.attributes&selectedAttrs) || !inherited.Matches(stage)) throw Failure{ERROR_INVALID_DATA};
        Phase(progress,CopyPhase::Publishing,stage);Control(cancel,progress);check();Parents(targetRoot,target,false,cancel);Absent(target);
        // Resume can follow arbitrary external changes. Reopen the staging
        // name rather than relying only on the handle opened before Pause.
        File currentStage(Open(stage,FILE_READ_ATTRIBUTES));Need(currentStage.h!=INVALID_HANDLE_VALUE);
        if(!Unchanged(prepared,Info(currentStage.h)) || !inherited.Matches(stage)) throw Failure{ERROR_FILE_INVALID};
        EmptyDirectory(stage);DirectoryStreams(stage);
        currentStage.Close();file.Close();Need(MoveFileExW(ApiPath(stage).c_str(),ApiPath(target).c_str(),0));outcome.published=true;
        Phase(progress,CopyPhase::Published,target);
        File published(Open(target,FILE_READ_ATTRIBUTES));Need(published.h!=INVALID_HANDLE_VALUE);
        const auto final=Info(published.h);EmptyDirectory(target);
        if(!SameId(owned,final) || final.time!=expected.time || (final.attributes&selectedAttrs)!=(expected.attributes&selectedAttrs) ||
            !inherited.Matches(target)) throw Failure{ERROR_INVALID_DATA};
        outcome.status=CopyStatus::Copied;progress.CompleteDisplay();
    } catch(...) {
        if(owned.identified && !outcome.published) {
            File file(Open(stage,FILE_READ_ATTRIBUTES));
            if(file.h!=INVALID_HANDLE_VALUE && SameId(owned,Info(file.h))) {
                file.Close();SetFileAttributesW(ApiPath(stage).c_str(),FILE_ATTRIBUTE_NORMAL);
                if(!RemoveDirectoryW(ApiPath(stage).c_str())) outcome.remainingStage=stage;
            } else outcome.remainingStage=stage;
        }
        throw;
    }
}
void SaveDirectory(const std::wstring& root,const Entry& expected,RecoveryItem& item,
                   std::atomic<bool>& cancel,CopyProgress& progress)
{
    progress.BeginDisplay(item.original,item.saved,TransferKind::Save,true);
    progress.phase=CopyPhase::Preparing;
    progress.errorSite=ErrorSite::Source;
    Control(cancel,progress); Parents(root,item.original,false,cancel);
    File original(Open(item.original,FILE_READ_ATTRIBUTES)); Need(original.h!=INVALID_HANDLE_VALUE);
    if(!Unchanged(expected,Info(original.h))) throw Failure{ERROR_FILE_INVALID};
    DirectoryStreams(item.original);
    progress.errorSite=ErrorSite::Target;
    auto data=item.saved.substr(0,item.saved.size()-expected.path.size()-1);
    Parents(data,item.saved,true,cancel);
    Need(CreateDirectoryW(ApiPath(item.saved).c_str(),nullptr));
    // The recovery marker now exists. Metadata failures after this point are
    // published effects, never permission to retry CREATE_NEW or silently skip.
    item.copy.published=true;
    File saved(Open(item.saved,FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES)); Need(saved.h!=INVALID_HANDLE_VALUE);
    FILETIME time{DWORD(expected.time),DWORD(expected.time>>32)};
    Need(SetFileTime(saved.h,nullptr,nullptr,&time));
    Need(SetFileAttributesW(ApiPath(item.saved).c_str(),(expected.attributes & selectedAttrs)|FILE_ATTRIBUTE_DIRECTORY));
    const auto observed=Info(saved.h);
    if(!observed.directory || observed.time!=expected.time ||
        (observed.attributes & selectedAttrs)!=(expected.attributes & selectedAttrs)) throw Failure{ERROR_INVALID_DATA};
    if(!Unchanged(expected,Info(original.h))) throw Failure{ERROR_FILE_INVALID};
    item.savedEntry=observed;item.copy.status=CopyStatus::Copied;progress.CompleteDisplay();
}
}
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
void SetRecoveryLogHook(std::function<DWORD(const std::wstring&)> callback) { recoveryLogHook=std::move(callback); }
#endif
static RecoveryReport SaveOriginalsImpl(Result snapshot,const std::vector<size_t>& visible,const std::wstring& recoveryRoot,
    std::atomic<bool>& cancel,Progress& checking,CopyProgress& progress,bool deletion)
{
    RecoveryReport report; std::wstring left,right;
    File journal;
    try {
        std::set<size_t> unique;
        for (size_t i:visible) {
            if (i>=snapshot.rows.size() || !unique.insert(i).second) continue;
            const auto& row=snapshot.rows[i];
            if (deletion?IsDeletion(row):IsOriginalToSave(row)) {
                RecoveryItem item; item.row=i; item.right=deletion || row.action==Action::CopyRight; report.items.push_back(std::move(item));
            }
            else if (row.action!=Action::None) ++report.omitted;
        }
        if (report.items.empty()) return report;
        Control(cancel,progress);
        if (!SnapshotRoots(snapshot,left,right) || !Revalidate(snapshot,cancel,checking)) {
            report.stale=true; report.cancelled=cancel.load(); return report;
        }
        std::wstring root;
        if (!NormalizeRoot(recoveryRoot,root)) throw Failure{ERROR_INVALID_NAME};
        root=RecoveryCanonical(root,cancel);
        for (const auto& input:{RecoveryCanonical(left,cancel),RecoveryCanonical(right,cancel)})
            if (RecoveryWithin(root,input) || RecoveryWithin(input,root)) throw Failure{ERROR_INVALID_PARAMETER};
        auto pair=Join(root,RecoveryPair(RecoveryCanonical(left,cancel),RecoveryCanonical(right,cancel)));
        RecoveryDirectory(pair,cancel); report.runDirectory=RecoveryRun(pair,cancel);
        File run(Open(report.runDirectory,FILE_READ_ATTRIBUTES)); Need(run.h!=INVALID_HANDLE_VALUE); const Entry runId=Info(run.h); run.Close();
        report.journal=Join(report.runDirectory,L"manifest.jsonl");
        journal.h=Open(report.journal,GENERIC_WRITE,FILE_SHARE_READ,CREATE_NEW); Need(journal.h!=INVALID_HANDLE_VALUE);
        auto append=[&](const std::wstring& line) {
            try { RecoveryLine(journal.h,line); }
            catch (const Failure& failure) { report.journalError=failure.error; throw; }
        };
        append(L"{\"event\":\"run\",\"schema\":1,\"operation\":\"save_originals_only\",\"left\":"+
            Json(left)+L",\"right\":"+Json(right)+L",\"runDirectory\":"+Json(report.runDirectory)+L"}");
        for (size_t n=0;n<report.items.size();++n) {
            Control(cancel,progress);
            auto& item=report.items[n]; const auto& row=snapshot.rows[item.row];
            const auto& entry=item.right?row.right:row.left;
            item.original=Join(item.right?right:left,row.path);
            auto side=Join(report.runDirectory,item.right?L"right":L"left");
            auto operation=Join(side,std::to_wstring(n+1));auto data=Join(operation,L"data");
            item.saved=Join(data,row.path); item.copy.row=item.row; item.copy.target=item.saved;
            append(L"{\"event\":\"planned\",\"item\":"+std::to_wstring(n+1)+L",\"side\":"+
                Json(item.right?L"right":L"left")+L",\"relative\":"+Json(row.path)+L",\"original\":"+Json(item.original)+
                L",\"saved\":"+Json(item.saved)+L",\"kind\":"+Json(entry.directory?L"directory":L"file")+L",\"size\":"+std::to_wstring(entry.size)+L",\"mtime\":"+
                std::to_wstring(entry.time)+L",\"attributes\":"+std::to_wstring(entry.attributes)+L"}");
        }
        size_t n=0;
        for (;n<report.items.size();++n) {
            auto& item=report.items[n]; const auto& row=snapshot.rows[item.row]; progress.item=n;
            ExecuteCopyItem(item.copy,TransferKind::Save,item.original,item.saved,snapshot,cancel,checking,
                progress,report.cancelled,report.stale,[&] {
                    progress.BeginDisplay(item.original,item.saved,TransferKind::Save,false);
                    progress.phase=CopyPhase::Preparing;progress.errorSite=ErrorSite::Target;
                    File currentRun(Open(report.runDirectory,FILE_READ_ATTRIBUTES));Need(currentRun.h!=INVALID_HANDLE_VALUE);
                    if(!SameId(runId,Info(currentRun.h)))throw Failure{ERROR_FILE_INVALID};
                    Row original;original.path=row.path;original.hasLeft=true;
                    original.left=item.right?row.right:row.left;original.action=Action::CopyRight;
                    auto data=item.saved.substr(0,item.saved.size()-row.path.size()-1);
                    {
                        auto side=Join(report.runDirectory,item.right?L"right":L"left");
                        RecoveryDirectory(side,cancel);RecoveryDirectory(Join(side,std::to_wstring(n+1)),cancel);RecoveryDirectory(data,cancel);
                    }
                    if(original.left.directory)SaveDirectory(item.right?right:left,original.left,item,cancel,progress);
                    else CopyOne(item.right?right:left,data,original,item.copy,cancel,progress,nullptr,nullptr,true,&item.savedEntry);
                },[&](bool retry){std::vector<size_t> pending;
                    for(size_t i=n+(retry?0:1);i<report.items.size();++i)pending.push_back(report.items[i].row);return pending;});
            append(OutcomeLine(n,item.copy));item.logged=true;
            if(item.copy.status!=CopyStatus::Copied && item.copy.status!=CopyStatus::Skipped) {
                report.error=item.copy.error?item.copy.error:ERROR_INVALID_DATA;++n;break;
            }
        }
        for(;n<report.items.size();++n){append(OutcomeLine(n,report.items[n].copy));report.items[n].logged=true;}
        const bool processed=!report.error && !report.stale && !report.cancelled;
        const bool complete=processed && std::all_of(report.items.begin(),report.items.end(),
            [](const RecoveryItem& item){return item.logged && item.copy.status==CopyStatus::Copied;});
        append(L"{\"event\":\"end\",\"status\":"+Json(report.error?(report.cancelled?L"cancelled":L"failed"):complete?L"complete":L"incomplete")+L"}");
        report.processed=processed;report.complete=complete;
    } catch (const Failure& failure) {
        report.error=failure.error;
        report.cancelled=cancel.load() || failure.error==ERROR_CANCELLED || failure.error==ERROR_OPERATION_ABORTED || failure.error==ERROR_REQUEST_ABORTED;
    }
    catch (...) { report.error=ERROR_NOT_ENOUGH_MEMORY; }
    Phase(progress,CopyPhase::Finished,L""); return report;
}
RecoveryReport SaveOriginals(Result snapshot,const std::vector<size_t>& visible,const std::wstring& recoveryRoot,
    std::atomic<bool>& cancel,Progress& checking,CopyProgress& progress)
{ return SaveOriginalsImpl(std::move(snapshot),visible,recoveryRoot,cancel,checking,progress,false); }
bool IsFileDeletion(const Row& row)
{
    const auto& e=row.right;
    return row.action==Action::DeleteRight && row.state==State::RightOnly && !row.hasLeft && row.hasRight &&
        e.identified && !e.directory && !e.error && !Unsupported(e.attributes) && Relative(row.path);
}
bool IsDeletion(const Row& row)
{
    const auto& e=row.right;
    return row.action==Action::DeleteRight && row.state==State::RightOnly && !row.hasLeft && row.hasRight &&
        e.identified && !e.error && !Unsupported(e.attributes) && Relative(row.path);
}
std::vector<size_t> DeletionRows(const Result& snapshot,const std::vector<size_t>& visible)
{
    std::set<size_t> selected;
    if(!SnapshotAsymmetric(snapshot)) return {};
    for(size_t i:visible) if(i<snapshot.rows.size() && IsDeletion(snapshot.rows[i])) selected.insert(i);
    auto less=[](const std::wstring& a,const std::wstring& b) {
        return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_LESS_THAN;
    };
    std::set<std::wstring,decltype(less)> retained(less);
    for(size_t i=0;i<snapshot.rows.size();++i) if(!selected.count(i)) retained.insert(snapshot.rows[i].path);
    std::vector<size_t> result;
    for(size_t i:selected) {
        const auto& row=snapshot.rows[i]; bool eligible=true;
        if(row.right.directory) {
            auto first=retained.lower_bound(row.path+L"\\");
            eligible=first==retained.end() || !RecoveryWithin(row.path,*first);
        }
        if(eligible) result.push_back(i);
    }
    std::stable_sort(result.begin(),result.end(),[&](size_t a,size_t b) {
        const auto& x=snapshot.rows[a]; const auto& y=snapshot.rows[b];
        if(x.right.directory!=y.right.directory) return !x.right.directory;
        if(x.right.directory) return std::count(x.path.begin(),x.path.end(),L'\\')>std::count(y.path.begin(),y.path.end(),L'\\');
        return a<b;
    });
    return result;
}
DeleteReport DeleteFiles(Result snapshot,const std::vector<size_t>& visible,bool keepOriginals,
    const std::wstring& recoveryRoot,std::atomic<bool>& cancel,Progress& checking,CopyProgress& progress)
{
    DeleteReport report;report.keepOriginals=keepOriginals;std::wstring left,right;File journal;
    std::map<size_t,const RecoveryItem*> originals;
    std::vector<size_t> removed;
    try {
        if(!SnapshotAsymmetric(snapshot)){report.stale=true;return report;}
        const auto selected=DeletionRows(snapshot,visible);
        const std::set<size_t> eligible(selected.begin(),selected.end());std::set<size_t> unique;
        for(size_t i:visible)if(i<snapshot.rows.size() && unique.insert(i).second && !eligible.count(i) && snapshot.rows[i].action!=Action::None)++report.omitted;
        for(size_t i:selected){DeleteOutcome item;item.row=i;report.items.push_back(std::move(item));}
        if(report.items.empty())return report;
        Control(cancel,progress);
        if(!SnapshotRoots(snapshot,left,right) || !Revalidate(snapshot,cancel,checking)) {
            report.stale=true;report.cancelled=cancel.load();return report;
        }
        auto less=[](const std::wstring& a,const std::wstring& b){return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_LESS_THAN;};
        std::map<std::wstring,size_t,decltype(less)> directories(less);
        for(size_t n=0;n<report.items.size();++n) {
            auto& item=report.items[n];const auto& row=snapshot.rows[item.row];item.target=Join(right,row.path);
            if(row.right.directory)directories.emplace(row.path,n);
            // Preserve the existing eager refusal for stop-first callers.
            if(progress.errors.stopOnFirstError || !progress.errors.prompt) {
                try {if(row.right.directory)DirectoryStreams(item.target);
                    if(row.right.attributes & FILE_ATTRIBUTE_READONLY)throw Failure{ERROR_ACCESS_DENIED};}
                catch(const Failure& failure){item.status=DeleteStatus::Failed;item.error=item.lastError=failure.error;throw;}
            }
        }
        auto protectParents=[&](const DeleteOutcome& retained) {
            auto path=snapshot.rows[retained.row].path;
            while(path.find_last_of(L'\\')!=std::wstring::npos) {
                path.resize(path.find_last_of(L'\\'));const auto found=directories.find(path);
                if(found==directories.end())continue;auto& parent=report.items[found->second];
                if(parent.status==DeleteStatus::NotStarted) {
                    parent.status=DeleteStatus::DependencyBlocked;parent.blockedBy=retained.target;
                    parent.error=parent.lastError=retained.error;
                }
            }
        };
        auto pending=[&](size_t first) {std::vector<size_t> rows;
            for(size_t i=first;i<report.items.size();++i)if(report.items[i].status!=DeleteStatus::DependencyBlocked)rows.push_back(report.items[i].row);
            return rows;};
        if(keepOriginals) {
            report.recovery=SaveOriginalsImpl(snapshot,selected,recoveryRoot,cancel,checking,progress,true);
            for(const auto& saved:report.recovery.items)if(!saved.right || !originals.emplace(saved.row,&saved).second)throw Failure{ERROR_INVALID_DATA};
            for(auto& item:report.items) {
                auto found=originals.find(item.row);if(found==originals.end() || found->second->copy.row!=item.row)throw Failure{ERROR_INVALID_DATA};
                const auto& saved=*found->second;
                if(saved.logged && saved.copy.status==CopyStatus::Skipped) {
                    item.status=DeleteStatus::DependencyBlocked;item.blockedBy=item.target;
                    item.error=item.lastError=saved.copy.error;item.automaticSkip=saved.copy.automaticSkip;
                } else if(report.recovery.processed && (!saved.logged || saved.copy.status!=CopyStatus::Copied || !saved.savedEntry.identified))throw Failure{ERROR_INVALID_DATA};
            }
            if(!report.recovery.processed || report.recovery.items.size()!=report.items.size()) {
                report.stale=report.recovery.stale;report.cancelled=report.recovery.cancelled;
                report.error=report.recovery.error?report.recovery.error:ERROR_INVALID_DATA;return report;
            }
            for(const auto& item:report.items)if(item.status==DeleteStatus::DependencyBlocked)protectParents(item);
            if(!AdvanceExecutionSnapshot(snapshot,pending(0),cancel,checking)) {
                report.cancelled=cancel.load();report.stale=!report.cancelled;
                report.error=report.cancelled?ERROR_CANCELLED:ERROR_FILE_INVALID;return report;
            }
            report.journal=Join(report.recovery.runDirectory,L"delete.jsonl");
            journal.h=Open(report.journal,GENERIC_WRITE,FILE_SHARE_READ,CREATE_NEW);
            if(journal.h==INVALID_HANDLE_VALUE){report.journalError=GetLastError();throw Failure{report.journalError};}
        }
        auto append=[&](const std::wstring& line) {
            if(!keepOriginals)return;
            try{RecoveryLine(journal.h,line);}catch(const Failure& failure){report.journalError=failure.error;throw;}
        };
        append(L"{\"event\":\"run\",\"schema\":1,\"operation\":\"delete_right\",\"keepOriginals\":true,\"left\":"+
            Json(left)+L",\"right\":"+Json(right)+L",\"recoveryManifest\":"+Json(report.recovery.journal)+L"}");
        for(size_t n=0;n<report.items.size();++n)append(L"{\"event\":\"planned\",\"item\":"+std::to_wstring(n+1)+
            L",\"target\":"+Json(report.items[n].target)+L",\"kind\":"+
            Json(snapshot.rows[report.items[n].row].right.directory?L"directory":L"file")+
            L",\"saved\":"+Json(keepOriginals?originals.at(report.items[n].row)->saved:L"")+L"}");
        auto resultLine=[&](size_t n) {
            const auto& item=report.items[n];const wchar_t* status=item.status==DeleteStatus::Deleted?L"deleted":
                item.status==DeleteStatus::Cancelled?L"cancelled":item.status==DeleteStatus::Failed?L"failed":
                item.status==DeleteStatus::Skipped?L"skipped":item.status==DeleteStatus::DependencyBlocked?L"dependency_blocked":L"not_started";
            return L"{\"event\":\"result\",\"item\":"+std::to_wstring(n+1)+L",\"status\":"+Json(status)+
                L",\"error\":"+std::to_wstring(item.error)+L",\"removed\":"+(item.removed?L"true":L"false")+
                L",\"attempts\":"+std::to_wstring(item.attempts)+L",\"lastError\":"+std::to_wstring(item.lastError)+
                L",\"automaticSkip\":"+(item.automaticSkip?L"true":L"false")+L",\"blockedBy\":"+Json(item.blockedBy)+L"}";
        };
        size_t n=0;
        for(;n<report.items.size();++n) {
            auto& item=report.items[n];const auto& row=snapshot.rows[item.row];progress.item=n+(keepOriginals?report.items.size():0);
            Control(cancel,progress);
            if(item.status!=DeleteStatus::DependencyBlocked) {
                CopyOutcome attempt;attempt.row=item.row;attempt.target=item.target;
                ExecuteCopyItem(attempt,TransferKind::Delete,L"",item.target,snapshot,cancel,checking,progress,report.cancelled,report.stale,[&] {
                    progress.BeginDisplay(L"",item.target,TransferKind::Delete,row.right.directory);
                    progress.errorSite=ErrorSite::Target;Phase(progress,CopyPhase::Deleting,item.target);Control(cancel,progress);
                    if(keepOriginals) {
                        const auto& saved=*originals.at(item.row);Parents(report.recovery.runDirectory,saved.saved,false,cancel);
                        File copy(Open(saved.saved,FILE_READ_ATTRIBUTES));
                        if(copy.h==INVALID_HANDLE_VALUE || !Unchanged(saved.savedEntry,Info(copy.h)))throw Failure{ERROR_FILE_INVALID};
                    }
                    // No recursive fallback, attribute clearing or adoption of a new counterpart.
                    try{Parents(left,Join(left,row.path),false,cancel);}
                    catch(const Failure& failure){if(failure.error!=ERROR_FILE_NOT_FOUND && failure.error!=ERROR_PATH_NOT_FOUND)throw;}
                    Absent(Join(left,row.path));Parents(right,item.target,false,cancel);
                    File current(Open(item.target,FILE_READ_ATTRIBUTES));Need(current.h!=INVALID_HANDLE_VALUE);auto observed=Info(current.h);
                    if(row.right.directory) {
                        if(!SameId(row.right,observed) || !observed.directory || observed.attributes!=row.right.attributes)throw Failure{ERROR_FILE_INVALID};
                        DirectoryStreams(item.target);
                    }else if(!Unchanged(row.right,observed))throw Failure{ERROR_FILE_INVALID};
                    if(observed.attributes & FILE_ATTRIBUTE_READONLY)throw Failure{ERROR_ACCESS_DENIED};
                    Stop(cancel);current.Close();
                    const BOOL success=row.right.directory?RemoveDirectoryW(ApiPath(item.target).c_str()):DeleteFileW(ApiPath(item.target).c_str());
                    if(!success) {
                        const DWORD error=GetLastError();File remaining(Open(item.target,FILE_READ_ATTRIBUTES));
                        if(remaining.h==INVALID_HANDLE_VALUE || !Unchanged(observed,Info(remaining.h)))throw Failure{ERROR_FILE_INVALID};
                        throw Failure{error};
                    }
                    attempt.published=true;attempt.status=CopyStatus::Copied;progress.CompleteDisplay();Phase(progress,CopyPhase::Deleted,item.target);
                },[&](bool retry){if(!retry) {item.error=attempt.error;protectParents(item);}return pending(n+(retry?0:1));},true,removed);
                item.removed=attempt.published;item.error=attempt.error;item.lastError=attempt.lastError;
                item.attempts=attempt.attempts;item.automaticSkip=attempt.automaticSkip;
                item.status=attempt.status==CopyStatus::Copied?DeleteStatus::Deleted:attempt.status==CopyStatus::Skipped?DeleteStatus::Skipped:
                    attempt.status==CopyStatus::Cancelled?DeleteStatus::Cancelled:DeleteStatus::Failed;
                if(item.removed)removed.push_back(item.row);
            }
            append(resultLine(n));
            if(item.status!=DeleteStatus::Deleted && item.status!=DeleteStatus::Skipped && item.status!=DeleteStatus::DependencyBlocked) {
                report.error=item.error?item.error:ERROR_INVALID_DATA;++n;break;
            }
        }
        for(;n<report.items.size();++n)append(resultLine(n));
        const bool complete=!report.error && !report.cancelled && !report.stale &&
            std::all_of(report.items.begin(),report.items.end(),[](const DeleteOutcome& item){return item.status==DeleteStatus::Deleted;});
        append(L"{\"event\":\"end\",\"status\":"+Json(report.error?(report.cancelled?L"cancelled":L"failed"):complete?L"complete":L"incomplete")+L"}");
        report.complete=complete;
    }catch(const Failure& failure) {
        report.error=failure.error;report.cancelled=cancel.load() || failure.error==ERROR_CANCELLED || failure.error==ERROR_OPERATION_ABORTED || failure.error==ERROR_REQUEST_ABORTED;
    }catch(...){report.error=ERROR_NOT_ENOUGH_MEMORY;}
    Phase(progress,CopyPhase::Finished,L"");return report;
}
bool IsFileOverwrite(const Row& row) { return IsOriginalToSave(row); }
OverwriteReport OverwriteFiles(Result snapshot,const std::vector<size_t>& visible,bool keepOriginals,
    const std::wstring& recoveryRoot,std::atomic<bool>& cancel,Progress& checking,CopyProgress& progress)
{
    OverwriteReport report;report.keepOriginals=keepOriginals;
    std::wstring left,right;File journal;
    using RecoveryKey=std::pair<size_t,bool>;
    std::map<RecoveryKey,const RecoveryItem*> originals;
    try {
        std::set<size_t> unique;std::vector<size_t> selected;
        for(size_t i:visible) {
            if(i>=snapshot.rows.size() || !unique.insert(i).second)continue;
            const auto& row=snapshot.rows[i];
            if(IsFileOverwrite(row)){report.items.push_back({i});selected.push_back(i);}
            else if(row.action!=Action::None)++report.omitted;
        }
        if(report.items.empty())return report;
        Control(cancel,progress);
        if(!SnapshotRoots(snapshot,left,right) || !Revalidate(snapshot,cancel,checking)) {
            report.stale=true;report.cancelled=cancel.load();return report;
        }
        for(auto& item:report.items) {
            const auto& row=snapshot.rows[item.row];const bool toRight=row.action==Action::CopyRight;
            item.target=Join(toRight?right:left,row.path);
            // Preserve the existing preflight for noninteractive commands.
            // Interactive copies can Skip this row without changing its flags.
            if((progress.errors.stopOnFirstError || !progress.errors.prompt) &&
                ((toRight?row.right:row.left).attributes & FILE_ATTRIBUTE_READONLY)) {
                item.status=CopyStatus::Failed;item.error=item.lastError=ERROR_ACCESS_DENIED;throw Failure{item.error};
            }
        }
        if(keepOriginals) {
            report.recovery=SaveOriginals(snapshot,selected,recoveryRoot,cancel,checking,progress);
            for(const auto& saved:report.recovery.items)
                if(!originals.emplace(RecoveryKey{saved.row,saved.right},&saved).second)throw Failure{ERROR_INVALID_DATA};
            for(auto& item:report.items) {
                const bool rightSide=snapshot.rows[item.row].action==Action::CopyRight;
                const auto found=originals.find({item.row,rightSide});
                if(found==originals.end() || found->second->copy.row!=item.row)throw Failure{ERROR_INVALID_DATA};
                const auto& saved=*found->second;
                if(saved.copy.status==CopyStatus::Skipped && saved.logged) {
                    item.status=CopyStatus::DependencyBlocked;item.error=item.lastError=saved.copy.error;
                    item.automaticSkip=saved.copy.automaticSkip;
                } else if(report.recovery.processed && (!saved.logged || saved.copy.status!=CopyStatus::Copied || !saved.savedEntry.identified))
                    throw Failure{ERROR_INVALID_DATA};
            }
            if(!report.recovery.processed || report.recovery.items.size()!=report.items.size()) {
                report.stale=report.recovery.stale;report.cancelled=report.recovery.cancelled;
                report.error=report.recovery.error?report.recovery.error:ERROR_INVALID_DATA;return report;
            }
            // Saving originals makes no confirmed target changes. Check every
            // still-eligible source/target again before any replacement.
            std::vector<size_t> pending;
            for(const auto& item:report.items)if(item.status!=CopyStatus::DependencyBlocked)pending.push_back(item.row);
            if(!AdvanceExecutionSnapshot(snapshot,pending,cancel,checking)) {
                report.cancelled=cancel.load();report.stale=!report.cancelled;
                report.error=report.cancelled?ERROR_CANCELLED:ERROR_FILE_INVALID;return report;
            }
            report.journal=Join(report.recovery.runDirectory,L"overwrite.jsonl");
            journal.h=Open(report.journal,GENERIC_WRITE,FILE_SHARE_READ,CREATE_NEW);
            if(journal.h==INVALID_HANDLE_VALUE){report.journalError=GetLastError();throw Failure{report.journalError};}
        }
        auto append=[&](const std::wstring& line) {
            if(!keepOriginals)return;
            try {RecoveryLine(journal.h,line);}
            catch(const Failure& failure){report.journalError=failure.error;throw;}
        };
        append(L"{\"event\":\"run\",\"schema\":1,\"operation\":\"overwrite\",\"keepOriginals\":true,\"left\":"+
            Json(left)+L",\"right\":"+Json(right)+L",\"recoveryManifest\":"+Json(report.recovery.journal)+L"}");
        for(size_t n=0;n<report.items.size();++n) {
            const auto& item=report.items[n];const auto& row=snapshot.rows[item.row];const bool toRight=row.action==Action::CopyRight;
            append(L"{\"event\":\"planned\",\"item\":"+std::to_wstring(n+1)+L",\"source\":"+Json(Join(toRight?left:right,row.path))+
                L",\"target\":"+Json(item.target)+L",\"saved\":"+Json(keepOriginals?originals.at({item.row,toRight})->saved:L"")+L"}");
        }
        size_t n=0;
        for(;n<report.items.size();++n) {
            auto& item=report.items[n];const auto& row=snapshot.rows[item.row];const bool toRight=row.action==Action::CopyRight;
            progress.item=n+(keepOriginals?report.items.size():0);
            if(item.status!=CopyStatus::DependencyBlocked) {
                ExecuteCopyItem(item,TransferKind::Overwrite,Join(toRight?left:right,row.path),item.target,snapshot,
                    cancel,checking,progress,report.cancelled,report.stale,[&] {
                        progress.BeginDisplay(Join(toRight?left:right,row.path),item.target,TransferKind::Overwrite,false,(toRight?row.left:row.right).size);
                        progress.errorSite=ErrorSite::Target;
                        if(keepOriginals) {
                            const auto& saved=*originals.at({item.row,toRight});
                            Parents(report.recovery.runDirectory,saved.saved,false,cancel);
                            File copy(Open(saved.saved,FILE_READ_ATTRIBUTES));
                            if(copy.h==INVALID_HANDLE_VALUE || !Unchanged(saved.savedEntry,Info(copy.h)))throw Failure{ERROR_FILE_INVALID};
                        }
                        Security originalSecurity(item.target);const Entry& original=toRight?row.right:row.left;
                        CopyOne(toRight?left:right,toRight?right:left,row,item,cancel,progress,&original,&originalSecurity);
                    },[&](bool retry){std::vector<size_t> pending;
                        for(size_t i=n+(retry?0:1);i<report.items.size();++i)
                            if(report.items[i].status!=CopyStatus::DependencyBlocked)pending.push_back(report.items[i].row);
                        return pending;});
            }
            append(OutcomeLine(n,item,L"replaced"));
            if(item.status!=CopyStatus::Copied && item.status!=CopyStatus::Skipped && item.status!=CopyStatus::DependencyBlocked) {
                report.error=item.error?item.error:ERROR_INVALID_DATA;++n;break;
            }
        }
        for(;n<report.items.size();++n)append(OutcomeLine(n,report.items[n],L"replaced"));
        const bool complete=!report.error && !report.cancelled && !report.stale &&
            std::all_of(report.items.begin(),report.items.end(),[](const CopyOutcome& item){return item.status==CopyStatus::Copied;});
        append(L"{\"event\":\"end\",\"status\":"+Json(report.error?(report.cancelled?L"cancelled":L"failed"):complete?L"complete":L"incomplete")+L"}");
        report.complete=complete;
    }catch(const Failure& failure) {
        report.error=failure.error;
        report.cancelled=cancel.load() || failure.error==ERROR_CANCELLED || failure.error==ERROR_OPERATION_ABORTED || failure.error==ERROR_REQUEST_ABORTED;
    }catch(...) {report.error=ERROR_NOT_ENOUGH_MEMORY;}
    Phase(progress,CopyPhase::Finished,L"");return report;
}
CopyReport CopyNewFiles(Result snapshot,const std::vector<size_t>& visible,std::atomic<bool>& cancel,Progress& checking,CopyProgress& progress)
{
    CopyReport report; std::wstring left,right;
    std::set<size_t> unique;
    for (size_t i:visible) {
        if (i>=snapshot.rows.size() || !unique.insert(i).second) continue;
        const auto& r=snapshot.rows[i];
        if (IsNewCopy(r)) report.items.push_back({i});
        else if (r.action!=Action::None) ++report.omitted;
    }
    if (report.items.empty()) return report;
    if(!progress.Checkpoint(cancel)) {report.cancelled=true;return report;}
    if (!SnapshotRoots(snapshot,left,right) || !Revalidate(snapshot,cancel,checking)) {
        report.stale=true; report.cancelled=cancel.load(); return report;
    }
    for (size_t n=0;n<report.items.size();++n) {
        auto& outcome=report.items[n]; const auto& row=snapshot.rows[outcome.row];
        bool toRight=row.action==Action::CopyRight; outcome.target=Join(toRight?right:left,row.path); progress.item=n;
        ExecuteCopyItem(outcome,TransferKind::Copy,Join(toRight?left:right,row.path),outcome.target,
            snapshot,cancel,checking,progress,report.cancelled,report.stale,[&] {
                if((toRight?row.left:row.right).directory)CopyEmptyDirectory(toRight?left:right,toRight?right:left,row,outcome,cancel,progress);
                else CopyOne(toRight?left:right,toRight?right:left,row,outcome,cancel,progress);
            },[&](bool retry){std::vector<size_t> pending;
                for(size_t i=n+(retry?0:1);i<report.items.size();++i)pending.push_back(report.items[i].row);return pending;});
        if(outcome.status!=CopyStatus::Copied && outcome.status!=CopyStatus::Skipped)break;
    }
    Phase(progress,CopyPhase::Finished,L""); return report;
}
SyncPlan BuildSyncPlan(const Result& snapshot,const std::vector<size_t>& visible,SyncSelection selection)
{
    SyncPlan plan;const auto eligibleDeletes=DeletionRows(snapshot,visible);
    std::set<size_t> unique,deletions(eligibleDeletes.begin(),eligibleDeletes.end());
    if(selection.deleteRight)plan.deletions=eligibleDeletes;
    for(size_t i:visible) if(i<snapshot.rows.size() && unique.insert(i).second) {
        const auto& row=snapshot.rows[i];
        const bool copy=IsNewCopy(row),overwrite=IsFileOverwrite(row),remove=deletions.count(i)!=0;
        const bool allowed=remove?selection.deleteRight:row.action==Action::CopyRight?selection.toRight:selection.toLeft;
        if((copy || overwrite || remove) && !allowed)plan.excluded.push_back(i);
        else if(copy)plan.copies.push_back(i);
        else if(overwrite)plan.overwrites.push_back(i);
        else if(!remove)++plan.omitted;
    }
    return plan;
}
SyncReport Synchronize(Result snapshot,const std::vector<size_t>& visible,bool keepOriginals,
    const std::wstring& recoveryRoot,std::atomic<bool>& cancel,Progress& checking,CopyProgress& progress,std::atomic<SyncStage>& stage,SyncSelection selection)
{
    // One user command spans every phase. Retain matching Skip all decisions
    // across its adapters, but never inherit them from a previous command.
    progress.errors.skipped.clear();
    SyncReport report;report.plan=BuildSyncPlan(snapshot,visible,selection);report.keepOriginals=keepOriginals;stage=SyncStage::Checking;
    auto finish=[&]() {
        report.cancelled=report.cancelled || cancel.load();
        if(report.cancelled){report.complete=false;if(!report.error)report.error=ERROR_CANCELLED;}
        stage=SyncStage::Finished;Phase(progress,CopyPhase::Finished,L"");report.finished=true;return std::move(report);
    };
    try {
        if(!report.plan.Count()) {report.complete=true;return finish();}
        Control(cancel,progress);
        if(!Revalidate(snapshot,cancel,checking)) {report.stale=true;report.error=ERROR_FILE_INVALID;return finish();}
        if(keepOriginals && (!report.plan.overwrites.empty() || !report.plan.deletions.empty())) {
            std::wstring root,left,right;
            if(!NormalizeRoot(recoveryRoot,root) || !SnapshotRoots(snapshot,left,right)) throw Failure{ERROR_INVALID_NAME};
            root=RecoveryCanonical(root,cancel);
            for(const auto& input:{RecoveryCanonical(left,cancel),RecoveryCanonical(right,cancel)})
                if(RecoveryWithin(root,input) || RecoveryWithin(input,root)) throw Failure{ERROR_INVALID_PARAMETER};
        }
        auto checkpoint=[&](const std::vector<size_t>& pending) {
            if(!progress.Checkpoint(cancel)) {report.cancelled=true;report.error=ERROR_CANCELLED;return false;}
            if(!AdvanceExecutionSnapshot(snapshot,pending,cancel,checking)) {
                report.cancelled=cancel.load();report.stale=!report.cancelled;
                report.error=report.cancelled?ERROR_CANCELLED:ERROR_FILE_INVALID;return false;
            }
            return true;
        };
        bool allCompleted=true;
        auto copySettled=[&](const std::vector<CopyOutcome>& items,size_t expected) {
            bool settled=items.size()==expected;
            for(const auto& item:items) {
                if(item.status!=CopyStatus::Copied)allCompleted=false;
                if(item.status!=CopyStatus::Copied && item.status!=CopyStatus::Skipped && item.status!=CopyStatus::DependencyBlocked) {
                    settled=false;if(!report.error)report.error=item.error?item.error:ERROR_FILE_INVALID;
                }
            }
            if(!settled && !report.error)report.error=ERROR_FILE_INVALID;return settled;
        };
        if(!report.plan.copies.empty()) {
            stage=SyncStage::NewItems;
            report.copies=CopyNewFiles(snapshot,report.plan.copies,cancel,checking,progress);
            report.stale=report.copies.stale;report.cancelled=report.copies.cancelled;
            const bool settled=copySettled(report.copies.items,report.plan.copies.size());
            if(report.stale || report.cancelled || !settled) {
                if(!report.error)report.error=report.cancelled?ERROR_CANCELLED:ERROR_FILE_INVALID;return finish();
            }
        }
        if(!report.plan.overwrites.empty()) {
            auto pending=report.plan.overwrites;pending.insert(pending.end(),report.plan.deletions.begin(),report.plan.deletions.end());
            if(!checkpoint(pending)) return finish();stage=SyncStage::Overwrites;
            report.overwrites=OverwriteFiles(snapshot,report.plan.overwrites,keepOriginals,recoveryRoot,cancel,checking,progress);
            report.stale=report.overwrites.stale;report.cancelled=report.overwrites.cancelled;
            report.error=report.overwrites.error;
            const bool settled=copySettled(report.overwrites.items,report.plan.overwrites.size());
            if(report.stale || report.cancelled || report.error || !settled) {
                if(!report.error)report.error=report.cancelled?ERROR_CANCELLED:ERROR_FILE_INVALID;return finish();
            }
        }
        if(!report.plan.deletions.empty()) {
            if(!checkpoint(report.plan.deletions)) return finish();stage=SyncStage::Deletions;
            report.deletions=DeleteFiles(snapshot,report.plan.deletions,keepOriginals,recoveryRoot,cancel,checking,progress);
            report.stale=report.deletions.stale;report.cancelled=report.deletions.cancelled;
            report.error=report.deletions.error;bool settled=report.deletions.items.size()==report.plan.deletions.size();
            for(const auto& item:report.deletions.items) {
                if(item.status!=DeleteStatus::Deleted)allCompleted=false;
                if(item.status!=DeleteStatus::Deleted && item.status!=DeleteStatus::Skipped && item.status!=DeleteStatus::DependencyBlocked) {
                    settled=false;if(!report.error)report.error=item.error?item.error:ERROR_FILE_INVALID;
                }
            }
            if(report.stale || report.cancelled || report.error || !settled) {
                if(!report.error)report.error=report.cancelled?ERROR_CANCELLED:ERROR_FILE_INVALID;return finish();
            }
        }
        report.complete=allCompleted && !cancel.load();
    } catch(const Failure& failure) {report.error=failure.error;}
    catch(...) {report.error=ERROR_NOT_ENOUGH_MEMORY;}
    return finish();
}
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
void SetCopyHook(std::function<void(CopyPhase,const std::wstring&)> value) { hook=std::move(value); }
#endif
} // namespace dirsync
