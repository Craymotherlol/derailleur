#ifndef DR_HOST_PICKER_H
#define DR_HOST_PICKER_H

#include "DrCommon.h"
#include <functional>
#include <QScrollArea>
#include <QStringList>

class QGridLayout;
class QToolButton;
struct DrHostEntry;

/// "Start Game" page: a title-screen button per host and per custom ROM.
class DrHostPicker : public QScrollArea
{
  Q_OBJECT

public:
  DrHostPicker(QWidget *parent = nullptr);

signals:
  /// `rom` is the content to boot (empty = stock); `customRom` its basename
  /// when it is a custom ROM, else empty.
  void hostChosen(const DrHostEntry *entry, const QString &rom, const QString &customRom);

private:
  void addButton(const QIcon &icon, const QString &name, const QString &rom, dr_core core,
    std::function<void()> start);

  QWidget *m_page = nullptr;
  QGridLayout *m_grid = nullptr;
  int m_count = 0;
};

#endif
