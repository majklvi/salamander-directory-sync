// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// Never included in the shipped SPL. Exercise the real Release vtable method
// with a worker in this DLL, then let the caller immediately FreeLibrary it.
#include "../src/dirsync.cpp"
#include <map>

namespace
{
std::unique_ptr<Preview> probe;
std::atomic<bool> finished{false};
std::atomic<bool> copyBoundary{false};
INT_PTR CALLBACK EmptyDialog(HWND,UINT,WPARAM,LPARAM) { return FALSE; }
bool StaticCaptionFits(HWND window,int id)
{
    HWND caption=GetDlgItem(window,id);wchar_t type[32]{};
    GetClassNameW(caption,type,int(std::size(type)));
    if(wcscmp(type,L"Static") || (GetWindowLongPtrW(caption,GWL_STYLE)&(WS_TABSTOP|WS_BORDER|WS_VSCROLL)))return false;
    RECT bounds{};GetClientRect(caption,&bounds);RECT text{0,0,bounds.right,0};
    HDC dc=GetDC(caption);auto font=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(caption,WM_GETFONT,0,0)));
    const auto value=ControlText(window,id);
    DrawTextW(dc,value.c_str(),int(value.size()),&text,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
    SelectObject(dc,font);ReleaseDC(caption,dc);
    return text.bottom<=bounds.bottom;
}
bool RecoveryLogged()
{
    if(probe->overwriting) return !probe->overwriteReport.keepOriginals || probe->overwriteReport.recovery.complete;
    return !probe->saving || (!probe->recoveryReport.journalError && !probe->recoveryReport.complete &&
        std::all_of(probe->recoveryReport.items.begin(),probe->recoveryReport.items.end(),
            [](const dirsync::RecoveryItem& item) { return item.logged; }));
}
}
// Real shipping progress resources and production callbacks; no file writes.
extern "C" __declspec(dllexport) BOOL WINAPI ProbeTransferDialog() try {
    struct RestoreLanguage {HINSTANCE previous=language;~RestoreLanguage(){language=previous;}} restore;
    bool ok=true;
    ok &= InsertTextValues(L"%n {a} {a} {unknown}",{{L"{a}",L"{b}"},{L"{b}",L"expanded"}})==L"%n {b} {b} {unknown}";
    struct NativeProgress final:CGUIProgressBarAbstract {
        DWORD value=0;
        void WINAPI SetProgress(DWORD progress,const char*) override {value=progress;}
        void WINAPI SetSelfMoveTime(DWORD) override {}
        void WINAPI SetSelfMoveSpeed(DWORD) override {}
        void WINAPI Stop() override {}
        void WINAPI SetProgress2(const CQuadWord&,const CQuadWord&,const char*) override {}
    } native;
    TransferBar(nullptr,0,&native,5,10);ok &= native.value==500;
    TransferBar(nullptr,0,&native,11,10);ok &= native.value==1000;
    TransferBar(nullptr,0,&native,0,0);ok &= native.value==DWORD(-1);
    uint64_t speed=7;
    ok &= ParseTransferSpeed(L"1",0,speed) && speed==1;
    ok &= ParseTransferSpeed(L"1",1,speed) && speed==1024;
    ok &= ParseTransferSpeed(L"1048576",2,speed) && speed==(uint64_t(1)<<40);
    ok &= ParseTransferSpeed(L"1024",3,speed) && speed==(uint64_t(1)<<40);
    ok &= ParseTransferSpeed(L" 2 ",2,speed) && speed==2*1024*1024;
    for(const auto* invalid:{L"",L" ",L"0",L"-1",L"1.5",L"1e3",L"1 2",L"1099511627777",L"999999999999999999999999"}) {
        speed=7;ok &= !ParseTransferSpeed(invalid,0,speed) && speed==7;
    }
    speed=7;ok &= !ParseTransferSpeed(L"1",4,speed) && speed==7;
    dirsync::TransferDisplay weighted;weighted.total=1000;weighted.copied=500;
    ok &= TransferWork(weighted,dirsync::CopyPhase::Copying)==682;
    weighted.verifyTotal=2000;weighted.verified=1000;
    ok &= TransferWork(weighted,dirsync::CopyPhase::Verifying)==2730;
    weighted.copied=weighted.total;weighted.verified=0;
    ok &= TransferWork(weighted,dirsync::CopyPhase::Copying)==TransferWork(weighted,dirsync::CopyPhase::Preparing) &&
        TransferWork(weighted,dirsync::CopyPhase::Preparing)==TransferWork(weighted,dirsync::CopyPhase::Verifying);
    weighted.finished=true;ok &= TransferWork(weighted,dirsync::CopyPhase::Verifying)==4096;
    for(const wchar_t* name:{L"dirsync_ui_tests.exe",L"dirsync_ui_czech_tests.exe"}) {
        HMODULE resources=LoadLibraryExW(name,nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
        if(!resources)return FALSE;language=resources;
        struct Template {DLGTEMPLATE dialog;WORD menu=0,cls=0,title=0;} owner{};
        owner.dialog.style=WS_POPUP;owner.dialog.cx=355;owner.dialog.cy=154;
        HWND parent=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&owner.dialog,nullptr,EmptyDialog,0);
        if(!parent){FreeLibrary(resources);return FALSE;}
        {
            HDC dc=GetDC(parent);auto old=SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));
            for(const auto& path:{std::wstring(L"C:\\short\\A & B-ž.txt"),L"C:\\"+std::wstring(300,L'漢')+L"\\žluťoučký.txt",
                L"C:\\"+std::wstring(300,L'漢')+L"-\U0001f4c1.txt",std::wstring(L"\\\\server\\share\\A & B-ž.txt")}) {
                for(int width:{20,100,450,6000}) {
                    const auto shown=ErrorDisplayPath(dc,path,width);SIZE full{},size{};
                    GetTextExtentPoint32W(dc,path.c_str(),int(path.size()),&full);
                    GetTextExtentPoint32W(dc,shown.c_str(),int(shown.size()),&size);ok &= size.cx<=width;
                    if(full.cx<=width)ok &= shown==path;
                    else {ok &= shown.find(L"...")!=std::wstring::npos;
                        for(size_t i=0;i<shown.size();++i){if(IS_HIGH_SURROGATE(shown[i])){ok &= i+1<shown.size() && IS_LOW_SURROGATE(shown[i+1]);++i;}
                            else ok &= !IS_LOW_SURROGATE(shown[i]);}
                        if(width>=100)ok &= shown.ends_with(L".txt");}
                }
            }
            SelectObject(dc,old);ReleaseDC(parent,dc);
        }
        // Real modeless error dialog and channel: a worker can be joined after
        // Stop/Close without dispatching another UI message or answering it.
        for(auto kind:{dirsync::TransferKind::Copy,dirsync::TransferKind::Save,dirsync::TransferKind::Overwrite,dirsync::TransferKind::Delete})for(bool combined:{false,true})for(int scenario=0;scenario<7;++scenario) {
            Preview p;p.window=parent;p.running=true;p.synchronizing=combined;p.copying=!combined && kind==dirsync::TransferKind::Copy;
            p.saving=!combined && kind==dirsync::TransferKind::Save;p.overwriting=!combined && kind==dirsync::TransferKind::Overwrite;p.deleting=!combined && kind==dirsync::TransferKind::Delete;
            std::atomic<bool> posted{false},done{false};dirsync::ErrorDecision result=dirsync::ErrorDecision::Cancel;
            dirsync::OperationError error;error.code=ERROR_SHARING_VIOLATION;error.kind=kind;error.site=kind==dirsync::TransferKind::Delete?dirsync::ErrorSite::Target:dirsync::ErrorSite::Source;
            error.source=L"C:\\"+std::wstring(280,L'漢')+L"\\žluťoučký.txt";error.target=L"D:\\kopie.txt";
            p.copyProgress.awaitingDecision=true;
            p.worker=std::thread([&]{result=p.errorQuestion.Ask(error,p.cancel,[&]{posted=true;return true;});done=true;});
            for(int attempt=0;attempt<200 && !posted;++attempt)Sleep(5);
            ok &= posted.load();ShowOperationError(p);HWND question=p.errorWindow;
            ok &= IsWindow(question) && IsWindowVisible(question) && IsWindowEnabled(parent) &&
                GetWindow(question,GW_OWNER)==p.transferWindow && ControlText(question,IDC_ERROR_SOURCE)==error.source &&
                !IsWindowEnabled(GetDlgItem(p.transferWindow,IDC_TRANSFER_PAUSE));
            ok &= ControlText(question,IDC_ERROR_CONTEXT)==Text(kind==dirsync::TransferKind::Delete?IDS_ERROR_DELETE_TARGET:kind==dirsync::TransferKind::Save?IDS_ERROR_SAVE_SOURCE:
                kind==dirsync::TransferKind::Overwrite?IDS_ERROR_OVERWRITE_SOURCE:IDS_ERROR_SOURCE);
            ok &= StaticCaptionFits(question,IDC_ERROR_DETAILS);
            for(int id:{IDC_ERROR_SOURCE,IDC_ERROR_TARGET}) {
                HWND path=GetDlgItem(question,id);const auto& value=id==IDC_ERROR_SOURCE?error.source:error.target;
                NMTTDISPINFOW tip{};tip.hdr.code=TTN_GETDISPINFOW;tip.hdr.idFrom=UINT_PTR(path);
                SendMessageW(question,WM_NOTIFY,0,LPARAM(&tip));
                ok &= tip.lpszText && std::wstring(tip.lpszText)==value;
                SetFocus(path);SendMessageW(path,EM_SETSEL,0,-1);DWORD start=0,end=0;
                SendMessageW(path,EM_GETSEL,WPARAM(&start),LPARAM(&end));
                ok &= start==0 && end==value.size() && ControlText(question,id)==value;
                SetFocus(GetDlgItem(question,IDC_ERROR_RETRY));
                // Rendering into a separate DC must not replace the control's
                // complete value with the visible ellipsis presentation.
                HDC dc=GetDC(path),memory=CreateCompatibleDC(dc);RECT box{};GetClientRect(path,&box);
                HBITMAP bitmap=CreateCompatibleBitmap(dc,box.right,box.bottom);auto old=SelectObject(memory,bitmap);
                SendMessageW(path,WM_PRINTCLIENT,WPARAM(memory),PRF_CLIENT);
                ok &= ControlText(question,id)==value;
                SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(path,dc);
            }
            if(scenario<4)SendMessageW(question,WM_COMMAND,scenario==0?IDC_ERROR_RETRY:scenario==1?IDC_ERROR_SKIP:scenario==2?IDC_ERROR_SKIP_ALL:IDCANCEL,0);
            else if(scenario==4)SendMessageW(question,WM_CLOSE,0,0);
            else if(scenario==5)SendMessageW(p.transferWindow,WM_CLOSE,0,0);
            else p.Stop();
            for(int attempt=0;attempt<200 && !done;++attempt)Sleep(5);
            ok &= done.load();if(!done)p.Stop();p.Join();
            ok &= result==(scenario==0?dirsync::ErrorDecision::Retry:scenario==1?dirsync::ErrorDecision::Skip:
                scenario==2?dirsync::ErrorDecision::SkipAll:dirsync::ErrorDecision::Cancel);
            ok &= !p.errorWindow && !IsWindow(question);p.running=false;p.CloseTransfer();
        }
        // Long confirmation/result explanations remain fully visible without
        // an edit border, scrollbar or an extra keyboard stop, also on resize.
        for(bool confirm:{false,true}) {
            bool captions=false;SetWindowLongPtrW(parent,GWLP_USERDATA,LONG_PTR(&captions));
            SetTimer(parent,59,20,[](HWND owner,UINT,UINT_PTR,DWORD){
                HWND dialog=GetLastActivePopup(owner);if(dialog==owner)return;KillTimer(owner,59);
                auto* data=reinterpret_cast<Information*>(GetWindowLongPtrW(dialog,DWLP_USER));
                bool fits=data && ControlText(dialog,IDC_INFO_NOTES)==data->notes && StaticCaptionFits(dialog,IDC_INFO_NOTES);
                RECT box{};GetWindowRect(dialog,&box);
                SetWindowPos(dialog,nullptr,0,0,box.right-box.left+160,box.bottom-box.top+80,SWP_NOMOVE|SWP_NOZORDER);
                fits &= StaticCaptionFits(dialog,IDC_INFO_NOTES);
                SetWindowPos(dialog,nullptr,0,0,box.right-box.left,box.bottom-box.top,SWP_NOMOVE|SWP_NOZORDER);
                fits &= StaticCaptionFits(dialog,IDC_INFO_NOTES);
                *reinterpret_cast<bool*>(GetWindowLongPtrW(owner,GWLP_USERDATA))=fits;
                SendMessageW(dialog,WM_COMMAND,IDCANCEL,0);
            });
            InfoRows rows(12,{L"Unicode žluťoučký 漢字 &",L"C:\\test & data\\漢字"});
            InformationBox(parent,IDS_SYNC_CONFIRM,std::move(rows),Text(IDS_UI_RESULT_NOTE)+L"\n\n"+Text(IDS_SYNC_POLICY)+L"\nA & B",confirm);
            ok &= captions;KillTimer(parent,59);SetWindowLongPtrW(parent,GWLP_USERDATA,0);
        }
        // A skipped original between two successful saves must not truncate
        // the saved count or claim that no independent overwrite ran.
        {
            Preview p;p.window=parent;p.overwriteReport.keepOriginals=true;
            p.overwriteReport.items.resize(3);p.overwriteReport.recovery.items.resize(3);
            for(int i=0;i<3;++i){auto& copy=p.overwriteReport.recovery.items[i];copy.logged=true;
                copy.copy.status=i==1?dirsync::CopyStatus::Skipped:dirsync::CopyStatus::Copied;
                p.overwriteReport.items[i].status=i==1?dirsync::CopyStatus::DependencyBlocked:dirsync::CopyStatus::Copied;}
            bool summary=false;SetWindowLongPtrW(parent,GWLP_USERDATA,LONG_PTR(&summary));
            SetTimer(parent,59,20,[](HWND owner,UINT,UINT_PTR,DWORD){
                HWND dialog=GetLastActivePopup(owner);if(dialog==owner)return;KillTimer(owner,59);
                auto* data=reinterpret_cast<Information*>(GetWindowLongPtrW(dialog,DWLP_USER));
                if(data){auto has=[&](int id,const wchar_t* value){return std::find(data->rows.begin(),data->rows.end(),
                    std::make_pair(Text(id),std::wstring(value)))!=data->rows.end();};
                    *reinterpret_cast<bool*>(GetWindowLongPtrW(owner,GWLP_USERDATA))=
                        data->title==IDS_UI_OVERWRITE_RESULT && has(IDS_UI_RECOVERED,L"2 / 3") &&
                        has(IDS_UI_REPLACED,L"2") && has(IDS_UI_ERRORS,L"0") && has(IDS_UI_DEPENDENCY_BLOCKED,L"1");}
                SendMessageW(dialog,WM_COMMAND,IDOK,0);
            });
            OverwriteSummary(p);ok &= summary;KillTimer(parent,59);SetWindowLongPtrW(parent,GWLP_USERDATA,0);
        }
        {
            Preview p;p.window=parent;p.result.rows.resize(3);p.deleteReport.keepOriginals=true;
            p.deleteReport.items.resize(3);p.deleteReport.recovery.items.resize(3);
            for(int i=0;i<3;++i){p.deleteReport.items[i].row=i;p.deleteReport.items[i].status=i==1?dirsync::DeleteStatus::DependencyBlocked:dirsync::DeleteStatus::Deleted;
                auto& saved=p.deleteReport.recovery.items[i];saved.logged=true;saved.copy.status=i==1?dirsync::CopyStatus::Skipped:dirsync::CopyStatus::Copied;}
            bool summary=false;SetWindowLongPtrW(parent,GWLP_USERDATA,LONG_PTR(&summary));
            SetTimer(parent,59,20,[](HWND owner,UINT,UINT_PTR,DWORD){
                HWND dialog=GetLastActivePopup(owner);if(dialog==owner)return;KillTimer(owner,59);
                auto* data=reinterpret_cast<Information*>(GetWindowLongPtrW(dialog,DWLP_USER));
                if(data){auto has=[&](int id,const wchar_t* value){return std::find(data->rows.begin(),data->rows.end(),std::make_pair(Text(id),std::wstring(value)))!=data->rows.end();};
                    *reinterpret_cast<bool*>(GetWindowLongPtrW(owner,GWLP_USERDATA))=data->title==IDS_UI_DELETE_RESULT && has(IDS_UI_RECOVERED,L"2 / 3") &&
                        has(IDS_UI_DELETED,L"2") && has(IDS_UI_ERRORS,L"0") && has(IDS_UI_DELETE_BLOCKED,L"1");}
                SendMessageW(dialog,WM_COMMAND,IDOK,0);
            });
            DeleteSummary(p);ok &= summary;KillTimer(parent,59);SetWindowLongPtrW(parent,GWLP_USERDATA,0);
        }
        {
            Preview p;p.window=parent;auto& r=p.syncReport;
            r.plan.copies.resize(3);r.plan.overwrites.resize(3);r.plan.deletions.resize(3);
            r.copies.items.resize(3);r.overwrites.items.resize(3);r.deletions.items.resize(3);
            r.copies.items[0].status=dirsync::CopyStatus::Copied;r.copies.items[1].status=dirsync::CopyStatus::Skipped;r.copies.items[1].error=ERROR_SHARING_VIOLATION;
            r.overwrites.items[0].status=dirsync::CopyStatus::Copied;r.overwrites.items[1].status=dirsync::CopyStatus::DependencyBlocked;
            r.overwrites.items[2].status=dirsync::CopyStatus::Failed;r.overwrites.items[2].error=r.error=ERROR_WRITE_FAULT;
            r.deletions.items[0].status=dirsync::DeleteStatus::Deleted;r.deletions.items[1].status=dirsync::DeleteStatus::Skipped;
            r.deletions.items[2].status=dirsync::DeleteStatus::DependencyBlocked;
            bool summary=false;SetWindowLongPtrW(parent,GWLP_USERDATA,LONG_PTR(&summary));
            SetTimer(parent,59,20,[](HWND owner,UINT,UINT_PTR,DWORD){
                HWND dialog=GetLastActivePopup(owner);if(dialog==owner)return;KillTimer(owner,59);
                auto* data=reinterpret_cast<Information*>(GetWindowLongPtrW(dialog,DWLP_USER));
                if(data){auto has=[&](int id,const wchar_t* value){return std::find(data->rows.begin(),data->rows.end(),std::make_pair(Text(id),std::wstring(value)))!=data->rows.end();};
                    *reinterpret_cast<bool*>(GetWindowLongPtrW(owner,GWLP_USERDATA))=data->title==IDS_SYNC_RESULT &&
                        std::any_of(data->rows.begin(),data->rows.end(),[](const auto& row){return row.first==Text(IDS_UI_ERROR_DETAILS) && row.second.find(Text(IDS_COPY_ERROR)+Number(ERROR_WRITE_FAULT))!=std::wstring::npos;}) &&
                        has(IDS_SYNC_NEW,L"1 / 3") && has(IDS_SYNC_REPLACED,L"1 / 3") && has(IDS_SYNC_DELETE,L"1 / 3") &&
                        has(IDS_UI_ERRORS,L"1") && has(IDS_UI_PENDING,L"1") && has(IDS_UI_SKIPPED,L"2") && has(IDS_STORED_DEPENDENCY_BLOCKED,L"2");}
                SendMessageW(dialog,WM_COMMAND,IDOK,0);
            });
            SyncSummary(p);ok &= summary;KillTimer(parent,59);SetWindowLongPtrW(parent,GWLP_USERDATA,0);
        }
        // Exercise the real modal speed dialog, not just its parser/template.
        for(int scenario=0;scenario<3;++scenario) {
            struct SpeedProbe {HWND dialog=nullptr;int scenario=0;bool ok=true;} speedProbe;
            speedProbe.scenario=scenario;SetWindowLongPtrW(parent,GWLP_USERDATA,LONG_PTR(&speedProbe));
            SetTimer(parent,58,20,[](HWND owner,UINT,UINT_PTR,DWORD) {
                auto* state=reinterpret_cast<SpeedProbe*>(GetWindowLongPtrW(owner,GWLP_USERDATA));
                HWND dialog=state->dialog;if(!dialog)return;KillTimer(owner,58);
                state->ok &= SendDlgItemMessageW(dialog,IDC_TRANSFER_SPEED_UNITS,CB_GETCOUNT,0,0)==4;
                if(state->scenario==0) {
                    state->ok &= IsDlgButtonChecked(dialog,IDC_TRANSFER_SPEED_UNLIMITED)==BST_CHECKED &&
                        !IsWindowEnabled(GetDlgItem(dialog,IDC_TRANSFER_SPEED_VALUE)) &&
                        ControlText(dialog,IDC_TRANSFER_SPEED_VALUE)==L"4" &&
                        SendDlgItemMessageW(dialog,IDC_TRANSFER_SPEED_UNITS,CB_GETCURSEL,0,0)==3;
                    SendDlgItemMessageW(dialog,IDC_TRANSFER_SPEED_ENABLE,BM_CLICK,0,0);
                    state->ok &= IsWindowEnabled(GetDlgItem(dialog,IDC_TRANSFER_SPEED_VALUE)) &&
                        IsDlgButtonChecked(dialog,IDC_TRANSFER_SPEED_UNLIMITED)==BST_UNCHECKED;
                    SetDlgItemTextW(dialog,IDC_TRANSFER_SPEED_VALUE,L"17");
                    SendDlgItemMessageW(dialog,IDC_TRANSFER_SPEED_UNITS,CB_SETCURSEL,0,0);
                }else if(state->scenario==1) {
                    state->ok &= IsDlgButtonChecked(dialog,IDC_TRANSFER_SPEED_ENABLE)==BST_CHECKED &&
                        ControlText(dialog,IDC_TRANSFER_SPEED_VALUE)==L"17";
                    SendDlgItemMessageW(dialog,IDC_TRANSFER_SPEED_UNLIMITED,BM_CLICK,0,0);
                    state->ok &= !IsWindowEnabled(GetDlgItem(dialog,IDC_TRANSFER_SPEED_UNITS));
                }else SetDlgItemTextW(dialog,IDC_TRANSFER_SPEED_VALUE,L"99");
                SendMessageW(dialog,WM_COMMAND,state->scenario==2?IDCANCEL:IDOK,0);
            });
            uint64_t chosen=scenario?17:0;
            const bool accepted=ChooseTransferSpeed(parent,chosen,&speedProbe.dialog);
            ok &= speedProbe.ok && accepted==(scenario!=2) && chosen==(scenario==1?0:17) && !speedProbe.dialog;
            KillTimer(parent,58);SetWindowLongPtrW(parent,GWLP_USERDATA,0);
        }
        {
            Preview p;p.window=parent;p.running=true;p.copying=true;p.transferStarted=GetTickCount64();p.operationReport.items.resize(4);
            const auto source=L"C:\\"+std::wstring(280,L'漢')+L"\\žluťoučký.txt";
            const auto target=L"D:\\"+std::wstring(280,L'Ž')+L"\\kopie.txt";
            p.copyProgress.BeginDisplay(source,target,dirsync::TransferKind::Copy,false,1000);
            p.copyProgress.CopyDisplay(1000,500);p.copyProgress.phase=dirsync::CopyPhase::Copying;p.copyProgress.completed=1;
            ShowTransfer(p);HWND progress=p.transferWindow;
            ok &= IsWindow(progress) && IsWindowVisible(progress) && GetWindow(progress,GW_OWNER)==parent;
            for(int id:{IDC_TRANSFER_FILE_BAR,IDC_TRANSFER_TOTAL_BAR}) {
                HWND bar=GetDlgItem(progress,id);
                ok &= !(GetWindowLongPtrW(bar,GWL_STYLE)&WS_BORDER) &&
                    !(GetWindowLongPtrW(bar,GWL_EXSTYLE)&(WS_EX_CLIENTEDGE|WS_EX_STATICEDGE));
            }
            ok &= ControlText(progress,IDC_TRANSFER_SOURCE)==source && ControlText(progress,IDC_TRANSFER_TARGET)==target;
            for(int id:{IDC_TRANSFER_SOURCE,IDC_TRANSFER_TARGET,IDC_TRANSFER_PHASE}) {
                HWND path=GetDlgItem(progress,id);ok &= (GetWindowLongPtrW(path,GWL_STYLE)&SS_NOTIFY)!=0;
                NMTTDISPINFOW tip{};tip.hdr.code=TTN_GETDISPINFOW;tip.hdr.idFrom=UINT_PTR(path);
                SendMessageW(progress,WM_NOTIFY,0,LPARAM(&tip));
                ok &= tip.lpszText && std::wstring(tip.lpszText)==(id==IDC_TRANSFER_SOURCE?source:id==IDC_TRANSFER_TARGET?target:p.transferDetails);
            }
            ok &= SendDlgItemMessageW(progress,IDC_TRANSFER_FILE_BAR,PBM_GETPOS,0,0)==500;
            ok &= SendDlgItemMessageW(progress,IDC_TRANSFER_TOTAL_BAR,PBM_GETPOS,0,0)==250;
            ok &= p.transferDetails.find(L"1 / 4")!=std::wstring::npos;
            ok &= p.transferDetails.find(L'{')==std::wstring::npos &&
                p.transferDetails.find(L"0:00:")!=std::wstring::npos;
            // Limited copying and unrestricted verification have different
            // rates: do not project the slow copy rate over remaining reads.
            p.transferBytesTotal=1000;p.transferWorkTotal=3000;
            p.transferSampleTick=GetTickCount64();p.transferWorkRate=1000;p.copyProgress.copied=500;
            const auto eta=InsertTextValues(Text(IDS_TRANSFER_LEFT),{{L"{time}",TransferTime(3)}});
            UpdateTransfer(p);const auto unlimitedLine=ControlText(progress,IDC_TRANSFER_PHASE);
            p.copyProgress.bytesPerSecond=1024;UpdateTransfer(p);
            const auto limitedLine=ControlText(progress,IDC_TRANSFER_PHASE);
            ok &= unlimitedLine.find(eta)!=std::wstring::npos && limitedLine.find(eta)==std::wstring::npos;
            p.copyProgress.copied=1000;UpdateTransfer(p);
            ok &= ControlText(progress,IDC_TRANSFER_PHASE).find(eta)!=std::wstring::npos;
            p.copyProgress.bytesPerSecond=0;p.transferBytesTotal=p.transferWorkTotal=0;
            p.copyProgress.VerifyDisplay(1000);p.copyProgress.VerifiedDisplay(250);p.copyProgress.phase=dirsync::CopyPhase::Verifying;
            UpdateTransfer(p);ok &= SendDlgItemMessageW(progress,IDC_TRANSFER_FILE_BAR,PBM_GETPOS,0,0)==250;
            ok &= ControlText(progress,IDC_TRANSFER_PHASE).find(Text(IDS_PROGRESS_VERIFY))!=std::wstring::npos;
            p.copyProgress.BeginDisplay(source,target,dirsync::TransferKind::Save,true);UpdateTransfer(p);
            ok &= (GetWindowLongPtrW(GetDlgItem(progress,IDC_TRANSFER_FILE_BAR),GWL_STYLE)&PBS_MARQUEE)!=0;
            ok &= ControlText(progress,IDC_TRANSFER_FILE_BAR)==L"—";
            SendMessageW(progress,WM_COMMAND,IDC_TRANSFER_PAUSE,0);
            ok &= p.copyProgress.pauseRequested && ControlText(progress,IDC_TRANSFER_PAUSE)==Text(IDS_TRANSFER_RESUME);
            SendMessageW(GetDlgItem(progress,IDC_TRANSFER_MINIMIZE),WM_KEYDOWN,VK_TAB,0);
            ok &= GetFocus()==GetDlgItem(progress,IDC_TRANSFER_PAUSE);
            SendMessageW(GetDlgItem(progress,IDC_TRANSFER_PAUSE),WM_KEYDOWN,VK_RETURN,0);
            ok &= !p.copyProgress.pauseRequested;
            SendMessageW(progress,WM_COMMAND,IDC_TRANSFER_MINIMIZE,0);ok &= IsIconic(progress)!=FALSE;
            ShowWindow(progress,SW_RESTORE);ok &= !IsIconic(progress);
            SendMessageW(progress,WM_CLOSE,0,0);
            ok &= p.cancel && IsWindow(progress) && !IsWindowEnabled(GetDlgItem(progress,IDCANCEL));
            p.running=false;SendMessageW(progress,WM_TIMER,1,0);
            ok &= !p.transferWindow && !IsWindow(progress);
            p.cancel=false;p.running=true;ShowTransfer(p);progress=p.transferWindow;
            SendMessageW(GetDlgItem(progress,IDCANCEL),WM_KEYDOWN,VK_ESCAPE,0);
            ok &= p.cancel && IsWindow(progress);p.CloseTransfer();
            p.cancel=false;p.running=true;ShowTransfer(p);progress=p.transferWindow;
            SetWindowLongPtrW(parent,GWLP_USERDATA,LONG_PTR(&p));
            SetTimer(parent,57,20,[](HWND owner,UINT,UINT_PTR, DWORD) {
                auto* pending=reinterpret_cast<Preview*>(GetWindowLongPtrW(owner,GWLP_USERDATA));
                pending->running=false;pending->CloseTransfer();KillTimer(owner,57);
            });
            speed=7;
            ok &= !ChooseTransferSpeed(progress,speed,&p.transferSpeedWindow) && speed==7 &&
                !p.transferSpeedWindow && !p.transferWindow && !IsWindow(progress);
            KillTimer(parent,57);SetWindowLongPtrW(parent,GWLP_USERDATA,0);
            ok &= TransferPosition(UINT64_MAX-1,UINT64_MAX)==999 && TransferPosition(5,10)==500 && TransferPosition(11,10)==1000;
        }
        std::atomic<bool> ended{false};HWND visible=nullptr;
        {
            Preview p;p.window=parent;p.running=true;p.copying=true;
            p.worker=std::thread([&]{while(!p.cancel)Sleep(1);ended=true;});ShowTransfer(p);visible=p.transferWindow;
            ok &= IsWindow(visible)!=FALSE;
            // Preview destruction requests Stop, joins, then removes the owned window.
        }
        ok &= ended && !IsWindow(visible);
        DestroyWindow(parent);language=restore.previous;FreeLibrary(resources);
    }
    return ok;
}catch(...){return FALSE;}
// Exercise the same Unicode path expansion used by the native folder picker.
extern "C" __declspec(dllexport) BOOL WINAPI ProbeTimeComparison()
{
    dirsync::Options options;bool ok=true;
    auto read=[&](int enabled,int seconds,bool present=true) {
        ReadTimeComparison(options,[&](int id,int& value) {
            if(id==SALCFG_COMPDIRSUSETIMERES) {value=enabled;return present;}
            if(id==SALCFG_COMPDIRTIMERES) {value=seconds;return present;}
            return false;
        });
    };
    read(1,2);ok &= options.tolerance==20000000 && options.wholeSecondTolerance;
    const auto snapshot=options;
    read(0,2);ok &= options.tolerance==0 && !options.wholeSecondTolerance;
    ok &= snapshot.tolerance==20000000 && snapshot.wholeSecondTolerance;
    for(int seconds:{0,1,5,3600}) {
        read(1,seconds);ok &= options.tolerance==uint64_t(seconds)*10000000 && options.wholeSecondTolerance;
    }
    for(int seconds:{-1,3601}) {
        read(1,seconds);ok &= options.tolerance==30000000 && !options.wholeSecondTolerance;
    }
    read(2,2);ok &= options.tolerance==30000000 && !options.wholeSecondTolerance;
    read(1,2,false);ok &= options.tolerance==30000000 && !options.wholeSecondTolerance;
    ReadTimeComparison(options,[](int id,int& value) {value=1;return id==SALCFG_COMPDIRSUSETIMERES;});
    ok &= options.tolerance==30000000 && !options.wholeSecondTolerance;
    return ok;
}
extern "C" __declspec(dllexport) unsigned WINAPI ProbeDisplayFormatting()
{
    unsigned checks=0;
    auto check=[&](bool ok) {if(!ok) throw std::runtime_error("panel display formatting");++checks;};
    try {
        BYTE lower[256]{},upper[256]{};
        for(unsigned i=0;i<256;++i) lower[i]=upper[i]=BYTE(i);
        for(unsigned i='A';i<='Z';++i) lower[i]=BYTE(i+'a'-'A');
        for(unsigned i='a';i<='z';++i) upper[i]=BYTE(i+'A'-'a');
        auto name=[&](const char* input,int format,bool directory=false) {
            return dirsync::UnicodeNameCase(input,format,directory,lower,upper);
        };
        check(name("HELLO WORLD.TXT",1)=="Hello World.Txt");
        check(name("HELLO WORLD.TXT",2)=="hello world.txt");
        check(name("hello world.txt",3)=="HELLO WORLD.TXT");
        check(name("MiXeD.Txt",4)=="MiXeD.Txt");
        check(name("FOO.TXT",5)=="Foo.txt");
        check(name("LONGERTHAN8.TXT",5)=="LONGERTHAN8.TXT");
        check(name("mixed.txt",5)=="mixed.txt");
        check(name("FOO.TXT",6)=="foo.txt");
        check(name("foo.txt",6,true)=="FOO.TXT");
        check(name("HELLO WORLD.TXT",7)=="Hello World.txt");
        // Ordinary u8 literals have char8_t in C++20: keep UTF-8 in byte strings.
        std::wstring original=std::wstring(300,L'Ž')+L" žluťoučký-漢字-😀.TXT";
        auto utf8=[](const std::wstring& text) {
            int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.c_str(),-1,nullptr,0,nullptr,nullptr);
            std::string bytes(size_t(n),'\0');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.c_str(),-1,bytes.data(),n,nullptr,nullptr);
            bytes.resize(size_t(n)-1);return bytes;
        };
        const auto bytes=utf8(original);
        for(int mode:{1,2,3,4,6,7}) {
            auto transformed=dirsync::UnicodeNameCase(bytes,mode,false,lower,upper);std::wstring wide;
            check(dirsync::DecodePath(transformed.c_str(),wide) && wide.find(L"漢字-😀")!=std::wstring::npos);
            if(mode==2) check(wide==std::wstring(300,L'ž')+L" žluťoučký-漢字-😀.txt");
            if(mode==3) check(wide==std::wstring(300,L'Ž')+L" ŽLUŤOUČKÝ-漢字-😀.TXT");
        }
        std::vector<bool> kinds;
        const auto path=L"Parent\\"+original;
        const auto displayed=dirsync::PanelRelativePath(path,false,6,[&](const std::wstring& part,int mode,bool dir) {
            kinds.push_back(dir);std::wstring text;
            dirsync::DecodePath(dirsync::UnicodeNameCase(utf8(part),mode,dir,lower,upper).c_str(),text);return text;
        });
        check(kinds==std::vector<bool>({true,false}));
        check(displayed==L"PARENT\\"+std::wstring(300,L'ž')+L" žluťoučký-漢字-😀.txt");
        check(path==L"Parent\\"+original);
        for(int column=0;column<10;++column) {
            check(ColumnCaptionId(column,true)==IDS_COLUMNS+column);
            check(ColumnCaptionId(column,false)==(column==0 || column==9?IDS_COLUMN_NAME:
                column==1 || column==4?IDS_COLUMN_SIZE:column==2 || column==5?IDS_COLUMN_MODIFIED:IDS_COLUMNS+column));
        }
        check(ColumnOrder==std::array<int,10>{0,1,2,3,5,4,9,6,7,8});
        check(dirsync::MetadataDifference(0,UINT64_MAX)==UINT64_MAX);
        check(dirsync::MetadataDifference(UINT64_MAX,0)==UINT64_MAX);
        check(dirsync::MetadataDifference(42,42)==0);
        const std::array<std::wstring,4> units{L"d",L"h",L"min",L"s"};
        check(dirsync::RelativeDuration(10000000,units,L"sub")==L"1 s");
        check(dirsync::RelativeDuration(9999999,units,L"sub")==L"sub");
        check(dirsync::RelativeDuration(937840000000,units,L"sub")==L"1 d 2 h 3 min 4 s");
        check(dirsync::RelativeDuration(36000000000,units,L"sub")==L"1 h");
        check(dirsync::RelativeDuration(UINT64_MAX,units,L"sub")==L"21350398 d 5 h 36 min 10 s");
        check(dirsync::RelativeValue(L"o {0} B větší",L"1 024")==L"o 1 024 B větší");
        dirsync::Row row;row.path=path;row.hasLeft=true;
        Preview p;p.left=L"C:\\left";p.result.rows.push_back(row);
        check(RowPaths(p,{0},0)==path); // Clipboard retains the real spelling.
        return checks;
    } catch(...) {return 0;}
}
extern "C" __declspec(dllexport) unsigned WINAPI ProbeFolderPath(const wchar_t* expected)
{
    std::wstring full;
    if(!FolderSelectionPath(expected,full) || full!=expected) return 0;
    auto api=dirsync::ApiPath(expected);
    DWORD size=GetShortPathNameW(api.c_str(),nullptr,0);
    if(!size) return 0;
    std::vector<wchar_t> alias(size);
    DWORD count=GetShortPathNameW(api.c_str(),alias.data(),size);
    if(!count || count>=size || !FolderSelectionPath(alias.data(),full) || full!=expected) return 0;
    if(FolderSelectionPath(std::wstring(expected)+L"\\missing-picker-folder",full) || !full.empty()) return 0;
    return 3;
}
// Check the production proposal/path helpers without executing file operations.
extern "C" __declspec(dllexport) unsigned WINAPI ProbeProposalRules(const wchar_t* left,const wchar_t* right)
{
    Preview p;p.left=left;p.right=right;
    unsigned checks=0;auto check=[&](bool ok) {if(!ok) throw std::runtime_error("proposal rule");++checks;};
    try {
        using A=dirsync::Action;using S=dirsync::State;
        dirsync::Row pair;pair.hasLeft=pair.hasRight=true;pair.state=S::Different;pair.action=A::CopyRight;
        pair.path=std::wstring(90,L'階')+L"\\"+std::wstring(90,L'長')+L"\\"+std::wstring(90,L'文')+L"\\žluťoučký-漢字.txt";
        auto onlyLeft=pair;onlyLeft.hasRight=false;onlyLeft.state=S::LeftOnly;onlyLeft.path=L"left-漢字.txt";
        auto onlyRight=pair;onlyRight.hasLeft=false;onlyRight.state=S::RightOnly;onlyRight.action=A::CopyLeft;onlyRight.path=L"right-ž.txt";
        auto equal=pair;equal.state=S::Equal;equal.action=A::None;equal.path=L"equal.txt";
        auto blocked=pair;blocked.state=S::Blocked;blocked.action=A::Blocked;
        auto folder=onlyLeft;folder.left.directory=true;
        p.result.rows={pair,onlyLeft,onlyRight,equal,blocked,folder};
        for(const auto& row:p.result.rows) p.defaults.push_back(row.action);
        A action{};
        // Rows: two-sided, left-only, right-only, equal, blocked, directory.
        const bool allowed[6][7]={{true,true,true,false,true,true,true},
            {true,true,false,false,true,true,false},{true,false,true,false,true,true,false},
            {true,true,true,false,true,true,false},{false,false,false,false,false,false,false},
            {false,false,false,false,false,false,false}};
        for(size_t row=0;row<6;++row) for(int command=0;command<7;++command)
            check(Proposal(p,row,command,action)==allowed[row][command]);
        check(!Proposal(p,99,0,action));
        p.result.rows[5].left.empty=true;
        check(Proposal(p,5,0,action) && action==A::None);
        check(Proposal(p,5,1,action) && action==A::CopyRight);
        check(Proposal(p,5,RestoreProposal,action) && action==A::CopyRight);
        p.result.rows[5].left.empty=false;
        // Menu commands need a valid change for at least one selected row.
        check(!CanChangeProposals(p,{0,1,2,3,4,5},RestoreProposal));
        check(!CanChangeProposals(p,{0,1},int(A::CopyRight)));
        check(!CanChangeProposals(p,{2},int(A::CopyLeft)));
        check(CanChangeProposals(p,{0,2},int(A::CopyLeft)));
        check(!CanChangeProposals(p,{0,2,4,5},int(A::CopyRight)));
        check(CanChangeProposals(p,{1,2,3,4,5},int(A::CopyRight)));
        check(!CanChangeProposals(p,{1,4,5,99},int(A::CopyLeft)));
        check(!CanChangeProposals(p,{},RestoreProposal));
        check(!CanChangeProposals(p,{99},int(A::CopyRight)));
        check(!CanChangeProposals(p,{3},int(A::None)));
        check(CanChangeProposals(p,{0,3,4,5},int(A::None)));
        auto counts=ChangeProposals(p,{0,4,5},0);
        check(counts.first==1 && counts.second==2);
        check(p.result.rows[0].action==A::None && p.result.rows[1].action==A::CopyRight && p.result.rows[4].action==A::Blocked);
        check(CanChangeProposals(p,{0,1,2,4,5},RestoreProposal));
        check(CanChangeProposals(p,{0,1,4,5},int(A::CopyRight)));
        counts=ChangeProposals(p,{0},RestoreProposal);check(counts.first==1 && p.result.rows[0].action==A::CopyRight);
        check(!CanChangeProposals(p,{0,1,2,4,5},RestoreProposal));
        counts=ChangeProposals(p,{0,1,2,4,5},int(A::CopyLeft));
        check(counts.first==1 && counts.second==4 && p.result.rows[0].action==A::CopyLeft);
        counts=ChangeProposals(p,{0,1,2,4,5},RestoreProposal);
        check(counts.first==1 && counts.second==4 && p.result.rows[0].action==A::CopyRight);
        counts=ChangeProposals(p,{0},ReverseProposal);check(counts.first==1 && p.result.rows[0].action==A::CopyLeft);
        p.options.asymmetric=true;
        check(!Proposal(p,0,2,action) && !Proposal(p,0,ReverseProposal,action));
        check(Proposal(p,2,3,action) && action==A::DeleteRight);
        check(!Proposal(p,1,3,action) && !Proposal(p,0,3,action));
        check(!Proposal(p,2,RestoreProposal,action)); // a mode change must discard old results
        auto relative=RowPaths(p,{0,1,2},0);check(relative==pair.path+L"\r\n"+onlyLeft.path+L"\r\n"+onlyRight.path);
        std::wstring root;check(dirsync::NormalizeRoot(left,root));
        check(RowPaths(p,{0,1,2},1)==root+L"\\"+pair.path+L"\r\n"+root+L"\\"+onlyLeft.path);
        check(dirsync::NormalizeRoot(right,root));
        check(RowPaths(p,{0,1,2},2)==root+L"\\"+pair.path+L"\r\n"+root+L"\\"+onlyRight.path);
        check(RowPaths(p,{},0).empty() && RowPaths(p,{99},1).empty() && RowPaths(p,{1},2).empty());
        check(ChangeProposals(p,{},0).first==0);
        INITCOMMONCONTROLSEX classes{sizeof(classes),ICC_LISTVIEW_CLASSES};check(InitCommonControlsEx(&classes)!=FALSE);
        struct OwnedWindow {HWND value;~OwnedWindow(){if(value) DestroyWindow(value);}};
        OwnedWindow list{CreateWindowExW(0,WC_LISTVIEWW,L"",WS_POPUP|LVS_REPORT|LVS_OWNERDATA,0,0,400,200,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)};
        check(list.value!=nullptr);LVCOLUMNW column{};column.mask=LVCF_WIDTH;column.cx=400;
        SendMessageW(list.value,LVM_INSERTCOLUMNW,0,LPARAM(&column));ListView_SetItemCount(list.value,6);
        RECT bounds{};check(ListView_GetItemRect(list.value,5,&bounds,LVIR_BOUNDS)!=FALSE);
        check(PreviewHit(list.value,{10,(bounds.top+bounds.bottom)/2})==5);
        check(PreviewHit(list.value,{10,bounds.bottom+10})==-1);
        check(PreviewHit(list.value,{-1,bounds.top})==-1);
        check(!PlanEnabled(p,PlanSummary));
        p.result.snapshot=dirsync::Compare(left,right,p.options,p.cancel,p.progress).snapshot;
        check(PlanEnabled(p,PlanSummary));
        check(!PlanEnabled(p,PlanCopy) && !PlanEnabled(p,PlanOverwrite));
        for(auto& row:p.result.rows) row.left.identified=row.right.identified=true;
        p.visible={1};check(PlanEnabled(p,PlanCopy));
        check(!PlanEnabled(p,PlanOverwrite) && !PlanEnabled(p,PlanSave));
        p.visible={0};check(PlanEnabled(p,PlanOverwrite));
        const bool keep=settings.keepOriginals;settings.keepOriginals=true;
        bool saveOn=PlanEnabled(p,PlanSave);settings.keepOriginals=false;
        bool saveOff=PlanEnabled(p,PlanSave);settings.keepOriginals=keep;
        check(saveOn && !saveOff);
        p.visible={2};p.result.rows[2].action=A::DeleteRight;
        check(PlanEnabled(p,PlanDelete) && !PlanEnabled(p,PlanCopy));
        p.result.stale=true;check(!PlanEnabled(p,PlanSummary) && !PlanEnabled(p,PlanDelete));p.result.stale=false;
        p.result.cancelled=true;check(!PlanEnabled(p,PlanSummary));p.result.cancelled=false;
        p.failed=true;check(!PlanEnabled(p,PlanDelete));p.failed=false;
        p.running=true;check(!PlanEnabled(p,PlanSummary));p.running=false;
        p.options.asymmetric=false;p.result.snapshot=dirsync::Compare(left,right,p.options,p.cancel,p.progress).snapshot;
        check(!PlanEnabled(p,PlanDelete));
        check(!PlanEnabled(p,PlanExport));p.reportReady=true;
        p.result.stale=true;p.result.snapshot.reset();check(PlanEnabled(p,PlanExport) && !PlanEnabled(p,PlanSync));
        p.running=true;check(!PlanEnabled(p,PlanExport));p.running=false;
        p.result.cancelled=true;p.failed=true;check(PlanEnabled(p,PlanExport));
    } catch(...) {return 0;}
    return checks;
}
// Real native toggle states, option handlers and list/status geometry; no executor.
extern "C" __declspec(dllexport) BOOL WINAPI ProbeModelessPreview() try {
    bool ok=true;std::atomic<bool> ended{false};
    struct Template {DLGTEMPLATE dialog;WORD menu=0,cls=0,title=0;} layout{};
    layout.dialog.style=WS_POPUP|WS_CAPTION;layout.dialog.cx=180;layout.dialog.cy=70;
    auto ownerProcedure=+[](HWND window,UINT message,WPARAM,LPARAM)->INT_PTR {
        if(message==WM_APP+27){SetWindowLongPtrW(window,DWLP_USER,1);return TRUE;}return FALSE;
    };
    HWND host=CreateDialogIndirectParamW(instance,&layout.dialog,nullptr,ownerProcedure,0);
    if(!host)return FALSE;
    auto procedure=+[](HWND window,UINT message,WPARAM w,LPARAM l)->INT_PTR {
        if(message==WM_INITDIALOG){auto* p=reinterpret_cast<Preview*>(l);p->window=window;
            SetWindowLongPtrW(window,DWLP_USER,l);return TRUE;}
        return Dialog(window,message,w,l);
    };
    modelessPreview=std::make_unique<Preview>();auto& p=*modelessPreview;p.modeless=true;active=&p;
    HWND window=CreateDialogIndirectParamW(instance,&layout.dialog,nullptr,procedure,LPARAM(&p));
    if(!window){active=nullptr;modelessPreview.reset();DestroyWindow(host);return FALSE;}
    SetWindowPos(window,nullptr,0,0,4000,4000,SWP_NOZORDER|SWP_NOACTIVATE);
    PositionPreview(p,host);RECT fitted{};MONITORINFO monitor{sizeof(monitor)};
    GetWindowRect(window,&fitted);GetMonitorInfoW(MonitorFromWindow(host,MONITOR_DEFAULTTONEAREST),&monitor);
    ok &= fitted.left>=monitor.rcWork.left && fitted.top>=monitor.rcWork.top &&
        fitted.right<=monitor.rcWork.right && fitted.bottom<=monitor.rcWork.bottom;
    HWND first=CreateWindowExW(0,L"BUTTON",L"First",WS_CHILD|WS_VISIBLE|WS_TABSTOP,5,5,50,20,window,HMENU(17),instance,nullptr);
    HWND second=CreateWindowExW(0,L"BUTTON",L"Second",WS_CHILD|WS_VISIBLE|WS_TABSTOP,60,5,50,20,window,HMENU(18),instance,nullptr);
    previewKeyboardHook=SetWindowsHookExW(WH_GETMESSAGE,PreviewKeyboard,nullptr,GetCurrentThreadId());
    ok &= previewKeyboardHook && IsWindowEnabled(host) && GetWindow(window,GW_OWNER)==nullptr;
    ShowWindow(window,SW_SHOW);SetFocus(first);
    PostMessageW(first,WM_KEYDOWN,VK_TAB,0);PostMessageW(host,WM_APP+27,0,0);
    MSG message{};for(int iteration=0;iteration<20 && !GetWindowLongPtrW(host,DWLP_USER);++iteration)
        if(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
    ok &= GetFocus()==second && GetWindowLongPtrW(host,DWLP_USER)==1 && IsWindowEnabled(host);
    // An owned modal dialog has its own message loop. The preview's hook must
    // leave both its F5 input and its Tab navigation to that loop.
    auto popupProcedure=+[](HWND popup,UINT msg,WPARAM key,LPARAM)->INT_PTR {
        if(msg==WM_KEYDOWN && key==VK_F5){SetWindowLongPtrW(popup,DWLP_USER,1);return TRUE;}
        if(msg==WM_APP+28){SetWindowLongPtrW(popup,DWLP_USER,GetWindowLongPtrW(popup,DWLP_USER)|2);return TRUE;}
        return FALSE;
    };
    HWND popup=CreateDialogIndirectParamW(instance,&layout.dialog,window,popupProcedure,0);
    if(popup) {
        HWND popupFirst=CreateWindowExW(0,L"BUTTON",L"First",WS_CHILD|WS_VISIBLE|WS_TABSTOP,5,5,50,20,popup,HMENU(17),instance,nullptr);
        HWND popupSecond=CreateWindowExW(0,L"BUTTON",L"Second",WS_CHILD|WS_VISIBLE|WS_TABSTOP,60,5,50,20,popup,HMENU(18),instance,nullptr);
        EnableWindow(window,FALSE);ShowWindow(popup,SW_SHOW);SetFocus(popupFirst);
        PostMessageW(popupFirst,WM_KEYDOWN,VK_TAB,0);PostMessageW(popup,WM_KEYDOWN,VK_F5,0);PostMessageW(popup,WM_APP+28,0,0);
        for(int iteration=0;iteration<30 && !(GetWindowLongPtrW(popup,DWLP_USER)&2);++iteration)
            if(PeekMessageW(&message,nullptr,0,0,PM_REMOVE) && !IsDialogMessageW(popup,&message)){
                TranslateMessage(&message);DispatchMessageW(&message);}
        ok &= GetWindow(popup,GW_OWNER)==window && !IsWindowEnabled(window) &&
            GetFocus()==popupSecond && GetWindowLongPtrW(popup,DWLP_USER)==3;
        EnableWindow(window,TRUE);DestroyWindow(popup);SetActiveWindow(window);SetFocus(second);
    }else ok=false;
    p.running=true;p.worker=std::thread([&]{while(!p.cancel)Sleep(1);ended=true;});
    SendMessageW(window,WM_CLOSE,0,0);
    ok &= p.cancel && p.closing && IsWindow(window) && IsWindowEnabled(host);
    p.Join();p.running=false;EndPreview(p);
    ok &= ended && !p.window && !active && !previewKeyboardHook && !IsWindow(window) && IsWindowEnabled(host);
    modelessPreview.reset();
    // Forced release must synchronously destroy a modeless window and join its
    // worker before the caller is allowed to FreeLibrary this module.
    modelessPreview=std::make_unique<Preview>();auto& forced=*modelessPreview;forced.modeless=true;active=&forced;
    window=CreateDialogIndirectParamW(instance,&layout.dialog,nullptr,procedure,LPARAM(&forced));
    ended=false;forced.running=true;forced.worker=std::thread([&]{while(!forced.cancel)Sleep(1);ended=true;});
    ok &= !plugin.Release(nullptr,FALSE) && !forced.cancel;
    ok &= plugin.Release(nullptr,TRUE) && ended && !active && !modelessPreview && !IsWindow(window) && IsWindowEnabled(host);
    DestroyWindow(host);return ok;
}catch(...){return FALSE;}
extern "C" __declspec(dllexport) BOOL WINAPI ProbePreviewControls(const wchar_t* left,const wchar_t* right) try
{
    struct Template {DLGTEMPLATE dialog;WORD menu=0,cls=0,title=0;} dialog{};
    dialog.dialog.style=WS_POPUP;dialog.dialog.cx=610;dialog.dialog.cy=470;
    auto procedure=+[](HWND window,UINT message,WPARAM w,LPARAM l)->INT_PTR {
        return (message==WM_COMMAND || message==WM_NOTIFY)?DialogBody(window,message,w,l):FALSE;
    };
    HWND window=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&dialog.dialog,nullptr,procedure,0);
    if(!window) return FALSE;
    bool ok=true;Preview p;p.window=window;p.left=left;p.right=right;p.panelLeft=L"panel-left";p.panelRight=L"panel-right";
    SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(&p));
    const int filters[]={IDC_EQUAL,IDC_DIFFERENT,IDC_SINGLE,IDC_TORIGHT,IDC_TOLEFT};
    for(size_t i=0;i<5;++i) {
        HWND button=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|BS_CHECKBOX|BS_PUSHLIKE|BS_ICON,0,0,24,24,window,HMENU(INT_PTR(filters[i])),instance,nullptr);
        ok &= button!=nullptr;
        p.filterChecks[i]=true;CheckDlgButton(window,filters[i],BST_CHECKED);
    }
    for(int id:{IDC_COMPARE,IDC_SYNCHRONIZE})
        ok &= CreateWindowExW(0,L"BUTTON",L"",WS_CHILD,645,id==IDC_COMPARE?39:69,132,21,window,HMENU(INT_PTR(id)),instance,nullptr)!=nullptr;
    MoveWindow(GetDlgItem(window,IDC_EQUAL),528,39,21,21,FALSE);
    for(int id:{IDC_EQUAL,IDC_COMPARE,IDC_SYNCHRONIZE}) {
        RECT r{};HWND child=GetDlgItem(window,id);GetWindowRect(child,&r);
        MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);p.controls.push_back({child,r});
    }
    for(int id:{IDC_RECURSIVE,IDC_CONTENT,IDC_IGNORE,IDC_ASYMMETRIC,IDC_EMPTY,IDC_SELECTED})
        ok &= CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|BS_AUTOCHECKBOX,0,0,20,20,window,HMENU(INT_PTR(id)),instance,nullptr)!=nullptr;
    for(int id:{IDC_LEFT,IDC_RIGHT})
        ok &= CreateWindowExW(0,L"EDIT",id==IDC_LEFT?left:right,WS_CHILD,0,0,20,20,window,HMENU(INT_PTR(id)),instance,nullptr)!=nullptr;
    HWND list=CreateWindowExW(0,WC_LISTVIEWW,L"",WS_CHILD|LVS_REPORT,8,100,500,300,window,HMENU(INT_PTR(IDC_LIST)),instance,nullptr);
    HWND bar=CreateWindowExW(0,STATUSCLASSNAMEW,L"",WS_CHILD,0,440,610,30,window,HMENU(INT_PTR(IDC_STATUS)),instance,nullptr);
    ok &= list!=nullptr && bar!=nullptr;
    SetWindowSubclass(list,PreviewList,1,0);SetWindowSubclass(ListView_GetHeader(list),PreviewHeader,1,0);
    for(int i=0;i<10;++i) {LVCOLUMNW column{};column.mask=LVCF_WIDTH;column.cx=100;SendMessageW(list,LVM_INSERTCOLUMNW,i,LPARAM(&column));}
    ok &= ListView_SetColumnOrderArray(list,int(ColumnOrder.size()),ColumnOrder.data())!=FALSE;
    std::array<int,10> nativeOrder{};
    ok &= ListView_GetColumnOrderArray(list,int(nativeOrder.size()),nativeOrder.data())!=FALSE && nativeOrder==ColumnOrder;
    RECT operation{},rightTime{},rightSize{},rightName{};
    HWND headerWindow=ListView_GetHeader(list);
    ok &= Header_GetItemRect(headerWindow,3,&operation) && Header_GetItemRect(headerWindow,5,&rightTime) &&
        Header_GetItemRect(headerWindow,4,&rightSize) && Header_GetItemRect(headerWindow,9,&rightName) &&
        operation.right==rightTime.left && rightTime.right==rightSize.left && rightSize.right==rightName.left;
    p.result=dirsync::Compare(left,right,p.options,p.cancel,p.progress);auto snapshot=p.result.snapshot;
    dirsync::Row row;row.path=L"skipped-漢字.txt";row.state=dirsync::State::Different;row.action=dirsync::Action::None;
    p.result.rows={row};p.defaults={dirsync::Action::CopyRight};p.visible={0};Filter(p);
    CheckDlgButton(window,IDC_RECURSIVE,BST_CHECKED);p.options.selection={{L"item",false}};
    for(int id:{IDC_RECURSIVE,IDC_CONTENT,IDC_IGNORE,IDC_ASYMMETRIC,IDC_EMPTY,IDC_SELECTED}) {
        CheckDlgButton(window,id,Checked(window,id)?BST_UNCHECKED:BST_CHECKED);
        DialogBody(window,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),0);
        ok &= p.result.snapshot==snapshot && p.result.rows.size()==1 && p.result.rows[0].action==dirsync::Action::None &&
            p.defaults[0]==dirsync::Action::CopyRight && ListView_GetItemCount(list)==1;
    }
    ok &= PendingOptions(p) && !p.options.asymmetric && !p.options.content && !p.options.onlySelected && p.options.recursive;
    ok &= ControlText(window,IDC_LEFT)==left && ControlText(window,IDC_RIGHT)==right;
    for(unsigned bits=0;bits<32;++bits) {
        // Reach every combination through native click notification delivery,
        // not by assigning the desired model/check state directly.
        for(size_t i=0;i<5;++i) if(p.filterChecks[i]!=((bits&(1u<<i))!=0))
            SendDlgItemMessageW(window,filters[i],BM_CLICK,0,0);
        for(size_t i=0;i<5;++i) {
            HWND button=GetDlgItem(window,filters[i]);SendMessageW(button,WM_KILLFOCUS,0,0);SendMessageW(button,WM_PAINT,0,0);
            ok &= Checked(window,filters[i])==((bits&(1u<<i))!=0);
            ok &= p.filterChecks[i]==((bits&(1u<<i))!=0);
            ok &= (SendMessageW(button,BM_GETSTATE,0,0)&BST_PUSHED)==0;
        }
        ok &= p.visible.size()==((bits&2u)?1u:0u) && ListView_GetItemCount(list)==int(p.visible.size());
    }
    for(int pass=0;pass<3;++pass) for(bool enabled:{false,true}) for(int id:filters) {
        SendDlgItemMessageW(window,id,BM_CLICK,0,0);
        ok &= Checked(window,id)==enabled;
    }
    for(bool enabled:p.filterChecks) ok &= enabled;
    // Categories combine by union; direction switches further restrict copies.
    std::array<bool,5> mixed{false,true,true,true,false};
    row.action=dirsync::Action::CopyRight;ok &= RowVisible(row,mixed);
    row.state=dirsync::State::LeftOnly;ok &= RowVisible(row,mixed);
    row.action=dirsync::Action::CopyLeft;ok &= !RowVisible(row,mixed);
    mixed[4]=true;ok &= RowVisible(row,mixed);
    row.state=dirsync::State::Equal;row.action=dirsync::Action::None;ok &= !RowVisible(row,mixed);
    mixed[0]=true;ok &= RowVisible(row,mixed);
    row.state=dirsync::State::Blocked;mixed.fill(false);ok &= RowVisible(row,mixed);
    const bool previousRowColors=settings.colorRows,previousDark=dark;
    for(bool night:{false,true}) {
        dark=night;
        COLORREF bg=night?RGB(32,32,32):RGB(255,255,255);
        row.state=dirsync::State::Different;row.action=dirsync::Action::None;
        ok &= PreviewColor(row,0,bg,RGB(1,2,3))==MarkColor(Mark::Skip);
        COLORREF base=MarkColor(Mark::Different),faded=PreviewColor(row,6,bg,RGB(1,2,3));
        ok &= GetRValue(faded)==(GetRValue(base)+GetRValue(bg))/2 &&
            GetGValue(faded)==(GetGValue(base)+GetGValue(bg))/2 && GetBValue(faded)==(GetBValue(base)+GetBValue(bg))/2;
        row.action=dirsync::Action::CopyRight;ok &= PreviewColor(row,6,bg,RGB(1,2,3))==base;
        for(bool enabled:{false,true}) {
            settings.colorRows=enabled;
            for(auto action:{dirsync::Action::CopyRight,dirsync::Action::CopyLeft,dirsync::Action::DeleteRight,
                             dirsync::Action::Decide,dirsync::Action::Blocked,dirsync::Action::None}) {
                row.action=action;
                for(int column:{0,1,2,4,5,7,8,9}) {
                    const COLORREF expected=action==dirsync::Action::None?MarkColor(Mark::Skip):
                        enabled?MarkColor(RowMark(row,true)):RGB(1,2,3);
                    ok &= PreviewColor(row,column,bg,RGB(1,2,3))==expected;
                }
            }
        }
    }
    settings.colorRows=previousRowColors;dark=previousDark;
    // Native status parts track the operation; each metric is a separate pane.
    auto partCount=[&]() {Status(p);return int(SendMessageW(bar,SB_GETPARTS,0,0));};
    ok &= partCount()==5;
    p.running=true;ok &= partCount()==3;
    p.progress.readBytes=1024;ok &= partCount()==4;
    p.copying=true;p.copyRows={0,1};ok &= partCount()==5;
    p.cancel=true;ok &= partCount()==5;
    p.cancel=false;p.copying=false;p.running=false;p.progress.readBytes=0;
    for(int dpi:{96,120,144,192,240}) {
        HFONT font=CreateFontW(-MulDiv(11,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"MS Shell Dlg");
        SendMessageW(bar,WM_SETFONT,WPARAM(font),FALSE);
        int initial[5]={0,0,0,0,-1};SendMessageW(bar,SB_SETPARTS,5,LPARAM(initial));
        const std::wstring full=L"Unicode žluťoučký 漢字: 18446744073709551615";
        for(int i=0;i<5;++i) StatusPane(window,i,full);
        for(int width:{450,900,1600,700}) {
            SetWindowPos(window,nullptr,0,0,width,600,SWP_NOMOVE|SWP_NOZORDER);SizeStatus(window);
            int edges[5]={};ok &= SendMessageW(bar,SB_GETPARTS,5,LPARAM(edges))==5;
            RECT area{};GetClientRect(bar,&area);
            ok &= edges[0]>0 && edges[0]>=area.right/5 && edges[4]==-1;
            for(int i=1;i<4;++i) ok &= edges[i]>edges[i-1] && edges[i]<area.right;
            wchar_t tip[128]{};SendMessageW(bar,SB_GETTIPTEXTW,MAKEWPARAM(4,128),LPARAM(tip));
            ok &= full==tip;
        }
        SendMessageW(bar,WM_SETFONT,WPARAM(GetStockObject(DEFAULT_GUI_FONT)),FALSE);DeleteObject(font);
    }
    Status(p);
    for(int width:{900,1200,900,1200}) {
        SetWindowPos(window,nullptr,0,0,width,600,SWP_NOMOVE|SWP_NOZORDER);SizeStatus(window);
        LayoutFilters(p,width-900);
        RECT previous{};
        for(size_t i=0;i<5;++i) {
            RECT r{};HWND button=GetDlgItem(window,filters[i]);GetWindowRect(button,&r);
            MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);
            ok &= r.right-r.left==21 && r.bottom-r.top==21 && Checked(window,filters[i]);
            ok &= r.top==(width==1200 || i<3?39:69);
            if(i && (width==1200 || i!=3)) ok &= r.left>previous.right;
            if(width==1200) ok &= r.right<645+(width-900);
            previous=r;
        }
        RECT client{},bounds{},status{};GetClientRect(window,&client);GetWindowRect(list,&bounds);GetWindowRect(bar,&status);
        MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&bounds),2);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&status),2);
        ok &= bounds.left==0 && bounds.right==client.right && bounds.bottom==status.top;
    }
    // Headers are native groups, never synthetic file rows. Duplicate basenames
    // remain distinct model indices through filtering and proposal changes.
    p.filterChecks.fill(true);p.result.rows.clear();
    for(const auto* path:{L"root.txt",L"alpha\\same.txt",L"alpha\\nested\\same.txt",L"beta-漢字\\same.txt",L"delete-dir"}) {
        dirsync::Row entry;entry.path=path;entry.state=dirsync::State::LeftOnly;
        entry.action=dirsync::Action::CopyRight;entry.hasLeft=true;p.result.rows.push_back(entry);
    }
    p.result.rows[4].hasLeft=false;p.result.rows[4].hasRight=true;p.result.rows[4].right.directory=true;
    p.result.rows[4].state=dirsync::State::RightOnly;p.result.rows[4].action=dirsync::Action::DeleteRight;
    Filter(p);
    ok &= p.visible.size()==5 && ListView_GetItemCount(list)==5 && ListView_GetGroupCount(list)==4;
    ok &= p.groups==std::vector<std::wstring>{L"",L"alpha",L"alpha\\nested",L"beta-漢字"};
    ok &= Cell(p.result.rows[1],0,p)==L"same.txt" && Cell(p.result.rows[1],9,p).empty();
    ok &= Cell(p.result.rows[4],0,p).empty() && Cell(p.result.rows[4],9,p)==L"delete-dir\\";
    auto paired=p.result.rows[1];paired.hasRight=true;
    ok &= Cell(paired,0,p)==L"same.txt" && Cell(paired,9,p)==L"same.txt";
    paired.right.directory=true;
    ok &= Cell(paired,0,p)==L"same.txt" && Cell(paired,9,p)==L"same.txt\\";
    for(size_t i=0;i<p.visible.size();++i) {
        LVITEMW item{};item.mask=LVIF_GROUPID;item.iItem=int(i);
        ok &= SendMessageW(list,LVM_GETITEMW,0,LPARAM(&item))!=FALSE;
        ok &= p.groups[size_t(item.iGroupId)]==RowDirectory(p.result.rows[p.visible[i]]);
    }
    RECT header{};ok &= ListView_GetGroupRect(list,0,LVGGR_HEADER,&header)!=FALSE;
    ok &= PreviewHit(list,{10,(header.top+header.bottom)/2})==-1;
    const auto selected=std::find(p.visible.begin(),p.visible.end(),size_t(3))-p.visible.begin();
    ListView_SetItemState(list,int(selected),LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    ok &= SelectedRows(p)==std::vector<size_t>{3};
    p.result.rows[3].action=dirsync::Action::None;Filter(p);
    ok &= SelectedRows(p)==std::vector<size_t>{3} && p.result.rows[1].action==dirsync::Action::CopyRight;
    ok &= RowPaths(p,SelectedRows(p),0)==L"beta-漢字\\same.txt";
    p.filterChecks[3]=false;Filter(p);
    ok &= p.visible==std::vector<size_t>({4,3}) && ListView_GetGroupCount(list)==2;
    p.filterChecks[2]=false;Filter(p);
    ok &= p.visible.empty() && ListView_GetGroupCount(list)==0 && ListView_GetItemCount(list)==0;
    p.filterChecks.fill(true);Filter(p);
    ok &= SelectedRows(p).empty() && p.visible.size()==5 && ListView_GetGroupCount(list)==4;
    ListView_SetItemState(list,-1,LVIS_SELECTED,LVIS_SELECTED);
    auto all=SelectedRows(p);std::sort(all.begin(),all.end());
    ok &= all==std::vector<size_t>({0,1,2,3,4});
    ok &= GroupLabel(L"",4)==L"\\";
    const auto longDirectory=std::wstring(140,L'漢')+L"\\"+std::wstring(140,L'Ž');
    p.result.rows[0].path=longDirectory+L"\\same.txt";Filter(p);
    ok &= std::find(p.groups.begin(),p.groups.end(),longDirectory)!=p.groups.end();
    // Sorting stays inside each directory and retains model selection. Numeric
    // values must not be ordered as their formatted strings (2, 10, 1000).
    p.result.rows.clear();
    for(size_t i=0;i<6;++i) {
        dirsync::Row entry;entry.path=(i<3?L"alpha\\":L"beta-漢字\\")+std::wstring(i%3==0?L"z.txt":i%3==1?L"a.txt":L"m.txt");
        entry.hasLeft=true;entry.state=dirsync::State::LeftOnly;entry.action=dirsync::Action::CopyRight;
        entry.left.size=i%3==0?2:i%3==1?1000:10;entry.left.time=i%3==0?300:i%3==1?100:200;
        p.result.rows.push_back(entry);
    }
    Filter(p);for(int i=0;i<10;++i) p.columnWidths[size_t(i)]=100;
    ListView_SetItemState(list,-1,0,LVIS_SELECTED|LVIS_FOCUSED);
    auto picked=std::find(p.visible.begin(),p.visible.end(),size_t(2))-p.visible.begin();
    ListView_SetItemState(list,int(picked),LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    SortPreview(p,1);ok &= p.visible==std::vector<size_t>({0,2,1,3,5,4}) && SelectedRows(p)==std::vector<size_t>{2};
    SortPreview(p,1);ok &= p.visible==std::vector<size_t>({1,2,0,4,5,3}) && SelectedRows(p)==std::vector<size_t>{2};
    SortPreview(p,2);ok &= p.visible==std::vector<size_t>({1,2,0,4,5,3});
    SortPreview(p,2);ok &= p.visible==std::vector<size_t>({0,2,1,3,5,4});
    for(int column=0;column<10;++column) for(bool descending:{false,true}) {
        settings.sortColumn=column;settings.sortDescending=descending;Filter(p);ColumnStyle(p);
        for(size_t i=1;i<p.visible.size();++i) if(RowDirectory(p.result.rows[p.visible[i-1]])==RowDirectory(p.result.rows[p.visible[i]])) {
            int comparison=CompareRows(p.result.rows[p.visible[i-1]],p.result.rows[p.visible[i]],p,column);
            ok &= descending?comparison>=0:comparison<=0;
        }
        HDITEMW arrow{};arrow.mask=HDI_FORMAT;SendMessageW(ListView_GetHeader(list),HDM_GETITEMW,column,LPARAM(&arrow));
        ok &= (arrow.fmt&(HDF_SORTUP|HDF_SORTDOWN))==(descending?HDF_SORTDOWN:HDF_SORTUP);
    }
    SortPreview(p,8);
    const auto beforeHide=p.visible;const auto beforeSelection=SelectedRows(p);
    for(int column:{1,2,4,5,6,7,8}) {ToggleColumn(p,column);ok &= ListView_GetColumnWidth(list,column)==0;}
    ok &= settings.visibleColumns==0x209 && settings.sortColumn==0 && !settings.sortDescending;
    ListView_SetColumnWidth(list,0,0);ListView_SetColumnWidth(list,3,0);ListView_SetColumnWidth(list,6,100);
    ok &= ListView_GetColumnWidth(list,0)>=24 && ListView_GetColumnWidth(list,3)>=24 && ListView_GetColumnWidth(list,6)==0;
    ok &= p.visible.size()==beforeHide.size() && SelectedRows(p)==beforeSelection;
    ToggleColumn(p,0);ToggleColumn(p,3);ToggleColumn(p,9);ok &= settings.visibleColumns==0x209;
    for(int column:{1,2,4,5,6,7,8}) {ToggleColumn(p,column);ok &= ListView_GetColumnWidth(list,column)>0;}
    ok &= settings.visibleColumns==0x3ff;
    p.running=true;SortPreview(p,4);ToggleColumn(p,4);p.running=false;
    ok &= settings.sortColumn==0 && settings.visibleColumns==0x3ff;
    auto x=p.result.rows[0],y=x;x.left.size=2;y.left.size=1000000000000ULL;
    ok &= CompareRows(x,y,p,1)<0 && CompareRows(y,x,p,1)>0;
    x.left.time=UINT64_MAX;y.left.time=1;ok &= CompareRows(x,y,p,2)>0;
    x.hasLeft=false;y.left.size=0;ok &= CompareRows(x,y,p,1)<0;
    x.hasLeft=true;x.left.directory=true;ok &= CompareRows(x,y,p,1)<0;
    x.error=2;y.error=10;ok &= CompareRows(x,y,p,8)<0;
    x.hasRight=y.hasRight=true;x.right.size=10;y.right.size=2;
    x.right.time=1;y.right.time=UINT64_MAX;
    ok &= CompareRows(x,y,p,4)>0 && CompareRows(x,y,p,5)<0;
    settings.relativeSizes=settings.relativeTimes=true;
    x.left.directory=false;x.hasLeft=y.hasLeft=true;
    // Sorting always uses absolute metadata, even when displayed differences
    // would sort in the opposite order (or are below the date tolerance).
    x.left.size=10;x.right.size=1000;y.left.size=20;y.right.size=21;
    x.left.time=100;x.right.time=200;y.left.time=101;y.right.time=102;
    ok &= CompareRows(x,y,p,1)<0 && CompareRows(x,y,p,2)<0;
    x.hasRight=false;
    ok &= Cell(x,1,p)==PanelSize(x.left.size,p.sizeFormat) && Cell(x,2,p)==Modified(x.left.time) && Cell(x,4,p).empty();
    x.hasRight=true;x.right.size=x.left.size;x.right.time=x.left.time;
    ok &= Cell(x,1,p)==PanelSize(x.left.size,p.sizeFormat) && Cell(x,4,p)==Cell(x,1,p) && Cell(x,2,p)==Cell(x,5,p);
    x.right.directory=true;
    ok &= Cell(x,1,p)==PanelSize(x.left.size,p.sizeFormat) && Cell(x,4,p).empty();
    settings.relativeSizes=settings.relativeTimes=false;
    settings.sortColumn=0;settings.sortDescending=false;
    p.result.rows.clear();
    for(size_t i=0;i<6000;++i) {
        dirsync::Row entry;entry.path=L"group-"+std::to_wstring(i/100)+L"\\file-"+std::to_wstring(i)+L".txt";
        entry.state=dirsync::State::LeftOnly;entry.action=dirsync::Action::CopyRight;p.result.rows.push_back(entry);
    }
    const auto started=GetTickCount64();Filter(p);
    ok &= p.visible.size()==6000 && ListView_GetItemCount(list)==6000 && ListView_GetGroupCount(list)==60;
    fprintf(stdout,"Grouped list: 6000 items / 60 groups rebuilt in %llu ms\n",GetTickCount64()-started);
    // Group post-paint must never touch the column-header band when a partially
    // scrolled group overlaps it. Paint into a sentinel bitmap to detect damage.
    RECT headerBand{};GetWindowRect(ListView_GetHeader(list),&headerBand);
    MapWindowPoints(nullptr,list,reinterpret_cast<POINT*>(&headerBand),2);
    HDC memory=CreateCompatibleDC(nullptr);BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bitmapInfo.bmiHeader.biWidth=600;
    bitmapInfo.bmiHeader.biHeight=-600;bitmapInfo.bmiHeader.biPlanes=1;bitmapInfo.bmiHeader.biBitCount=32;
    void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(memory,&bitmapInfo,DIB_RGB_COLORS,&pixels,nullptr,0);
    ok &= memory && bitmap && pixels;
    if(memory && bitmap && pixels) {
        auto oldBitmap=SelectObject(memory,bitmap);bool overlap=false;
        for(int step=0;step<10 && !overlap;++step) {
            ListView_Scroll(list,0,5);RECT group{};ListView_GetGroupRect(list,0,LVGGR_HEADER,&group);
            overlap=group.top<headerBand.bottom && group.bottom>headerBand.top;
            if(!overlap) continue;
            HBRUSH sentinel=CreateSolidBrush(RGB(171,37,193));RECT all{0,0,600,600};FillRect(memory,&all,sentinel);DeleteObject(sentinel);
            NMLVCUSTOMDRAW draw{};draw.nmcd.hdr={list,IDC_LIST,NM_CUSTOMDRAW};draw.nmcd.dwDrawStage=CDDS_POSTPAINT;draw.nmcd.hdc=memory;
            DialogBody(window,WM_NOTIFY,IDC_LIST,LPARAM(&draw));
            for(int y=std::max(0L,headerBand.top);y<headerBand.bottom;++y)
                ok &= GetPixel(memory,10,y)==RGB(171,37,193);
        }
        fprintf(stdout,"Scrolled group overlaps header band: %d; preview checks: %d\n",int(overlap),int(ok));
        ok &= overlap;SelectObject(memory,oldBitmap);
    }
    if(bitmap) DeleteObject(bitmap);if(memory) DeleteDC(memory);
    const auto previousProfiles=settings.profiles;
    for(int id:{IDC_PROFILE_SAVE,IDC_PROFILE_MENU})
        ok &= CreateWindowExW(0,L"BUTTON",L"",WS_CHILD,0,0,30,15,window,HMENU(INT_PTR(id)),instance,nullptr)!=nullptr;
    ok &= CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|CBS_DROPDOWNLIST,0,0,200,100,window,HMENU(INT_PTR(IDC_PROFILE)),instance,nullptr)!=nullptr;
    ok &= CreateWindowExW(0,L"EDIT",L"*.*",WS_CHILD,0,0,30,15,window,HMENU(INT_PTR(IDC_MASK)),instance,nullptr)!=nullptr;
    dirsync::Profile task;task.id=1;task.name=L"Žluťoučký 漢字 😀";task.left=left;task.right=right;task.flags=23;task.filters=10;
    settings.profiles.items={task};settings.profiles.selected=1;
    CheckDlgButton(window,IDC_SELECTED,BST_CHECKED);ApplyProfile(p,task);ProfileControls(p);
    ok &= !p.result.snapshot && p.result.rows.empty() && p.visible.empty() && p.defaults.empty();
    ok &= dirsync::SameProfile(CaptureProfile(p),task) && !DirtyProfile(p) && !Checked(window,IDC_SELECTED);
    ok &= !p.running && !p.worker.joinable();
    SetDlgItemTextW(window,IDC_MASK,L"*.txt");
    ok &= DirtyProfile(p) && GetWindowTextLengthW(window)>0;
    settings.profiles.items[0]=CaptureProfile(p);settings.profiles.items[0].id=1;settings.profiles.items[0].name=task.name;
    ProfileControls(p);ok &= !DirtyProfile(p);
    p.running=true;Busy(p,true);
    for(int id:{IDC_PROFILE,IDC_PROFILE_SAVE,IDC_PROFILE_MENU}) ok &= !IsWindowEnabled(GetDlgItem(window,id));
    p.running=false;Busy(p,false);ok &= IsWindowEnabled(GetDlgItem(window,IDC_PROFILE)) && IsWindowEnabled(GetDlgItem(window,IDC_PROFILE_MENU));
    ProfileManager manager{p,settings.profiles};
    const auto storedMask=settings.profiles.items[0].mask;
    SetDlgItemTextW(window,IDC_MASK,L"*.md");
    ProfileCommand(p,1,manager.draft,window);
    ok &= manager.draft.items[0].mask==L"*.md" && settings.profiles.items[0].mask==storedMask;
    manager.draft.items[0].name=L"Draft name";
    ok &= settings.profiles.items[0].name==task.name;
    // Dropping the manager draft (Cancel) leaves both metadata and inputs alone.
    ok &= CaptureProfile(p).mask==L"*.md" && !p.running && !p.worker.joinable();
    struct DetailLanguage {
        HINSTANCE previous=language;
        HMODULE resources=LoadLibraryExW(L"dirsync_ui_tests.exe",nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
        ~DetailLanguage() {language=previous;if(resources) FreeLibrary(resources);}
    } detailLanguage;
    ok &= detailLanguage.resources!=nullptr;language=detailLanguage.resources;
    HWND details=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&dialog.dialog,nullptr,EmptyDialog,0);
    ok &= details!=nullptr;
    CreateWindowExW(0,L"LISTBOX",L"",WS_CHILD|LBS_NOTIFY,0,0,200,100,details,HMENU(INT_PTR(IDC_PROFILE)),instance,nullptr);
    for(int id:{IDC_PROFILE_LEFT,IDC_PROFILE_RIGHT,IDC_PROFILE_MASK})
        CreateWindowExW(0,L"EDIT",L"",WS_CHILD|ES_READONLY|ES_AUTOHSCROLL,0,0,200,14,details,HMENU(INT_PTR(id)),instance,nullptr);
    for(int i=0;i<10;++i)
        CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,40,14,details,HMENU(INT_PTR(IDC_PROFILE_OPTION_FIRST+i)),instance,nullptr);
    manager.draft.items[0].mask=L"*.漢字;"+std::wstring(400,L'ž');
    for(uint32_t bits=0;bits<32;++bits) {
        manager.draft.items[0].flags=bits;manager.draft.items[0].filters=31-bits;
        ManagerControls(details,manager);
        ok &= ControlText(details,IDC_PROFILE_MASK)==manager.draft.items[0].mask &&
            ControlText(details,IDC_PROFILE_LEFT)==left && ControlText(details,IDC_PROFILE_RIGHT)==right;
        for(int i=0;i<5;++i) {
            const auto option=ControlText(details,IDC_PROFILE_OPTION_FIRST+i),filter=ControlText(details,IDC_PROFILE_FILTER_FIRST+i);
            ok &= !option.empty() && option==Text((bits&(1u<<i))?IDS_PROFILE_ON:IDS_PROFILE_OFF);
            ok &= !filter.empty() && filter==Text(((31-bits)&(1u<<i))?IDS_PROFILE_ON:IDS_PROFILE_OFF);
        }
    }
    manager.draft.selected=0;ManagerControls(details,manager);
    for(int id=IDC_PROFILE_MASK;id<=IDC_PROFILE_FILTER_FIRST+4;++id) ok &= ControlText(details,id).empty();
    ok &= ControlText(details,IDC_PROFILE_LEFT).empty() && ControlText(details,IDC_PROFILE_RIGHT).empty();
    DestroyWindow(details);
    const auto unchanged=settings.profiles;
    const auto keep=settings.keepOriginals;const auto recovery=settings.recoveryRoot;const auto colors=settings.colors;
    SetDlgItemTextW(window,IDC_MASK,L"*.close");
    bool persisted=false;
    auto failure=SaveClosingProfile(p,[&]{persisted=true;return false;});
    ok &= failure==ProfileSaveResult::Failed && persisted && DirtyProfile(p) &&
        dirsync::SameProfile(settings.profiles.items[0],unchanged.items[0]);
    SetDlgItemTextW(window,IDC_MASK,L"<invalid>");persisted=false;
    ok &= SaveClosingProfile(p,[&]{persisted=true;return true;})==ProfileSaveResult::Invalid && !persisted;
    SetDlgItemTextW(window,IDC_MASK,L"*.close");
    ok &= SaveClosingProfile(p,[&]{persisted=true;return true;})==ProfileSaveResult::Saved && persisted && !DirtyProfile(p);
    ok &= settings.profiles.items[0].name==task.name && settings.profiles.items[0].id==task.id &&
        settings.profiles.selected==task.id && settings.keepOriginals==keep && settings.recoveryRoot==recovery && settings.colors==colors;
    // Real dialog procedure: default Save; Cancel/close do not mutate storage.
    HWND close=CreateDialogParamW(language,MAKEINTRESOURCEW(IDD_PROFILE_CLOSE),window,ProfileCloseProc,0);
    ok &= close && GetDlgItem(close,IDYES) && GetDlgItem(close,IDNO) && GetDlgItem(close,IDCANCEL) &&
        (SendMessageW(close,DM_GETDEFID,0,0)&0xffff)==IDYES;
    if(close) {ProfileCloseProc(close,WM_COMMAND,IDCANCEL,0);DestroyWindow(close);}
    ok &= !DirtyProfile(p) && settings.profiles.items[0].mask==L"*.close";
    settings.profiles=previousProfiles;
    fprintf(stdout,"Profile controls: load clears results, no worker, dirty title, transient selection, busy lock and isolated manager draft: %d\n",int(ok));
    DestroyWindow(window);p.window=nullptr;return ok;
} catch(const std::exception& error) {fprintf(stderr,"Preview controls: %s\n",error.what());return FALSE;}
// Exercise the real SDK configuration callbacks without touching user registry data.
extern "C" __declspec(dllexport) BOOL WINAPI ProbeStopControls()
{
    struct Template {DLGTEMPLATE dialog;WORD menu=0,cls=0,title=0;} dialog{};
    dialog.dialog.style=WS_POPUP;dialog.dialog.cx=100;dialog.dialog.cy=80;
    HWND window=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&dialog.dialog,nullptr,EmptyDialog,0);
    if(!window) return FALSE;
    bool ok=true;Preview p;p.window=window;
    SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(&p));
    for(int id:{IDC_STOP,IDC_STOP_SYNC,IDC_COMPARE,IDC_SYNCHRONIZE,IDC_SETTINGS})
        ok &= CreateWindowExW(0,L"BUTTON",L"",WS_CHILD,0,0,30,15,window,HMENU(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr)!=nullptr;
    auto enabled=[&](int id) {return IsWindowEnabled(GetDlgItem(window,id))!=FALSE;};
    Busy(p,false);ok &= !enabled(IDC_STOP) && !enabled(IDC_STOP_SYNC) && enabled(IDC_COMPARE) && enabled(IDC_SYNCHRONIZE) && enabled(IDC_SETTINGS);
    for(auto intent:{Preview::Intent::None,Preview::Intent::Apply,Preview::Intent::Summary,Preview::Intent::Copy,
        Preview::Intent::Save,Preview::Intent::Overwrite,Preview::Intent::Delete,Preview::Intent::ImmediateDelete,Preview::Intent::Sync}) {
        p.running=true;p.validating=intent!=Preview::Intent::None;p.intent=intent;p.cancel=false;
        Busy(p,true);int activeStop=StopControl(p),inactiveStop=activeStop==IDC_STOP?IDC_STOP_SYNC:IDC_STOP;
        const bool files=intent==Preview::Intent::Copy || intent==Preview::Intent::Save || intent==Preview::Intent::Overwrite || intent==Preview::Intent::Delete || intent==Preview::Intent::ImmediateDelete || intent==Preview::Intent::Sync;
        ok &= activeStop==(files?IDC_STOP_SYNC:IDC_STOP) && enabled(activeStop) && !enabled(inactiveStop);
        ok &= !enabled(IDC_COMPARE) && !enabled(IDC_SYNCHRONIZE) && !enabled(IDC_SETTINGS);
        DialogBody(window,WM_COMMAND,inactiveStop,0);ok &= !p.cancel;
        DialogBody(window,WM_COMMAND,activeStop,0);ok &= p.cancel;
    }
    p.validating=false;p.intent=Preview::Intent::None;
    for(bool* flag:{&p.copying,&p.saving,&p.overwriting,&p.deleting,&p.immediateDeleting,&p.synchronizing}) {
        *flag=true;Busy(p,true);ok &= enabled(IDC_STOP_SYNC) && !enabled(IDC_STOP);*flag=false;
    }
    p.running=false;p.cancel=false;Busy(p,false);
    DialogBody(window,WM_COMMAND,IDC_STOP_SYNC,0);DialogBody(window,WM_COMMAND,IDC_STOP,0);
    ok &= !p.cancel && !enabled(IDC_STOP) && !enabled(IDC_STOP_SYNC);
    DestroyWindow(window);p.window=nullptr;return ok;
}
// Exercise the real SDK configuration callbacks without touching user registry data.
extern "C" __declspec(dllexport) unsigned WINAPI ProbeSettingsTest()
{
    struct Registry : CSalamanderRegistryAbstract {
        struct Value { DWORD type; std::vector<BYTE> bytes; };
        std::map<std::string,Value> values;
        bool fail=false;
        BOOL WINAPI ClearKey(HKEY) override { values.clear(); return TRUE; }
        BOOL WINAPI CreateKey(HKEY,const char*,HKEY&) override { return FALSE; }
        BOOL WINAPI OpenKey(HKEY,const char*,HKEY&) override { return FALSE; }
        void WINAPI CloseKey(HKEY) override {}
        BOOL WINAPI DeleteKey(HKEY,const char*) override { return FALSE; }
        BOOL WINAPI GetValue(HKEY,const char* name,DWORD type,void* data,DWORD size) override {
            auto at=values.find(name); if(at==values.end() || at->second.type!=type || at->second.bytes.size()!=size) return FALSE;
            memcpy(data,at->second.bytes.data(),size); return TRUE;
        }
        BOOL WINAPI SetValue(HKEY,const char* name,DWORD type,const void* data,DWORD size) override {
            if(fail) return FALSE;
            auto bytes=static_cast<const BYTE*>(data); values[name]={type,std::vector<BYTE>(bytes,bytes+size)}; return TRUE;
        }
        BOOL WINAPI DeleteValue(HKEY,const char* name) override { return values.erase(name)!=0; }
        BOOL WINAPI GetSize(HKEY,const char* name,DWORD type,DWORD& size) override {
            auto at=values.find(name); if(at==values.end() || at->second.type!=type) return FALSE;
            size=DWORD(at->second.bytes.size()); return TRUE;
        }
    } registry;
    const auto previous=settings; unsigned checks=0;
    auto check=[&](bool ok) { if(!ok) throw std::runtime_error("settings round trip"); ++checks; };
    try {
        HKEY key=reinterpret_cast<HKEY>(UINT_PTR(1)); // opaque key used only by the fake SDK registry
        plugin.LoadConfiguration(nullptr,nullptr,&registry);
        check(settings.keepOriginals && settings.recoveryRoot.empty());
        check(!settings.relativeSizes && !settings.relativeTimes);
        check(!settings.colorRows);
        check(!settings.stopOnFirstError);
        for(bool enabled:{true,false}) {
            settings.stopOnFirstError=enabled;plugin.SaveConfiguration(nullptr,key,&registry);
            settings={};plugin.LoadConfiguration(nullptr,key,&registry);check(settings.stopOnFirstError==enabled);
        }
        registry.values["StopOnFirstErrorV1"].bytes={2,0,0,0};
        plugin.LoadConfiguration(nullptr,key,&registry);check(!settings.stopOnFirstError);
        for(bool enabled:{true,false}) {
            settings.colorRows=enabled;plugin.SaveConfiguration(nullptr,key,&registry);
            settings={};plugin.LoadConfiguration(nullptr,key,&registry);check(settings.colorRows==enabled);
        }
        registry.values["ColorRowsV1"].bytes={2,0,0,0};
        plugin.LoadConfiguration(nullptr,key,&registry);check(!settings.colorRows);
        check(!settings.sideNames);
        settings.sideNames=true;plugin.SaveConfiguration(nullptr,key,&registry);settings={};plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.sideNames);
        registry.values["SideNamesV1"].bytes={2,0,0,0};plugin.LoadConfiguration(nullptr,key,&registry);check(!settings.sideNames);
        settings.relativeSizes=true;plugin.SaveConfiguration(nullptr,key,&registry);settings={};plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.relativeSizes && !settings.relativeTimes);
        settings.relativeSizes=false;settings.relativeTimes=true;plugin.SaveConfiguration(nullptr,key,&registry);settings={};plugin.LoadConfiguration(nullptr,key,&registry);
        check(!settings.relativeSizes && settings.relativeTimes);
        registry.values["RelativeSizesV1"].bytes={2,0,0,0};registry.values["RelativeTimesV1"].bytes={255,0,0,0};
        plugin.LoadConfiguration(nullptr,key,&registry);check(!settings.relativeSizes && !settings.relativeTimes);
        std::wstring root=L"C:\\obnova-漢字\\"+std::wstring(200,L'階')+L"\\😀";
        settings.keepOriginals=false; settings.recoveryRoot=root;
        settings.actionLabels=true;settings.colors[0][2]=RGB(12,34,56);settings.colors[1][6]=RGB(98,76,54);
        plugin.SaveConfiguration(nullptr,key,&registry);
        settings={}; plugin.LoadConfiguration(nullptr,key,&registry);
        check(!settings.keepOriginals && settings.recoveryRoot==root);
        check(settings.actionLabels && settings.colors[0][2]==RGB(12,34,56) && settings.colors[1][6]==RGB(98,76,54));
        settings.visibleColumns=0x39;settings.sortColumn=5;settings.sortDescending=true;
        plugin.SaveConfiguration(nullptr,key,&registry);settings={};plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.visibleColumns==0x239 && settings.sortColumn==5 && settings.sortDescending);
        registry.values["VisibleColumnsV1"].bytes={0,0,0,0};
        plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.visibleColumns==0x209 && settings.sortColumn==0 && !settings.sortDescending);
        registry.values["SortColumnV1"].bytes={99,0,0,0};
        registry.values["SortDescendingV1"].bytes={2,0,0,0};
        plugin.LoadConfiguration(nullptr,key,&registry);check(settings.sortColumn==0 && !settings.sortDescending);
        registry.values["ActionLabels"].bytes={2,0,0,0};
        registry.values["DarkColorsV1"].bytes={0,0,0,0};
        registry.values["LightColorsV1"].bytes.back()=255;
        plugin.LoadConfiguration(nullptr,key,&registry);
        check(!settings.actionLabels && settings.colors[0]==DefaultPalette(false) && settings.colors[1]==DefaultPalette(true));
        check(registry.values["KeepOriginals"].type==REG_DWORD && registry.values["RecoveryRootUtf16"].type==REG_BINARY);
        check(WideCharToMultiByte(CP_UTF8,0,root.c_str(),-1,nullptr,0,nullptr,nullptr)>260);
        settings.recoveryRoot=L"C:\\"+std::wstring(300,L'x'); settings.keepOriginals=true;
        plugin.SaveConfiguration(nullptr,key,&registry); settings={}; plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.keepOriginals && settings.recoveryRoot.size()>260);
        bool saved=false; SettingsCallback(FALSE,key,&registry,&saved); check(saved);
        registry.fail=true; saved=true; SettingsCallback(FALSE,key,&registry,&saved); check(!saved);
        registry.fail=false;
        registry.values["KeepOriginals"].bytes={2,0,0,0};
        registry.values["RecoveryRootUtf16"].bytes={0,0,0};
        plugin.LoadConfiguration(nullptr,key,&registry); check(settings.keepOriginals && settings.recoveryRoot.empty());
        for(const std::vector<BYTE>& invalid:{std::vector<BYTE>{'C',0},std::vector<BYTE>{0,0,'a',0,0,0},std::vector<BYTE>{0,0xd8,0,0}}) {
            registry.values["RecoveryRootUtf16"].bytes=invalid;
            plugin.LoadConfiguration(nullptr,key,&registry); check(settings.recoveryRoot.empty());
        }
        registry.values["RecoveryRootUtf16"].bytes.resize(65524);
        plugin.LoadConfiguration(nullptr,key,&registry); check(settings.recoveryRoot.empty());
        registry.values.clear(); plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.keepOriginals && settings.recoveryRoot.empty());
        settings.keepOriginals=false;settings.recoveryRoot=L"C:\\shared-recovery-漢字";settings.colorRows=true;
        dirsync::Profile task;task.id=9;task.name=L"Dokumenty 😀";task.left=L"C:\\left-漢字";task.right=L"C:\\right-žluťoučký";task.filters=9;task.flags=23;
        settings.profiles.items.push_back(task);settings.profiles.selected=9;
        plugin.SaveConfiguration(nullptr,key,&registry);settings={};plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.profiles.selected==9 && settings.profiles.items.size()==1 && dirsync::SameProfile(settings.profiles.items[0],task));
        check(!settings.keepOriginals && settings.colorRows && settings.recoveryRoot==L"C:\\shared-recovery-漢字");
        registry.values["ProfilesV1"].bytes={1,2,3};plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.profiles.items.empty() && !settings.profiles.selected && !settings.keepOriginals && settings.colorRows);
        registry.values.erase("ProfilesV1");plugin.LoadConfiguration(nullptr,key,&registry);
        check(settings.profiles.items.empty() && settings.recoveryRoot==L"C:\\shared-recovery-漢字");
        settings=previous; return checks;
    } catch(...) { settings=previous; return 0; }
}
// Drive the real Stop/Close handlers without accessibility enumeration timing.
// Test-only mode bits: 1 = Verify, 2 = after one completed file, 4 = save originals,
// 8 = overwrite, 16 = disable recovery, 32 = deletion, 64 = deletion backup,
// 512 = modeless destruction handlers, keyboard hook and progress window.
extern "C" __declspec(dllexport) BOOL WINAPI ProbeCopyStart(const wchar_t* left,const wchar_t* right,BOOL mode)
{
    try {
        if(probe) return FALSE;
        probe=std::make_unique<Preview>(); probe->left=left; probe->right=right;
        probe->options.asymmetric=(mode&32)!=0;
        probe->result=dirsync::Compare(left,right,probe->options,probe->cancel,probe->progress);
        if(probe->result.rows.size()!=((mode&2)?3u:2u)) { probe.reset(); return FALSE; }
        if(mode&(4|8|32)) {
            if(!(mode&32)) for(auto& row:probe->result.rows) row.action=dirsync::Action::CopyRight;
            probe->recoveryRoot=probe->left.substr(0,probe->left.find_last_of(L'\\'))+L"\\recovery";
        }
        struct Template { DLGTEMPLATE dialog; WORD menu=0,cls=0,title=0; } dialog{};
        dialog.dialog.style=WS_POPUP; dialog.dialog.cx=10; dialog.dialog.cy=10;
        probe->modeless=(mode&512)!=0;
        auto procedure=+[](HWND window,UINT message,WPARAM w,LPARAM l)->INT_PTR {
            if(message==WM_DESTROY || message==WM_NCDESTROY)return Dialog(window,message,w,l);
            return EmptyDialog(window,message,w,l);
        };
        probe->window=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&dialog.dialog,nullptr,
            probe->modeless?procedure:EmptyDialog,0);
        if(!probe->window) { probe.reset(); return FALSE; }
        SetWindowLongPtrW(probe->window,DWLP_USER,reinterpret_cast<LONG_PTR>(probe.get()));
        if(probe->modeless) {
            previewKeyboardHook=SetWindowsHookExW(WH_GETMESSAGE,PreviewKeyboard,nullptr,GetCurrentThreadId());
            probe->transferWindow=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&dialog.dialog,
                probe->window,EmptyDialog,0);
            if(!previewKeyboardHook || !probe->transferWindow)throw std::runtime_error("modeless copy fixture");
        }
        copyBoundary=false; finished=false; active=probe.get(); probe->saving=(mode&4)!=0;
        probe->overwriting=(mode&8)!=0; probe->deleting=(mode&32)!=0;
        probe->copying=!probe->saving && !probe->overwriting && !probe->deleting; probe->running=true;
        probe->transferCount=(probe->overwriting || probe->deleting) && !(mode&16)?probe->result.rows.size():0;
        probe->worker=std::thread([mode] {
            try {
                dirsync::SetCopyHook([mode,stage=std::wstring{},copiedBase=uint64_t{0},verifiedBase=uint64_t{0}]
                    (dirsync::CopyPhase phase,const std::wstring& path) mutable {
                    if(phase==dirsync::CopyPhase::Copying && stage!=path) {
                        stage=path; copiedBase=probe->copyProgress.copied.load(); verifiedBase=probe->copyProgress.verified.load();
                    }
                    if((mode&2) && probe->copyProgress.completed<=probe->transferCount) return;
                    if((mode&8) && path.rfind(probe->right+L"\\",0)!=0) return;
                    bool reached=(mode&1) ? phase==dirsync::CopyPhase::Verifying && probe->copyProgress.verified>verifiedBase :
                        phase==dirsync::CopyPhase::Copying && probe->copyProgress.copied>copiedBase;
                    if((mode&32) && !(mode&64)) reached=phase==dirsync::CopyPhase::Deleting;
                    if(!reached || copyBoundary.exchange(true)) return;
                    ULONGLONG deadline=GetTickCount64()+5000;
                    while(!probe->cancel && GetTickCount64()<deadline) Sleep(1);
                });
                std::vector<size_t> rows{0,1}; if(mode&2) rows.push_back(2);
                if(mode&32) {
                    probe->deleteReport=dirsync::DeleteFiles(probe->result,rows,!(mode&16),probe->recoveryRoot,probe->cancel,probe->progress,probe->copyProgress);
                } else if(mode&8) {
                    probe->overwriteReport=dirsync::OverwriteFiles(probe->result,rows,!(mode&16),probe->recoveryRoot,probe->cancel,probe->progress,probe->copyProgress);
                    probe->copyReport.cancelled=probe->overwriteReport.cancelled;
                    probe->copyReport.items=probe->overwriteReport.items;
                } else if(mode&4) {
                    probe->recoveryReport=dirsync::SaveOriginals(probe->result,rows,probe->recoveryRoot,probe->cancel,probe->progress,probe->copyProgress);
                    probe->copyReport.cancelled=probe->recoveryReport.cancelled;
                    for(const auto& item:probe->recoveryReport.items) probe->copyReport.items.push_back(item.copy);
                } else probe->copyReport=dirsync::CopyNewFiles(probe->result,rows,probe->cancel,probe->progress,probe->copyProgress);
                dirsync::SetCopyHook({});
            } catch(...) { probe->failed=true; }
            finished=true;
        });
        return TRUE;
    } catch(...) { if(probe && probe->window) DestroyWindow(probe->window); active=nullptr; probe.reset(); return FALSE; }
}
extern "C" __declspec(dllexport) unsigned WINAPI ProbeCopyStatus()
{ return (copyBoundary?1u:0u) | (finished?2u:0u); }
// Same real dialog handlers and Release contract, with deletion-specific reports.
extern "C" __declspec(dllexport) unsigned WINAPI ProbeDeleteFinish(unsigned mode)
{
    if(!probe || !probe->deleting) return 0;
    unsigned status=0; const bool force=(mode&128)!=0,prior=(mode&2)!=0,backup=(mode&64)!=0;
    if(force) {
        if(!plugin.Release(nullptr,FALSE) && !probe->cancel && !finished) status|=1;
        if(plugin.Release(nullptr,TRUE)) status|=2;
    } else {
        DialogBody(probe->window,(mode&256)?WM_CLOSE:WM_COMMAND,(mode&256)?0:IDC_STOP_SYNC,0);
        if(probe->cancel) status|=1;
        if(probe->closing==((mode&256)!=0)) status|=2;
        probe->Join();
    }
    if(!probe->worker.joinable() && finished && !probe->failed) status|=4;
    const auto& r=probe->deleteReport; size_t at=prior?1:0;
    bool ok=r.cancelled && !r.complete && r.items.size()==at+2;
    if(backup) ok=ok && r.recovery.cancelled && r.items[0].status==dirsync::DeleteStatus::NotStarted;
    else ok=ok && (!r.keepOriginals || r.recovery.complete) && r.items[at].status==dirsync::DeleteStatus::Cancelled;
    if(ok && r.items[at+1].status==dirsync::DeleteStatus::NotStarted && !r.items[at].removed && !r.items[at+1].removed) status|=8;
    if((!prior || (r.items[0].status==dirsync::DeleteStatus::Deleted && r.items[0].removed)) &&
        probe->copyProgress.completed==(backup?0:probe->transferCount+at)) status|=16;
    DestroyWindow(probe->window); probe->window=nullptr; active=nullptr; probe.reset();
    return status;
}
extern "C" __declspec(dllexport) unsigned WINAPI ProbeCopyRelease(BOOL force)
{
    if(!probe) return plugin.Release(nullptr,force)?1u:0u;
    const size_t backups=probe->overwriting?static_cast<size_t>(probe->transferCount):0;
    const size_t total=static_cast<size_t>(probe->copyProgress.completed.load());
    const size_t completed=total>backups?total-backups:0;
    const bool modeless=probe->modeless;
    const HWND previewWindow=probe->window,progressWindow=probe->transferWindow;
    unsigned status=plugin.Release(nullptr,force)?1u:0u;
    if(!force) return status;
    if(probe->cancel && !probe->worker.joinable() && finished && !probe->failed) status|=2;
    const auto& r=probe->copyReport;
    if(RecoveryLogged() && r.cancelled && r.items.size()==completed+2 &&
       r.items[completed].status==dirsync::CopyStatus::Cancelled &&
       r.items[completed+1].status==dirsync::CopyStatus::NotStarted) status|=4;
    bool preserved=r.items.size()==completed+2;
    for(size_t i=0;i<completed && preserved;++i)
        preserved=r.items[i].status==dirsync::CopyStatus::Copied && r.items[i].published;
    if(preserved && !r.items[completed].published && r.items[completed].remainingStage.empty()) status|=8;
    if(probe->copyProgress.completed==completed+backups) status|=16;
    if(modeless && !active && !probe->window && !probe->transferWindow && !previewKeyboardHook &&
       !IsWindow(previewWindow) && !IsWindow(progressWindow)) status|=32;
    if(IsWindow(probe->window)) DestroyWindow(probe->window);
    probe->window=nullptr; active=nullptr; probe.reset();
    return status;
}
extern "C" __declspec(dllexport) unsigned WINAPI ProbeCopyCancel(BOOL close)
{
    if(!probe) return 0;
    // Same handler called by the dialog message loop, on its owner thread.
    DialogBody(probe->window,close?WM_CLOSE:WM_COMMAND,close?0:IDC_STOP_SYNC,0);
    unsigned status=probe->cancel?1u:0u;
    if(probe->closing==(close!=FALSE)) status|=2;
    probe->Join();
    if(!probe->worker.joinable() && finished && !probe->failed) status|=4;
    const auto& r=probe->copyReport;
    if(RecoveryLogged() && r.cancelled && r.items.size()==2 && r.items[0].status==dirsync::CopyStatus::Cancelled &&
       r.items[1].status==dirsync::CopyStatus::NotStarted) status|=8;
    if(r.items.size()==2 && !r.items[0].published && r.items[0].remainingStage.empty() &&
        probe->copyProgress.completed==(probe->overwriting?probe->transferCount:0)) status|=16;
    DestroyWindow(probe->window); probe->window=nullptr; active=nullptr; probe.reset();
    return status;
}
extern "C" __declspec(dllexport) BOOL WINAPI ProbeStart(const wchar_t* left, const wchar_t* right, BOOL content)
{
    try
    {
        if (probe) return FALSE;
        probe = std::make_unique<Preview>();
        probe->left = left; probe->right = right;
        probe->options.content = content != FALSE;
        if (!content)
        {
            probe->options.mask = L"*.* | ";
            for (int i = 0; i < 2000; ++i) probe->options.mask += L"never" + std::to_wstring(i) + L"*;";
        }
        finished = false; active = probe.get(); probe->running = true;
        probe->worker = std::thread([] {
            try { probe->result = dirsync::Compare(probe->left, probe->right, probe->options, probe->cancel, probe->progress); }
            catch (...) { probe->failed = true; }
            finished = true;
        });
        return TRUE;
    }
    catch (...) { active = nullptr; probe.reset(); return FALSE; }
}
extern "C" __declspec(dllexport) unsigned WINAPI ProbeStatus()
{
    if (!probe) return 0;
    return (!finished ? 1u : 0u) | (probe->progress.scanned > 0 ? 2u : 0u) |
        (probe->progress.readBytes > 0 ? 4u : 0u) | (probe->cancel ? 8u : 0u);
}
extern "C" __declspec(dllexport) unsigned WINAPI ProbeRelease(BOOL force)
{
    if(probe && probe->deleting) {
        unsigned status=plugin.Release(nullptr,force)?1u:0u;
        if(force) { DestroyWindow(probe->window); active=nullptr; probe.reset(); }
        return status;
    }
    if(probe && (probe->copying || probe->saving || probe->overwriting)) return ProbeCopyRelease(force);
    unsigned status = plugin.Release(nullptr, force) ? 1u : 0u;
    if (force && probe)
    {
        if (!probe->worker.joinable()) status |= 32;
        if (probe->result.cancelled) status |= 64;
        if (!probe->failed && std::all_of(probe->result.rows.begin(), probe->result.rows.end(),
            [](const dirsync::Row& row) { return row.action == dirsync::Action::Blocked; })) status |= 128;
        active = nullptr; probe.reset();
    }
    return status;
}

