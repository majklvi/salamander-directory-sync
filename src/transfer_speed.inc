// SPDX-License-Identifier: GPL-2.0-or-later
// Compact layout follows Samandarin's IDD_SETSPEEDLIM. The transfer itself
// remains owned by this plugin; no private host worker or dialog API is used.
constexpr uint64_t MaxTransferSpeed=uint64_t(1)<<40;
bool ParseTransferSpeed(const std::wstring& text,int unit,uint64_t& bytesPerSecond) {
    if(text.empty() || (unit!=0 && unit!=1))return false;
    const uint64_t factor=unit?1024*1024:1024,limit=MaxTransferSpeed/factor;
    uint64_t value=0;
    for(wchar_t digit:text){
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
            const uint64_t initial=choice->original?choice->original:uint64_t(10)*1024*1024;
            const int unit=initial%(1024*1024)==0?1:0;
            const uint64_t factor=unit?1024*1024:1024;
            SetDlgItemTextW(window,IDC_TRANSFER_SPEED_VALUE,std::to_wstring(initial/factor+(initial%factor?1:0)).c_str());
            SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_VALUE,EM_LIMITTEXT,19,0);
            for(int id:{IDS_TRANSFER_SPEED_KB,IDS_TRANSFER_SPEED_MB})
                SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_UNITS,CB_ADDSTRING,0,LPARAM(Text(id).c_str()));
            SendDlgItemMessageW(window,IDC_TRANSFER_SPEED_UNITS,CB_SETCURSEL,unit,0);
            CheckDlgButton(window,IDC_TRANSFER_SPEED_ENABLE,choice->original?BST_CHECKED:BST_UNCHECKED);
            EnableTransferSpeed(window);
            if(general){ConfigureDark(window);general->MultiMonCenterWindow(window,GetParent(window),FALSE);}
            return TRUE;
        }
        if(!choice)return FALSE;
        LRESULT brush=0;if(dark && DarkModeHandleCtlColor(message,w,l,brush))return brush;
        switch(message){
        case WM_COMMAND:
            if(LOWORD(w)==IDC_TRANSFER_SPEED_ENABLE && HIWORD(w)==BN_CLICKED){EnableTransferSpeed(window);return TRUE;}
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
