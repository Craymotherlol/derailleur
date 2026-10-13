#include "DrGuestGcn.h"

DrGuestGcn::DrGuestGcn(const QString &subdir, const std::string &disc, QObject *parent)
  : DrGuestDolphin(new DrRetroGcn(subdir), disc, false, parent)
{
  /**
   * Nothing yet!
   * @todo GC Mic stuff will probably go in here
   */
}
