// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine.h"
#include <mutex>
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
#include <functional>
#endif

namespace dirsync {
enum class CopyPhase { Checking, Copying, Preparing, Verifying, Publishing, Published, Deleting, Deleted, Finished };
enum class TransferKind { Copy, Overwrite, Save, Delete, Recycle };
struct TransferDisplay {
    std::wstring source,target;
    TransferKind kind=TransferKind::Copy;
    bool directory=false,finished=false;
    uint64_t total=0,copied=0,verifyTotal=0,verified=0;
    uint64_t workBase=0,copyBase=0;
};
struct CopyProgress {
    std::atomic<CopyPhase> phase{CopyPhase::Checking};
    std::atomic<uint64_t> copied{0}, verified{0}, completed{0}, item{0};
    std::atomic<uint64_t> completedWork{0},completedCopyBytes{0};
    // UI requests are cooperative. Never suspend a worker that may own a file
    // handle or be completing an atomic publish/journal sequence.
    std::atomic<bool> pauseRequested{false}, paused{false};
    std::atomic<uint64_t> bytesPerSecond{0}, pausedMilliseconds{0}; // zero: unlimited
    // Call before starting a batch, with the previous worker already joined.
    void ResetControls() noexcept;
    bool Checkpoint(const std::atomic<bool>& cancel) noexcept;
    // cumulativeCopied includes named streams and preceding recovery copies.
    bool Throttle(const std::atomic<bool>& cancel,uint64_t cumulativeCopied) noexcept;
    void BeginDisplay(const std::wstring& source,const std::wstring& target,TransferKind kind,bool directory,uint64_t total=0) {
        std::lock_guard<std::mutex> lock(displayMutex);display={source,target,kind,directory,false,total,0,0,0,completedWork.load(),completedCopyBytes.load()};phase=CopyPhase::Checking;
    }
    void CopyDisplay(uint64_t total,uint64_t done) {std::lock_guard<std::mutex> lock(displayMutex);display.total=total;display.copied=done;}
    void VerifyDisplay(uint64_t total) {std::lock_guard<std::mutex> lock(displayMutex);display.verifyTotal=total;display.verified=0;}
    void VerifiedDisplay(uint64_t bytes) {std::lock_guard<std::mutex> lock(displayMutex);display.verified+=bytes;}
    void CompleteDisplay();
    TransferDisplay Display() const {std::lock_guard<std::mutex> lock(displayMutex);return display;}
    void ResetDisplay() {std::lock_guard<std::mutex> lock(displayMutex);display={};}
private:
    // Single-worker state; only ResetControls is used by the idle UI thread.
    uint64_t throttleTick=0,throttleCopied=0,throttleRate=0;
    double throttleDebt=0;
    mutable std::mutex displayMutex;
    TransferDisplay display;
};
enum class CopyStatus { NotStarted, Copied, Cancelled, Failed };
struct CopyOutcome {
    size_t row = 0;
    std::wstring target, remainingStage;
    CopyStatus status = CopyStatus::NotStarted;
    DWORD error = 0;
    bool published = false;
};
struct CopyReport {
    std::vector<CopyOutcome> items;
    bool stale = false, cancelled = false;
    size_t omitted = 0;
};
bool IsNewFileCopy(const Row& row);
bool IsNewCopy(const Row& row);
CopyReport CopyNewFiles(Result snapshot, const std::vector<size_t>& visible,
                       std::atomic<bool>& cancel, Progress& checking, CopyProgress& progress);
#if defined(DIRSYNC_ENGINE_TEST) || defined(DIRSYNC_COPY_TEST_HOOKS)
// Deterministic ordinary failure/cancellation boundaries; never in shipped SPL.
void SetCopyHook(std::function<void(CopyPhase, const std::wstring&)> hook);
#endif
}
