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

// Local
#include "gmenumodelexporter.h"
#include "registry.h"
#include "logging.h"
#include "qtunityextraactionhandler.h"

#include <QDebug>
#include <QTimerEvent>

#include <functional>

namespace {

// Derive an action name from the label by removing spaces and Capitilizing the words.
// Also remove mnemonics from the label.
inline QString getActionString(QString label)
{
    QRegExp re("\\W");
    label = label.replace(QRegExp("(&|_)"), "");

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList parts = label.split(re, Qt::SkipEmptyParts);
#else
    QStringList parts = label.split(re, QString::SkipEmptyParts);
#endif

    QString result;
    Q_FOREACH(const QString& part, parts) {
        result += part[0].toUpper();
        result += part.right(part.length()-1);
    }
    return result;
}

static void activate_cb(GSimpleAction *action, GVariant *, gpointer user_data)
{
    qCDebug(unityappmenu, "Activate menu action '%s'", g_action_get_name(G_ACTION(action)));
    auto item = static_cast<UnityPlatformMenuItem*>(user_data);
    item->activated();
}

static uint s_menuId = 0;

#define MENU_OBJECT_PATH "/io/unity8/Menu/%1"

} // namespace


UnityMenuBarExporter::UnityMenuBarExporter(UnityPlatformMenuBar * bar)
    : UnityGMenuModelExporter(bar)
{
    qCDebug(unityappmenu, "UnityMenuBarExporter::UnityMenuBarExporter");

    connect(bar, &UnityPlatformMenuBar::structureChanged, this, [this]() {
        m_structureTimer.start();
    });
    connect(&m_structureTimer, &QTimer::timeout, this, [this, bar]() {
        clear();
        Q_FOREACH(QPlatformMenu *platformMenu, bar->menus()) {
            GMenuItem* item = createSubmenu(platformMenu, nullptr);
            if (item) {
                g_menu_append_item(m_gmainMenu, item);
                g_object_unref(item);
            }

            UnityPlatformMenu* gplatformMenu = static_cast<UnityPlatformMenu*>(platformMenu);
            if (gplatformMenu) {
                // Sadly we don't have a better way to propagate a enabled change in a top level menu
                // than reseting the whole menubar
                connect(gplatformMenu, &UnityPlatformMenu::enabledChanged, bar, &UnityPlatformMenuBar::structureChanged);
            }
        }
    });

    connect(bar, &UnityPlatformMenuBar::ready, this, [this]() {
        exportModels();
    });
}

UnityMenuBarExporter::~UnityMenuBarExporter()
{
    qCDebug(unityappmenu, "UnityMenuBarExporter::~UnityMenuBarExporter");
}

UnityMenuExporter::UnityMenuExporter(UnityPlatformMenu *menu)
    : UnityGMenuModelExporter(menu)
{
    qCDebug(unityappmenu, "UnityMenuExporter::UnityMenuExporter");

    connect(menu, &UnityPlatformMenu::structureChanged, this, [this]() {
        m_structureTimer.start();
    });
    connect(&m_structureTimer, &QTimer::timeout, this, [this, menu]() {
        clear();
        addSubmenuItems(menu, m_gmainMenu);
    });
    addSubmenuItems(menu, m_gmainMenu);
}

UnityMenuExporter::~UnityMenuExporter()
{
    qCDebug(unityappmenu, "UnityMenuExporter::~UnityMenuExporter");
}

UnityGMenuModelExporter::UnityGMenuModelExporter(QObject *parent)
    : QObject(parent)
    , m_connection(nullptr)
    , m_gmainMenu(g_menu_new())
    , m_gactionGroup(g_simple_action_group_new())
    , m_exportedModel(0)
    , m_exportedActions(0)
    , m_qtunityExtraHandler(nullptr)
    , m_menuPath(QStringLiteral(MENU_OBJECT_PATH).arg(s_menuId++))
{
    m_structureTimer.setSingleShot(true);
    m_structureTimer.setInterval(0);
}

