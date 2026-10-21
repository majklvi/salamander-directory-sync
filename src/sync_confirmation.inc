// SPDX-License-Identifier: GPL-2.0-or-later
struct SyncCounts {uint64_t files=0,dirs=0,overwrites=0,bytes=0;};
SyncCounts CountSyncDirection(const dirsync::Result& result,const dirsync::SyncPlan& plan,bool right) {
    SyncCounts count;
    for(const auto* rows:{&plan.copies,&plan.overwrites})for(auto index:*rows) {
        const auto& row=result.rows[index];if((row.action==dirsync::Action::CopyRight)!=right)continue;
        const auto& source=right?row.left:row.right;
        if(rows==&plan.overwrites)++count.overwrites;else if(source.directory)++count.dirs;else ++count.files;
        if(!source.directory)count.bytes=TransferAdd(count.bytes,source.size);
    }
    return count;
}
std::wstring SyncCountText(const SyncCounts& count) {
    return InsertTextValues(Text(IDS_SYNC_COUNTS),{{L"{files}",Number(count.files)},{L"{dirs}",Number(count.dirs)},
        {L"{overwrites}",Number(count.overwrites)},{L"{bytes}",TransferSize(count.bytes)}});
}
struct SyncConfirmation {
    Preview& preview;bool keep;std::wstring recovery;
    dirsync::SyncSelection selection;
    dirsync::SyncPlan available,plan;
};
InfoRows SyncConfirmationFields(const SyncConfirmation& choice) {
    const auto& p=choice.preview;const auto& plan=choice.plan;
    auto right=CountSyncDirection(p.result,plan,true),left=CountSyncDirection(p.result,plan,false);
    uint64_t deleteDirs=0,originalBytes=0;
    for(auto i:plan.overwrites){const auto& row=p.result.rows[i];originalBytes=TransferAdd(originalBytes,(row.action==dirsync::Action::CopyRight?row.right:row.left).size);}
    for(auto i:plan.deletions){const auto& e=p.result.rows[i].right;if(e.directory)++deleteDirs;else originalBytes=TransferAdd(originalBytes,e.size);}
    InfoRows fields;Field(fields,IDS_UI_SCOPE_LABEL,Text(IDS_SYNC_SCOPE_SHORT));
    Field(fields,IDS_SYNC_ROOT_LEFT,p.left);Field(fields,IDS_SYNC_ROOT_RIGHT,p.right);
    Field(fields,IDS_UI_TO_RIGHT,SyncCountText(right));Field(fields,IDS_UI_TO_LEFT,SyncCountText(left));
    Field(fields,IDS_SYNC_NEW_COUNTS,Number(right.files+left.files)+L" / "+Number(right.dirs+left.dirs));
    Field(fields,IDS_SYNC_OVERWRITE,uint64_t(plan.overwrites.size()));
    Field(fields,IDS_SYNC_DELETE_COUNTS,Number(plan.deletions.size()-deleteDirs)+L" / "+Number(deleteDirs));
    Field(fields,IDS_UI_ORIGINAL_BYTES,originalBytes);Field(fields,IDS_UI_OMITTED,uint64_t(plan.omitted));
    Field(fields,IDS_SYNC_EXCLUDED,uint64_t(plan.excluded.size()));Field(fields,IDS_UI_HIDDEN,uint64_t(p.result.rows.size()-p.visible.size()));
    Field(fields,IDS_UI_RECOVERY,Text(choice.keep?IDS_UI_ON:IDS_UI_OFF));if(choice.keep)Field(fields,IDS_UI_RECOVERY_FOLDER,choice.recovery);
    return fields;
}
void UpdateSyncConfirmation(HWND window,SyncConfirmation& choice) {
    const auto& p=choice.preview;
    choice.selection={Checked(window,IDC_SYNC_RIGHT),Checked(window,IDC_SYNC_LEFT),Checked(window,IDC_SYNC_DELETE),Checked(window,IDC_SYNC_OVERWRITE)};
    choice.plan=dirsync::BuildSyncPlan(p.result,p.visible,choice.selection);
    // Available counts remain visible while unchecked; Details describes only
    // the selected subset. Choices never mutate the comparison or its filters.
    SetDlgItemTextW(window,IDC_SYNC_RIGHT_COUNTS,SyncCountText(CountSyncDirection(p.result,choice.available,true)).c_str());
    SetDlgItemTextW(window,IDC_SYNC_LEFT_COUNTS,SyncCountText(CountSyncDirection(p.result,choice.available,false)).c_str());
    EnableWindow(GetDlgItem(window,IDC_SYNC_RIGHT_COUNTS),choice.selection.toRight);
    EnableWindow(GetDlgItem(window,IDC_SYNC_LEFT_COUNTS),choice.selection.toLeft);
    uint64_t directories=0;for(auto i:choice.available.deletions)if(p.result.rows[i].right.directory)++directories;
    SetDlgItemTextW(window,IDC_SYNC_DELETE_COUNTS,InsertTextValues(Text(IDS_SYNC_DELETE_SHORT),
        {{L"{files}",Number(choice.available.deletions.size()-directories)},{L"{dirs}",Number(directories)}}).c_str());
    SetDlgItemTextW(window,IDC_SYNC_RECOVERY,Text(choice.keep?IDS_SYNC_KEEP_ON:IDS_SYNC_KEEP_OFF).c_str());
    SetDlgItemTextW(window,IDC_SYNC_RECOVERY_ROOT,!choice.keep?L"":choice.recovery.empty()?Text(IDS_SYNC_RECOVERY_EMPTY).c_str():choice.recovery.c_str());
    const bool destructive=!choice.plan.overwrites.empty() || !choice.plan.deletions.empty();
    SetDlgItemTextW(window,IDC_SYNC_NOTICE,Text(IDS_SYNC_WARNING_SHORT).c_str());
    EnableWindow(GetDlgItem(window,IDOK),choice.plan.Count()!=0);
    // Stack measured native captions and remove inapplicable rows. No blank
    // deletion/recovery panels; translated wrapping is measured at actual DPI.
    RECT units{7,3,300,12};MapDialogRect(window,&units);
    const int margin=units.left,gap=units.top,width=units.right-2*margin,line=units.bottom;
    int y=margin;HDC dc=GetDC(window);auto font=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(window,WM_GETFONT,0,0)));
    auto place=[&](int id,bool show,int indent,bool wrap) {
        HWND control=GetDlgItem(window,id);ShowWindow(control,show?SW_SHOW:SW_HIDE);if(!show)return;
        RECT measured{0,0,width-indent,0};auto text=ControlText(window,id);
        if(wrap)DrawTextW(dc,text.c_str(),int(text.size()),&measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
        int height=wrap?std::max(line,int(measured.bottom)):line;
        MoveWindow(control,margin+indent,y,width-indent,height,TRUE);y+=height+gap;
    };
    auto separator=[&](int id){RECT height{0,0,0,1};MapDialogRect(window,&height);y+=gap;
        MoveWindow(GetDlgItem(window,id),margin,y,width,std::max(1,int(height.bottom)),TRUE);y+=std::max(1,int(height.bottom))+2*gap;};
    place(IDC_SYNC_LEFT_ROOT,true,0,false);place(IDC_SYNC_RIGHT_ROOT,true,0,false);
    separator(IDC_SYNC_ROOT_SEPARATOR);
    for(auto pair:{std::pair{IDC_SYNC_RIGHT,IDC_SYNC_RIGHT_COUNTS},std::pair{IDC_SYNC_LEFT,IDC_SYNC_LEFT_COUNTS},std::pair{IDC_SYNC_DELETE,IDC_SYNC_DELETE_COUNTS}}) {
        const bool show=pair.first!=IDC_SYNC_DELETE || !choice.available.deletions.empty();
        place(pair.first,show,0,false);const int rowY=y;
        place(pair.second,show,pair.first==IDC_SYNC_DELETE?margin:4*margin,true);
        if(pair.first!=IDC_SYNC_DELETE){const int mark=pair.first==IDC_SYNC_RIGHT?IDC_SYNC_RIGHT_MARK:IDC_SYNC_LEFT_MARK;
            MoveWindow(GetDlgItem(window,mark),2*margin,rowY,2*margin,line,TRUE);}
    }
    place(IDC_SYNC_OVERWRITE,true,0,false);
    separator(IDC_SYNC_RECOVERY_SEPARATOR);
    // A single compact row: measured state caption, ellipsized configured path,
    // and the native Preferences entry point. OFF hides the unused path.
    RECT settingsButton{};GetWindowRect(GetDlgItem(window,IDC_SYNC_SETTINGS),&settingsButton);
    const int settingsWidth=settingsButton.right-settingsButton.left,rowHeight=std::max(line,int(settingsButton.bottom-settingsButton.top));
    RECT caption{};auto state=ControlText(window,IDC_SYNC_RECOVERY);
    DrawTextW(dc,state.c_str(),int(state.size()),&caption,DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX);
    const int captionWidth=std::min(int(caption.right),width-settingsWidth-4*gap-margin);
    MoveWindow(GetDlgItem(window,IDC_SYNC_RECOVERY),margin,y+(rowHeight-line)/2,captionWidth,line,TRUE);
    const int pathX=margin+captionWidth+gap,buttonX=margin+width-settingsWidth;
    MoveWindow(GetDlgItem(window,IDC_SYNC_RECOVERY_ROOT),pathX,y+(rowHeight-line)/2,buttonX-gap-pathX,line,TRUE);
    ShowWindow(GetDlgItem(window,IDC_SYNC_RECOVERY_ROOT),choice.keep?SW_SHOW:SW_HIDE);
    MoveWindow(GetDlgItem(window,IDC_SYNC_SETTINGS),buttonX,y,settingsWidth,rowHeight,TRUE);y+=rowHeight+2*gap;
    place(IDC_SYNC_NOTICE,!choice.keep && destructive,0,true);y+=gap;
    SelectObject(dc,font);ReleaseDC(window,dc);
    RECT button{};GetWindowRect(GetDlgItem(window,IDOK),&button);const int buttonHeight=button.bottom-button.top;
    for(int id:{IDC_SYNC_DETAILS,IDOK,IDCANCEL}){RECT r{};GetWindowRect(GetDlgItem(window,id),&r);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);
        MoveWindow(GetDlgItem(window,id),r.left,y,r.right-r.left,buttonHeight,TRUE);}
    RECT client{},outer{};GetClientRect(window,&client);GetWindowRect(window,&outer);
    SetWindowPos(window,nullptr,0,0,outer.right-outer.left,y+buttonHeight+margin+outer.bottom-outer.top-client.bottom,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    // Moving transparent native statics can leave their old captions painted
    // on the parent (especially when a recovery row disappears). Erase the
    // complete layout after every refresh, not just the controls' new bounds.
    RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
}
INT_PTR CALLBACK SyncConfirmationDialog(HWND window,UINT message,WPARAM w,LPARAM l) {
    PreviewCallback callback;
    auto* choice=reinterpret_cast<SyncConfirmation*>(GetWindowLongPtrW(window,DWLP_USER));
    try {
        if(message==WM_INITDIALOG) {
            choice=reinterpret_cast<SyncConfirmation*>(l);SetWindowLongPtrW(window,DWLP_USER,l);choice->preview.syncConfirmationWindow=window;
            auto& p=choice->preview;choice->available=dirsync::BuildSyncPlan(p.result,p.visible);
            for(auto side:{std::pair{IDC_SYNC_RIGHT,true},std::pair{IDC_SYNC_LEFT,false}}) {
                auto counts=CountSyncDirection(p.result,choice->available,side.second);bool enabled=counts.files+counts.dirs+counts.overwrites!=0;
                CheckDlgButton(window,side.first,enabled?BST_CHECKED:BST_UNCHECKED);EnableWindow(GetDlgItem(window,side.first),enabled);
            }
            CheckDlgButton(window,IDC_SYNC_DELETE,choice->available.deletions.empty()?BST_UNCHECKED:BST_CHECKED);
            EnableWindow(GetDlgItem(window,IDC_SYNC_DELETE),!choice->available.deletions.empty());
            CheckDlgButton(window,IDC_SYNC_OVERWRITE,choice->available.overwrites.empty()?BST_UNCHECKED:BST_CHECKED);
            EnableWindow(GetDlgItem(window,IDC_SYNC_OVERWRITE),!choice->available.overwrites.empty());
            SetDlgItemTextW(window,IDC_SYNC_LEFT_ROOT,(Text(IDS_SYNC_ROOT_LEFT)+L": "+p.left).c_str());
            SetDlgItemTextW(window,IDC_SYNC_RIGHT_ROOT,(Text(IDS_SYNC_ROOT_RIGHT)+L": "+p.right).c_str());
            HWND tips=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
                CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,window,nullptr,instance,nullptr);
            if(tips){SendMessageW(tips,TTM_SETMAXTIPWIDTH,0,700);
                for(int id:{IDC_SYNC_LEFT_ROOT,IDC_SYNC_RIGHT_ROOT,IDC_SYNC_RECOVERY_ROOT,IDC_SYNC_RIGHT,IDC_SYNC_LEFT,IDC_SYNC_DELETE}) {
                    TOOLINFOW tool{sizeof(tool)};tool.uFlags=TTF_IDISHWND|TTF_SUBCLASS;tool.hwnd=window;
                    tool.uId=UINT_PTR(GetDlgItem(window,id));tool.lpszText=LPSTR_TEXTCALLBACKW;SendMessageW(tips,TTM_ADDTOOLW,0,LPARAM(&tool));}}
            UpdateSyncConfirmation(window,*choice);DarkModeApplyWindow(window);DarkModeRefreshTitleBar(window);DarkModeApplyTree(window);
            if(general)general->MultiMonCenterWindow(window,GetParent(window),FALSE);return TRUE;
        }
        if(!choice)return FALSE;
        if(message==WM_DRAWITEM && (w==IDC_SYNC_RIGHT_MARK || w==IDC_SYNC_LEFT_MARK)){
            auto* draw=reinterpret_cast<DRAWITEMSTRUCT*>(l);const auto mark=w==IDC_SYNC_RIGHT_MARK?Mark::Right:Mark::Left;
            HBRUSH background=backgroundBrush?backgroundBrush:GetSysColorBrush(COLOR_BTNFACE);
            FillRect(draw->hDC,&draw->rcItem,background);
            COLORREF color=MarkColor(mark);
            if(!Checked(window,w==IDC_SYNC_RIGHT_MARK?IDC_SYNC_RIGHT:IDC_SYNC_LEFT)) {
                LOGBRUSH brush{};brush.lbColor=GetSysColor(COLOR_BTNFACE);GetObjectW(background,sizeof(brush),&brush);
                color=RGB((GetRValue(color)+GetRValue(brush.lbColor))/2,
                    (GetGValue(color)+GetGValue(brush.lbColor))/2,(GetBValue(color)+GetBValue(brush.lbColor))/2);
            }
            PaintMark(draw->hDC,draw->rcItem,mark,color,true);return TRUE;
        }
        LRESULT brush=0;if(dark && DarkModeHandleCtlColor(message,w,l,brush))return brush;
        if(message==WM_NOTIFY && reinterpret_cast<NMHDR*>(l)->code==TTN_GETDISPINFOW) {
            auto* tip=reinterpret_cast<NMTTDISPINFOW*>(l);auto id=GetDlgCtrlID(reinterpret_cast<HWND>(tip->hdr.idFrom));
            auto& p=choice->preview;
            p.tipText=id==IDC_SYNC_LEFT_ROOT?p.left:id==IDC_SYNC_RIGHT_ROOT?p.right:
                id==IDC_SYNC_RECOVERY_ROOT?(choice->recovery.empty()?Text(IDS_SYNC_RECOVERY_EMPTY):choice->recovery):Text(IDS_SYNC_SCOPE_SHORT);
            tip->lpszText=p.tipText.data();
            return TRUE;
        }
        if(message==WM_COMMAND) {
            const int id=LOWORD(w);
            if(id==IDC_SYNC_RIGHT || id==IDC_SYNC_LEFT || id==IDC_SYNC_DELETE || id==IDC_SYNC_OVERWRITE){UpdateSyncConfirmation(window,*choice);return TRUE;}
            if(id==IDC_SYNC_DETAILS){InformationBox(window,IDS_SYNC_CONFIRM,SyncConfirmationFields(*choice),Text(IDS_UI_ADS_NOTE)+L"\n\n"+Text(IDS_SYNC_POLICY),false,true,&choice->preview.syncDetailsWindow);return TRUE;}
            if(id==IDC_SYNC_SETTINGS){
                auto& p=choice->preview;ConfigurePreviewAppearance(p,window,&p.syncSettingsWindow);
                if(!IsWindow(window) || !p.window)return TRUE;
                choice->keep=settings.keepOriginals;choice->recovery=settings.recoveryRoot;
                UpdateSyncConfirmation(window,*choice);DarkModeApplyTree(window);DarkModeRefreshTitleBar(window);return TRUE;
            }
            if(id==IDCANCEL || (id==IDOK && choice->plan.Count())){EndDialog(window,id);return TRUE;}
        }
        if(message==WM_CLOSE){EndDialog(window,IDCANCEL);return TRUE;}
        if(message==WM_THEMECHANGED || message==WM_SETTINGCHANGE){DarkModeApplyTree(window);DarkModeRefreshTitleBar(window);return TRUE;}
    }catch(...){EndDialog(window,IDCANCEL);return TRUE;}
    return FALSE;
}
bool ChooseSynchronization(SyncConfirmation& choice) {
    PreviewCallback callback;
    const auto answer=DialogBoxParamW(language,MAKEINTRESOURCEW(IDD_SYNC_CONFIRMATION),choice.preview.window,SyncConfirmationDialog,LPARAM(&choice));
    choice.preview.syncConfirmationWindow=nullptr;
    if(answer==-1)Message(choice.preview.window,IDS_FAILED);
    return answer==IDOK;
}
