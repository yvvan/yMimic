#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "logger.h"
#include "repeater.h"

#include <QMainWindow>
#include <memory>

namespace Ui {
class MainWindow;
}

struct _PROCESS_INFORMATION;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
protected:
  virtual void closeEvent(QCloseEvent *event);

private slots:
  void on_actionOpen_triggered();

  void on_tabWidget_tabCloseRequested(int index);

  void on_actionRepeat_triggered();

  void on_actionStartLogging_triggered();

  void on_actionTrackMouse_toggled(bool arg1);

  void on_actionSave_triggered();

private:
#ifdef Q_OS_LINUX
  void initLinux();
#elif defined Q_OS_WIN32
  void initWin();
#endif
  void fillEditor();

  Ui::MainWindow *ui;
  bool track_mouse_;

  bool logStarted_;

  std::vector<std::string> kbd_events_, mouse_events_;

  //std::unique_ptr<_PROCESS_INFORMATION> pi_;
  Logger logger_;
  Repeater repeater_;
};

#endif // MAINWINDOW_H