// Actual Shell deletion on isolated synthetic NTFS data, not a mocked executor.
extern "C" __declspec(dllexport) unsigned WINAPI ProbeImmediateDeletion(const wchar_t* left,const wchar_t* right)
{
    unsigned checks=0;std::vector<std::wstring> files,dirs;
    auto cleanup=[&]() {for(const auto& f:files) {SetFileAttributesW(dirsync::ApiPath(f).c_str(),FILE_ATTRIBUTE_NORMAL);DeleteFileW(dirsync::ApiPath(f).c_str());}
        for(auto it=dirs.rbegin();it!=dirs.rend();++it) RemoveDirectoryW(dirsync::ApiPath(*it).c_str());};
    auto check=[&](bool ok) {if(!ok) throw std::runtime_error("immediate Shell deletion");++checks;};
    auto exists=[](const std::wstring& path){return GetFileAttributesW(dirsync::ApiPath(path).c_str())!=INVALID_FILE_ATTRIBUTES;};
    auto data=[&](const std::wstring& path) {
        HANDLE h=CreateFileW(dirsync::ApiPath(path).c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        check(h!=INVALID_HANDLE_VALUE);if(h==INVALID_HANDLE_VALUE) return;
        files.push_back(path);DWORD n=0;bool ok=WriteFile(h,"sync",4,&n,nullptr)!=FALSE && n==4;CloseHandle(h);check(ok);
    };
    auto read=[](const std::wstring& path) {
        HANDLE h=CreateFileW(dirsync::ApiPath(path).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
        if(h==INVALID_HANDLE_VALUE) return false;char bytes[4]{};DWORD n=0;
        bool ok=ReadFile(h,bytes,4,&n,nullptr)!=FALSE && n==4 && memcmp(bytes,"sync",4)==0;CloseHandle(h);return ok;
    };
    try {
        std::wstring l=left,r=right;
        data(l+L"\\pair-ž漢.txt");data(l+L"\\pair-ž漢.txt:note");data(r+L"\\pair-ž漢.txt");
        data(l+L"\\keep.txt");data(r+L"\\keep.txt");
        data(l+L"\\hidden.txt");data(r+L"\\hidden.txt");SetFileAttributesW((r+L"\\hidden.txt").c_str(),FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
        check(CreateDirectoryW((r+L"\\protected").c_str(),nullptr)!=FALSE);dirs.push_back(r+L"\\protected");data(r+L"\\protected\\unselected.txt");
        check(CreateDirectoryW((r+L"\\empty").c_str(),nullptr)!=FALSE);dirs.push_back(r+L"\\empty");
        std::atomic<bool> cancel{false};dirsync::Progress progress;dirsync::CopyProgress transfer;dirsync::Options options;options.asymmetric=true;options.emptyDirectories=true;
        auto scan=[&]() {return dirsync::Compare(l,r,options,cancel,progress);};auto result=scan();
        auto index=[&](const dirsync::Result& model,const std::wstring& name) {for(size_t i=0;i<model.rows.size();++i) if(model.rows[i].path==name) return i;throw std::runtime_error("missing immediate fixture row");};
        auto pair=index(result,L"pair-ž漢.txt");
        check(dirsync::ImmediateSelection(result,{pair,pair},2).size()==2);
        check(dirsync::ImmediateSelection(result,{pair},0).size()==1 && dirsync::ImmediateSelection(result,{pair},1).size()==1);
        check(dirsync::ImmediateSelection(result,{},2).empty() && dirsync::ImmediateSelection(result,{SIZE_MAX},2).empty());
        auto item=dirsync::ImmediateSelection(result,{pair},0).front();dirsync::ImmediateOutcome outcome;
        dirsync::ImmediateSink sink(item,outcome,cancel,true);
        check(sink.PreDeleteItem(0,nullptr)==HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) && exists(item.path));
        cancel=true;check(sink.PreDeleteItem(TSF_DELETE_RECYCLE_IF_POSSIBLE,nullptr)==HRESULT_FROM_WIN32(ERROR_CANCELLED));cancel=false;
        result.rows[pair].action=dirsync::Action::None; // Explicit delete is independent of Skip/proposal.
        auto recycled=dirsync::ImmediateDelete(result,{pair},0,true,nullptr,cancel,progress,transfer);
        check(SUCCEEDED(recycled.error) && recycled.items.size()==1 && recycled.items[0].deleted && !exists(item.path));
        check(!recycled.items[0].recycledPath.empty() && read(recycled.items[0].recycledPath) && read(recycled.items[0].recycledPath+L":note"));
        check(exists(r+L"\\pair-ž漢.txt") && exists(l+L"\\keep.txt") && exists(r+L"\\protected\\unselected.txt"));
        result=scan();pair=index(result,L"pair-ž漢.txt");
        auto permanent=dirsync::ImmediateDelete(result,{pair},1,false,nullptr,cancel,progress,transfer);
        check(SUCCEEDED(permanent.error) && permanent.items.size()==1 && permanent.items[0].deleted && permanent.items[0].recycledPath.empty());
        result=scan();auto hidden=index(result,L"hidden.txt");
        auto both=dirsync::ImmediateDelete(result,{hidden},2,false,nullptr,cancel,progress,transfer);
        check(SUCCEEDED(both.error) && both.items.size()==2 && both.items[0].deleted && both.items[1].deleted);
        result=scan();auto protectedRow=index(result,L"protected");
        auto blocked=dirsync::ImmediateDelete(result,{protectedRow},1,false,nullptr,cancel,progress,transfer);
        check(blocked.error==HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY) && exists(r+L"\\protected\\unselected.txt"));
        result=scan();auto empty=index(result,L"empty");
        auto emptyRemoved=dirsync::ImmediateDelete(result,{empty},1,true,nullptr,cancel,progress,transfer);
        check(SUCCEEDED(emptyRemoved.error) && emptyRemoved.items.size()==1 && emptyRemoved.items[0].deleted && !exists(r+L"\\empty"));
        result=scan();auto keep=index(result,L"keep.txt");
        cancel=true;auto stopped=dirsync::ImmediateDelete(result,{keep},2,false,nullptr,cancel,progress,transfer);cancel=false;
        check(stopped.cancelled && exists(l+L"\\keep.txt") && exists(r+L"\\keep.txt"));
        HANDLE lock=CreateFileW((r+L"\\keep.txt").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
        check(lock!=INVALID_HANDLE_VALUE);
        auto locked=dirsync::ImmediateDelete(result,{keep},1,false,nullptr,cancel,progress,transfer);CloseHandle(lock);
        check(FAILED(locked.error) && exists(r+L"\\keep.txt"));
        result=scan();keep=index(result,L"keep.txt");data(r+L"\\changed-after-scan.txt");
        auto stale=dirsync::ImmediateDelete(result,{keep},2,false,nullptr,cancel,progress,transfer);
        check(stale.stale && FAILED(stale.error) && stale.items.empty() && exists(l+L"\\keep.txt") && exists(r+L"\\keep.txt"));
        result=scan();protectedRow=index(result,L"protected");auto child=index(result,L"protected\\unselected.txt");
        auto tree=dirsync::ImmediateDelete(result,{protectedRow,child},1,false,nullptr,cancel,progress,transfer);
        check(SUCCEEDED(tree.error) && tree.items.size()==2 && !exists(r+L"\\protected"));
        data(l+L"\\stop-a.txt");data(l+L"\\stop-b.txt");result=scan();
        auto first=index(result,L"stop-a.txt"),second=index(result,L"stop-b.txt");
        dirsync::ImmediateTestHook()=[&](size_t completed){if(completed==1) cancel=true;};
        auto partial=dirsync::ImmediateDelete(result,{first,second},0,false,nullptr,cancel,progress,transfer);
        dirsync::ImmediateTestHook()={};cancel=false;
        check(partial.cancelled && partial.items.size()==1 && !exists(l+L"\\stop-a.txt") && exists(l+L"\\stop-b.txt"));
        std::wstring relative;
        for(wchar_t c:{L'階',L'Ž'}) {relative+=std::wstring(110,c)+L"\\";
            auto folder=l+L"\\"+relative.substr(0,relative.size()-1);check(CreateDirectoryW(dirsync::ApiPath(folder).c_str(),nullptr)!=FALSE);dirs.push_back(folder);}
        relative+=L"dlouhý-漢字.txt";auto longFile=l+L"\\"+relative;data(longFile);data(longFile+L":note");
        result=scan();auto longIndex=index(result,relative);
        auto longReport=dirsync::ImmediateDelete(result,{longIndex},0,true,nullptr,cancel,progress,transfer);
        std::printf("Immediate long Unicode recycle: HRESULT=0x%08lx, UTF16=%zu\n",ULONG(longReport.error),longFile.size());
        check(longFile.size()>260 && ((SUCCEEDED(longReport.error) && longReport.items.size()==1 && longReport.items[0].deleted &&
            read(longReport.items[0].recycledPath) && read(longReport.items[0].recycledPath+L":note")) ||
            (FAILED(longReport.error) && exists(longFile) && read(longFile+L":note"))));
        data(r+L"\\readonly.txt");check(SetFileAttributesW((r+L"\\readonly.txt").c_str(),FILE_ATTRIBUTE_READONLY)!=FALSE);
        result=scan();auto readonly=index(result,L"readonly.txt");check(dirsync::ImmediateSelection(result,{readonly},1).empty() && exists(r+L"\\readonly.txt"));
        check(CreateDirectoryW((r+L"\\directory-ads").c_str(),nullptr)!=FALSE);dirs.push_back(r+L"\\directory-ads");data(r+L"\\directory-ads:note");
        result=scan();auto directoryAds=index(result,L"directory-ads");
        auto adsBlocked=dirsync::ImmediateDelete(result,{directoryAds},1,false,nullptr,cancel,progress,transfer);
        check(FAILED(adsBlocked.error) && exists(r+L"\\directory-ads") && read(r+L"\\directory-ads:note"));
        cleanup();return checks;
    } catch(const std::exception& e) {std::printf("Immediate deletion failed after %u checks: %s\n",checks,e.what());dirsync::ImmediateTestHook()={};cleanup();return 0;}
}
