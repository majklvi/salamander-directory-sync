// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from Samandarin's IDD_SETSPEEDLIM and CSetSpeedLimDialog (GPL-2.0-or-later).
// The transfer itself
// remains owned by this plugin; no private host worker or dialog API is used.
bool ParseTransferSpeed(const std::wstring& text,int unit,uint64_t& bytesPerSecond) {
    if(text.empty() || unit<0 || unit>3)return false;
    const uint64_t factor=uint64_t(1)<<(unit*10),limit=MaxTransferSpeed/factor;
    uint64_t value=0;
    size_t first=0,last=text.size();
    while(first<last && text[first]<=L' ')++first;
    while(last>first && text[last-1]<=L' ')--last;
    for(size_t i=first;i<last;++i){const wchar_t digit=text[i];
        if(digit<L'0' || digit>L'9')return false;
        const uint64_t number=uint64_t(digit-L'0');
        if(value>limit/10 || (value==limit/10 && number>limit%10))return false;
        value=value*10+number;
    }
    if(!value)return false;
    bytesPerSecond=value*factor;return true;
}
struct TransferSpeedChoice {uint64_t original=0,result=0;HWND* activeWindow=nullptr;};
void EnableTransferSpeed(HWND window) {
    const bool enabled=IsDlgButtonChecked(window,IDC_TRANSFER_SPEED_ENABLE)==BST_CHECKED;
    EnableWindow(GetDlgItem(window,IDC_TRANSFER_SPEED_VALUE),enabled);
    EnableWindow(GetDlgItem(window,IDC_TRANSFER_SPEED_UNITS),enabled);
}
INT_PTR CALLBACK TransferSpeedDialog(HWND window,UINT message,WPARAM w,LPARAM l) {
    auto* choice=reinterpret_cast<TransferSpeedChoice*>(GetWindowLongPtrW(window,DWLP_USER));
    try{
        if(message==WM_INITDIALOG){
            choice=reinterpret_cast<TransferSpeedChoice*>(l);
            SetWindowLongPtrW(window,DWLP_USER,reinterpret_cast<LONG_PTR>(choice));
            if(choice->activeWindow)*choice->activeWindow=window;
            uint64_t number=choice->original?choice->original:settings.lastTransferSpeed;
            int unit=0;while(unit<3 && number%1024==0){number/=1024;++unit;}
            SetDlgItemTextW(window,IDC_TRANSFER_SPEED_VALUE,std::to_wstring(number).c_str());
            SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_VALUE,EM_LIMITTEXT,19,0);
            for(int id:{IDS_TRANSFER_SPEED_B,IDS_TRANSFER_SPEED_KB,IDS_TRANSFER_SPEED_MB,IDS_TRANSFER_SPEED_GB})
                SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_UNITS,CB_ADDSTRING,0,LPARAM(Text(id).c_str()));
            SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_UNITS,CB_SETCURSEL,unit,0);
            CheckRadioButton(window,IDC_TRANSFER_SPEED_ENABLE,IDC_TRANSFER_SPEED_UNLIMITED,
                choice->original?IDC_TRANSFER_SPEED_ENABLE:IDC_TRANSFER_SPEED_UNLIMITED);
            EnableTransferSpeed(window);
            if(gui)gui->ArrangeHorizontalLines(window);
            if(general){ConfigureDark(window);general->MultiMonCenterWindow(window,GetParent(window),FALSE);}
            return TRUE;
        }
        if(!choice)return FALSE;
        LRESULT brush=0;if(dark && DarkModeHandleCtlColor(message,w,l,brush))return brush;
        switch(message){
        case WM_COMMAND:
            if((LOWORD(w)==IDC_TRANSFER_SPEED_ENABLE || LOWORD(w)==IDC_TRANSFER_SPEED_UNLIMITED) && HIWORD(w)==BN_CLICKED){EnableTransferSpeed(window);return TRUE;}
            if(LOWORD(w)==IDHELP){Message(window,IDS_TRANSFER_SPEED_HELP);return TRUE;}
            if(LOWORD(w)==IDCANCEL){EndDialog(window,IDCANCEL);return TRUE;}
            if(LOWORD(w)==IDOK){
                uint64_t value=0;
                if(IsDlgButtonChecked(window,IDC_TRANSFER_SPEED_ENABLE)==BST_CHECKED){
                    wchar_t text[32]{};GetDlgItemTextW(window,IDC_TRANSFER_SPEED_VALUE,text,int(std::size(text)));
                    const int unit=int(SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_UNITS,CB_GETCURSEL,0,0));
                    if(!ParseTransferSpeed(text,unit,value)){
                        DarkModeMessageBoxW(window,Text(IDS_TRANSFER_SPEED_INVALID).c_str(),Text(IDS_NAME).c_str(),MB_OK|MB_ICONEXCLAMATION);
                        SetFocus(GetDlgItem(window,IDC_TRANSFER_SPEED_VALUE));
                        SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_VALUE,EM_SETSEL,0,-1);return TRUE;
                    }
                }
                choice->result=value;EndDialog(window,IDOK);return TRUE;
            }
            break;
        case WM_CLOSE:EndDialog(window,IDCANCEL);return TRUE;
        case WM_HELP:Message(window,IDS_TRANSFER_SPEED_HELP);return TRUE;
        case WM_DESTROY:if(choice->activeWindow && *choice->activeWindow==window)*choice->activeWindow=nullptr;return TRUE;
        case WM_SETTINGCHANGE:case WM_THEMECHANGED:if(general)ConfigureDark(window);return TRUE;
        }
    }catch(...){EndDialog(window,IDCANCEL);return TRUE;}
    return FALSE;
}
bool ChooseTransferSpeed(HWND owner,uint64_t& bytesPerSecond,HWND* activeWindow=nullptr) {
    TransferSpeedChoice choice{bytesPerSecond,bytesPerSecond,activeWindow};
    if(DialogBoxParamW(language,MAKEINTRESOURCEW(IDD_TRANSFER_SPEED),owner,TransferSpeedDialog,LPARAM(&choice))!=IDOK)return false;
    bytesPerSecond=choice.result;return true;
}
