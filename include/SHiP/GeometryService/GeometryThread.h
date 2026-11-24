// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <thread>
#include <type_traits>

#include <concepts>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stop_token>
#include <utility>
#include <version>

namespace SHiP {

/// Runs posted tasks on one dedicated thread until destroyed.
///
/// Geant4 MT split-class state (logical/physical volumes, solids) is only
/// valid on the thread that created it, so every creation or mutation of
/// converted geometry must be funnelled through here. Callers block until
/// their task finishes; exceptions propagate. Safe to call from multiple
/// threads concurrently: tasks queue and run one at a time.
class GeometryThread {
   public:
    // jthread requests stop and joins; queued tasks are drained first.
    ~GeometryThread() = default;

    GeometryThread(const GeometryThread&) = delete;
    GeometryThread& operator=(const GeometryThread&) = delete;
    GeometryThread(GeometryThread&&) = delete;
    GeometryThread& operator=(GeometryThread&&) = delete;

    /// Run f on the geometry thread and return its result. Reentrant: a call
    /// made from a task already running on the geometry thread executes f
    /// inline instead of deadlocking on its own queue slot.
    // Constrained on lvalue invocation: f is called as an lvalue here and
    // by the packaged_task below.
    template <typename F>
        requires std::invocable<F&>
    std::invoke_result_t<F&> run(F&& f) {
        if (std::this_thread::get_id() == m_thread.get_id()) {
            return f();
        }
        using R = std::invoke_result_t<F&>;
        std::packaged_task<R()> task{std::forward<F>(f)};
        auto result = task.get_future();
#ifdef __cpp_lib_move_only_function
        post(std::move(task));
#else
        // Task is copyable here (see below), so share the move-only
        // packaged_task instead of moving it in.
        post([task = std::make_shared<std::packaged_task<R()> >(std::move(task))] { (*task)(); });
#endif
        return result.get();
    }

   private:
#ifdef __cpp_lib_move_only_function
    using Task = std::move_only_function<void()>;
#else
    // libc++ (macOS) has no std::move_only_function yet.
    using Task = std::function<void()>;
#endif

    // Constructible only through geometry_thread(): a second instance would
    // break the single geometry-creating-thread invariant documented there.
    GeometryThread();
    friend GeometryThread& geometry_thread();

    void post(Task task);
    void loop(std::stop_token stopToken);

    std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::deque<Task> m_tasks;
    std::jthread m_thread;  ///< last: starts using the members above
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

/// Empties the Geant4 geometry stores (regions, physical and logical volumes,
/// solids) on the geometry thread, after opening the geometry. Call it once,
/// when the last user of the converted geometry is gone.
///
/// Without it the stores' static destructors delete the volumes on the main
/// thread at process exit. The volumes' thread-local data only exists on the
/// geometry thread, so that deletion crashes. A locked store's Clean() also
/// skips the cross-store back-references the individual destructors would
/// follow. Every pointer into the converted geometry dangles afterwards.
void clean_geant4_geometry_stores();

}  // namespace SHiP
