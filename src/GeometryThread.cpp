// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GeometryService/GeometryThread.h"

#include <utility>

namespace ship {

GeometryThread::GeometryThread() : m_thread{[this] { loop(); }} {}

GeometryThread::~GeometryThread() {
    {
        std::lock_guard lk{m_mutex};
        m_stop = true;
    }
    m_cv.notify_one();
    m_thread.join();
}

void GeometryThread::post(std::function<void()> task) {
    {
        std::lock_guard lk{m_mutex};
        m_tasks.push_back(std::move(task));
    }
    m_cv.notify_one();
}

void GeometryThread::loop() {
    std::unique_lock lk{m_mutex};
    while (true) {
        m_cv.wait(lk, [this] { return m_stop || !m_tasks.empty(); });
        if (!m_tasks.empty()) {
            auto task = std::move(m_tasks.front());
            m_tasks.pop_front();
            lk.unlock();
            task();
            lk.lock();
        } else if (m_stop) {
            return;
        }
    }
}

GeometryThread& geometry_thread() {
    // Intentionally leaked: the thread must outlive static teardown, where
    // store destructors may still post tasks (see the header contract).
    static GeometryThread* thread = new GeometryThread;
    return *thread;
}

}  // namespace ship
