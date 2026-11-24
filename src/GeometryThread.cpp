// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SHiP/GeometryService/GeometryThread.h"

#include <G4GeometryManager.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4RegionStore.hh>
#include <G4SolidStore.hh>

#include <utility>

namespace SHiP {

GeometryThread::GeometryThread()
    : m_thread{[this](std::stop_token stopToken) { loop(std::move(stopToken)); }} {}

void GeometryThread::post(Task task) {
    {
        const std::scoped_lock lk{m_mutex};
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

void clean_geant4_geometry_stores() {
    geometry_thread().run([] {
        G4GeometryManager::GetInstance()->OpenGeometry();
        G4RegionStore::Clean();
        G4PhysicalVolumeStore::Clean();
        G4LogicalVolumeStore::Clean();
        G4SolidStore::Clean();
    });
}

}  // namespace SHiP
