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
#include "gmenumodelplatformmenu.h"
#include "gmenumodelexporter.h"
#include "registry.h"
#include "menuregistrar.h"
#include "logging.h"

// Qt
#include <QDebug>
#include <QWindow>
#include <QCoreApplication>

#define BAR_DEBUG_MSG qCDebug(unityappmenu).nospace() << "UnityPlatformMenuBar[" << (void*)this <<"]::" << __func__
#define MENU_DEBUG_MSG qCDebug(unityappmenu).nospace() << "UnityPlatformMenu[" << (void*)this <<"]::" << __func__
#define ITEM_DEBUG_MSG qCDebug(unityappmenu).nospace() << "UnityPlatformMenuItem[" << (void*)this <<"]::" << __func__

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define qtendl Qt::endl
#else
#define qtendl endl
#endif

namespace {

int logRecusion = 0;

}

QDebug operator<<(QDebug stream, UnityPlatformMenuBar* bar) {
    if (bar) return bar->operator<<(stream);
    return stream;
}
QDebug operator<<(QDebug stream, UnityPlatformMenu* menu) {
    if (menu) return menu->operator<<(stream);
    return stream;
}
QDebug operator<<(QDebug stream, UnityPlatformMenuItem* menuItem) {
    if (menuItem) return menuItem->operator<<(stream);
    return stream;
}

UnityPlatformMenuBar::UnityPlatformMenuBar()
    : m_exporter(new UnityMenuBarExporter(this))
    , m_registrar(new UnityMenuRegistrar())
    , m_ready(false)
{
    BAR_DEBUG_MSG << "()";

    connect(this, &UnityPlatformMenuBar::menuInserted, this, &UnityPlatformMenuBar::structureChanged);
    connect(this,&UnityPlatformMenuBar::menuRemoved, this, &UnityPlatformMenuBar::structureChanged);
}

UnityPlatformMenuBar::~UnityPlatformMenuBar()
{
    BAR_DEBUG_MSG << "()";
}

void UnityPlatformMenuBar::insertMenu(QPlatformMenu *menu, QPlatformMenu *before)
{
    BAR_DEBUG_MSG << "(menu=" << menu << ", before=" <<  before << ")";

    if (m_menus.contains(menu)) return;

    if (!before) {
        m_menus.push_back(menu);
    } else {
        for (auto iter = m_menus.begin(); iter != m_menus.end(); ++iter) {
            if (*iter == before) {
                m_menus.insert(iter, menu);
                break;
            }
        }
    }
    Q_EMIT menuInserted(menu);
}

void UnityPlatformMenuBar::removeMenu(QPlatformMenu *menu)
{
    BAR_DEBUG_MSG << "(menu=" << menu << ")";

    QMutableListIterator<QPlatformMenu*> iterator(m_menus);
    while(iterator.hasNext()) {
        if (iterator.next() == menu) {
            iterator.remove();
            break;
        }
    }
    Q_EMIT menuRemoved(menu);
}

void UnityPlatformMenuBar::syncMenu(QPlatformMenu *menu)
{
    BAR_DEBUG_MSG << "(menu=" << menu << ")";

    Q_UNUSED(menu)
}

void UnityPlatformMenuBar::handleReparent(QWindow *parentWindow)
{
    BAR_DEBUG_MSG << "(parentWindow=" << parentWindow << ")";

    setReady(true);
    m_registrar->registerMenuForWindow(parentWindow, QDBusObjectPath(m_exporter->menuPath()));
}

QPlatformMenu *UnityPlatformMenuBar::menuForTag(quintptr tag) const
{
    Q_FOREACH(QPlatformMenu* menu, m_menus) {
        if (menu->tag() == tag) {
            return menu;
        }
    }
    return nullptr;
}

const QList<QPlatformMenu *> UnityPlatformMenuBar::menus() const
{
    return m_menus;
}

QDebug UnityPlatformMenuBar::operator<<(QDebug stream)
{
    stream.nospace().noquote() << QString("%1").arg("", logRecusion, QLatin1Char('\t'))
            << "UnityPlatformMenuBar(this=" << (void*)this << ")" << qtendl;
    Q_FOREACH(QPlatformMenu* menu, m_menus) {
        auto myMenu = static_cast<UnityPlatformMenu*>(menu);
        if (myMenu) {
            logRecusion++;
            stream << myMenu;
            logRecusion--;
        }
    }

    return stream;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
QPlatformMenu *UnityPlatformMenuBar::createMenu() const
{
    return new UnityPlatformMenu();
}
#endif

void UnityPlatformMenuBar::setReady(bool isReady)
{
    if (m_ready != isReady) {
        m_ready = isReady;
        Q_EMIT ready();
    }
}

//////////////////////////////////////////////////////////////

UnityPlatformMenu::UnityPlatformMenu()
    : m_tag(reinterpret_cast<quintptr>(this))
    , m_parentWindow(nullptr)
    , m_exporter(nullptr)
    , m_registrar(nullptr)
{
    MENU_DEBUG_MSG << "()";

    connect(this, &UnityPlatformMenu::menuItemInserted, this, &UnityPlatformMenu::structureChanged);
    connect(this, &UnityPlatformMenu::menuItemRemoved, this, &UnityPlatformMenu::structureChanged);
}

UnityPlatformMenu::~UnityPlatformMenu()
{
    MENU_DEBUG_MSG << "()";
}

void UnityPlatformMenu::insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before)
{
    MENU_DEBUG_MSG << "(menuItem=" << menuItem << ", before=" << before << ")";

    if (m_menuItems.contains(menuItem)) return;

    if (!before) {
        m_menuItems.push_back(menuItem);
    } else {
        for (auto iter = m_menuItems.begin(); iter != m_menuItems.end(); ++iter) {
            if (*iter == before) {
                m_menuItems.insert(iter, menuItem);
                break;
            }
        }
    }

    Q_EMIT menuItemInserted(menuItem);
}