UnityGMenuModelExporter::~UnityGMenuModelExporter()
{
    unexportModels();
    clear();

    g_object_unref(m_gmainMenu);
    g_object_unref(m_gactionGroup);
}

// Clear the menu and actions that have been created.
void UnityGMenuModelExporter::clear()
{
    Q_FOREACH(const QVector<QMetaObject::Connection>& menuPropertyConnections, m_propertyConnections) {
        Q_FOREACH(const QMetaObject::Connection& connection, menuPropertyConnections) {
            QObject::disconnect(connection);
        }
    }
    m_propertyConnections.clear();

    g_menu_remove_all(m_gmainMenu);

    Q_FOREACH(const QSet<QByteArray>& menuActions, m_actions) {
        Q_FOREACH(const QByteArray& action, menuActions) {
            g_action_map_remove_action(G_ACTION_MAP(m_gactionGroup), action.constData());
        }
    }
    m_actions.clear();

    m_gmenusForMenus.clear();
}

void UnityGMenuModelExporter::timerEvent(QTimerEvent *e)
{
    // Find the menu, it's a very short list
    auto it = m_reloadMenuTimers.begin();
    for (; it != m_reloadMenuTimers.end(); ++it) {
        if (e->timerId() == it.value())
            break;
    }

    if (it != m_reloadMenuTimers.end()) {
        UnityPlatformMenu* gplatformMenu = it.key();
        GMenu *menu = m_gmenusForMenus.value(gplatformMenu);
        if (menu) {
            Q_FOREACH(const QMetaObject::Connection& connection, m_propertyConnections[menu]) {
                QObject::disconnect(connection);
            }
            m_propertyConnections.remove(menu);
            Q_FOREACH(const QByteArray& action, m_actions[menu]) {
                g_action_map_remove_action(G_ACTION_MAP(m_gactionGroup), action.constData());
            }
            m_actions.remove(menu);
            g_menu_remove_all(menu);
            addSubmenuItems(gplatformMenu, menu);
        } else {
            qWarning() << "Got an update timer for a menu that has no GMenu" << gplatformMenu;
        }

        m_reloadMenuTimers.erase(it);
    } else {
        qWarning() << "Got an update timer for a timer that was not running";
    }
    killTimer(e->timerId());
}

// Export the model on dbus
void UnityGMenuModelExporter::exportModels()
{
    GError *error = nullptr;
    m_connection = g_bus_get_sync (G_BUS_TYPE_SESSION, nullptr, &error);
    if (!m_connection) {
        qCWarning(unityappmenu, "Failed to retreive session bus - %s", error ? error->message : "unknown error");
        g_error_free (error);
        return;
    }

    QByteArray menuPath(m_menuPath.toUtf8());

    if (m_exportedModel == 0) {
        m_exportedModel = g_dbus_connection_export_menu_model(m_connection, menuPath.constData(), G_MENU_MODEL(m_gmainMenu), &error);
        if (m_exportedModel == 0) {
            qCWarning(unityappmenu, "Failed to export menu - %s", error ? error->message : "unknown error");
            g_error_free (error);
            error = nullptr;
        } else {
            qCDebug(unityappmenu, "Exported menu on %s", g_dbus_connection_get_unique_name(m_connection));
        }
    }

    if (m_exportedActions == 0) {
        m_exportedActions = g_dbus_connection_export_action_group(m_connection, menuPath.constData(), G_ACTION_GROUP(m_gactionGroup), &error);
        if (m_exportedActions == 0) {
            qCWarning(unityappmenu, "Failed to export actions - %s", error ? error->message : "unknown error");
            g_error_free (error);
            error = nullptr;
        } else {
            qCDebug(unityappmenu, "Exported actions on %s", g_dbus_connection_get_unique_name(m_connection));
        }
    }

    if (!m_qtunityExtraHandler) {
        m_qtunityExtraHandler = new QtUnityExtraActionHandler();
        if (!m_qtunityExtraHandler->connect(m_connection, menuPath, this)) {
            delete m_qtunityExtraHandler;
            m_qtunityExtraHandler = nullptr;
        }
    }
}

