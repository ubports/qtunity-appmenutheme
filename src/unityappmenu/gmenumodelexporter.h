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

#ifndef GMENUMODELEXPORTER_H
#define GMENUMODELEXPORTER_H

#include "gmenumodelplatformmenu.h"

#include <gio/gio.h>

#include <QTimer>
#include <QMap>
#include <QSet>
#include <QMetaObject>

class QtUnityExtraActionHandler;

// Base class for a gmenumodel exporter
class UnityGMenuModelExporter : public QObject
{
    Q_OBJECT
public:
    virtual ~UnityGMenuModelExporter();

    void exportModels();
    void unexportModels();

    QString menuPath() const { return m_menuPath;}

    void aboutToShow(quint64 tag);

protected:
    UnityGMenuModelExporter(QObject *parent);

    GMenuItem *createSubmenu(QPlatformMenu* platformMenu, UnityPlatformMenuItem* forItem);
    GMenuItem *createMenuItem(QPlatformMenuItem* platformMenuItem, GMenu *parentMenu);
    GMenuItem *createSection(QList<QPlatformMenuItem*>::const_iterator iter, QList<QPlatformMenuItem*>::const_iterator end);
    void addAction(const QByteArray& name, UnityPlatformMenuItem* gplatformItem, GMenu *parentMenu);

    void addSubmenuItems(UnityPlatformMenu* gplatformMenu, GMenu* menu);
    void processItemForGMenu(QPlatformMenuItem* item, GMenu* gmenu);

    void clear();

    void timerEvent(QTimerEvent *e) override;

protected:
    GDBusConnection *m_connection;
    GMenu *m_gmainMenu;
    GSimpleActionGroup *m_gactionGroup;
    guint m_exportedModel;
    guint m_exportedActions;
    QtUnityExtraActionHandler *m_qtunityExtraHandler;
    QTimer m_structureTimer;
    QString m_menuPath;

    // UnityPlatformMenu::tag -> UnityPlatformMenu
    QMap<quint64, UnityPlatformMenu*> m_submenusWithTag;

    // UnityPlatformMenu -> reload TimerId (startTimer)
    QHash<UnityPlatformMenu*, int> m_reloadMenuTimers;

    QHash<UnityPlatformMenu*, GMenu*> m_gmenusForMenus;

    QHash<GMenu*, QSet<QByteArray>> m_actions;
    QHash<GMenu*, QVector<QMetaObject::Connection>> m_propertyConnections;

};

// Class which exports a qt platform menu bar.
class UnityMenuBarExporter : public UnityGMenuModelExporter
{
public:
    UnityMenuBarExporter(UnityPlatformMenuBar *parent);
    ~UnityMenuBarExporter();
};

// Class which exports a qt platform menu.
// This will allow exporting of context menus.
class UnityMenuExporter : public UnityGMenuModelExporter
{
public:
    UnityMenuExporter(UnityPlatformMenu *parent);
    ~UnityMenuExporter();
};

#endif // GMENUMODELEXPORTER_H
