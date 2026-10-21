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
            SetDlgItemTextW(window,IDC_ERROR_CONTEXT,Text(error.site==dirsync::ErrorSite::Source?IDS_ERROR_SOURCE:
                error.site==dirsync::ErrorSite::Target?IDS_ERROR_TARGET:IDS_ERROR_DATA).c_str());
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
    if(!p.running || !p.copying || p.cancel || p.errorWindow || !p.errorQuestion.Peek(p.operationError))return;
    ShowTransfer(p);
    HWND window=CreateDialogParamW(language,MAKEINTRESOURCEW(IDD_OPERATION_ERROR),
        p.transferWindow?p.transferWindow:p.window,OperationErrorDialog,LPARAM(&p));
    if(!window){p.Stop();return;}
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(GetDlgItem(window,IDC_ERROR_RETRY));UpdateTransfer(p);
}
