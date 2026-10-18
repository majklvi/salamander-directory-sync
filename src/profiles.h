// SPDX-FileCopyrightText: 2026 Michal Várady; Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace dirsync {
struct Profile {
    uint32_t id=0,flags=1,filters=31;
    std::wstring name,left,right,mask=L"*.*";
};
inline bool SameProfile(const Profile& a,const Profile& b) {
    return a.flags==b.flags && a.filters==b.filters && a.left==b.left && a.right==b.right && a.mask==b.mask;
}
inline std::wstring ProfileName(std::wstring value) {
    auto first=std::find_if_not(value.begin(),value.end(),[](wchar_t c){return iswspace(c)!=0;});
    value.erase(value.begin(),first);
    auto last=std::find_if_not(value.rbegin(),value.rend(),[](wchar_t c){return iswspace(c)!=0;}).base();
    value.erase(last,value.end());return value;
}
inline bool ProfileText(const std::wstring& value,size_t maximum,bool empty=false) {
    return (empty || !value.empty()) && value.size()<=maximum && value.find(L'\0')==std::wstring::npos &&
        (value.empty() || WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0,nullptr,nullptr)>0);
}
struct Profiles {
    std::vector<Profile> items;
    uint32_t selected=0;
    Profile* Find(uint32_t id) {for(auto& item:items) if(item.id==id) return &item;return nullptr;}
    const Profile* Find(uint32_t id) const {for(const auto& item:items) if(item.id==id) return &item;return nullptr;}
    bool NameAvailable(const std::wstring& name,uint32_t except=0) const {
        if(!ProfileText(name,128) || ProfileName(name)!=name || name.find_first_of(L"\r\n\t")!=std::wstring::npos) return false;
        for(const auto& item:items) if(item.id!=except && CompareStringOrdinal(item.name.c_str(),-1,name.c_str(),-1,TRUE)==CSTR_EQUAL) return false;
        return true;
    }
    uint32_t NextId() const {
        uint32_t id=1;while(Find(id)) {if(id==UINT32_MAX) return 0;++id;}return id;
    }
    bool Remove(uint32_t id) {
        auto at=std::find_if(items.begin(),items.end(),[&](const Profile& item){return item.id==id;});
        if(at==items.end()) return false;
        items.erase(at);if(selected==id) selected=0;return true;
    }
};
constexpr size_t ProfileDataLimit=16*1024*1024,ProfileCountLimit=1000;
inline std::vector<BYTE> EncodeProfiles(const Profiles& profiles) {
    if(profiles.items.size()>ProfileCountLimit) throw std::runtime_error("Too many profiles");
    std::vector<BYTE> bytes;
    auto number=[&](uint32_t n) {for(int i=0;i<4;++i) bytes.push_back(BYTE(n>>(i*8)));};
    auto text=[&](const std::wstring& value) {
        number(uint32_t(value.size()));const auto* data=reinterpret_cast<const BYTE*>(value.data());
        bytes.insert(bytes.end(),data,data+value.size()*sizeof(wchar_t));
    };
    number(0x31505344);number(uint32_t(profiles.items.size()));number(profiles.selected);
    Profiles seen;
    for(const auto& item:profiles.items) {
        if(!item.id || seen.Find(item.id) || !seen.NameAvailable(item.name) || !ProfileText(item.left,32759) ||
            !ProfileText(item.right,32759) || !ProfileText(item.mask,32759) || item.flags>31 || item.filters>31)
            throw std::runtime_error("Invalid profile");
        number(item.id);number(item.flags);number(item.filters);
        text(item.name);text(item.left);text(item.right);text(item.mask);seen.items.push_back(item);
        if(bytes.size()>ProfileDataLimit) throw std::runtime_error("Profiles too large");
    }
    return bytes;
}
inline bool DecodeProfiles(const std::vector<BYTE>& bytes,Profiles& output) {
    try {
        if(bytes.size()>ProfileDataLimit) return false;
        size_t offset=0;
        auto number=[&]() {
            if(bytes.size()-offset<4) throw std::runtime_error("Truncated profile data");
            uint32_t value=0;for(int i=0;i<4;++i) value|=uint32_t(bytes[offset++])<<(i*8);return value;
        };
        auto text=[&](size_t maximum) {
            size_t count=number();if(count>maximum || count>(bytes.size()-offset)/sizeof(wchar_t)) throw std::runtime_error("Invalid text length");
            std::wstring value(count,L'\0');if(count) memcpy(value.data(),bytes.data()+offset,count*sizeof(wchar_t));
            offset+=count*sizeof(wchar_t);return value;
        };
        if(number()!=0x31505344) return false;
        size_t count=number();if(count>ProfileCountLimit) return false;
        Profiles draft;draft.selected=number();
        for(size_t i=0;i<count;++i) {
            Profile item;item.id=number();item.flags=number();item.filters=number();
            item.name=text(128);item.left=text(32759);item.right=text(32759);item.mask=text(32759);
            if(!item.id || draft.Find(item.id) || !draft.NameAvailable(item.name) || !ProfileText(item.left,32759) ||
                !ProfileText(item.right,32759) || !ProfileText(item.mask,32759) || item.flags>31 || item.filters>31) return false;
            draft.items.push_back(std::move(item));
        }
        if(offset!=bytes.size()) return false;
        if(!draft.Find(draft.selected)) draft.selected=0;
        output=std::move(draft);return true;
    } catch(...) {return false;}
}
// Portable, versioned profile collection. Import is additive and transactional
// in memory; IDs/active selection belong to the destination configuration.
inline bool ImportProfiles(const std::vector<BYTE>& bytes,Profiles& destination) {
    Profiles incoming;
    if(!DecodeProfiles(bytes,incoming) || incoming.items.empty() ||
        incoming.items.size()+destination.items.size()>ProfileCountLimit) return false;
    Profiles draft=destination;
    for(auto item:incoming.items) {
        const auto base=item.name;
        for(size_t suffix=2;!draft.NameAvailable(item.name);++suffix) {
            auto tail=L" ("+std::to_wstring(suffix)+L")";
            auto prefix=base.substr(0,128-tail.size());
            if(!prefix.empty() && prefix.back()>=0xd800 && prefix.back()<=0xdbff) prefix.pop_back();
            item.name=prefix+tail;
        }
        item.id=draft.NextId();if(!item.id) return false;
        draft.items.push_back(std::move(item));
    }
    try {EncodeProfiles(draft);} catch(...) {return false;}
    destination=std::move(draft);return true;
}
}
