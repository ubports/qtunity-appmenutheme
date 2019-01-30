/*
 * Copyright (C) 2017 Canonical, Ltd.
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

#include "qtunityextraactionhandler.h"

#include "gmenumodelexporter.h"
#include "logging.h"

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='qtunity.actions.extra'>"
  "    <method name='aboutToShow'>"
  "      <arg type='t' name='tag' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static void handle_method_call (GDBusConnection       *,
                                const gchar           *,
                                const gchar           *,
                                const gchar           *,
                                const gchar           *method_name,
                                GVariant              *parameters,
                                GDBusMethodInvocation *invocation,
                                gpointer               user_data)
{

    if (g_strcmp0 (method_name, "aboutToShow") == 0)
    {
        if (g_variant_check_format_string(parameters, "(t)", false)) {
            auto obj = static_cast<UnityGMenuModelExporter*>(user_data);
            guint64 tag;

            g_variant_get (parameters, "(t)", &tag);
            obj->aboutToShow(tag);
        }

        g_dbus_method_invocation_return_value (invocation, NULL);
    } else {
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_UNKNOWN_METHOD,
                                              "Unknown method");
    }
}


static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL,
  NULL
};

QtUnityExtraActionHandler::QtUnityExtraActionHandler()
 : m_registration_id(0)
{
    m_introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
}

QtUnityExtraActionHandler::~QtUnityExtraActionHandler()
{
    g_clear_pointer(&m_introspection_data, g_dbus_node_info_unref);
}

bool QtUnityExtraActionHandler::connect(GDBusConnection *connection, const QByteArray &menuPath, UnityGMenuModelExporter *gmenuexporter)
{
    if (m_registration_id != 0) {
        qCWarning(unityappmenu, "Called connect in an already connected QtUnityExtraActionHandler");
        return false;
    }

    GError *error = nullptr;
    m_registration_id = g_dbus_connection_register_object (connection, menuPath.constData(),
                            m_introspection_data->interfaces[0],
                            &interface_vtable,
                            gmenuexporter,
                            nullptr,
                            &error);

    if (!m_registration_id) {
        qCWarning(unityappmenu, "Failed to extra actions - %s", error ? error->message : "unknown error");
        g_clear_error(&error);
    }

    return m_registration_id != 0;
}

void QtUnityExtraActionHandler::disconnect(GDBusConnection *connection) {
    if (m_registration_id) {
        g_dbus_connection_unregister_object (connection, m_registration_id);
    }
}
