// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <set>
#include <tuple>
#include <chrono>

namespace dirsync {
enum class CopyPhase { Checking, Copying, Preparing, Verifying, Publishing, Published, Deleting, Deleted, Finished };
enum class TransferKind { Copy, Overwrite, Save, Delete, Recycle };
enum class ErrorSite { Source, Target, Data };
enum class ErrorDecision { Retry, Skip, SkipAll, Cancel };
struct OperationError {
    TransferKind kind=TransferKind::Copy;
    CopyPhase phase=CopyPhase::Checking;
    ErrorSite site=ErrorSite::Source;
    DWORD code=0;
    std::wstring source,target;
    bool changed=false,remainingStage=false;
};
inline bool Recoverable(const OperationError& e) {
    if(e.changed || e.remainingStage || e.phase==CopyPhase::Verifying || e.phase==CopyPhase::Published ||
       e.phase==CopyPhase::Deleted || e.phase==CopyPhase::Finished)return false;
    switch(e.code) {
    case ERROR_ACCESS_DENIED:case ERROR_SHARING_VIOLATION:case ERROR_LOCK_VIOLATION:
    case ERROR_DISK_FULL:case ERROR_HANDLE_DISK_FULL:case ERROR_READ_FAULT:case ERROR_WRITE_FAULT:
    case ERROR_NOT_READY:case ERROR_IO_DEVICE:case ERROR_SEM_TIMEOUT:return true;
    default:return false;
    }
}
struct ErrorPolicy {
    // Existing noninteractive entry points keep their stop-on-error behavior.
    bool stopOnFirstError=true;
    std::function<ErrorDecision(const OperationError&)> prompt;
    using Key=std::tuple<TransferKind,CopyPhase,ErrorSite,DWORD>;
    std::set<Key> skipped;
    ErrorDecision Decide(const OperationError& e,bool& automatic) {
        automatic=false;
        if(stopOnFirstError || !prompt || !Recoverable(e))return ErrorDecision::Cancel;
        Key key{e.kind,e.phase,e.site,e.code};
        if(skipped.count(key)){automatic=true;return ErrorDecision::Skip;}
        auto decision=prompt(e);
        if(decision==ErrorDecision::SkipAll){skipped.insert(key);return ErrorDecision::Skip;}
        return decision;
    }
};
// A single worker requests a copied decision; UI destruction never waits for UI
// messages to be dispatched. Cancel wakes the worker before any join/unload.
class ErrorQuestion {
    std::mutex mutex;
    std::condition_variable wake;
    OperationError error;
    ErrorDecision answer=ErrorDecision::Cancel;
    bool pending=false,resolved=false,closed=false;
public:
    void Reset(){std::lock_guard<std::mutex> lock(mutex);pending=resolved=closed=false;}
    void Cancel(){std::lock_guard<std::mutex> lock(mutex);closed=true;pending=false;wake.notify_all();}
    bool Peek(OperationError& value){std::lock_guard<std::mutex> lock(mutex);if(!pending || closed)return false;value=error;return true;}
    void Answer(ErrorDecision value){std::lock_guard<std::mutex> lock(mutex);if(!pending || closed)return;answer=value;pending=false;resolved=true;wake.notify_all();}
    ErrorDecision Ask(const OperationError& value,const std::atomic<bool>& cancel,const std::function<bool()>& notify) {
        std::unique_lock<std::mutex> lock(mutex);
        if(closed || cancel)return ErrorDecision::Cancel;
        error=value;resolved=false;pending=true;
        lock.unlock();bool posted=false;
        try {posted=notify();}catch(...){Cancel();throw;}
        lock.lock();
        if(!posted){pending=false;return ErrorDecision::Cancel;}
        while(!closed && !cancel && !resolved)wake.wait_for(lock,std::chrono::milliseconds(25));
        pending=false;
        return closed || cancel?ErrorDecision::Cancel:answer;
    }
};
}
