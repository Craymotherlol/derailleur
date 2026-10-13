#include "DrHostRetro.h"

#include <QRetro.h>
#include <memory>

/* Frames the core runs before it is fitted to its widget */
#define DR_HOST_RESIZE_FRAMES 10

void DrHostRetro::setRetro(DrRetro *retro)
{
  m_retro = retro;
  if (m_retro)
    connect(m_retro, &DrRetro::logMessage, this, &DrHost::logMessage);
}

void DrHostRetro::startCore()
{
  QRetro *c = core();

  if (!c)
    return;

  auto conn = std::make_shared<QMetaObject::Connection>();
  *conn = connect(c, &QRetro::frameBegin, this, [this, c, conn]() {
    if (c->frames() < DR_HOST_RESIZE_FRAMES)
      return;
    QObject::disconnect(*conn);
    QMetaObject::invokeMethod(m_retro, [this]() { m_retro->resizeToContainer(); },
      Qt::QueuedConnection);
  }, Qt::DirectConnection);

  m_retro->startCore();
}
