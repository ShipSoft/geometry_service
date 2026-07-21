// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/GeometryThread.h"

#include <utility>

namespace ship {

GeometryThread::GeometryThread()
    : m_thread{[this](std::stop_token stopToken) { loop(std::move(stopToken)); }} {}

void GeometryThread::post(std::function<void()> task) {
    {
        std::lock_guard lk{m_mutex};
        m_tasks.push_back(std::move(task));
    }
    m_cv.notify_one();
}

void GeometryThread::loop(std::stop_token stopToken) {
    std::unique_lock lk{m_mutex};
    // The interruptible wait returns false only once a stop is requested
    // and the queue is empty, so pending tasks are drained before shutdown.
    while (m_cv.wait(lk, stopToken, [this] { return !m_tasks.empty(); })) {
        auto task = std::move(m_tasks.front());
        m_tasks.pop_front();
        lk.unlock();
        task();
        lk.lock();
    }
}

GeometryThread& geometry_thread() {
    // Intentionally leaked: the thread must outlive static teardown, where
    // store destructors may still post tasks (see the header contract).
    static GeometryThread* thread = new GeometryThread;
    return *thread;
}

}  // namespace ship
