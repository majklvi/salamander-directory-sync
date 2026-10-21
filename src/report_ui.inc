// SPDX-License-Identifier: GPL-2.0-or-later
dirsync::ReportContext ReportContext(const Preview& p) {
    dirsync::ReportContext c;c.left=p.left;c.right=p.right;c.options=p.options;c.filters=p.filterChecks;
    c.visible=p.visible;c.defaults=p.defaults;c.failed=p.failed;c.generated=dirsync::ReportTime();
    c.pendingOptions=Checked(p.window,IDC_RECURSIVE)!=p.options.recursive || Checked(p.window,IDC_CONTENT)!=p.options.content ||
        Checked(p.window,IDC_IGNORE)!=p.options.ignoreDate || Checked(p.window,IDC_ASYMMETRIC)!=p.options.asymmetric ||
        Checked(p.window,IDC_EMPTY)!=p.options.emptyDirectories || Checked(p.window,IDC_SELECTED)!=p.options.onlySelected;
    if(auto* profile=settings.profiles.Find(settings.profiles.selected))c.profile=profile->name;
    return c;
}
void BeginReport(Preview& p,const std::wstring& kind,bool keep,const std::wstring& root,
    const std::vector<size_t>& selected={},int side=0,dirsync::SyncSelection selection={}) {
    p.copyProgress.ResetControls();p.copyProgress.ResetDisplay();p.transferStarted=GetTickCount64();
    p.copyProgress.bytesPerSecond=settings.transferSpeed;
    p.errorQuestion.Reset();
    p.operationReport=dirsync::PrepareReport(p.result,ReportContext(p),kind,keep,root,selected,side,selection);
    TransferBudget(p);
}
void FinishReport(Preview& p) {
    if(p.synchronizing)dirsync::CompleteReport(p.operationReport,p.syncReport);
    else if(p.copying)dirsync::CompleteReport(p.operationReport,p.copyReport);
    else if(p.saving)dirsync::CompleteReport(p.operationReport,p.recoveryReport);
    else if(p.overwriting)dirsync::CompleteReport(p.operationReport,p.overwriteReport);
    else if(p.deleting)dirsync::CompleteReport(p.operationReport,p.deleteReport);
    else if(p.immediateDeleting)dirsync::CompleteReport(p.operationReport,p.immediateReport);
    else return;
    p.operationReport.finished=dirsync::ReportTime();p.operationReport.exception=p.failed;
}
void ExportResults(Preview& p) {
    if(p.running || !p.reportReady)return;
    IFileSaveDialog* dialog=nullptr;HRESULT hr=CoCreateInstance(CLSID_FileSaveDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog));
    if(FAILED(hr)){Message(p.window,IDS_REPORT_FAILED);return;}
    struct ReleaseDialog{IFileSaveDialog* value;~ReleaseDialog(){value->Release();}} release{dialog};
    const auto csvName=Text(IDS_REPORT_CSV),textName=Text(IDS_REPORT_TEXT);
    COMDLG_FILTERSPEC types[]{{csvName.c_str(),L"*.csv"},{textName.c_str(),L"*.txt"}};
    dialog->SetFileTypes(2,types);dialog->SetFileTypeIndex(1);dialog->SetDefaultExtension(L"csv");
    dialog->SetFileName(L"directory-sync-report");dialog->SetTitle(Text(IDS_REPORT_EXPORT).c_str());
    DWORD flags=0;dialog->GetOptions(&flags);dialog->SetOptions(flags|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST|FOS_OVERWRITEPROMPT|FOS_NOCHANGEDIR);
    hr=dialog->Show(p.window);if(hr==HRESULT_FROM_WIN32(ERROR_CANCELLED))return;
    IShellItem* item=nullptr;PWSTR name=nullptr;
    if(SUCCEEDED(hr))hr=dialog->GetResult(&item);
    if(SUCCEEDED(hr))hr=item->GetDisplayName(SIGDN_FILESYSPATH,&name);
    struct ReleaseResult{IShellItem* item;PWSTR name;~ReleaseResult(){if(item)item->Release();CoTaskMemFree(name);}} result{item,name};
    if(FAILED(hr)){Message(p.window,IDS_REPORT_FAILED);return;}
    UINT type=1;dialog->GetFileTypeIndex(&type);DWORD error=0;
    struct Cursor{HCURSOR old;Cursor():old(SetCursor(LoadCursor(nullptr,IDC_WAIT))){}~Cursor(){SetCursor(old);}} cursor;
    try{auto context=ReportContext(p);auto batch=p.operationReport;if(!batch.kind.empty() && batch.finished.empty())batch.exception=p.failed;
        auto data=dirsync::RenderReport(p.result,context,batch.kind.empty()?nullptr:&batch,type==1);
        error=dirsync::WriteReport(name,data,true,{p.left,p.right,settings.recoveryRoot,p.operationReport.recoveryRoot});
    }catch(...){error=ERROR_NOT_ENOUGH_MEMORY;}
    if(error)InformationBox(p.window,IDS_REPORT_FAILED,{{Text(IDS_STORED_SAVED),name},{Text(IDS_STORED_ERROR),Number(error)}},Text(IDS_REPORT_POLICY));
    else StatusPane(p.window,0,Text(IDS_REPORT_SAVED)+L" "+name);
}
