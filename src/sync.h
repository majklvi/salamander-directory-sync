// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "overwrite.h"
#include "delete.h"
namespace dirsync {
enum class SyncStage {Checking,NewItems,Overwrites,Deletions,Finished};
struct SyncPlan {
    std::vector<size_t> copies,overwrites,deletions;
    size_t omitted=0;
    size_t Count() const {return copies.size()+overwrites.size()+deletions.size();}
};
struct SyncReport {
    SyncPlan plan;
    CopyReport copies;OverwriteReport overwrites;DeleteReport deletions;
    DWORD error=0;
    bool complete=false,cancelled=false,stale=false,keepOriginals=true;
};
SyncPlan BuildSyncPlan(const Result& snapshot,const std::vector<size_t>& visible);
SyncReport Synchronize(Result snapshot,const std::vector<size_t>& visible,bool keepOriginals,
    const std::wstring& recoveryRoot,std::atomic<bool>& cancel,Progress& checking,
    CopyProgress& progress,std::atomic<SyncStage>& stage);
}
