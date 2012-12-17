// Copyright © 2012 Canonical Ltd
// FIXME(loicm) Add copyright notice here.

#include "input.h"
#include "integration.h"
#include "base/logging.h"
#include <ubuntu/application/ui/ubuntu_application_ui.h>

QHybrisInput::QHybrisInput(QHybrisIntegration* integration)
    : QHybrisBaseInput(integration, UBUNTU_APPLICATION_UI_INPUT_EVENT_MAX_POINTER_COUNT)
    , sessionType_(0) {
  DLOG("QHybrisInput::QHybrisInput (this=%p integration=%p)", this, integration);
}

QHybrisInput::~QHybrisInput() {
  DLOG("QHybrisInput::~QHybrisInput");
}

void QHybrisInput::handleTouchEvent(
    QWindow* window, ulong timestamp, QTouchDevice* device,
    const QList<struct QWindowSystemInterface::TouchPoint> &points) {
  DLOG("QHybrisInput::handleTouchEvent (this=%p, window=%p, timestamp=%lu, device=%p)",
       this, window, timestamp, device);
  if (sessionType_ != 1) {
    QHybrisBaseInput::handleTouchEvent(window, timestamp, device, points);
  } else {
    // Ubuntu application API creates an input handler per window. Since system sessions have
    // fullscreen input handlers, the last created window has an input handler that takes precedence
    // over the others. Because of that, only the last created window receives touch input. In order
    // to fix that issue for system sessions, we pass the NULL pointer to the Qt handler as window
    // argument so that it pushes the event to the window that's located at the touch point.
    QHybrisBaseInput::handleTouchEvent(NULL, timestamp, device, points);
  }
}

void QHybrisInput::setSessionType(uint sessionType) {
  DLOG("QHybrisInput::setSessionType (this=%p, window=%u)", this, sessionType);
  sessionType_ = sessionType;
}