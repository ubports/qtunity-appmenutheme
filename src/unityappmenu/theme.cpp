/*
 * Copyright (C) 2016-2017 Canonical, Ltd.
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

#include "theme.h"
#include "gmenumodelplatformmenu.h"
#include "logging.h"

#include <QtCore/QVariant>
#include <QDebug>

Q_LOGGING_CATEGORY(unityappmenu, "unityappmenu", QtWarningMsg)
const char *UnityAppMenuTheme::name = "unityappmenu";

namespace {

bool useLocalMenu() {
    QByteArray menuProxy = qgetenv("UNITY_MENUPROXY");
    bool menuProxyIsZero = !menuProxy.isEmpty() && menuProxy.at(0) == '0';
    return menuProxyIsZero;
}

}

UnityAppMenuTheme::UnityAppMenuTheme():
    UnityTheme()
{
    qCDebug(unityappmenu, "UnityAppMenuTheme::UnityAppMenuTheme() - useLocalMenu=%s", useLocalMenu() ? "true" : "false");
}

QPlatformMenuItem *UnityAppMenuTheme::createPlatformMenuItem() const
{
    if (useLocalMenu()) return QGenericUnixTheme::createPlatformMenuItem();
    return new UnityPlatformMenuItem();
}

QPlatformMenu *UnityAppMenuTheme::createPlatformMenu() const
{
    if (useLocalMenu()) return QGenericUnixTheme::createPlatformMenu();
    return new UnityPlatformMenu();
}

QPlatformMenuBar *UnityAppMenuTheme::createPlatformMenuBar() const
{
    if (useLocalMenu()) return QGenericUnixTheme::createPlatformMenuBar();
    return new UnityPlatformMenuBar();
}

QPlatformSystemTrayIcon *UnityAppMenuTheme::createPlatformSystemTrayIcon() const
{
    // We can't use QGenericUnixTheme implementation since it needs the platformMenu to
    // be a subclass of QDBusPlatformMenu and ours isn't
    // TODO Investigate if we're fine with not supporting system trays or we should fix it
    return nullptr;
}