void UnityGMenuModelExporter::aboutToShow(quint64 tag)
{
    UnityPlatformMenu* gplatformMenu = m_submenusWithTag.value(tag);
    if (!gplatformMenu) {
        qWarning() << "Got an aboutToShow call with an unknown tag";
        return;
    }

    gplatformMenu->aboutToShow();
}

// Unexport the model
void UnityGMenuModelExporter::unexportModels()
{
    GError *error = nullptr;
    if (!m_connection) {
        qCWarning(unityappmenu, "Failed to retreive session bus - %s", error ? error->message : "unknown error");
        return;
    }

    if (m_exportedModel != 0) {
        g_dbus_connection_unexport_menu_model(m_connection, m_exportedModel);
        m_exportedModel = 0;
    }
    if (m_exportedActions != 0) {
        g_dbus_connection_unexport_action_group(m_connection, m_exportedActions);
        m_exportedActions = 0;
    }
    if (m_qtunityExtraHandler) {
        m_qtunityExtraHandler->disconnect(m_connection);
        delete m_qtunityExtraHandler;
        m_qtunityExtraHandler = nullptr;
    }
    g_object_unref(m_connection);
    m_connection = nullptr;
}

// Create a submenu for the given platform menu.
// Returns a gmenuitem entry for the menu, which must be cleaned up using g_object_unref.
// If forItem is suplied, use it's label.
GMenuItem *UnityGMenuModelExporter::createSubmenu(QPlatformMenu *platformMenu, UnityPlatformMenuItem *forItem)
{
    UnityPlatformMenu* gplatformMenu = static_cast<UnityPlatformMenu*>(platformMenu);
    if (!gplatformMenu) return nullptr;
    GMenu* menu = g_menu_new();

    m_gmenusForMenus.insert(gplatformMenu, menu);

    QByteArray label;
    bool enabled;
    if (forItem) {
        label = UnityPlatformMenuItem::get_text(forItem).toUtf8();
        enabled = UnityPlatformMenuItem::get_enabled(forItem);
    } else {
        label = UnityPlatformMenu::get_text(gplatformMenu).toUtf8();
        enabled = UnityPlatformMenu::get_enabled(gplatformMenu);
    }

    addSubmenuItems(gplatformMenu, menu);

    Q_FOREACH(QPlatformMenuItem *childItem, gplatformMenu->menuItems()) {
        UnityPlatformMenuItem* gplatformMenuItem = static_cast<UnityPlatformMenuItem*>(childItem);
        if (!gplatformMenuItem) continue;

        // Sadly we don't have a better way to propagate a enabled change in a item-that-is-submenu
        // than reseting the whole parent menu
        if (gplatformMenuItem->menu()) {
            connect(gplatformMenuItem, &UnityPlatformMenuItem::enabledChanged, gplatformMenu, &UnityPlatformMenu::structureChanged);
        }
        connect(gplatformMenuItem, &UnityPlatformMenuItem::visibleChanged, gplatformMenu, &UnityPlatformMenu::structureChanged);
    }

    GMenuItem* gmenuItem = g_menu_item_new_submenu(label.constData(), G_MENU_MODEL(menu));
    const quint64 tag = gplatformMenu->tag();
    if (tag != 0) {
        g_menu_item_set_attribute_value(gmenuItem, "qtunity-tag", g_variant_new_uint64 (tag));
        m_submenusWithTag.insert(gplatformMenu->tag(), gplatformMenu);
    }
    g_object_unref(menu);

    g_menu_item_set_attribute_value(gmenuItem, "submenu-enabled", g_variant_new_boolean(enabled));

    connect(gplatformMenu, &UnityPlatformMenu::structureChanged, this, [this, gplatformMenu]
        {
            if (!m_reloadMenuTimers.contains(gplatformMenu)) {
                const int timerId = startTimer(0);
                m_reloadMenuTimers.insert(gplatformMenu, timerId);
            }
        });

    connect(gplatformMenu, &UnityPlatformMenu::destroyed, this, [this, tag, gplatformMenu]
        {
            m_submenusWithTag.remove(tag);
            m_gmenusForMenus.remove(gplatformMenu);
            auto timerIdIt = m_reloadMenuTimers.find(gplatformMenu);
            if (timerIdIt != m_reloadMenuTimers.end()) {
                killTimer(*timerIdIt);
                m_reloadMenuTimers.erase(timerIdIt);
            }
        });

    return gmenuItem;
}

