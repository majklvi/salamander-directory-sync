// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "copy_space_faults.h"
#include <map>
namespace dirsync_test {
namespace {
SpaceFault fault=SpaceFault::None;
DWORD errorCode=ERROR_DISK_FULL;
unsigned ordinal=1,reservations=0,copies=0;
bool reached=false;
HANDLE retained=INVALID_HANDLE_VALUE;
std::map<HANDLE,std::wstring> outputs;
std::wstring retainAfterClose;
}
void ReleaseSpaceFault() { if(retained!=INVALID_HANDLE_VALUE) { ::CloseHandle(retained); retained=INVALID_HANDLE_VALUE; } }
void SetSpaceFault(SpaceFault value,DWORD error,unsigned item)
{ ReleaseSpaceFault(); fault=value; errorCode=error; ordinal=item; reservations=0; copies=0; reached=false;outputs.clear();retainAfterClose.clear(); }
bool SpaceFaultReached() { return reached; }
HANDLE WINAPI ReserveFile(LPCWSTR path,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES security,DWORD disposition,DWORD flags,HANDLE templateFile)
{
    const std::wstring name(path);const bool stage=name.find(L"\\.dirsync-")!=std::wstring::npos;
    const bool main=name.size()>=4 && name.compare(name.size()-4,4,L".tmp")==0;
    if(stage && main && disposition==CREATE_NEW &&
       ++reservations==ordinal && fault==SpaceFault::Reserve) {
        reached=true; SetLastError(errorCode); return INVALID_HANDLE_VALUE;
    }
    HANDLE file=::CreateFileW(path,access,share,security,disposition,flags,templateFile);
    if(file!=INVALID_HANDLE_VALUE && stage && access==GENERIC_WRITE){
        if(main && disposition==OPEN_EXISTING)++copies;
        outputs[file]=name;
    }
    return file;
}
BOOL WINAPI TransferBytes(HANDLE file,LPCVOID data,DWORD size,LPDWORD written,LPOVERLAPPED overlapped)
{
    const auto out=outputs.find(file);
    const bool ads=out!=outputs.end() && out->second.find(L".tmp:")!=std::wstring::npos;
    const bool fail=out!=outputs.end() && copies==ordinal &&
        ((ads && fault==SpaceFault::NamedStream) || (!ads && (fault==SpaceFault::MainData || fault==SpaceFault::RetainStage)));
    const BOOL result=::WriteFile(file,data,size,written,overlapped);
    if(!result || !fail)return result;
    // Leave genuine partial bytes in the owned stage, then report an OS write
    // failure. This is fault injection, not a real full-volume test.
    reached=true;if(fault==SpaceFault::RetainStage)retainAfterClose=out->second;
    SetLastError(errorCode);return FALSE;
}
BOOL WINAPI CloseFile(HANDLE file)
{
    const auto out=outputs.find(file);const bool hold=out!=outputs.end() && out->second==retainAfterClose;
    outputs.erase(file);const BOOL result=::CloseHandle(file);
    if(hold){retained=::CreateFileW(retainAfterClose.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);retainAfterClose.clear();}
    return result;
}
BOOL WINAPI PublishFile(LPCWSTR source,LPCWSTR target,DWORD flags)
{
    if(fault==SpaceFault::Publish && copies==ordinal) { reached=true; SetLastError(errorCode); return FALSE; }
    return ::MoveFileExW(source,target,flags);
}
}
// Only this test executable redirects the real adapter's four WinAPI calls.
// Production and lifecycle projects compile ../src/copy.cpp directly.
#define CreateFileW ::dirsync_test::ReserveFile
#define WriteFile ::dirsync_test::TransferBytes
#define CloseHandle ::dirsync_test::CloseFile
#define MoveFileExW ::dirsync_test::PublishFile
#include "../src/copy.cpp"
