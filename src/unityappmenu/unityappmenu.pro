TARGET = unityappmenu
TEMPLATE = lib

QT -= gui
QT += core-private theme_support-private dbus

CONFIG += plugin no_keywords

# CONFIG += c++11 # only enables C++0x
QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden -std=c++11 -Werror -Wall
QMAKE_LFLAGS += -std=c++11 -Wl,-no-undefined

CONFIG += link_pkgconfig
PKGCONFIG += gio-2.0

DBUS_INTERFACES += io.unity8.MenuRegistrar.xml

HEADERS += \
    theme.h \
    gmenumodelexporter.h \
    gmenumodelplatformmenu.h \
    logging.h \
    menuregistrar.h \
    registry.h \
    themeplugin.h \
    qtunityextraactionhandler.h \
    ../shared/unitytheme.h

SOURCES += \
    theme.cpp \
    gmenumodelexporter.cpp \
    gmenumodelplatformmenu.cpp \
    menuregistrar.cpp \
    registry.cpp \
    themeplugin.cpp \
    qtunityextraactionhandler.cpp

OTHER_FILES += \
    unityappmenu.json

# Installation path
target.path +=  $$[QT_INSTALL_PLUGINS]/platformthemes

INSTALLS += target
