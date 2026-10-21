// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine.h"
namespace dirsync {
enum class StoredCopy { Unconfirmed, Recorded, Missing, Different, Unsafe, Skipped };
enum class StoredOperation { Unknown, Replaced, Deleted, NotStarted, Failed, Cancelled, Skipped, DependencyBlocked };
struct StoredOriginal {
    uint64_t item=0,size=0;
    std::wstring original,saved;
    bool directory=false;
    StoredCopy copy=StoredCopy::Unconfirmed;
    StoredOperation operation=StoredOperation::Unknown;
    DWORD error=0;
};
struct StoredRun {
    std::wstring path,left,right;
    FILETIME created{};
    std::vector<StoredOriginal> items;
    bool complete=false;
    DWORD error=0;
};
struct RecoveryCatalog {
    std::vector<StoredRun> runs;
    DWORD error=0;
    bool cancelled=false,limited=false;
};
// Reads existing JSONL records and basic copy availability only. Never creates,
// restores, deletes, repairs or verifies the contents of a recovery copy.
RecoveryCatalog ReadRecoveryCatalog(const std::wstring& root,std::atomic<bool>& cancel);
// Recheck before opening a location: no relative paths or reparse traversal.
bool RecoveryLocation(const std::wstring& path,bool directory,std::wstring& folder);
bool RecoverySavedLocation(const std::wstring& run,const std::wstring& path,bool directory,std::wstring& folder);
}
