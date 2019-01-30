/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "registry.h"
#include "logging.h"
#include "menuregistrar_interface.h"

#include <QDBusObjectPath>
#include <QDBusServiceWatcher>

Q_LOGGING_CATEGORY(unityappmenuRegistrar, "unityappmenu.registrar", QtWarningMsg)

#define REGISTRAR_SERVICE "io.unity8.MenuRegistrar"
#define REGISTRY_OBJECT_PATH "/io/unity8/MenuRegistrar"

UnityMenuRegistry *UnityMenuRegistry::instance()
{
    static UnityMenuRegistry* registry(new UnityMenuRegistry());
    return registry;
}

UnityMenuRegistry::UnityMenuRegistry(QObject* parent)
    : QObject(parent)
    , m_serviceWatcher(new QDBusServiceWatcher(REGISTRAR_SERVICE, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
    , m_interface(new IoUnity8MenuRegistrarInterface(REGISTRAR_SERVICE, REGISTRY_OBJECT_PATH, QDBusConnection::sessionBus(), this))
    , m_connected(m_interface->isValid())
{
    connect(m_serviceWatcher.data(), &QDBusServiceWatcher::serviceOwnerChanged, this, &UnityMenuRegistry::serviceOwnerChanged);
}

UnityMenuRegistry::~UnityMenuRegistry()
{
}

void UnityMenuRegistry::registerApplicationMenu(pid_t pid, QDBusObjectPath menuObjectPath, const QString &service)
{
    qCDebug(unityappmenuRegistrar, "UnityMenuRegistry::registerMenu(pid=%d, menuObjectPath=%s, service=%s)",
            pid,
            qPrintable(menuObjectPath.path()),
            qPrintable(service));

    m_interface->RegisterAppMenu(pid, menuObjectPath, menuObjectPath, service);
}

void UnityMenuRegistry::unregisterApplicationMenu(pid_t pid, QDBusObjectPath menuObjectPath)
{
    qCDebug(unityappmenuRegistrar, "UnityMenuRegistry::unregisterSurfaceMenu(pid=%d, menuObjectPath=%s)",
            pid,
            qPrintable(menuObjectPath.path()));

    m_interface->UnregisterAppMenu(pid, menuObjectPath);
}

void UnityMenuRegistry::registerSurfaceMenu(const QString &surfaceId, QDBusObjectPath menuObjectPath, const QString &service)
{
    qCDebug(unityappmenuRegistrar, "UnityMenuRegistry::registerMenu(surfaceId=%s, menuObjectPath=%s, service=%s)",
            qPrintable(surfaceId),
            qPrintable(menuObjectPath.path()),
            qPrintable(service));

    m_interface->RegisterSurfaceMenu(surfaceId, menuObjectPath, menuObjectPath, service);
}

void UnityMenuRegistry::unregisterSurfaceMenu(const QString &surfaceId, QDBusObjectPath menuObjectPath)
{
    qCDebug(unityappmenuRegistrar, "UnityMenuRegistry::unregisterSurfaceMenu(surfaceId=%s, menuObjectPath=%s)",
            qPrintable(surfaceId),
            qPrintable(menuObjectPath.path()));

    m_interface->UnregisterSurfaceMenu(surfaceId, menuObjectPath);
}


void UnityMenuRegistry::serviceOwnerChanged(const QString &serviceName, const QString& oldOwner, const QString &newOwner)
{
    qCDebug(unityappmenuRegistrar, "UnityMenuRegistry::serviceOwnerChanged(newOwner=%s)", qPrintable(newOwner));

    if (serviceName != REGISTRAR_SERVICE) return;

    if (oldOwner != newOwner) {
        m_connected = !newOwner.isEmpty();
        Q_EMIT serviceChanged();
    }
}
