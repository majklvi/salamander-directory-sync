// SPDX-License-Identifier: GPL-2.0-or-later
#include "precomp.h"
#include "recovery_catalog.h"
#include <map>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace dirsync {
namespace {
constexpr size_t FileLimit=8*1024*1024,ItemLimit=20000,RunLimit=1000;
struct Bad {DWORD code;};
void Need(bool ok,DWORD code=ERROR_INVALID_DATA) {if(!ok) throw Bad{code};}
bool Equal(const std::wstring& a,const std::wstring& b) {
    return CompareStringOrdinal(a.c_str(),int(a.size()),b.c_str(),int(b.size()),TRUE)==CSTR_EQUAL;
}
std::wstring Full(const std::wstring& path) {
    Need(!path.empty() && path.find(L'\0')==std::wstring::npos);
    Need((path.size()>2 && path[1]==L':' && path[2]==L'\\') || path.rfind(L"\\\\",0)==0);
    std::wstring result;Need(NormalizeRoot(path,result));return result;
}
bool Within(const std::wstring& path,const std::wstring& root) {
    return path.size()>root.size() && Equal(path.substr(0,root.size()),root) &&
        (root.back()==L'\\' || path[root.size()]==L'\\');
}
bool Plain(const std::wstring& path) {
    // Inspect every existing component. A copied/moved recovery tree may contain
    // a junction even when the final file itself is an ordinary file.
    auto full=Full(path);size_t begin=3;
    if(full.rfind(L"\\\\",0)==0) {
        begin=full.find(L'\\',full.find(L'\\',2)+1);
        if(begin==std::wstring::npos) begin=full.size();
    }
    for(size_t n=begin;;) {
        auto part=full.substr(0,n);DWORD a=GetFileAttributesW(ApiPath(part).c_str());
        if(a==INVALID_FILE_ATTRIBUTES || (a&FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        if(n==full.size()) return true;
        n=full.find(L'\\',n+1);if(n==std::wstring::npos) n=full.size();
    }
}
struct Value {std::wstring text;bool string=false;};
using Object=std::map<std::wstring,Value>;
class Json {
    const std::wstring& s;size_t n=0;
    void Space(){while(n<s.size() && (s[n]==L' ' || s[n]==L'\r' || s[n]==L'\t')) ++n;}
    bool Take(wchar_t c){Space();if(n<s.size() && s[n]==c){++n;return true;}return false;}
    unsigned Hex(){unsigned value=0;for(int i=0;i<4;++i){Need(n<s.size());auto c=s[n++];value*=16;
        if(c>=L'0' && c<=L'9') value+=c-L'0';else if(c>=L'a' && c<=L'f') value+=c-L'a'+10;
        else if(c>=L'A' && c<=L'F') value+=c-L'A'+10;else throw Bad{ERROR_INVALID_DATA};}return value;}
    std::wstring String(){Need(Take(L'"'));std::wstring out;
        while(n<s.size()) {wchar_t c=s[n++];if(c==L'"') return out;Need(c>=32);
            if(c==L'\\') {Need(n<s.size());c=s[n++];switch(c){
            case L'"':case L'\\':case L'/':break;case L'b':c=L'\b';break;case L'f':c=L'\f';break;
            case L'n':c=L'\n';break;case L'r':c=L'\r';break;case L't':c=L'\t';break;
            case L'u': {unsigned u=Hex();Need(u!=0);c=wchar_t(u);
                if(u>=0xd800 && u<=0xdbff){Need(n+2<=s.size() && s[n]==L'\\' && s[n+1]==L'u');n+=2;
                    unsigned low=Hex();Need(low>=0xdc00 && low<=0xdfff);out+=c;out+=wchar_t(low);continue;}
                Need(u<0xdc00 || u>0xdfff);break;}
            default:throw Bad{ERROR_INVALID_DATA};}}
            Need(c!=0);out+=c;
        }throw Bad{ERROR_INVALID_DATA};}
public:
    explicit Json(const std::wstring& text):s(text){}
    Object Read(){Object result;Need(Take(L'{'));if(Take(L'}')){Space();Need(n==s.size());return result;}
        do {auto key=String();Need(Take(L':'));Space();Value value;
            if(n<s.size() && s[n]==L'"'){value.string=true;value.text=String();}
            else {size_t start=n;while(n<s.size() && s[n]!=L',' && s[n]!=L'}' && s[n]!=L' ' && s[n]!=L'\r' && s[n]!=L'\t')++n;
                value.text=s.substr(start,n-start);Need(!value.text.empty());
                if(value.text!=L"true" && value.text!=L"false" && value.text!=L"null") {
                    Need(value.text.size()==1 || value.text[0]!=L'0');for(auto c:value.text)Need(c>=L'0' && c<=L'9');}}
            Need(result.emplace(key,value).second);
        }while(Take(L','));Need(Take(L'}'));Space();Need(n==s.size());return result;}
};
std::wstring Text(const Object& o,const wchar_t* key) {auto i=o.find(key);Need(i!=o.end() && i->second.string);return i->second.text;}
uint64_t Number(const Object& o,const wchar_t* key) {auto i=o.find(key);Need(i!=o.end() && !i->second.string);uint64_t value=0;
    Need(!i->second.text.empty());for(auto c:i->second.text){Need(c>=L'0' && c<=L'9');unsigned digit=c-L'0';
        Need(value<=(std::numeric_limits<uint64_t>::max()-digit)/10);value=value*10+digit;}return value;}
bool Flag(const Object& o,const wchar_t* key) {auto i=o.find(key);Need(i!=o.end() && !i->second.string && (i->second.text==L"true" || i->second.text==L"false"));return i->second.text==L"true";}
struct Handle {HANDLE h;~Handle(){if(h!=INVALID_HANDLE_VALUE) CloseHandle(h);}};
std::wstring CanonicalDirectory(const std::wstring& path) {
    Handle directory{CreateFileW(ApiPath(path).c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr)};
    Need(directory.h!=INVALID_HANDLE_VALUE,GetLastError());
    BY_HANDLE_FILE_INFORMATION info{};Need(GetFileInformationByHandle(directory.h,&info)!=FALSE,GetLastError());
    Need((info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && !(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT),ERROR_DIRECTORY);
    DWORD size=GetFinalPathNameByHandleW(directory.h,nullptr,0,VOLUME_NAME_DOS);Need(size!=0,GetLastError());
    std::vector<wchar_t> buffer(size_t(size)+1);
    DWORD used=GetFinalPathNameByHandleW(directory.h,buffer.data(),DWORD(buffer.size()),VOLUME_NAME_DOS);
    Need(used && used<buffer.size(),GetLastError());return Full(std::wstring(buffer.data(),used));
}
std::vector<Object> Lines(const std::wstring& path,std::atomic<bool>& cancel,bool optional=false,DWORD* warning=nullptr) {
    Handle file{CreateFileW(ApiPath(path).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,
        OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr)};
    if(file.h==INVALID_HANDLE_VALUE){DWORD error=GetLastError();if(optional && error==ERROR_FILE_NOT_FOUND)return {};throw Bad{error};}
    BY_HANDLE_FILE_INFORMATION info{};if(!GetFileInformationByHandle(file.h,&info))throw Bad{GetLastError()};
    Need(!(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)),ERROR_ACCESS_DENIED);
    std::string bytes;char block[65536];DWORD read=0;
    for(;;){Need(!cancel,ERROR_CANCELLED);if(!ReadFile(file.h,block,sizeof(block),&read,nullptr))throw Bad{GetLastError()};if(!read)break;
        Need(bytes.size()+read<=FileLimit,ERROR_FILE_TOO_LARGE);bytes.append(block,read);}
    Need(!bytes.empty());
    if(bytes.back()!='\n') {if(warning)*warning=ERROR_HANDLE_EOF;auto end=bytes.find_last_of('\n');Need(end!=std::string::npos);bytes.resize(end+1);}
    if(bytes.rfind("\xef\xbb\xbf",0)==0)bytes.erase(0,3);
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(bytes.size()),nullptr,0);Need(count>0);
    std::wstring wide(size_t(count),L'\0');Need(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(bytes.size()),wide.data(),count)==count);
    std::vector<Object> result;size_t start=0;
    while(start<wide.size()){Need(!cancel,ERROR_CANCELLED);auto end=wide.find(L'\n',start);Need(end!=std::wstring::npos && end-start<=1024*1024);
        auto line=wide.substr(start,end-start);
        if(!line.empty() && line!=L"\r") {try{result.push_back(Json(line).Read());}catch(const Bad& e){if(warning)*warning=e.code;break;}}
        start=end+1;
        Need(result.size()<=2*ItemLimit+2,ERROR_FILE_TOO_LARGE);}
    Need(!result.empty());return result;
}
void ReadRun(StoredRun& run,std::atomic<bool>& cancel,size_t& total) {
    auto manifest=Lines(run.path+L"\\manifest.jsonl",cancel,false,&run.error);auto& header=manifest.front();
    Need(Text(header,L"event")==L"run" && Number(header,L"schema")==1 && Text(header,L"operation")==L"save_originals_only",ERROR_REVISION_MISMATCH);
    run.left=Full(Text(header,L"left"));run.right=Full(Text(header,L"right"));
    Need(Equal(Full(Text(header,L"runDirectory")),run.path));std::map<uint64_t,size_t> ids;std::map<uint64_t,bool> results;
    bool ended=false;
    for(size_t i=1;i<manifest.size();++i) {
        Need(!cancel,ERROR_CANCELLED);Need(!ended);const auto& record=manifest[i];auto event=Text(record,L"event");
        if(event==L"planned") {
            Need(total<ItemLimit,ERROR_FILE_TOO_LARGE);StoredOriginal item;item.item=Number(record,L"item");Need(item.item!=0 && !ids.count(item.item));
            item.original=Full(Text(record,L"original"));item.saved=Full(Text(record,L"saved"));
            auto kind=record.count(L"kind")?Text(record,L"kind"):L"file";Need(kind==L"file" || kind==L"directory");item.directory=kind==L"directory";item.size=Number(record,L"size");
            Need(Within(item.original,run.left) || Within(item.original,run.right));
            if(!Within(item.saved,run.path))item.copy=StoredCopy::Unsafe;
            ids[item.item]=run.items.size();run.items.push_back(std::move(item));++total;
        } else if(event==L"result") {
            auto id=Number(record,L"item");Need(ids.count(id) && !results.count(id));results[id]=true;auto& item=run.items[ids.at(id)];
            auto error=Number(record,L"error");Need(error<=MAXDWORD);item.error=DWORD(error);
            if(Text(record,L"status")==L"saved" && !item.error && Flag(record,L"published") && item.copy!=StoredCopy::Unsafe)item.copy=StoredCopy::Recorded;
            else if(Text(record,L"status")==L"skipped" && !Flag(record,L"published") && item.copy!=StoredCopy::Unsafe)item.copy=StoredCopy::Skipped;
        } else if(event==L"end") {ended=true;run.complete=Text(record,L"status")==L"complete";}
        else throw Bad{ERROR_INVALID_DATA};
    }
    if(run.complete)Need(results.size()==run.items.size() && std::all_of(run.items.begin(),run.items.end(),[](const StoredOriginal& item){return item.copy==StoredCopy::Recorded || item.copy==StoredCopy::Unsafe;}));
    for(auto& item:run.items) if(item.copy==StoredCopy::Recorded) {
        if(!Plain(item.saved)){item.copy=GetFileAttributesW(ApiPath(item.saved).c_str())==INVALID_FILE_ATTRIBUTES?StoredCopy::Missing:StoredCopy::Unsafe;continue;}
        WIN32_FILE_ATTRIBUTE_DATA data{};
        if(!GetFileAttributesExW(ApiPath(item.saved).c_str(),GetFileExInfoStandard,&data)){item.copy=StoredCopy::Missing;continue;}
        uint64_t size=(uint64_t(data.nFileSizeHigh)<<32)|data.nFileSizeLow;
        if(bool(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=item.directory || (!item.directory && size!=item.size))item.copy=StoredCopy::Different;
    }
    for(const auto& journal:{std::wstring(L"overwrite"),std::wstring(L"delete")}) {
        DWORD warning=0;auto records=Lines(run.path+L"\\"+journal+L".jsonl",cancel,true,&warning);if(warning)run.error=warning;if(records.empty())continue;
        Need(Text(records[0],L"event")==L"run" && Number(records[0],L"schema")==1);
        Need(Text(records[0],L"operation")== (journal==L"delete"?L"delete_right":L"overwrite"));
        Need(Equal(Full(Text(records[0],L"left")),run.left) && Equal(Full(Text(records[0],L"right")),run.right));
        std::map<uint64_t,size_t> targets;std::map<uint64_t,bool> seen;bool journalEnd=false;
        for(size_t i=1;i<records.size();++i) {
            Need(!cancel,ERROR_CANCELLED);Need(!journalEnd);const auto& rec=records[i];auto event=Text(rec,L"event");
            if(event==L"end"){journalEnd=true;continue;}
            auto id=Number(rec,L"item");
            if(event==L"planned") {
                Need(!targets.count(id));auto target=Full(Text(rec,L"target"));auto saved=Full(Text(rec,L"saved"));
                auto found=std::find_if(run.items.begin(),run.items.end(),[&](const StoredOriginal& item){return Equal(item.original,target) && Equal(item.saved,saved);});
                Need(found!=run.items.end());targets[id]=size_t(found-run.items.begin());
            } else if(event==L"result") {
                Need(targets.count(id) && !seen.count(id));seen[id]=true;auto& item=run.items[targets.at(id)];auto status=Text(rec,L"status");
                auto error=Number(rec,L"error");Need(error<=MAXDWORD);item.error=DWORD(error);
                if(status==L"replaced" && !error && journal==L"overwrite" && Flag(rec,L"published"))item.operation=StoredOperation::Replaced;
                else if(status==L"deleted" && !error && journal==L"delete" && Flag(rec,L"removed"))item.operation=StoredOperation::Deleted;
                else if(status==L"not_started")item.operation=StoredOperation::NotStarted;
                else if(status==L"failed")item.operation=StoredOperation::Failed;
                else if(status==L"cancelled")item.operation=StoredOperation::Cancelled;
                else if(status==L"skipped" && journal==L"overwrite" && !Flag(rec,L"published"))item.operation=StoredOperation::Skipped;
                else if(status==L"dependency_blocked" && journal==L"overwrite" && !Flag(rec,L"published"))item.operation=StoredOperation::DependencyBlocked;
                else throw Bad{ERROR_INVALID_DATA};
            } else throw Bad{ERROR_INVALID_DATA};
        }
        // A missing journal end is not proof of whether a write took place.
        if(!journalEnd)run.error=ERROR_HANDLE_EOF;
    }
}
template<class Callback> void Directories(const std::wstring& root,const wchar_t* mask,std::atomic<bool>& cancel,Callback callback) {
    WIN32_FIND_DATAW data{};HANDLE find=FindFirstFileW(ApiPath(root+L"\\"+mask).c_str(),&data);
    if(find==INVALID_HANDLE_VALUE){auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND)return;throw Bad{error};}
    struct CloseFind {HANDLE h;~CloseFind(){FindClose(h);}} close{find};
    do {Need(!cancel,ERROR_CANCELLED);if((data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && !(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))callback(data);}
    while(FindNextFileW(find,&data));Need(GetLastError()==ERROR_NO_MORE_FILES,GetLastError());
}
}
RecoveryCatalog ReadRecoveryCatalog(const std::wstring& input,std::atomic<bool>& cancel) {
    RecoveryCatalog result;size_t total=0;
    try {
        auto root=Full(input);Need(Plain(root),ERROR_ACCESS_DENIED);
        // The writer records a handle-normalized root. Match that spelling even
        // when Windows TEMP or the user's setting contains an ordinary 8.3 alias.
        root=CanonicalDirectory(root);
        Directories(root,L"pair-*",cancel,[&](const WIN32_FIND_DATAW& pair){
            auto path=root+L"\\"+pair.cFileName;
            Directories(path,L"run-*",cancel,[&](const WIN32_FIND_DATAW& data){
                if(result.runs.size()>=RunLimit || total>=ItemLimit){result.limited=true;throw Bad{ERROR_MORE_DATA};}
                StoredRun run;run.path=path+L"\\"+data.cFileName;run.created=data.ftCreationTime;
                try{ReadRun(run,cancel,total);}catch(const Bad& e){run.error=e.code;if(e.code==ERROR_FILE_TOO_LARGE)result.limited=true;}
                result.runs.push_back(std::move(run));
            });
        });
    }catch(const Bad& e){result.error=e.code;}catch(...){result.error=ERROR_NOT_ENOUGH_MEMORY;}
    result.cancelled=cancel.load();
    std::stable_sort(result.runs.begin(),result.runs.end(),[](const StoredRun& a,const StoredRun& b){return CompareFileTime(&a.created,&b.created)>0;});
    return result;
}
bool RecoveryLocation(const std::wstring& input,bool directory,std::wstring& folder) {
    try{auto path=Full(input);if(!directory){auto split=path.find_last_of(L'\\');path=path.substr(0,split==2?3:split);}
        if(!Plain(path))return false;auto attrs=GetFileAttributesW(ApiPath(path).c_str());
        if(attrs==INVALID_FILE_ATTRIBUTES || !(attrs&FILE_ATTRIBUTE_DIRECTORY))return false;folder=path;return true;
    }catch(...){return false;}
}
bool RecoverySavedLocation(const std::wstring& run,const std::wstring& input,bool directory,std::wstring& folder) {
    try{auto path=Full(input);auto root=Full(run);if(!Within(path,root) || !Plain(root))return false;
        return RecoveryLocation(path,directory,folder);
    }catch(...){return false;}
}
}
