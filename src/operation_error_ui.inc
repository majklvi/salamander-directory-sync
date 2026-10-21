// SPDX-License-Identifier: GPL-2.0-or-later
// Like the host's CStaticText path ellipsis: preserve the filename first;
// if that alone is too wide, preserve its tail (including the extension).
std::wstring ErrorDisplayPath(HDC dc,const std::wstring& value,int width) {
    auto fits=[&](const std::wstring& text){SIZE size{};return GetTextExtentPoint32W(dc,text.c_str(),int(text.size()),&size) && size.cx<=width;};
    if(fits(value))return value;
    const std::wstring dots=L"...";if(!fits(dots))return L"";
    auto slash=value.find_last_of(L"\\/");if(slash==std::wstring::npos)slash=0;
    auto prefix=[&](size_t n){if(n && IS_HIGH_SURROGATE(value[n-1]))--n;return value.substr(0,n);};
    auto suffix=[&](size_t n){if(n<value.size() && IS_LOW_SURROGATE(value[n]))++n;return value.substr(n);};
    if(fits(dots+suffix(slash))) {
        size_t low=0,high=slash;
        while(low<high){const auto mid=low+(high-low+1)/2;if(fits(prefix(mid)+dots+suffix(slash)))low=mid;else high=mid-1;}
        return prefix(low)+dots+suffix(slash);
    }
    size_t low=slash,high=value.size();
    while(low<high){const auto mid=low+(high-low)/2;if(fits(dots+suffix(mid)))high=mid;else low=mid+1;}
    return dots+suffix(low);
}
// Native path-ellipsis presentation while idle; focusing the read-only edit
// restores normal selection/scrolling and Ctrl+C of the complete Unicode path.
LRESULT CALLBACK ErrorPath(HWND path,UINT message,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR) {
    if((message==WM_PAINT || message==WM_PRINTCLIENT) && GetFocus()!=path) {
        PAINTSTRUCT paint{};HDC dc=message==WM_PAINT?BeginPaint(path,&paint):reinterpret_cast<HDC>(w);
        if(dc) {
            const int saved=SaveDC(dc);RECT rect{};GetClientRect(path,&rect);
            auto brush=reinterpret_cast<HBRUSH>(SendMessageW(GetParent(path),WM_CTLCOLORSTATIC,WPARAM(dc),LPARAM(path)));
            if(!brush)brush=GetSysColorBrush(COLOR_BTNFACE);
            FillRect(dc,&rect,brush);SetBkMode(dc,TRANSPARENT);
            SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(path,WM_GETFONT,0,0)));
            const auto text=ErrorDisplayPath(dc,ControlText(GetParent(path),GetDlgCtrlID(path)),rect.right);
            DrawTextW(dc,text.c_str(),int(text.size()),&rect,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
            RestoreDC(dc,saved);
        }
        if(message==WM_PAINT)EndPaint(path,&paint);return 0;
    }
    if(message==WM_SETFOCUS || message==WM_KILLFOCUS) {
        const auto result=DefSubclassProc(path,message,w,l);InvalidateRect(path,nullptr,TRUE);return result;
    }
    if(message==WM_NCDESTROY)RemoveWindowSubclass(path,ErrorPath,id);
    return DefSubclassProc(path,message,w,l);
}
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
            HWND tips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
                CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,window,nullptr,instance,nullptr);
            if(tips)SendMessageW(tips,TTM_SETMAXTIPWIDTH,0,700);
            for(int id:{IDC_ERROR_SOURCE,IDC_ERROR_TARGET}) {
                HWND path=GetDlgItem(window,id);SetWindowSubclass(path,ErrorPath,1,0);
                if(tips){TOOLINFOW info{sizeof(info)};info.uFlags=TTF_IDISHWND|TTF_SUBCLASS;
                    info.hwnd=window;info.uId=UINT_PTR(path);info.lpszText=LPSTR_TEXTCALLBACKW;
                    SendMessageW(tips,TTM_ADDTOOLW,0,LPARAM(&info));}
            }
            const int base=error.kind==dirsync::TransferKind::Delete?IDS_ERROR_DELETE_SOURCE:error.kind==dirsync::TransferKind::Save?IDS_ERROR_SAVE_SOURCE:
                error.kind==dirsync::TransferKind::Overwrite?IDS_ERROR_OVERWRITE_SOURCE:IDS_ERROR_SOURCE;
            // Buffered writes identify the target as the failing site. They
            // still transfer data; preparation/publication has not started.
            const bool writing=error.site==dirsync::ErrorSite::Target && error.phase==dirsync::CopyPhase::Copying;
            SetDlgItemTextW(window,IDC_ERROR_CONTEXT,Text(base+(writing?2:error.site==dirsync::ErrorSite::Source?0:
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
        if(message==WM_NOTIFY && reinterpret_cast<NMHDR*>(l)->code==TTN_GETDISPINFOW) {
            auto* tip=reinterpret_cast<NMTTDISPINFOW*>(l);
            tip->lpszText=(tip->hdr.idFrom==UINT_PTR(GetDlgItem(window,IDC_ERROR_SOURCE))?
                p->operationError.source:p->operationError.target).data();return TRUE;
        }
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
    if(!p.running || !(p.copying || p.saving || p.overwriting || p.deleting || p.synchronizing) || p.cancel || p.errorWindow || !p.errorQuestion.Peek(p.operationError))return;
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
