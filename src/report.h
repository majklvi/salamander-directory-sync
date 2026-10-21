// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "sync.h"
#include <array>
namespace dirsync {
struct ImmediateReport;
struct ReportOperation {
    size_t row=0;
    std::wstring kind,source,target,saved,stage,status=L"not_reported";
    uint64_t error=0;
    uint64_t attempts=0,lastError=0;
    bool automaticSkip=false;
    bool changed=false,completed=false,logged=false;
};
struct ReportBatch {
    std::wstring kind,started,finished,recoveryRoot;
    std::array<bool,5> filters{};
    std::vector<size_t> visible;
    std::vector<ReportOperation> items;
    bool keepOriginals=false,complete=false,cancelled=false,stale=false,exception=false;
    uint64_t error=0,journalError=0,omitted=0;
};
struct ReportContext {
    std::wstring left,right,profile,generated;
    Options options;
    std::array<bool,5> filters{};
    std::vector<size_t> visible;
    std::vector<Action> defaults;
    bool failed=false,pendingOptions=false;
};
std::wstring ReportTime();
ReportBatch PrepareReport(const Result& result,const ReportContext& context,const std::wstring& kind,
    bool keep,const std::wstring& recoveryRoot,const std::vector<size_t>& selected={},int side=0);
void CompleteReport(ReportBatch& batch,const CopyReport& result);
void CompleteReport(ReportBatch& batch,const RecoveryReport& result);
void CompleteReport(ReportBatch& batch,const OverwriteReport& result);
void CompleteReport(ReportBatch& batch,const DeleteReport& result);
void CompleteReport(ReportBatch& batch,const SyncReport& result);
void CompleteReport(ReportBatch& batch,const ImmediateReport& result);
// Stable English field names and codes, exact bytes/UTC times, all model rows.
// CSV cells are quoted and text cells with spreadsheet formula prefixes guarded.
std::wstring RenderReport(const Result& result,const ReportContext& context,const ReportBatch* batch,bool csv);
// UTF-8 BOM; publish a complete same-directory staging file. Existing report is
// replaced only when replace=true (the native Save dialog confirms it first).
DWORD WriteReport(const std::wstring& path,const std::wstring& text,bool replace,
    const std::vector<std::wstring>& excludedRoots);
}
