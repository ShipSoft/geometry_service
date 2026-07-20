// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <thread>
#include <type_traits>

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <utility>

namespace ship {

/// Runs posted tasks on one dedicated thread until destroyed.
///
/// Geant4 MT split-class state (logical/physical volumes, solids) is only
/// valid on the thread that created it, so every creation or mutation of
/// converted geometry must be funnelled through here. Callers block until
/// their task finishes; exceptions propagate. Safe to call from multiple
/// threads concurrently: tasks queue and run one at a time.
class GeometryThread {
   public:
    ~GeometryThread();

    GeometryThread(const GeometryThread&) = delete;
    GeometryThread& operator=(const GeometryThread&) = delete;
    GeometryThread(GeometryThread&&) = delete;
    GeometryThread& operator=(GeometryThread&&) = delete;

    /// Run f on the geometry thread and return its result. Reentrant: a call
    /// made from a task already running on the geometry thread executes f
    /// inline instead of deadlocking on its own queue slot.
    template <typename F>
    std::invoke_result_t<F&> run(F&& f) {
        if (std::this_thread::get_id() == m_thread.get_id())
            return f();
        using R = std::invoke_result_t<F&>;
        std::packaged_task<R()> task{std::forward<F>(f)};
        auto result = task.get_future();
        // The reference capture is safe: result.get() below keeps this frame
        // alive until the task has run.
        post([&task] { task(); });
        return result.get();
    }

   private:
    // Constructible only through geometry_thread(): a second instance would
    // break the single geometry-creating-thread invariant documented there.
    GeometryThread();
    friend GeometryThread& geometry_thread();

    void post(std::function<void()> task);
    void loop();

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::function<void()>> m_tasks;
    bool m_stop{false};
    std::thread m_thread;  ///< last: starts using the members above
};

/// The process-wide Geant4 geometry thread, shared by every geometry user
/// (GENIE geometry analyzers, the Geant4 run-manager master) and never torn
/// down (a parked thread at exit is harmless). Geant4 MT allows only a
/// single geometry-*creating* thread per process: G4GeomSplitter's slot
/// count is shared across threads while the data array is thread-local, so
/// a second creating thread writes through its still-null array pointer the
/// moment the first thread has grown the shared count (Geant4 bug #2747,
/// verified with the conda Geant4 11.3 build). Defined in the shared
/// library so dlopened plugins all see the same instance.
GeometryThread& geometry_thread();

}  // namespace ship
