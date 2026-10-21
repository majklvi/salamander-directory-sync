// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "overwrite.h"
#include "delete.h"
namespace dirsync {
enum class SyncStage {Checking,NewItems,Overwrites,Deletions,Finished};
struct SyncSelection { bool toRight=true,toLeft=true,deleteRight=true,overwrite=true; };
struct SyncPlan {
    std::vector<size_t> copies,overwrites,deletions;
    std::vector<size_t> excluded; // eligible visible proposals declined in confirmation
    size_t omitted=0;
    size_t Count() const {return copies.size()+overwrites.size()+deletions.size();}
};
struct SyncReport {
    SyncPlan plan;
    CopyReport copies;OverwriteReport overwrites;DeleteReport deletions;
    DWORD error=0;
    bool complete=false,cancelled=false,stale=false,keepOriginals=true;
    bool finished=false; // returned terminal report, including Skip/failure/Stop
};
SyncPlan BuildSyncPlan(const Result& snapshot,const std::vector<size_t>& visible,SyncSelection selection={});
SyncReport Synchronize(Result snapshot,const std::vector<size_t>& visible,bool keepOriginals,
    const std::wstring& recoveryRoot,std::atomic<bool>& cancel,Progress& checking,
    CopyProgress& progress,std::atomic<SyncStage>& stage,SyncSelection selection={});
}
