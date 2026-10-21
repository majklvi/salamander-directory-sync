// SPDX-License-Identifier: GPL-2.0-or-later
INT_PTR CALLBACK OperationErrorDialog(HWND window,UINT message,WPARAM w,LPARAM l) {
    PreviewCallback callback;
    auto* p=reinterpret_cast<Preview*>(GetWindowLongPtrW(window,DWLP_USER));
    try {
        if(message==WM_INITDIALOG) {
            p=reinterpret_cast<Preview*>(l);p->errorWindow=window;
            SetWindowLongPtrW(window,DWLP_USER,l);
            if(general)ConfigureDark(window);
            const auto& error=p->operationError;
            SetDlgItemTextW(window,IDC_ERROR_SOURCE,error.source.c_str());
            SetDlgItemTextW(window,IDC_ERROR_TARGET,error.target.c_str());
            const int base=error.kind==dirsync::TransferKind::Save?IDS_ERROR_SAVE_SOURCE:
                error.kind==dirsync::TransferKind::Overwrite?IDS_ERROR_OVERWRITE_SOURCE:IDS_ERROR_SOURCE;
            SetDlgItemTextW(window,IDC_ERROR_CONTEXT,Text(base+(error.site==dirsync::ErrorSite::Source?0:
                error.site==dirsync::ErrorSite::Target?1:2)).c_str());
            LPWSTR system=nullptr;
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,error.code,0,reinterpret_cast<LPWSTR>(&system),0,nullptr);
            std::wstring details=Number(error.code)+L": "+(system?system:L"");if(system)LocalFree(system);
            SetDlgItemTextW(window,IDC_ERROR_DETAILS,details.c_str());
            if(general)general->MultiMonCenterWindow(window,GetParent(window),FALSE);
            return TRUE;
        }
        if(!p)return FALSE;
        LRESULT brush=0;if(dark && DarkModeHandleCtlColor(message,w,l,brush))return brush;
        if(message==WM_COMMAND || message==WM_CLOSE) {
            const int id=message==WM_CLOSE?IDCANCEL:LOWORD(w);
            if(id==IDC_ERROR_RETRY || id==IDC_ERROR_SKIP || id==IDC_ERROR_SKIP_ALL || id==IDCANCEL) {
                if(id==IDCANCEL)p->Stop();
                else p->errorQuestion.Answer(id==IDC_ERROR_RETRY?dirsync::ErrorDecision::Retry:
                    id==IDC_ERROR_SKIP_ALL?dirsync::ErrorDecision::SkipAll:dirsync::ErrorDecision::Skip);
                if(p->errorWindow==window)p->errorWindow=nullptr;
                if(IsWindow(window))DestroyWindow(window);
                UpdateTransfer(*p);return TRUE;
            }
        }
        if(message==WM_SETTINGCHANGE || message==WM_THEMECHANGED){if(general)ConfigureDark(window);return TRUE;}
        if(message==WM_NCDESTROY && p->errorWindow==window){p->errorWindow=nullptr;p->errorQuestion.Cancel();}
    }catch(...){if(p)p->Stop();return TRUE;}
    return FALSE;
}
void ShowOperationError(Preview& p) {
    if(!p.running || !(p.copying || p.saving || p.overwriting) || p.cancel || p.errorWindow || !p.errorQuestion.Peek(p.operationError))return;
    ShowTransfer(p);
    HWND window=CreateDialogParamW(language,MAKEINTRESOURCEW(IDD_OPERATION_ERROR),
        p.transferWindow?p.transferWindow:p.window,OperationErrorDialog,LPARAM(&p));
    if(!window){p.Stop();return;}
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(GetDlgItem(window,IDC_ERROR_RETRY));UpdateTransfer(p);
}
void ConfigureErrorPolicy(Preview& p) {
    p.copyProgress.errors.stopOnFirstError=settings.stopOnFirstError;
    p.copyProgress.errors.prompt=[&p](const dirsync::OperationError& error) {
        const auto started=GetTickCount64();p.copyProgress.awaitingDecision=true;
        struct Waiting {Preview& p;ULONGLONG started;~Waiting(){p.copyProgress.pausedMilliseconds.fetch_add(GetTickCount64()-started);p.copyProgress.awaitingDecision=false;}} waiting{p,started};
        return p.errorQuestion.Ask(error,p.cancel,[&p]{return PostMessageW(p.window,OperationErrorMessage,0,0)!=FALSE;});
    };
}