void UnityPlatformMenu::removeMenuItem(QPlatformMenuItem *menuItem)
{
    MENU_DEBUG_MSG << "(menuItem=" << menuItem << ")";

    QMutableListIterator<QPlatformMenuItem*> iterator(m_menuItems);
    while(iterator.hasNext()) {
        if (iterator.next() == menuItem) {
            iterator.remove();
            break;
        }
    }
    Q_EMIT menuItemRemoved(menuItem);
}

void UnityPlatformMenu::syncMenuItem(QPlatformMenuItem *menuItem)
{
    MENU_DEBUG_MSG << "(menuItem=" << menuItem << ")";

    Q_UNUSED(menuItem)
}

void UnityPlatformMenu::syncSeparatorsCollapsible(bool enable)
{
    MENU_DEBUG_MSG << "(enable=" << enable << ")";
    Q_UNUSED(enable)
}

void UnityPlatformMenu::setTag(quintptr tag)
{
    MENU_DEBUG_MSG << "(tag=" << tag << ")";
    m_tag = tag;
}

quintptr UnityPlatformMenu::tag() const
{
    return m_tag;
}

void UnityPlatformMenu::setText(const QString &text)
{
    MENU_DEBUG_MSG << "(text=" << text << ")";
    if (m_text != text) {
        m_text = text;
    }
}

void UnityPlatformMenu::setIcon(const QIcon &icon)
{
    MENU_DEBUG_MSG << "(icon=" << icon.name() << ")";

    if (!icon.isNull() || (!m_icon.isNull() && icon.isNull())) {
        m_icon = icon;
    }
}

void UnityPlatformMenu::setEnabled(bool enabled)
{
    MENU_DEBUG_MSG << "(enabled=" << enabled << ")";

    if (m_enabled != enabled) {
        m_enabled = enabled;
        Q_EMIT enabledChanged(enabled);
    }
}

void UnityPlatformMenu::setVisible(bool isVisible)
{
    MENU_DEBUG_MSG << "(visible=" << isVisible << ")";

    if (m_visible != isVisible) {
        m_visible = isVisible;
    }
}

void UnityPlatformMenu::setMinimumWidth(int width)
{
    MENU_DEBUG_MSG << "(width=" << width << ")";

    Q_UNUSED(width)
}

void UnityPlatformMenu::setFont(const QFont &font)
{
    MENU_DEBUG_MSG << "(font=" << font << ")";

    Q_UNUSED(font)
}

void UnityPlatformMenu::showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
{
    MENU_DEBUG_MSG << "(parentWindow=" << parentWindow << ", targetRect=" << targetRect << ", item=" << item << ")";

    if (!m_exporter) {
        m_exporter.reset(new UnityMenuExporter(this));
        m_exporter->exportModels();
    }

    if (parentWindow != m_parentWindow) {
        if (m_parentWindow) {
            m_registrar->unregisterMenu();
        }

        m_parentWindow = parentWindow;

        if (m_parentWindow) {
            if (!m_registrar) m_registrar.reset(new UnityMenuRegistrar);
            m_registrar->registerMenuForWindow(const_cast<QWindow*>(m_parentWindow),
                                                      QDBusObjectPath(m_exporter->menuPath()));
        }
    }

    Q_UNUSED(targetRect);
    Q_UNUSED(item);
    setVisible(true);
}

void UnityPlatformMenu::dismiss()
{
    MENU_DEBUG_MSG << "()";

    if (m_registrar) { m_registrar->unregisterMenu(); }
    if (m_exporter) { m_exporter->unexportModels(); }
}

QPlatformMenuItem *UnityPlatformMenu::menuItemAt(int position) const
{
    if (position < 0 || position >= m_menuItems.count()) return nullptr;
    return m_menuItems.at(position);
}

QPlatformMenuItem *UnityPlatformMenu::menuItemForTag(quintptr tag) const
{
    Q_FOREACH(QPlatformMenuItem* menuItem, m_menuItems) {
        if (menuItem->tag() == tag) {
            return menuItem;
        }
    }
    return nullptr;
}

