// SPDX-License-Identifier: GPL-2.0-or-later
#include "precomp.h"
#include "report.h"
#include "shell_delete.h"
#include "recovery_catalog.h"
#include "versinfo.rh2"
#include <algorithm>
#include <map>
#include <set>
#include <limits>
#include <stdexcept>
#include <climits>
namespace dirsync {
namespace {
std::wstring N(uint64_t n){return std::to_wstring(n);}
std::wstring B(bool n){return n?L"true":L"false";}
std::wstring Stamp(uint64_t value) {
    FILETIME ft{DWORD(value),DWORD(value>>32)};SYSTEMTIME time{};if(!FileTimeToSystemTime(&ft,&time))return {};
    wchar_t text[64]{};swprintf_s(text,L"%04u-%02u-%02uT%02u:%02u:%02u.%07lluZ",time.wYear,time.wMonth,time.wDay,
        time.wHour,time.wMinute,time.wSecond,static_cast<unsigned long long>(value%10000000));return text;
}
std::wstring Path(const std::wstring& root,const Row& row){return root+(!root.empty() && root.back()==L'\\'?L"":L"\\")+row.path;}
bool CopyDone(const CopyOutcome& item){return item.status==CopyStatus::Copied && item.published && !item.error;}
const wchar_t* CopyCode(CopyStatus status){switch(status){case CopyStatus::Copied:return L"copied";
    case CopyStatus::Cancelled:return L"cancelled";case CopyStatus::Failed:return L"failed";
    case CopyStatus::Skipped:return L"skipped";case CopyStatus::DependencyBlocked:return L"dependency_blocked";default:return L"not_started";}}
void Copies(ReportBatch& batch,const CopyReport& report,const std::wstring& kind) {
    for(const auto& item:report.items)for(auto& out:batch.items)if(out.row==item.row && out.kind==kind){
        out.status=CopyCode(item.status);out.error=item.error;out.changed=item.published;out.completed=CopyDone(item);out.stage=item.remainingStage;
        out.attempts=item.attempts;out.lastError=item.lastError;out.automaticSkip=item.automaticSkip;}
}
void Saved(ReportBatch& batch,const RecoveryReport& report) {
    for(const auto& item:report.items)for(auto& out:batch.items)if(out.row==item.row && out.kind==L"save_original"){
        out.saved=item.saved;out.status=CopyCode(item.copy.status);out.error=item.copy.error;out.stage=item.copy.remainingStage;
        out.changed=item.copy.published;out.logged=item.logged;out.completed=CopyDone(item.copy) && item.logged;
        out.attempts=item.copy.attempts;out.lastError=item.copy.lastError;out.automaticSkip=item.copy.automaticSkip;
        if(out.completed)out.status=L"saved_recorded";}
    if(report.journalError && !batch.journalError)batch.journalError=report.journalError;
}
void Removed(ReportBatch& batch,const DeleteReport& report) {
    for(const auto& item:report.items)for(auto& out:batch.items)if(out.row==item.row && out.kind==L"delete"){
        out.status=item.status==DeleteStatus::Deleted?L"deleted":item.status==DeleteStatus::Failed?L"failed":
            item.status==DeleteStatus::Cancelled?L"cancelled":item.status==DeleteStatus::Skipped?L"skipped":
            item.status==DeleteStatus::DependencyBlocked?L"dependency_blocked":L"not_started";
        out.error=item.error;out.changed=item.removed;out.completed=item.status==DeleteStatus::Deleted && item.removed && !item.error;
        out.attempts=item.attempts;out.lastError=item.lastError;out.automaticSkip=item.automaticSkip;out.blockedBy=item.blockedBy;}
    Saved(batch,report.recovery);if(report.journalError)batch.journalError=report.journalError;
}
void Replaced(ReportBatch& batch,const OverwriteReport& report) {
    CopyReport copies;copies.items=report.items;Copies(batch,copies,L"overwrite");
    for(auto& out:batch.items)if(out.kind==L"overwrite" && out.completed)out.status=L"replaced";
    Saved(batch,report.recovery);if(report.journalError)batch.journalError=report.journalError;
}
const wchar_t* ActionCode(Action a){switch(a){case Action::None:return L"none";case Action::CopyRight:return L"copy_right";
    case Action::CopyLeft:return L"copy_left";case Action::DeleteRight:return L"delete_right";case Action::Decide:return L"decide";default:return L"blocked";}}
const wchar_t* StateCode(State s){switch(s){case State::Equal:return L"equal";case State::LeftOnly:return L"left_only";
    case State::RightOnly:return L"right_only";case State::Different:return L"different";case State::Conflict:return L"conflict";default:return L"blocked";}}
const wchar_t* ReasonCode(Reason r){switch(r){case Reason::Metadata:return L"metadata";case Reason::Content:return L"content";
    case Reason::NewerLeft:return L"newer_left";case Reason::NewerRight:return L"newer_right";case Reason::Missing:return L"missing";
    case Reason::TypeConflict:return L"type_conflict";case Reason::ScanError:return L"scan_error";case Reason::Unsupported:return L"unsupported";
    case Reason::Changed:return L"changed";case Reason::Ambiguous:return L"ambiguous";case Reason::Cancelled:return L"cancelled";
    case Reason::InvalidRoot:return L"invalid_root";case Reason::InvalidMask:return L"invalid_mask";case Reason::Excluded:return L"excluded";default:return L"aliased";}}
using Fields=std::vector<std::pair<std::wstring,std::wstring>>;
const std::vector<std::wstring> Columns{L"record",L"key",L"value",L"row",L"relative_path",L"visible",L"default_action",L"action",L"skipped",
    L"state",L"reason",L"error",L"left_path",L"left_type",L"left_size_bytes",L"left_modified_utc",L"left_modified_ticks",L"left_attributes",L"left_error",
    L"right_path",L"right_type",L"right_size_bytes",L"right_modified_utc",L"right_modified_ticks",L"right_attributes",L"right_error",
    L"operation",L"source",L"target",L"saved_path",L"remaining_stage",L"status",L"filesystem_changed",L"completed",L"logged",L"execution_visible",
    L"attempts",L"last_error",L"automatic_skip",L"blocked_by"};
std::wstring Cell(std::wstring value,bool csv){
    if(csv){auto first=value.find_first_not_of(L" \t\r\n");
        if(first!=std::wstring::npos && (value[first]==L'=' || value[first]==L'+' || value[first]==L'-' || value[first]==L'@'))value.insert(0,1,L'\'');
        std::wstring out=L"\"";for(auto c:value){out+=c;if(c==L'"')out+=c;}return out+L"\"";}
    std::wstring out;for(auto c:value){if(c==L'\\')out+=L"\\\\";else if(c==L'\r')out+=L"\\r";else if(c==L'\n')out+=L"\\n";
        else if(c==L'\t')out+=L"\\t";else if(c<32){wchar_t hex[8]{};swprintf_s(hex,L"\\u%04x",unsigned(c));out+=hex;}else out+=c;}return out;
}
struct Document {
    bool csv;std::wstring text;
    explicit Document(bool c):csv(c){if(csv)Add(L"",{},true);else text=L"Directory Synchronization report 1\r\nSizes: exact bytes; timestamps: UTC. Proposals are not completed operations.\r\n\r\n";}
    void Add(const std::wstring& record,const Fields& fields,bool header=false){
        if(csv){std::map<std::wstring,std::wstring> values(fields.begin(),fields.end());values[L"record"]=record;
            for(size_t i=0;i<Columns.size();++i){if(i)text+=L",";text+=Cell(header?Columns[i]:values[Columns[i]],true);}text+=L"\r\n";
        }else{text+=L"["+record+L"]\r\n";for(const auto& field:fields)text+=field.first+L": "+Cell(field.second,false)+L"\r\n";text+=L"\r\n";}
    }
    void Meta(const std::wstring& key,const std::wstring& value,const std::wstring& kind=L"metadata"){Add(kind,{{L"key",key},{L"value",value}});}
};
void Filters(Document& doc,const std::array<bool,5>& filters,const std::wstring& prefix,const std::wstring& kind=L"metadata") {
    const wchar_t* names[]{L"equal",L"different",L"single",L"right",L"left"};for(size_t i=0;i<5;++i)doc.Meta(prefix+names[i],B(filters[i]),kind);
}
}
std::wstring ReportTime(){FILETIME now{};GetSystemTimeAsFileTime(&now);return Stamp((uint64_t(now.dwHighDateTime)<<32)|now.dwLowDateTime);}
ReportBatch PrepareReport(const Result& result,const ReportContext& context,const std::wstring& kind,bool keep,
    const std::wstring& recoveryRoot,const std::vector<size_t>& selected,int side) {
    ReportBatch batch;batch.kind=kind;batch.started=ReportTime();batch.keepOriginals=keep;batch.recoveryRoot=recoveryRoot;
    batch.filters=context.filters;batch.visible=context.visible;
    auto add=[&](size_t index,const std::wstring& operation,bool right){const auto& row=result.rows.at(index);ReportOperation out;
        out.row=index;out.kind=operation;out.target=Path(right?context.right:context.left,row);
        if(operation==L"new_copy" || operation==L"overwrite")out.source=Path(right?context.left:context.right,row);
        batch.items.push_back(std::move(out));};
    if(kind==L"recycle" || kind==L"permanent_delete"){
        for(const auto& item:ImmediateSelection(result,selected,side))for(size_t i=0;i<result.rows.size();++i)
            if(Path(item.side?context.right:context.left,result.rows[i])==item.path){add(i,kind,item.side!=0);break;}
        return batch;
    }
    std::set<size_t> deletions;for(auto i:DeletionRows(result,context.visible))deletions.insert(i);
    std::set<size_t> seen;
    for(auto i:context.visible)if(i<result.rows.size() && seen.insert(i).second){const auto& row=result.rows[i];bool added=false;
        if((kind==L"combined" || kind==L"new_copy") && IsNewCopy(row)){add(i,L"new_copy",row.action==Action::CopyRight);added=true;}
        if((kind==L"combined" || kind==L"overwrite") && IsFileOverwrite(row)){
            if(keep)add(i,L"save_original",row.action==Action::CopyRight);add(i,L"overwrite",row.action==Action::CopyRight);added=true;}
        if((kind==L"combined" || kind==L"delete") && deletions.count(i)){
            if(keep)add(i,L"save_original",true);add(i,L"delete",true);added=true;}
        if(kind==L"save_original" && IsOriginalToSave(row)){add(i,L"save_original",row.action!=Action::CopyLeft);added=true;}
        if(!added)++batch.omitted;
    }return batch;
}
void CompleteReport(ReportBatch& b,const CopyReport& r){Copies(b,r,L"new_copy");b.cancelled=r.cancelled;b.stale=r.stale;
    b.complete=!b.cancelled && !b.stale && std::all_of(b.items.begin(),b.items.end(),[](const auto& i){return i.completed;});}
void CompleteReport(ReportBatch& b,const RecoveryReport& r){Saved(b,r);b.error=r.error;b.complete=r.complete;b.cancelled=r.cancelled;b.stale=r.stale;}
void CompleteReport(ReportBatch& b,const OverwriteReport& r){Replaced(b,r);b.error=r.error;b.complete=r.complete;b.cancelled=r.cancelled;b.stale=r.stale;}
void CompleteReport(ReportBatch& b,const DeleteReport& r){Removed(b,r);b.error=r.error;b.complete=r.complete;b.cancelled=r.cancelled;b.stale=r.stale;}
void CompleteReport(ReportBatch& b,const SyncReport& r){Copies(b,r.copies,L"new_copy");Replaced(b,r.overwrites);Removed(b,r.deletions);
    b.error=r.error;b.complete=r.complete;b.cancelled=r.cancelled;b.stale=r.stale;}
void CompleteReport(ReportBatch& b,const ImmediateReport& r){for(const auto& i:r.items)for(auto& out:b.items)if(out.target==i.path){
    out.status=i.deleted?(FAILED(i.error)?L"removed_with_error":L"removed"):FAILED(i.error)?L"failed":L"not_confirmed";
    out.error=ULONG(i.error);out.changed=i.deleted;out.completed=i.deleted && SUCCEEDED(i.error);out.saved=i.recycledPath;}
    b.error=ULONG(r.error);b.cancelled=r.cancelled;b.stale=r.stale;
    b.complete=SUCCEEDED(r.error) && !r.cancelled && !r.stale && std::all_of(b.items.begin(),b.items.end(),[](const auto& i){return i.completed;});}
std::wstring RenderReport(const Result& result,const ReportContext& c,const ReportBatch* batch,bool csv) {
    Document doc(csv);std::set<size_t> executionVisible;
    if(batch)executionVisible.insert(batch->visible.begin(),batch->visible.end());
    doc.Meta(L"schema",L"1");doc.Meta(L"plugin_version",N(VERSINFO_MAJOR)+L"."+N(VERSINFO_MINORA)+L"."+N(VERSINFO_MINORB));doc.Meta(L"generated_utc",c.generated);
    doc.Meta(L"left_root",c.left);doc.Meta(L"right_root",c.right);doc.Meta(L"current_profile",c.profile);
    doc.Meta(L"scan_mask",c.options.mask);doc.Meta(L"scan_recursive",B(c.options.recursive));doc.Meta(L"scan_by_content",B(c.options.content));
    doc.Meta(L"scan_ignore_date",B(c.options.ignoreDate));doc.Meta(L"scan_asymmetric",B(c.options.asymmetric));
    doc.Meta(L"scan_empty_directories",B(c.options.emptyDirectories));doc.Meta(L"scan_only_selected",B(c.options.onlySelected));
    doc.Meta(L"scan_tolerance_ticks",N(c.options.tolerance));doc.Meta(L"scan_whole_second_tolerance",B(c.options.wholeSecondTolerance));
    if(c.options.onlySelected)for(const auto& item:c.options.selection)doc.Meta(item.directory?L"scan_selected_directory":L"scan_selected_file",item.name);
    doc.Meta(L"pending_scan_options",B(c.pendingOptions));doc.Meta(L"comparison_stale",B(result.stale));doc.Meta(L"comparison_cancelled",B(result.cancelled));doc.Meta(L"worker_failed",B(c.failed));
    Filters(doc,c.filters,L"filter_");std::set<size_t> visible;for(auto i:c.visible)if(i<result.rows.size())visible.insert(i);
    doc.Meta(L"comparison_rows",N(result.rows.size()));doc.Meta(L"visible_rows",N(visible.size()));doc.Meta(L"hidden_rows",N(result.rows.size()-visible.size()));
    doc.Meta(L"scope",L"all comparison model rows; visible and hidden; not only highlighted rows");
    for(size_t index=0;index<result.rows.size();++index){const auto& row=result.rows[index];
        Fields fields{{L"row",N(index+1)},{L"relative_path",row.path},{L"visible",B(visible.count(index)!=0)},
            {L"default_action",index<c.defaults.size()?ActionCode(c.defaults[index]):L"unknown"},{L"action",ActionCode(row.action)},
            {L"skipped",B(row.action==Action::None && row.state!=State::Equal)},
            {L"state",StateCode(row.state)},{L"reason",ReasonCode(row.reason)},{L"error",N(row.error)}};
        for(int side=0;side<2;++side)if(side?row.hasRight:row.hasLeft){auto prefix=side?std::wstring(L"right_"):std::wstring(L"left_");const auto& e=side?row.right:row.left;
            fields.push_back({prefix+L"path",Path(side?c.right:c.left,row)});fields.push_back({prefix+L"type",e.directory?L"directory":L"file"});
            if(e.identified && !e.error){if(!e.directory)fields.push_back({prefix+L"size_bytes",N(e.size)});
                fields.push_back({prefix+L"modified_utc",Stamp(e.time)});fields.push_back({prefix+L"modified_ticks",N(e.time)});}
            fields.push_back({prefix+L"attributes",N(e.attributes)});fields.push_back({prefix+L"error",N(e.error)});}
        if(batch)fields.push_back({L"execution_visible",B(executionVisible.count(index)!=0)});
        doc.Add(L"comparison",fields);
    }
    if(batch){const auto& b=*batch;const std::wstring kind=L"batch";
        doc.Meta(L"operation",b.kind,kind);doc.Meta(L"started_utc",b.started,kind);doc.Meta(L"finished_utc",b.finished,kind);
        doc.Meta(L"keep_originals",B(b.keepOriginals),kind);doc.Meta(L"recovery_root",b.recoveryRoot,kind);
        doc.Meta(L"complete",B(b.complete && !b.exception),kind);doc.Meta(L"cancelled",B(b.cancelled),kind);doc.Meta(L"stale",B(b.stale),kind);
        doc.Meta(L"worker_exception",B(b.exception),kind);doc.Meta(L"error",N(b.error),kind);doc.Meta(L"journal_error",N(b.journalError),kind);
        doc.Meta(L"omitted_visible_rows",N(b.omitted),kind);Filters(doc,b.filters,L"execution_filter_",kind);
        doc.Meta(L"execution_visible_rows",N(executionVisible.size()),kind);
        for(const auto& item:b.items)doc.Add(L"operation",{{L"row",N(item.row+1)},{L"execution_visible",B(executionVisible.count(item.row)!=0)},
            {L"operation",item.kind},{L"source",item.source},{L"target",item.target},{L"saved_path",item.saved},{L"remaining_stage",item.stage},
            {L"status",item.status},{L"error",N(item.error)},{L"filesystem_changed",item.status==L"not_reported"?L"":B(item.changed)},
            {L"completed",B(item.completed)},{L"logged",item.kind==L"save_original" && item.status!=L"not_reported"?B(item.logged):L""},
            {L"attempts",N(item.attempts)},{L"last_error",N(item.lastError)},{L"automatic_skip",B(item.automaticSkip)},{L"blocked_by",item.blockedBy}});
    }else doc.Meta(L"operation_results",L"none recorded for this comparison");
    return doc.text;
}
DWORD WriteReport(const std::wstring& input,const std::wstring& text,bool replace,const std::vector<std::wstring>& excludedRoots) {
    std::wstring path;if(!NormalizeRoot(input,path))return ERROR_INVALID_NAME;
    // A failed comparison can contain syntactically invalid input roots; those
    // are not filesystem trees. Still allow exporting their diagnostics.
    for(const auto& root:excludedRoots)if(!root.empty()){std::wstring base;if(!NormalizeRoot(root,base))continue;
        if(CompareStringOrdinal(path.c_str(),-1,base.c_str(),-1,TRUE)==CSTR_EQUAL ||
            (path.size()>base.size() && CompareStringOrdinal(path.c_str(),int(base.size()),base.c_str(),int(base.size()),TRUE)==CSTR_EQUAL &&
                (base.back()==L'\\' || path[base.size()]==L'\\')))return ERROR_ACCESS_DENIED;}
    std::wstring parent;if(!RecoveryLocation(path,false,parent))return ERROR_ACCESS_DENIED;
    DWORD attrs=GetFileAttributesW(ApiPath(path).c_str());if(attrs!=INVALID_FILE_ATTRIBUTES &&
        (attrs&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_READONLY)))return ERROR_ACCESS_DENIED;
    if(text.size()>INT_MAX)return ERROR_FILE_TOO_LARGE;
    int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0,nullptr,nullptr);
    if(!bytes && !text.empty())return ERROR_NO_UNICODE_TRANSLATION;
    std::string utf8="\xef\xbb\xbf";utf8.resize(size_t(bytes)+3);
    if(bytes && WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),utf8.data()+3,bytes,nullptr,nullptr)!=bytes)return ERROR_NO_UNICODE_TRANSLATION;
    GUID id{};if(FAILED(CoCreateGuid(&id)))return ERROR_GEN_FAILURE;wchar_t name[48]{};StringFromGUID2(id,name,48);
    const auto stage=parent+L"\\.dirsync-report-"+name+L".tmp";
    HANDLE file=CreateFileW(ApiPath(stage).c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return GetLastError();DWORD error=0;
    for(size_t at=0;at<utf8.size();){DWORD written=0,wanted=DWORD(std::min<size_t>(65536,utf8.size()-at));
        if(!WriteFile(file,utf8.data()+at,wanted,&written,nullptr)){error=GetLastError();break;}
        if(!written){error=ERROR_WRITE_FAULT;break;}at+=written;}
    if(!error && !FlushFileBuffers(file))error=GetLastError();CloseHandle(file);
    if(!error && !MoveFileExW(ApiPath(stage).c_str(),ApiPath(path).c_str(),replace?MOVEFILE_REPLACE_EXISTING:0))error=GetLastError();
    if(error)DeleteFileW(ApiPath(stage).c_str());return error;
}
}