// Add a platform menu's items to the given gmenu.
// The items are inserted into menus sections, split by the menu separators.
void UnityGMenuModelExporter::addSubmenuItems(UnityPlatformMenu* gplatformMenu, GMenu* menu)
{
    auto iter = gplatformMenu->menuItems().begin();
    auto lastSectionStart = iter;
    // Iterate through all the menu items adding sections when a separator is found.
    for (; iter != gplatformMenu->menuItems().end(); ++iter) {
        UnityPlatformMenuItem* gplatformMenuItem = static_cast<UnityPlatformMenuItem*>(*iter);
        if (!gplatformMenuItem) continue;

        // don't add a section until we have separator
        if (UnityPlatformMenuItem::get_separator(gplatformMenuItem)) {
            if (lastSectionStart != gplatformMenu->menuItems().begin()) {
                GMenuItem* section = createSection(lastSectionStart, iter);
                g_menu_append_item(menu, section);
                g_object_unref(section);
            }
            lastSectionStart = iter + 1;
        } else if (lastSectionStart == gplatformMenu->menuItems().begin()) {
            processItemForGMenu(gplatformMenuItem, menu);
        }
    }

    // Add the last section
    if (lastSectionStart != gplatformMenu->menuItems().begin() &&
            lastSectionStart != gplatformMenu->menuItems().end()) {
        GMenuItem* gsectionItem = createSection(lastSectionStart, gplatformMenu->menuItems().end());
        g_menu_append_item(menu, gsectionItem);
        g_object_unref(gsectionItem);
    }
}

// Create and return a gmenu item for the given platform menu item.
// Returned GMenuItem must be cleaned up using g_object_unref
GMenuItem *UnityGMenuModelExporter::createMenuItem(QPlatformMenuItem *platformMenuItem, GMenu *parentMenu)
{
    UnityPlatformMenuItem* gplatformMenuItem = static_cast<UnityPlatformMenuItem*>(platformMenuItem);
    if (!gplatformMenuItem) return nullptr;

    if (!UnityPlatformMenuItem::get_visible(gplatformMenuItem))
        return nullptr;

    QByteArray label(UnityPlatformMenuItem::get_text(gplatformMenuItem).toUtf8());
    QByteArray actionLabel(getActionString(UnityPlatformMenuItem::get_text(gplatformMenuItem)).toUtf8());
    QByteArray shortcut(UnityPlatformMenuItem::get_shortcut(gplatformMenuItem).toString(QKeySequence::NativeText).toUtf8());

    GMenuItem* gmenuItem = g_menu_item_new(label.constData(), nullptr);
    g_menu_item_set_attribute(gmenuItem, "accel", "s", shortcut.constData());
    g_menu_item_set_detailed_action(gmenuItem, ("unity." + actionLabel).constData());

    addAction(actionLabel, gplatformMenuItem, parentMenu);
    return gmenuItem;
}

