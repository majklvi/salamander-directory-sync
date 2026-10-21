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
    HWND control=id?GetDlgItem(window,id):window;int length=GetWindowTextLengthW(control);
    std::wstring previous(size_t(length)+1,L'\0');GetWindowTextW(control,previous.data(),length+1);previous.resize(size_t(length));
    if(previous!=value)SetWindowTextW(control,value.c_str());
}
void TransferBar(HWND window,int id,CGUIProgressBarAbstract* native,uint64_t done,uint64_t total) {
    if(native){native->SetProgress(total?DWORD(TransferPosition(done,total)):DWORD(-1),nullptr);return;}
    HWND bar=GetDlgItem(window,id);LONG_PTR style=GetWindowLongPtrW(bar,GWL_STYLE);bool marquee=(style&PBS_MARQUEE)!=0;
    if(marquee!=(total==0)){
        SendMessageW(bar,PBM_SETMARQUEE,FALSE,0);
        SetWindowLongPtrW(bar,GWL_STYLE,total?style&~LONG_PTR(PBS_MARQUEE):style|PBS_MARQUEE);
        if(!total)SendMessageW(bar,PBM_SETMARQUEE,TRUE,60);
    }
    if(total)SendMessageW(bar,PBM_SETPOS,TransferPosition(done,total),0);
    TransferText(window,id,total?Number(uint64_t(TransferPosition(done,total)/10))+L" %":L"—");
}
uint64_t TransferAdd(uint64_t a,uint64_t b) {return b>UINT64_MAX-a?UINT64_MAX:a+b;}
uint64_t TransferWeight(uint64_t size,bool data) {return data?std::max(uint64_t(4096),size>UINT64_MAX/3?UINT64_MAX:size*3):4096;}
void TransferBudget(Preview& p) {
    p.transferWorkTotal=p.transferBytesTotal=0;
    p.transferSampleTick=p.transferSampleCopied=p.transferSampleWork=p.transferRate=p.transferWorkRate=0;
    for(const auto& item:p.operationReport.items) {
        if(item.row>=p.result.rows.size())continue;
        const auto& row=p.result.rows[item.row];const dirsync::Entry* entry=nullptr;
        if(item.kind==L"new_copy" || item.kind==L"overwrite")entry=row.action==dirsync::Action::CopyRight?&row.left:&row.right;
        else if(item.kind==L"save_original")entry=row.action==dirsync::Action::CopyLeft?&row.left:&row.right;
        const bool data=entry && !entry->directory;
        p.transferWorkTotal=TransferAdd(p.transferWorkTotal,TransferWeight(data?entry->size:0,data));
        if(data)p.transferBytesTotal=TransferAdd(p.transferBytesTotal,entry->size);
    }
}
std::wstring TransferSize(uint64_t bytes) {
    if(!general)return Number(bytes);
    char buffer[128]{};general->PrintDiskSize(buffer,CQuadWord(DWORD(bytes),DWORD(bytes>>32)),4);
    std::wstring value;dirsync::DecodePath(buffer,value);return value;
}
std::wstring TransferTime(uint64_t seconds) {
    if(general){char buffer[128]{};general->PrintTimeLeft(buffer,CQuadWord(DWORD(seconds),DWORD(seconds>>32)));
        std::wstring value;dirsync::DecodePath(buffer,value);return value;}
    wchar_t value[96]{};swprintf_s(value,L"%llu:%02u:%02u",static_cast<unsigned long long>(seconds/3600),unsigned(seconds/60%60),unsigned(seconds%60));return value;
}
uint64_t TransferWork(const dirsync::TransferDisplay& display,dirsync::CopyPhase phase) {
    const bool data=!display.directory && (display.kind==dirsync::TransferKind::Copy ||
        display.kind==dirsync::TransferKind::Overwrite || display.kind==dirsync::TransferKind::Save);
    const auto weight=TransferWeight(display.total,data);
    if(display.finished)return TransferAdd(display.workBase,weight);
    if(!data || !display.total)return display.workBase;
    long double fraction=0;
    if(phase==dirsync::CopyPhase::Verifying)fraction=(1.0L+2.0L*std::min(1.0L,
        static_cast<long double>(display.verified)/std::max(uint64_t(1),display.verifyTotal)))/3.0L;
    else if(phase==dirsync::CopyPhase::Preparing)fraction=1.0L/3.0L;
    else if(phase==dirsync::CopyPhase::Publishing || phase==dirsync::CopyPhase::Published)fraction=1.0L;
    else if(phase==dirsync::CopyPhase::Copying)fraction=std::min(1.0L,static_cast<long double>(display.copied)/display.total)/3.0L;
    return TransferAdd(display.workBase,uint64_t(fraction*static_cast<long double>(weight)));
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

    TransferBar(p.transferWindow,IDC_TRANSFER_FILE_BAR,p.transferFileBar,
        phase==dirsync::CopyPhase::Verifying?display.verified:display.copied,
        display.directory?0:phase==dirsync::CopyPhase::Verifying?display.verifyTotal:display.total);
    uint64_t count=p.operationReport.items.size();
    if(!count)count=(p.synchronizing || p.overwriting || p.deleting || p.immediateDeleting)?p.transferCount:uint64_t(p.copyRows.size());
    auto completed=p.copyProgress.completed.load();
    auto work=TransferWork(display,phase);uint64_t total=p.transferWorkTotal;
    int totalPosition=total?TransferPosition(work,total):TransferPosition(completed,count);
    if(completed<count)totalPosition=std::min(totalPosition,999);
    else if(count)totalPosition=1000;
    TransferBar(p.transferWindow,IDC_TRANSFER_TOTAL_BAR,p.transferTotalBar,uint64_t(totalPosition),1000);
    auto counts=InsertTextValues(Text(IDS_PROGRESS_COUNTS),
        {{L"{completed}",Number(completed)},{L"{total}",Number(count)}});
    auto copied=TransferSize(p.copyProgress.copied.load()),verified=TransferSize(p.copyProgress.verified.load());
    auto bytes=InsertTextValues(Text(IDS_PROGRESS_BYTES),
        {{L"{copied}",copied},{L"{verified}",verified}});
    auto seconds=(GetTickCount64()-p.transferStarted)/1000;wchar_t elapsed[128]{};
    swprintf_s(elapsed,L"%llu:%02u:%02u",static_cast<unsigned long long>(seconds/3600),unsigned(seconds/60%60),unsigned(seconds%60));
    auto time=InsertTextValues(Text(IDS_PROGRESS_ELAPSED),{{L"{time}",elapsed}});
    p.transferDetails=status+L"\n"+counts+L"\n"+bytes+L"\n"+time;
    const bool paused=p.copyProgress.pauseRequested.load();
    const bool waiting=p.copyProgress.awaitingDecision.load();
    const auto tick=GetTickCount64()-p.copyProgress.pausedMilliseconds.load();
    const auto copiedBytes=p.copyProgress.copied.load();
    if(waiting){p.transferSampleTick=0;p.transferRate=p.transferWorkRate=0;}
    else if(!p.transferSampleTick || paused){p.transferSampleTick=tick;p.transferSampleCopied=copiedBytes;p.transferSampleWork=work;p.transferRate=p.transferWorkRate=0;}
    else if(tick-p.transferSampleTick>=500){
        const auto elapsed=tick-p.transferSampleTick;
        auto rate=[&](uint64_t current,uint64_t before){return current>=before?uint64_t(static_cast<long double>(current-before)*1000/elapsed):0;};
        p.transferRate=rate(copiedBytes,p.transferSampleCopied);p.transferWorkRate=rate(work,p.transferSampleWork);
        p.transferSampleTick=tick;p.transferSampleCopied=copiedBytes;p.transferSampleWork=work;
    }
    std::wstring line;
    if(waiting)line=Text(IDS_ERROR_WAIT);
    else if(paused)line=Text(p.copyProgress.paused?IDS_TRANSFER_PAUSED:IDS_TRANSFER_PAUSING);
    else if(p.cancel)line=Text(IDS_UI_STOPPING);
    else if(p.transferBytesTotal){
        line=copiedBytes<=p.transferBytesTotal?InsertTextValues(Text(IDS_TRANSFER_COPIED),{{L"{copied}",copied},{L"{total}",TransferSize(p.transferBytesTotal)}}):copied;
        const auto limit=p.copyProgress.bytesPerSecond.load();
        if(p.transferRate || limit){
            line+=InsertTextValues(Text(limit?IDS_TRANSFER_RATE_LIMIT:IDS_TRANSFER_RATE),{{L"{rate}",TransferSize(p.transferRate)},{L"{limit}",TransferSize(limit)}});}
        // A copy limit does not throttle verification. Extrapolating weighted
        // copy throughput over both phases can overstate the ETA by minutes.
        // Wait until all planned copying has finished before estimating reads.
        if(total>work && p.transferWorkRate &&
            (!p.copyProgress.bytesPerSecond.load() || copiedBytes>=p.transferBytesTotal))
            line+=InsertTextValues(Text(IDS_TRANSFER_LEFT),{{L"{time}",TransferTime((total-work)/p.transferWorkRate+1)}});
        if(phase==dirsync::CopyPhase::Verifying)line+=L" — "+Text(IDS_PROGRESS_VERIFY);
    }else line=status+L" — "+Number(completed)+L" / "+Number(count);
    TransferText(p.transferWindow,IDC_TRANSFER_PHASE,line);
    auto caption=L"("+Number(uint64_t(totalPosition/10))+L" %) "+p.transferCaption+(paused?L" "+Text(IDS_TRANSFER_PAUSED):L"");
    TransferText(p.transferWindow,0,caption);
    TransferText(p.transferWindow,IDC_TRANSFER_PAUSE,Text(paused?IDS_TRANSFER_RESUME:IDS_TRANSFER_PAUSE));
    EnableWindow(GetDlgItem(p.transferWindow,IDC_TRANSFER_PAUSE),!p.cancel && !waiting && !p.transferCancelQuestion);
    EnableWindow(GetDlgItem(p.transferWindow,IDCANCEL),!p.cancel);
}
void TransferPause(Preview& p) {
    if(p.cancel || p.copyProgress.awaitingDecision || p.transferCancelQuestion)return;
    p.copyProgress.pauseRequested=!p.copyProgress.pauseRequested.load();UpdateTransfer(p);
}
void TransferMenu(Preview& p,bool keyboard) {
    if(!gui || p.cancel || p.copyProgress.awaitingDecision || p.transferCancelQuestion)return;
    const bool wasPaused=p.copyProgress.pauseRequested.load();p.copyProgress.pauseRequested=true;
    MENU_TEMPLATE_ITEM items[]={{MNTT_PB,0,0,0,-1,0,nullptr},
        {MNTT_IT,wasPaused?IDS_TRANSFER_RESUME:IDS_TRANSFER_PAUSE,MNTS_B|MNTS_I|MNTS_A,1,-1,0,nullptr},
        {MNTT_IT,IDS_TRANSFER_SET_SPEED,MNTS_B|MNTS_I|MNTS_A,2,-1,0,nullptr},
        {MNTT_PE,0,0,0,-1,0,nullptr}};
    DWORD command=0;auto* menu=gui->CreateMenuPopup();
    if(menu){if(menu->LoadFromTemplate(language,items,nullptr,nullptr,nullptr)){
        RECT rect{};GetWindowRect(GetDlgItem(p.transferWindow,IDC_TRANSFER_PAUSE),&rect);
        command=menu->Track(MENU_TRACK_RETURNCMD|MENU_TRACK_NONOTIFY|(keyboard?MENU_TRACK_SELECT:0),rect.left,rect.bottom,p.transferWindow,nullptr);}
        gui->DestroyMenuPopup(menu);}
    if(!p.running || !p.transferWindow || p.cancel)return;
    if(command==2){auto speed=p.copyProgress.bytesPerSecond.load();
        const bool accepted=ChooseTransferSpeed(p.transferWindow,speed,&p.transferSpeedWindow);
        if(!p.running || !p.transferWindow || p.cancel)return;
        if(accepted){p.copyProgress.bytesPerSecond=speed;settings.transferSpeed=speed;
            if(speed)settings.lastTransferSpeed=speed;
            p.transferSampleTick=0;p.transferRate=p.transferWorkRate=0;
            PersistSettings(p.transferWindow);}}
    p.copyProgress.pauseRequested=command==1?!wasPaused:wasPaused;UpdateTransfer(p);
}
bool ConfirmTransferCancel(Preview& p,bool closePreview=false) {
    if(p.transferCancelQuestion)return false;
    if(!p.running || p.cancel)return true;
    PreviewCallback callback;
    const bool wasPaused=p.copyProgress.pauseRequested.load();
    p.transferCancelQuestion=true;p.copyProgress.pauseRequested=true;UpdateTransfer(p);
    struct Question {Preview& p;~Question(){p.transferCancelQuestion=false;}} questionState{p};
    // Match the core's Yes/No question: repeated Escape cannot dismiss it.
    const auto question=Text(IDS_TRANSFER_CANCEL_CONFIRM),caption=Text(IDS_NAME);
    const UINT flags=MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2;
    int answer=IDNO;
    const HWND owner=closePreview || !p.transferWindow?p.window:p.transferWindow;
    if(general) {
        // The host's message-box service decodes UTF-8 (also on the tested
        // 0.15.1 base), supplies its own localized buttons and ignores Escape
        // unless MSGBOXEX_ESCAPEENABLED is explicitly requested.
        auto utf8=[](const std::wstring& value) {
            const int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.c_str(),-1,nullptr,0,nullptr,nullptr);
            if(n<=0)throw std::runtime_error("message-box encoding");
            std::string text(size_t(n),'\0');
            if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.c_str(),-1,text.data(),n,nullptr,nullptr))
                throw std::runtime_error("message-box encoding");
            return text;
        };
        const auto message=utf8(question),title=utf8(caption);
        answer=general->SalMessageBox(owner,message.c_str(),title.c_str(),flags);
    } else answer=MessageBoxW(owner,question.c_str(),caption.c_str(),flags);
    p.transferCancelQuestion=false;
    if(answer==IDYES){if(closePreview)p.closing=true;p.Stop();}
    else if(!p.cancel)p.copyProgress.pauseRequested=wasPaused;
    UpdateTransfer(p);
    return answer==IDYES;
}
LRESULT CALLBACK TransferButton(HWND button,UINT message,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR) {
    if(message==WM_GETDLGCODE)return DefSubclassProc(button,message,w,l)|DLGC_WANTALLKEYS|DLGC_WANTTAB;
    if(message==WM_KEYDOWN){
        if(w==VK_ESCAPE){SendMessageW(GetParent(button),WM_COMMAND,IDCANCEL,0);return 0;}
        if(w==VK_TAB || w==VK_LEFT || w==VK_RIGHT){const int controls[]={IDC_TRANSFER_MINIMIZE,IDC_TRANSFER_PAUSE,IDCANCEL};
            const int direction=(w==VK_LEFT || (w==VK_TAB && GetKeyState(VK_SHIFT)<0))?-1:1;
            int current=0;while(current<2 && GetDlgCtrlID(button)!=controls[current])++current;
            for(int step=1;step<=3;++step){HWND next=GetDlgItem(GetParent(button),controls[(current+direction*step+3)%3]);
                if(IsWindowEnabled(next)){SetFocus(next);break;}}return 0;}
        if((w==VK_UP || w==VK_DOWN) && GetDlgCtrlID(button)==IDC_TRANSFER_PAUSE){SendMessageW(GetParent(button),WM_USER_BUTTONDROPDOWN,IDC_TRANSFER_PAUSE,TRUE);return 0;}
        if(w==VK_RETURN || w==VK_SPACE){SendMessageW(GetParent(button),WM_COMMAND,GetDlgCtrlID(button),0);return 0;}
    }
    if(message==WM_KEYUP && (w==VK_SPACE || w==VK_RETURN))return 0;
    if(message==WM_NCDESTROY)RemoveWindowSubclass(button,TransferButton,id);
    return DefSubclassProc(button,message,w,l);
}
INT_PTR CALLBACK TransferDialog(HWND window,UINT message,WPARAM w,LPARAM l) {
    auto* p=reinterpret_cast<Preview*>(GetWindowLongPtrW(window,DWLP_USER));
    try{
        if(message==WM_INITDIALOG){p=reinterpret_cast<Preview*>(l);p->transferWindow=window;
            SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(p));
            wchar_t caption[256]{};GetWindowTextW(window,caption,256);p->transferCaption=caption;
            if(general)ConfigureDark(window);
            // Same CProgressBar implementation as the host copying dialog, via the pinned SDK.
            for(int id:{IDC_TRANSFER_FILE_BAR,IDC_TRANSFER_TOTAL_BAR}){
                auto* bar=gui?gui->AttachProgressBar(window,id):nullptr;
                if(id==IDC_TRANSFER_FILE_BAR)p->transferFileBar=bar;else p->transferTotalBar=bar;
                if(!bar){ // Standard-control fallback for allocation failure and the standalone harness.
                    HWND placeholder=GetDlgItem(window,id);RECT rect{};GetWindowRect(placeholder,&rect);
                    MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&rect),2);DestroyWindow(placeholder);
                    // The standard progress class supplies its own frame too.
                    CreateWindowExW(0,PROGRESS_CLASSW,nullptr,WS_CHILD|WS_VISIBLE,
                        rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,window,HMENU(INT_PTR(id)),instance,nullptr);
                    SendDlgItemMessageW(window,id,PBM_SETRANGE32,0,1000);
                }
            }
            if(gui)gui->AttachButton(window,IDC_TRANSFER_PAUSE,BTF_DROPDOWN);
            for(int id:{IDC_TRANSFER_MINIMIZE,IDC_TRANSFER_PAUSE,IDCANCEL})SetWindowSubclass(GetDlgItem(window,id),TransferButton,1,0);
            auto icon=LoadIconW(instance,MAKEINTRESOURCEW(IDI_DIRSYNC));
            if(icon){SendMessageW(window,WM_SETICON,ICON_SMALL,LPARAM(icon));SendMessageW(window,WM_SETICON,ICON_BIG,LPARAM(icon));}
            HWND tips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
                CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,window,nullptr,instance,nullptr);
            if(tips){SendMessageW(tips,TTM_SETMAXTIPWIDTH,0,700);
                for(int id:{IDC_TRANSFER_SOURCE,IDC_TRANSFER_TARGET,IDC_TRANSFER_PHASE}){TOOLINFOW info{sizeof(info)};
                    info.uFlags=TTF_IDISHWND|TTF_SUBCLASS;info.hwnd=window;info.uId=UINT_PTR(GetDlgItem(window,id));info.lpszText=LPSTR_TEXTCALLBACKW;
                    SendMessageW(tips,TTM_ADDTOOLW,0,LPARAM(&info));}}
            if(general){general->MultiMonCenterWindow(window,GetAncestor(GetParent(window),GA_ROOTOWNER),FALSE);}
            UpdateTransfer(*p);SetTimer(window,1,100,nullptr);return TRUE;}
        if(!p)return FALSE;
        LRESULT brush=0;if(dark && DarkModeHandleCtlColor(message,w,l,brush))return brush;
        switch(message){
        case WM_TIMER:if(!p->running || !TransferActive(*p)){p->CloseTransfer();return TRUE;}UpdateTransfer(*p);return TRUE;
        case WM_SIZE:if(p->worker.joinable())SetThreadPriority(p->worker.native_handle(),w==SIZE_MINIMIZED?THREAD_PRIORITY_BELOW_NORMAL:THREAD_PRIORITY_NORMAL);return FALSE;
        case WM_USER_BUTTONDROPDOWN:if(w==IDC_TRANSFER_PAUSE){TransferMenu(*p,LOWORD(l)!=0);return TRUE;}break;
        case WM_COMMAND:
            if(LOWORD(w)==IDC_TRANSFER_MINIMIZE){ShowWindow(window,SW_MINIMIZE);return TRUE;}
            if(LOWORD(w)==IDC_TRANSFER_PAUSE){TransferPause(*p);return TRUE;}
            if(LOWORD(w)!=IDCANCEL)break;[[fallthrough]];
        case WM_CLOSE:ConfirmTransferCancel(*p);return TRUE;
        case WM_KEYDOWN:if(w==VK_ESCAPE){ConfirmTransferCancel(*p);return TRUE;}break;
        case WM_NOTIFY:if(reinterpret_cast<NMHDR*>(l)->code==TTN_GETDISPINFOW){auto* tip=reinterpret_cast<NMTTDISPINFOW*>(l);
            auto display=p->copyProgress.Display();p->transferTip=tip->hdr.idFrom==UINT_PTR(GetDlgItem(window,IDC_TRANSFER_SOURCE))?display.source:
                tip->hdr.idFrom==UINT_PTR(GetDlgItem(window,IDC_TRANSFER_TARGET))?display.target:p->transferDetails;
            tip->lpszText=p->transferTip.data();return TRUE;}break;
        case WM_SETTINGCHANGE:case WM_THEMECHANGED:if(general)ConfigureDark(window);return TRUE;
        case WM_DESTROY:p->transferFileBar=nullptr;p->transferTotalBar=nullptr;KillTimer(window,1);if(p->transferWindow==window)p->transferWindow=nullptr;return TRUE;
        }
    }catch(...){if(p)p->Stop();return TRUE;}
    return FALSE;
}
void ShowTransfer(Preview& p) {
    if(!p.running || !TransferActive(p) || p.transferWindow)return;
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);
    if(!p.transferStarted)p.transferStarted=GetTickCount64();
    HWND window=CreateDialogParamW(language,MAKEINTRESOURCEW(IDD_TRANSFER_PROGRESS),p.window,TransferDialog,LPARAM(&p));
    if(window){ShowWindow(window,SW_SHOW);SetFocus(GetDlgItem(window,IDC_TRANSFER_MINIMIZE));}
    // The existing status-bar progress/Stop remain available if window creation fails.
}
