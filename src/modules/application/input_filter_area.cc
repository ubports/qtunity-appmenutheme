// This file is part of QtUbuntu, a set of Qt components for Ubuntu.
// Copyright © 2013 Canonical Ltd.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; version 3.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "input_filter_area.h"
#include "logging.h"
#include <QDebug>
#include <ubuntu/ui/ubuntu_ui_session_service.h>

InputFilterArea::InputFilterArea(QQuickItem* parent)
    : QQuickItem(parent)
    , blockInput_(false)
    , trapHandle_(0) {
  DLOG("InputFilterArea::InputFilterArea (this=%p, parent=%p)", this, parent);
}

InputFilterArea::~InputFilterArea() {
  DLOG("InputFilterArea::~InputFilterArea");
  disableInputTrap();
}

void InputFilterArea::setBlockInput(bool blockInput) {
  DLOG("InputFilterArea::setBlockInput (this=%p, blockInput=%d)", this, blockInput);
  if (blockInput_ != blockInput) {
    blockInput_ = blockInput;
    if (blockInput) {
      enableInputTrap();
    } else {
      disableInputTrap();
    }
    emit blockInputChanged();
  }
}

void InputFilterArea::geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry) {
  DLOG("InputFilterArea::geometryChanged (this=%p)", this);
  qDebug() << newGeometry;
  if (newGeometry != oldGeometry) {
    geometry_ = newGeometry;
    if (blockInput_) {
      setInputTrap(relativeToAbsoluteGeometry(geometry_));
    }
  }
  QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void InputFilterArea::onAscendantChanged() {
  DLOG("InputFilterArea::onAscendantChanged (this=%p)", this);
  listenToAscendantsChanges();
  setInputTrap(relativeToAbsoluteGeometry(geometry_));
}

void InputFilterArea::onAscendantGeometryChanged() {
  DLOG("InputFilterArea::onAscendantGeometryChanged (this=%p)", this);
  setInputTrap(relativeToAbsoluteGeometry(geometry_));
}

void InputFilterArea::listenToAscendantsChanges() {
  DLOG("InputFilterArea::listenToAscendantsChanges (this=%p)", this);

  disconnectFromAscendantsChanges();

  // listen to geometry changes and parent changes on all the ascendants
  connections_.append(connect(this, &QQuickItem::parentChanged, this, &InputFilterArea::onAscendantChanged));
  QQuickItem* parent = parentItem();
  while (parent != NULL) {
    connections_.append(connect(parent, &QQuickItem::parentChanged, this, &InputFilterArea::onAscendantChanged));
    connections_.append(connect(parent, &QQuickItem::xChanged, this, &InputFilterArea::onAscendantGeometryChanged));
    connections_.append(connect(parent, &QQuickItem::yChanged, this, &InputFilterArea::onAscendantGeometryChanged));
    connections_.append(connect(parent, &QQuickItem::widthChanged, this, &InputFilterArea::onAscendantGeometryChanged));
    connections_.append(connect(parent, &QQuickItem::heightChanged, this, &InputFilterArea::onAscendantGeometryChanged));
    parent = parent->parentItem();
  }
}

void InputFilterArea::disconnectFromAscendantsChanges() {
  DLOG("InputFilterArea::disconnectFromAscendantsChanges (this=%p)", this);
  // disconnect all previously connected signals
  Q_FOREACH (QMetaObject::Connection connection, connections_) {
    disconnect(connection);
  }
  connections_.clear();
}

void InputFilterArea::setInputTrap(const QRect & geometry) {
  DLOG("InputFilterArea::setInputTrap (this=%p)", this);
  qDebug() << geometry;

  if (geometry != trapGeometry_) {
    trapGeometry_ = geometry;
    if (trapHandle_ != 0) {
      ubuntu_ui_unset_surface_trap(trapHandle_);
      trapHandle_ = 0;
    }
    if (geometry.isValid()) {
      trapHandle_ = ubuntu_ui_set_surface_trap(geometry.x(), geometry.y(), geometry.width(), geometry.height());
    }
  }
}

void InputFilterArea::enableInputTrap() {
  DLOG("InputFilterArea::enableInputTrap (this=%p)", this);
  setInputTrap(relativeToAbsoluteGeometry(geometry_));
  listenToAscendantsChanges();
  connect(this, &QQuickItem::parentChanged, this, &InputFilterArea::onAscendantChanged);
}

void InputFilterArea::disableInputTrap() {
  DLOG("InputFilterArea::disableInputTrap (this=%p)", this);
  if (trapHandle_ != 0) {
    ubuntu_ui_unset_surface_trap(trapHandle_);
    trapHandle_ = 0;
  }
  trapGeometry_ = QRect();
  disconnectFromAscendantsChanges();
}

QRect InputFilterArea::relativeToAbsoluteGeometry(QRectF relativeGeometry) {
  if (parentItem()) {
    return parentItem()->mapRectToScene(relativeGeometry).toRect();
  } else {
    return relativeGeometry.toRect();
  }
}