QPlatformMenuItem *UnityPlatformMenu::createMenuItem() const
{
    return new UnityPlatformMenuItem();
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
QPlatformMenu *UnityPlatformMenu::createSubMenu() const
{
    return new UnityPlatformMenu();
}
#endif

const QList<QPlatformMenuItem *> UnityPlatformMenu::menuItems() const
{
    return m_menuItems;
}

QDebug UnityPlatformMenu::operator<<(QDebug stream)
{
    stream.nospace().noquote() << QString("%1").arg("", logRecusion, QLatin1Char('\t'))
            << "UnityPlatformMenu(this=" << (void*)this << ", text=\"" << m_text << "\")" << qtendl;
    Q_FOREACH(QPlatformMenuItem* item, m_menuItems) {
        logRecusion++;
        auto myItem = static_cast<UnityPlatformMenuItem*>(item);
        if (myItem) {
            stream << myItem;
        }
        logRecusion--;
    }
    return stream;
}

//////////////////////////////////////////////////////////////

UnityPlatformMenuItem::UnityPlatformMenuItem()
    : m_menu(nullptr)
    , m_tag(reinterpret_cast<quintptr>(this))
{
    ITEM_DEBUG_MSG << "()";
}

UnityPlatformMenuItem::~UnityPlatformMenuItem()
{
    ITEM_DEBUG_MSG << "()";
}

void UnityPlatformMenuItem::setTag(quintptr tag)
{
    ITEM_DEBUG_MSG << "(tag=" << tag << ")";
    m_tag = tag;
}

quintptr UnityPlatformMenuItem::tag() const
{
    return m_tag;
}

void UnityPlatformMenuItem::setText(const QString &text)
{
    ITEM_DEBUG_MSG << "(text=" << text << ")";
    if (m_text != text) {
        m_text = text;
    }
}

void UnityPlatformMenuItem::setIcon(const QIcon &icon)
{
    ITEM_DEBUG_MSG << "(icon=" << icon.name() << ")";

    if (!icon.isNull() || (!m_icon.isNull() && icon.isNull())) {
        m_icon = icon;
    }
}

void UnityPlatformMenuItem::setVisible(bool isVisible)
{
    ITEM_DEBUG_MSG << "(visible=" << isVisible << ")";
    if (m_visible != isVisible) {
        m_visible = isVisible;
        Q_EMIT visibleChanged(m_visible);
    }
}

void UnityPlatformMenuItem::setIsSeparator(bool isSeparator)
{
    ITEM_DEBUG_MSG << "(separator=" << isSeparator << ")";
    if (m_separator != isSeparator) {
        m_separator = isSeparator;
    }
}

void UnityPlatformMenuItem::setFont(const QFont &font)
{
    ITEM_DEBUG_MSG << "(font=" << font << ")";
    Q_UNUSED(font);
}

void UnityPlatformMenuItem::setRole(QPlatformMenuItem::MenuRole role)
{
    ITEM_DEBUG_MSG << "(role=" << role << ")";
    Q_UNUSED(role);
}

void UnityPlatformMenuItem::setCheckable(bool checkable)
{
    ITEM_DEBUG_MSG << "(checkable=" << checkable << ")";
    if (m_checkable != checkable) {
        m_checkable = checkable;
    }
}

void UnityPlatformMenuItem::setChecked(bool isChecked)
{
    ITEM_DEBUG_MSG << "(checked=" << isChecked << ")";
    if (m_checked != isChecked) {
        m_checked = isChecked;
        Q_EMIT checkedChanged(isChecked);
    }
}

void UnityPlatformMenuItem::setShortcut(const QKeySequence &shortcut)
{
    ITEM_DEBUG_MSG << "(shortcut=" << shortcut << ")";
    if (m_shortcut != shortcut) {
        m_shortcut = shortcut;
    }
}

void UnityPlatformMenuItem::setEnabled(bool enabled)
{
    ITEM_DEBUG_MSG << "(enabled=" << enabled << ")";
    if (m_enabled != enabled) {
        m_enabled = enabled;
        Q_EMIT enabledChanged(enabled);
    }
}

void UnityPlatformMenuItem::setIconSize(int size)
{
    ITEM_DEBUG_MSG << "(size=" << size << ")";
    Q_UNUSED(size);
}

void UnityPlatformMenuItem::setMenu(QPlatformMenu *menu)
{
    ITEM_DEBUG_MSG << "(menu=" << menu << ")";
    if (m_menu != menu) {
        m_menu = menu;

        if (menu) {
            connect(menu, &QObject::destroyed,
                    this, [this] { setMenu(nullptr); });
        }
    }
}

QPlatformMenu *UnityPlatformMenuItem::menu() const
{
    return m_menu;
}

QDebug UnityPlatformMenuItem::operator<<(QDebug stream)
{
    QString properties = "text=\"" + m_text + "\"";

    stream.nospace().noquote() << QString("%1").arg("", logRecusion, QLatin1Char('\t'))
            << "UnityPlatformMenuItem(this=" << (void*)this << ", "
            << (m_separator ? "Separator" : properties) << ")" << qtendl;
    if (m_menu) {
        auto myMenu = static_cast<UnityPlatformMenu*>(m_menu);
        if (myMenu) {
            logRecusion++;
            stream << myMenu;
            logRecusion--;
        }
    }
    return stream;
}
