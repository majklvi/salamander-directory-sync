// SPDX-License-Identifier: GPL-2.0-or-later
// Included in the UI translation unit, after the localized formatting helpers.
constexpr UINT StoredReady=WM_APP+88;
struct StoredBrowser {
    HWND window=nullptr;
    std::wstring root,selected;
    dirsync::RecoveryCatalog catalog;
    std::atomic<bool> cancel{false};
    std::thread worker;
    bool loading=false;
    SIZE initial{},minimum{};
    std::vector<std::pair<HWND,RECT>> controls;
    int sizeFormat=0;
    void Join(){if(worker.joinable())worker.join();}
    ~StoredBrowser(){cancel=true;if(worker.joinable())CancelSynchronousIo(worker.native_handle());Join();}
    const dirsync::StoredRun* Run() const {
        int n=ListView_GetNextItem(GetDlgItem(window,IDC_STORED_RUNS),-1,LVNI_SELECTED);
        return !loading && n>=0 && size_t(n)<catalog.runs.size()?&catalog.runs[size_t(n)]:nullptr;
    }
    const dirsync::StoredOriginal* Item() const {
        auto run=Run();int n=ListView_GetNextItem(GetDlgItem(window,IDC_STORED_ITEMS),-1,LVNI_SELECTED);
        return run && n>=0 && size_t(n)<run->items.size()?&run->items[size_t(n)]:nullptr;
    }
};
int StoredCopyText(dirsync::StoredCopy copy) {
    using C=dirsync::StoredCopy;
    switch(copy){case C::Recorded:return IDS_STORED_RECORDED;case C::Missing:return IDS_STORED_MISSING;
    case C::Different:return IDS_STORED_DIFFERENT;case C::Unsafe:return IDS_STORED_UNSAFE;
    case C::Skipped:return IDS_UI_SKIPPED;default:return IDS_STORED_UNKNOWN;}
}
int StoredOperationText(dirsync::StoredOperation operation) {
    using O=dirsync::StoredOperation;
    switch(operation){case O::Replaced:return IDS_STORED_REPLACED;case O::Deleted:return IDS_STORED_DELETED;
    case O::NotStarted:return IDS_STORED_NOT_STARTED;case O::Failed:return IDS_STORED_FAILED;
    case O::Cancelled:return IDS_STORED_CANCELLED;case O::Skipped:return IDS_UI_SKIPPED;
    case O::DependencyBlocked:return IDS_UI_DEPENDENCY_BLOCKED;default:return IDS_STORED_UNKNOWN;}
}
void StoredRow(HWND list,int index,const std::vector<std::wstring>& cells) {
    for(size_t column=0;column<cells.size();++column){LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=index;
        item.iSubItem=int(column);item.pszText=const_cast<wchar_t*>(cells[column].c_str());
        SendMessageW(list,column?LVM_SETITEMTEXTW:LVM_INSERTITEMW,column?WPARAM(index):0,LPARAM(&item));}
}
void StoredButtons(StoredBrowser& b) {
    auto item=b.Item();auto run=b.Run();
    EnableWindow(GetDlgItem(b.window,IDC_STORED_REFRESH),!b.loading);
    for(int id:{IDC_STORED_COPY_ORIGINAL,IDC_STORED_COPY_SAVED,IDC_STORED_OPEN_ORIGINAL,IDC_STORED_OPEN_SAVED})
        EnableWindow(GetDlgItem(b.window,id),item && (id!=IDC_STORED_OPEN_SAVED || item->copy!=dirsync::StoredCopy::Unsafe));
    std::wstring note=Text(b.loading?IDS_STORED_LOADING:IDS_STORED_NOTE);
    if(!b.loading){note+=L"  "+InsertTextValues(Text(IDS_STORED_COUNTS),
            {{L"{runs}",Number(b.catalog.runs.size())},{L"{items}",Number(run?run->items.size():0)}});
        DWORD error=run && run->error?run->error:b.catalog.error;
        if(error)note+=L"  "+Text(IDS_STORED_ERROR)+L": "+Number(error);
        if(b.catalog.limited)note+=L"  "+Text(IDS_STORED_LIMIT);
        if(b.catalog.runs.empty() && !error)note=Text(IDS_STORED_EMPTY);}
    SetDlgItemTextW(b.window,IDC_STORED_SUMMARY,note.c_str());
}
void StoredItems(StoredBrowser& b) {
    HWND list=GetDlgItem(b.window,IDC_STORED_ITEMS);SendMessageW(list,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(list);
    if(auto run=b.Run()) {b.selected=run->path;int index=0;for(const auto& item:run->items)
        StoredRow(list,index++,{item.original,item.saved,item.directory?L"":PanelSize(item.size,b.sizeFormat),
            Text(StoredCopyText(item.copy)),Text(StoredOperationText(item.operation))});}
    if(ListView_GetItemCount(list))ListView_SetItemState(list,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    SendMessageW(list,WM_SETREDRAW,TRUE,0);InvalidateRect(list,nullptr,TRUE);StoredButtons(b);
}
void StoredLoad(StoredBrowser& b) {
    b.Join();b.cancel=false;b.loading=true;StoredButtons(b);
    b.worker=std::thread([&b]{try{b.catalog=dirsync::ReadRecoveryCatalog(b.root,b.cancel);}
        catch(...){b.catalog.error=ERROR_NOT_ENOUGH_MEMORY;}PostMessageW(b.window,StoredReady,0,0);});
}
void StoredCopyPath(StoredBrowser& b,const std::wstring& path) {
    if(!path.empty() && (path.size()>INT_MAX || !general->CopyTextToClipboardW(path.c_str(),int(path.size()),FALSE,b.window)))
        Message(b.window,IDS_UI_CLIPBOARD_ERROR);
}
void StoredOpen(StoredBrowser& b,const std::wstring& path,bool directory,bool saved) {
    std::wstring folder;auto run=b.Run();
    if(!run || (saved && !dirsync::RecoverySavedLocation(run->path,path,directory,folder)) ||
        (!saved && !dirsync::RecoveryLocation(path,directory,folder)) ||
        INT_PTR(ShellExecuteW(b.window,L"open",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)
        Message(b.window,IDS_STORED_OPEN_ERROR);
}
void StoredCommand(StoredBrowser& b,int command) {
    auto item=b.Item();auto run=b.Run();
    if(command==IDC_STORED_REFRESH){StoredLoad(b);return;}
    if(!run)return;
    if(command==IDS_STORED_COPY_RUN)StoredCopyPath(b,run->path);
    else if(command==IDS_STORED_COPY_MANIFEST)StoredCopyPath(b,run->path+L"\\manifest.jsonl");
    else if(command==IDS_STORED_OPEN_RUN)StoredOpen(b,run->path,true,false);
    else if(item)switch(command){
    case IDC_STORED_COPY_ORIGINAL:StoredCopyPath(b,item->original);break;
    case IDC_STORED_COPY_SAVED:StoredCopyPath(b,item->saved);break;
    case IDC_STORED_OPEN_ORIGINAL:StoredOpen(b,item->original,item->directory,false);break;
    case IDC_STORED_OPEN_SAVED:if(item->copy!=dirsync::StoredCopy::Unsafe)StoredOpen(b,item->saved,item->directory,true);break;
    }
}
void StoredMenu(StoredBrowser& b,HWND list,LPARAM position) {
    if(b.loading)return;
    POINT point{short(LOWORD(position)),short(HIWORD(position))};
    if(position==LPARAM(-1)){int n=ListView_GetNextItem(list,-1,LVNI_SELECTED);RECT box{};
        if(n<0 || !ListView_GetItemRect(list,n,&box,LVIR_BOUNDS))return;point={box.left,box.bottom};ClientToScreen(list,&point);
    }else{LVHITTESTINFO hit{};hit.pt=point;ScreenToClient(list,&hit.pt);int n=ListView_SubItemHitTest(list,&hit);
        if(n<0)return;ListView_SetItemState(list,n,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);}
    HMENU menu=CreatePopupMenu();if(!menu)return;
    bool runs=list==GetDlgItem(b.window,IDC_STORED_RUNS);
    auto add=[&](int command,const std::wstring& text,bool enabled=true){AppendMenuW(menu,MF_STRING|(enabled?0:MF_GRAYED),UINT_PTR(command),text.c_str());};
    if(runs){add(IDS_STORED_COPY_RUN,Text(IDS_STORED_COPY_RUN));add(IDS_STORED_COPY_MANIFEST,Text(IDS_STORED_COPY_MANIFEST));
        AppendMenuW(menu,MF_SEPARATOR,0,nullptr);add(IDS_STORED_OPEN_RUN,Text(IDS_STORED_OPEN_RUN));
    }else{for(int id:{IDC_STORED_COPY_ORIGINAL,IDC_STORED_COPY_SAVED,IDC_STORED_OPEN_ORIGINAL,IDC_STORED_OPEN_SAVED}){
        if(id==IDC_STORED_OPEN_ORIGINAL)AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
        const int labels[]{IDS_STORED_COPY_ORIGINAL_MENU,IDS_STORED_COPY_SAVED_MENU,IDS_STORED_OPEN_ORIGINAL_MENU,IDS_STORED_OPEN_SAVED_MENU};
        add(id,Text(labels[id-IDC_STORED_COPY_ORIGINAL]),IsWindowEnabled(GetDlgItem(b.window,id))!=FALSE);}}
    int command=int(TrackPopupMenuEx(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,b.window,nullptr));DestroyMenu(menu);
    if(command)StoredCommand(b,command);
}
INT_PTR CALLBACK StoredDialog(HWND window,UINT message,WPARAM w,LPARAM l) {
    auto* b=reinterpret_cast<StoredBrowser*>(GetWindowLongPtrW(window,DWLP_USER));
    try{
        if(message==WM_INITDIALOG){b=reinterpret_cast<StoredBrowser*>(l);b->window=window;
            SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(b));
            Preview format;ReadPanelFormat(format);b->sizeFormat=format.sizeFormat;
            for(int id:{IDC_STORED_RUNS,IDC_STORED_ITEMS}){HWND list=GetDlgItem(window,id);ListView_SetUnicodeFormat(list,TRUE);
                ListView_SetExtendedListViewStyle(list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_LABELTIP);
                std::vector<int> titles=id==IDC_STORED_RUNS?std::vector<int>{IDS_STORED_DATE,IDS_STORED_RUN,IDS_STORED_STATE}:
                    std::vector<int>{IDS_STORED_ORIGINAL,IDS_STORED_SAVED,IDS_STORED_SIZE,IDS_STORED_COPY,IDS_STORED_OPERATION};
                const int runWidths[]{115,220,140},itemWidths[]{120,120,40,100,100};
                for(size_t i=0;i<titles.size();++i){auto title=Text(titles[i]);RECT units{0,0,id==IDC_STORED_RUNS?runWidths[i]:itemWidths[i],0};MapDialogRect(window,&units);
                    LVCOLUMNW column{};column.mask=LVCF_TEXT|LVCF_WIDTH;column.pszText=title.data();column.cx=units.right;
                    SendMessageW(list,LVM_INSERTCOLUMNW,i,LPARAM(&column));}}
            RECT client{},outer{};GetClientRect(window,&client);GetWindowRect(window,&outer);
            b->initial={client.right,client.bottom};b->minimum={outer.right-outer.left,outer.bottom-outer.top};
            for(HWND child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
                RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);b->controls.push_back({child,r});}
            ConfigureDark(window);general->MultiMonCenterWindow(window,GetParent(window),TRUE);StoredLoad(*b);return TRUE;}
        if(!b)return FALSE;
        LRESULT brush=0;if(dark && DarkModeHandleCtlColor(message,w,l,brush))return brush;
        switch(message){
        case StoredReady:{b->Join();b->loading=false;HWND list=GetDlgItem(window,IDC_STORED_RUNS);
            ListView_DeleteAllItems(list);int selected=0;
            for(size_t i=0;i<b->catalog.runs.size();++i){const auto& run=b->catalog.runs[i];
                if(run.path==b->selected)selected=int(i);
                StoredRow(list,int(i),{Modified((uint64_t(run.created.dwHighDateTime)<<32)|run.created.dwLowDateTime),run.path.substr(run.path.find_last_of(L'\\')+1),
                    Text(run.error==ERROR_REVISION_MISMATCH?IDS_STORED_UNSUPPORTED:run.error?IDS_STORED_DAMAGED:run.complete?IDS_STORED_COMPLETE:IDS_STORED_INCOMPLETE)});}
            if(!b->catalog.runs.empty())ListView_SetItemState(list,selected,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
            StoredItems(*b);return TRUE;}
        case WM_NOTIFY:{auto* header=reinterpret_cast<NMHDR*>(l);
            if(header->idFrom==IDC_STORED_RUNS && header->code==LVN_ITEMCHANGED){if(!b->loading)StoredItems(*b);}
            if(header->idFrom==IDC_STORED_ITEMS && header->code==LVN_ITEMCHANGED)StoredButtons(*b);
            if((header->idFrom==IDC_STORED_RUNS || header->idFrom==IDC_STORED_ITEMS) && header->code==LVN_KEYDOWN){
                auto* key=reinterpret_cast<NMLVKEYDOWN*>(l);if(key->wVKey=='C' && (GetKeyState(VK_CONTROL)&0x8000)){
                    bool shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
                    StoredCommand(*b,header->idFrom==IDC_STORED_RUNS?(shift?IDS_STORED_COPY_MANIFEST:IDS_STORED_COPY_RUN):
                        (shift?IDC_STORED_COPY_SAVED:IDC_STORED_COPY_ORIGINAL));}
                else if(key->wVKey==VK_F5 && !b->loading)StoredLoad(*b);}
            if(header->idFrom==IDC_STORED_RUNS && header->code==NM_DBLCLK){if(auto run=b->Run())
                InformationBox(window,IDS_STORED_RUN,{{Text(IDS_STORED_RUN),run->path},
                    {Text(IDS_STORED_MANIFEST),run->path+L"\\manifest.jsonl"},
                    {Text(IDS_STORED_LEFT),run->left},{Text(IDS_STORED_RIGHT),run->right},
                    {Text(IDS_STORED_ERROR),Number(run->error)}},Text(IDS_STORED_NOTE));}
            if(header->idFrom==IDC_STORED_ITEMS && header->code==NM_DBLCLK){if(auto item=b->Item())
                InformationBox(window,IDS_STORED_SAVED,{{Text(IDS_STORED_ORIGINAL),item->original},{Text(IDS_STORED_SAVED),item->saved},
                    {Text(IDS_STORED_SIZE),item->directory?L"":PanelSize(item->size,b->sizeFormat)},
                    {Text(IDS_STORED_COPY),Text(StoredCopyText(item->copy))},{Text(IDS_STORED_OPERATION),Text(StoredOperationText(item->operation))},
                    {Text(IDS_STORED_ERROR),Number(item->error)}},Text(IDS_STORED_NOTE));}return FALSE;}
        case WM_CONTEXTMENU:if(HWND(w)==GetDlgItem(window,IDC_STORED_RUNS) || HWND(w)==GetDlgItem(window,IDC_STORED_ITEMS)){StoredMenu(*b,HWND(w),l);return TRUE;}break;
        case WM_GETMINMAXINFO:if(b->minimum.cx)reinterpret_cast<MINMAXINFO*>(l)->ptMinTrackSize={b->minimum.cx,b->minimum.cy};return TRUE;
        case WM_SIZE:if(b->initial.cx && w!=SIZE_MINIMIZED){int dx=LOWORD(l)-b->initial.cx,dy=HIWORD(l)-b->initial.cy;
            for(const auto& control:b->controls){auto r=control.second;int id=GetDlgCtrlID(control.first);
                if(id==IDC_STORED_ITEMS){r.right+=dx;r.bottom+=dy;}
                else if(id==IDC_STORED_RUNS)r.right+=dx;
                else if(id==IDC_STORED_SUMMARY){r.top+=dy;r.bottom+=dy;r.right+=dx;}
                else if(id==IDCANCEL || id==IDC_STORED_REFRESH || id==IDC_STORED_COPY_ORIGINAL || id==IDC_STORED_COPY_SAVED || id==IDC_STORED_OPEN_ORIGINAL || id==IDC_STORED_OPEN_SAVED){
                    r.top+=dy;r.bottom+=dy;if(id==IDCANCEL){r.left+=dx;r.right+=dx;}}
                MoveWindow(control.first,r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);}}return TRUE;
        case WM_SETTINGCHANGE:case WM_THEMECHANGED:ConfigureDark(window);return TRUE;
        case WM_COMMAND:if(LOWORD(w)!=IDCANCEL){StoredCommand(*b,LOWORD(w));return TRUE;}[[fallthrough]];
        case WM_CLOSE:b->cancel=true;if(b->worker.joinable())CancelSynchronousIo(b->worker.native_handle());b->Join();EndDialog(window,IDCANCEL);return TRUE;
        }
    }catch(...){b->cancel=true;EndDialog(window,IDCANCEL);return TRUE;}
    return FALSE;
}
void ShowStoredOriginals(HWND parent,const std::wstring& root) {
    StoredBrowser browser;browser.root=root;
    if(DialogBoxParamW(language,MAKEINTRESOURCEW(IDD_STORED_ORIGINALS),parent,StoredDialog,LPARAM(&browser))==-1)Message(parent,IDS_FAILED);
}

