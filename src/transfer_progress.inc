// SPDX-License-Identifier: GPL-2.0-or-later
constexpr UINT ShowTransferMessage=WM_APP+89;
bool TransferActive(const Preview& p) {
    return p.synchronizing || p.copying || p.saving || p.overwriting || p.deleting || p.immediateDeleting;
}
int TransferPosition(uint64_t done,uint64_t total) {
    if(!total)return 0;if(done>=total)return 1000;
    return std::min(999,int(static_cast<long double>(done)*1000/static_cast<long double>(total)));
}
void TransferText(HWND window,int id,const std::wstring& value) {
    HWND control=GetDlgItem(window,id);int length=GetWindowTextLengthW(control);
    std::wstring previous(size_t(length)+1,L'\0');GetWindowTextW(control,previous.data(),length+1);previous.resize(size_t(length));
    if(previous!=value)SetWindowTextW(control,value.c_str());
}
void TransferBar(HWND window,int id,int label,uint64_t done,uint64_t total) {
    HWND bar=GetDlgItem(window,id);LONG_PTR style=GetWindowLongPtrW(bar,GWL_STYLE);bool marquee=(style&PBS_MARQUEE)!=0;
    if(marquee!=(total==0)){
        SendMessageW(bar,PBM_SETMARQUEE,FALSE,0);
        SetWindowLongPtrW(bar,GWL_STYLE,total?style&~LONG_PTR(PBS_MARQUEE):style|PBS_MARQUEE);
        if(!total)SendMessageW(bar,PBM_SETMARQUEE,TRUE,60);
    }
    if(total)SendMessageW(bar,PBM_SETPOS,TransferPosition(done,total),0);
    TransferText(window,label,total?Number(uint64_t(TransferPosition(done,total)/10))+L"%":L"—");
}
void UpdateTransfer(Preview& p) {
    if(!p.transferWindow)return;
    auto display=p.copyProgress.Display();auto phase=p.copyProgress.phase.load();
    TransferText(p.transferWindow,IDC_TRANSFER_SOURCE,display.source);
    TransferText(p.transferWindow,IDC_TRANSFER_TARGET,display.target);
    int purpose=display.kind==dirsync::TransferKind::Save?IDS_UI_OP_SAVE:display.kind==dirsync::TransferKind::Overwrite?IDS_UI_OP_OVERWRITE:
        display.kind==dirsync::TransferKind::Delete?IDS_UI_OP_DELETE:display.kind==dirsync::TransferKind::Recycle?IDS_PROGRESS_RECYCLE:IDS_UI_OP_COPY;
    int state=phase==dirsync::CopyPhase::Checking?IDS_UI_VALIDATE:phase==dirsync::CopyPhase::Verifying?IDS_PROGRESS_VERIFY:
        phase==dirsync::CopyPhase::Preparing?IDS_PROGRESS_PREPARE:phase==dirsync::CopyPhase::Publishing?IDS_PROGRESS_PUBLISH:
        phase==dirsync::CopyPhase::Published?IDS_PROGRESS_PUBLISH:phase==dirsync::CopyPhase::Finished?IDS_PROGRESS_FINISH:
        phase==dirsync::CopyPhase::Deleting || phase==dirsync::CopyPhase::Deleted?IDS_UI_OP_DELETE:IDS_PROGRESS_DATA;
    auto status=p.cancel?Text(IDS_UI_STOPPING):display.source.empty()?Text(IDS_UI_VALIDATE):Text(purpose)+L" — "+Text(state);
    TransferText(p.transferWindow,IDC_TRANSFER_PHASE,status);
    TransferBar(p.transferWindow,IDC_TRANSFER_FILE_BAR,IDC_TRANSFER_FILE_PERCENT,
        phase==dirsync::CopyPhase::Verifying?display.verified:display.copied,
        display.directory?0:phase==dirsync::CopyPhase::Verifying?display.verifyTotal:display.total);
    uint64_t count=p.operationReport.items.size();
    if(!count)count=(p.synchronizing || p.overwriting || p.deleting || p.immediateDeleting)?p.transferCount:uint64_t(p.copyRows.size());
    auto completed=p.copyProgress.completed.load();
    TransferBar(p.transferWindow,IDC_TRANSFER_TOTAL_BAR,IDC_TRANSFER_TOTAL_PERCENT,completed,count);
    TransferText(p.transferWindow,IDC_TRANSFER_COUNTS,InsertTextValues(Text(IDS_PROGRESS_COUNTS),
        {{L"{completed}",Number(completed)},{L"{total}",Number(count)}}));
    auto copied=PanelSize(p.copyProgress.copied.load(),p.sizeFormat),verified=PanelSize(p.copyProgress.verified.load(),p.sizeFormat);
    TransferText(p.transferWindow,IDC_TRANSFER_BYTES,InsertTextValues(Text(IDS_PROGRESS_BYTES),
        {{L"{copied}",copied},{L"{verified}",verified}}));
    auto seconds=(GetTickCount64()-p.transferStarted)/1000;wchar_t elapsed[128]{};
    swprintf_s(elapsed,L"%llu:%02u:%02u",static_cast<unsigned long long>(seconds/3600),unsigned(seconds/60%60),unsigned(seconds%60));
    TransferText(p.transferWindow,IDC_TRANSFER_ELAPSED,InsertTextValues(Text(IDS_PROGRESS_ELAPSED),{{L"{time}",elapsed}}));
    EnableWindow(GetDlgItem(p.transferWindow,IDCANCEL),!p.cancel);
}
LRESULT CALLBACK TransferStopButton(HWND button,UINT message,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR) {
    // The host owns the outer modal message loop. This owned modeless dialog
    // has one tab stop; handle its keys without installing a thread hook.
    if(message==WM_GETDLGCODE)return DefSubclassProc(button,message,w,l)|DLGC_WANTALLKEYS|DLGC_WANTTAB;
    if(message==WM_KEYDOWN && (w==VK_ESCAPE || w==VK_RETURN)){
        SendMessageW(GetParent(button),WM_COMMAND,IDCANCEL,0);return 0;}
    if(message==WM_KEYDOWN && w==VK_TAB)return 0;
    if(message==WM_NCDESTROY)RemoveWindowSubclass(button,TransferStopButton,id);
    return DefSubclassProc(button,message,w,l);
}
INT_PTR CALLBACK TransferDialog(HWND window,UINT message,WPARAM w,LPARAM l) {
    auto* p=reinterpret_cast<Preview*>(GetWindowLongPtrW(window,DWLP_USER));
    try{
        if(message==WM_INITDIALOG){p=reinterpret_cast<Preview*>(l);p->transferWindow=window;
            SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(p));
            for(int id:{IDC_TRANSFER_FILE_BAR,IDC_TRANSFER_TOTAL_BAR})SendDlgItemMessageW(window,id,PBM_SETRANGE32,0,1000);
            SetWindowSubclass(GetDlgItem(window,IDCANCEL),TransferStopButton,1,0);
            HWND tips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
                CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,window,nullptr,instance,nullptr);
            if(tips){SendMessageW(tips,TTM_SETMAXTIPWIDTH,0,700);
                for(int id:{IDC_TRANSFER_SOURCE,IDC_TRANSFER_TARGET}){TOOLINFOW info{sizeof(info)};
                    info.uFlags=TTF_IDISHWND|TTF_SUBCLASS;info.hwnd=window;info.uId=UINT_PTR(GetDlgItem(window,id));info.lpszText=LPSTR_TEXTCALLBACKW;
                    SendMessageW(tips,TTM_ADDTOOLW,0,LPARAM(&info));}}
            if(general){ConfigureDark(window);general->MultiMonCenterWindow(window,GetAncestor(GetParent(window),GA_ROOTOWNER),FALSE);}
            UpdateTransfer(*p);SetTimer(window,1,150,nullptr);return TRUE;}
        if(!p)return FALSE;
        LRESULT brush=0;if(dark && DarkModeHandleCtlColor(message,w,l,brush))return brush;
        switch(message){
        case WM_TIMER:if(!p->running || !TransferActive(*p)){p->CloseTransfer();return TRUE;}UpdateTransfer(*p);return TRUE;
        case WM_COMMAND:if(LOWORD(w)!=IDCANCEL)break;[[fallthrough]];
        case WM_CLOSE:p->Stop();UpdateTransfer(*p);return TRUE;
        case WM_KEYDOWN:if(w==VK_ESCAPE){p->Stop();UpdateTransfer(*p);return TRUE;}break;
        case WM_NOTIFY:if(reinterpret_cast<NMHDR*>(l)->code==TTN_GETDISPINFOW){auto* tip=reinterpret_cast<NMTTDISPINFOW*>(l);
            auto display=p->copyProgress.Display();p->transferTip=tip->hdr.idFrom==UINT_PTR(GetDlgItem(window,IDC_TRANSFER_SOURCE))?display.source:display.target;
            tip->lpszText=p->transferTip.data();return TRUE;}break;
        case WM_SETTINGCHANGE:case WM_THEMECHANGED:if(general)ConfigureDark(window);return TRUE;
        case WM_DESTROY:KillTimer(window,1);if(p->transferWindow==window)p->transferWindow=nullptr;return TRUE;
        }
    }catch(...){if(p)p->Stop();return TRUE;}
    return FALSE;
}
void ShowTransfer(Preview& p) {
    if(!p.running || !TransferActive(p) || p.transferWindow)return;
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);
    if(!p.transferStarted)p.transferStarted=GetTickCount64();
    HWND window=CreateDialogParamW(language,MAKEINTRESOURCEW(IDD_TRANSFER_PROGRESS),p.window,TransferDialog,LPARAM(&p));
    if(window){ShowWindow(window,SW_SHOW);SetFocus(GetDlgItem(window,IDCANCEL));}
    // The existing status-bar progress/Stop remain available if window creation fails.
}