// Create a menu section for a section of separated menu items.
// Returned GMenuItem must be cleaned up using g_object_unref
GMenuItem *UnityGMenuModelExporter::createSection(QList<QPlatformMenuItem *>::const_iterator iter, QList<QPlatformMenuItem *>::const_iterator end)
{
    GMenu* gsectionMenu = g_menu_new();
    for (; iter != end; ++iter) {
        processItemForGMenu(*iter, gsectionMenu);
    }
    GMenuItem* gsectionItem = g_menu_item_new_section("", G_MENU_MODEL(gsectionMenu));
    g_object_unref(gsectionMenu);
    return gsectionItem;
}

// Add the given platform menu item to the menu.
// If it has an attached submenu, then create and add the submenu.
void UnityGMenuModelExporter::processItemForGMenu(QPlatformMenuItem *platformMenuItem, GMenu *gmenu)
{
    UnityPlatformMenuItem* gplatformMenuItem = static_cast<UnityPlatformMenuItem*>(platformMenuItem);
    if (!gplatformMenuItem) return;

    GMenuItem* gmenuItem = gplatformMenuItem->menu() ? createSubmenu(gplatformMenuItem->menu(), gplatformMenuItem) :
                                                       createMenuItem(gplatformMenuItem, gmenu);
    if (gmenuItem) {
        g_menu_append_item(gmenu, gmenuItem);
        g_object_unref(gmenuItem);
    }
}

// Create and add an action for a menu item.
void UnityGMenuModelExporter::addAction(const QByteArray &name, UnityPlatformMenuItem *gplatformMenuItem, GMenu *parentMenu)
{
    disconnect(gplatformMenuItem, &UnityPlatformMenuItem::checkedChanged, this, 0);
    disconnect(gplatformMenuItem, &UnityPlatformMenuItem::enabledChanged, this, 0);

    QSet<QByteArray> &actions = m_actions[parentMenu];
    QVector<QMetaObject::Connection> &propertyConnections = m_propertyConnections[parentMenu];

    if (actions.contains(name)) {
        g_action_map_remove_action(G_ACTION_MAP(m_gactionGroup), name.constData());
        actions.remove(name);
    }

    bool checkable = UnityPlatformMenuItem::get_checkable(gplatformMenuItem);

    GSimpleAction* action = nullptr;
    if (checkable) {
        bool checked = UnityPlatformMenuItem::get_checked(gplatformMenuItem);
        action = g_simple_action_new_stateful(name.constData(), nullptr, g_variant_new_boolean(checked));

        std::function<void(bool)> updateChecked = [gplatformMenuItem, action](bool checked) {
            auto type = g_action_get_state_type(G_ACTION(action));
            if (type && g_variant_type_equal(type, G_VARIANT_TYPE_BOOLEAN)) {
                g_simple_action_set_state(action, g_variant_new_boolean(checked ? TRUE : FALSE));
            }
        };
        // save the connection to disconnect in UnityGMenuModelExporter::clear()
        propertyConnections << connect(gplatformMenuItem, &UnityPlatformMenuItem::checkedChanged, this, updateChecked);
    } else {
        action = g_simple_action_new(name.constData(), nullptr);
    }

    // Enabled update
    std::function<void(bool)> updateEnabled = [gplatformMenuItem, action](bool enabled) {
        GValue value = G_VALUE_INIT;
        g_value_init (&value, G_TYPE_BOOLEAN);
        g_value_set_boolean(&value, enabled ? TRUE : FALSE);
        g_object_set_property(G_OBJECT(action), "enabled", &value);
    };
    updateEnabled(UnityPlatformMenuItem::get_enabled(gplatformMenuItem));
    // save the connection to disconnect in UnityGMenuModelExporter::clear()
    propertyConnections << connect(gplatformMenuItem, &UnityPlatformMenuItem::enabledChanged, this, updateEnabled);

    g_signal_connect(action, "activate", G_CALLBACK(activate_cb), gplatformMenuItem);

    actions.insert(name);
    g_action_map_add_action(G_ACTION_MAP(m_gactionGroup), G_ACTION(action));
    g_object_unref(action);
}
