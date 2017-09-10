#include <QtGlobal>
#include <QStringList>
#include <QtTest/QTest>
#include <QDesktopWidget>
#include <QSystemTrayIcon>
#include <QTimer>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include "repeater.h"

#ifdef Q_OS_LINUX
  #include <sys/file.h>
  #include <sys/stat.h>
  #include <sys/time.h>
  #include <linux/input.h>

  //mouse is moved with x events
  #include <X11/Xlib.h>
  #include <X11/X.h>
  #include <X11/Xutil.h>

  #ifndef  EXE_GREP
  #define EXE_GREP "/bin/grep"
  #endif

  #define COMMAND_KBD_DEVICES (EXE_GREP " -E 'Handlers|EV=' /proc/bus/input/devices | " EXE_GREP " -B1 'sysrq' | " EXE_GREP " -Eo 'event[0-9]+' ")
  #define COMMAND_MOUSE_DEVICES (EXE_GREP " -E 'Handlers|EV=' /proc/bus/input/devices | " EXE_GREP " -B1 'mouse' | " EXE_GREP " -Eo 'event[0-9]+' ")
  #define INPUT_EVENT_PATH "/dev/input/"

  #include <X11/Xlib.h>
#endif

#ifdef Q_OS_LINUX
int kbd_event_id, mouse_event_id;
#endif

static void MoveCursor(int x, int y) {
#ifdef Q_OS_LINUX
  Display *dpy;
  Window root_window;

  dpy = XOpenDisplay(0);
  root_window = XRootWindow(dpy, 0);
  XSelectInput(dpy, root_window, KeyReleaseMask);
  XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, x, y);
  XFlush(dpy);
  XCloseDisplay(dpy);
#endif
}

static void PutKey(const int code, const QString& direction) {
#ifdef Q_OS_LINUX
  struct input_event event_end;
  event_end.type = EV_SYN;
  event_end.code = SYN_REPORT;
  event_end.value = 0;

  struct input_event event;
  event.type = EV_KEY;
  event.code = code;
  if (direction == "up") {
    event.value = 0;
  } else if (direction == "down") {
    event.value = 1;
  } else {
    event.value = 2;
  }

  write(kbd_event_id, &event, sizeof(struct input_event));
  write(kbd_event_id, &event_end, sizeof(struct input_event));
#endif
}

static void PutMouse(const QString& btn, const QString& direction) {
#ifdef Q_OS_LINUX
  struct input_event event_end;
  event_end.type = EV_SYN;
  event_end.code = SYN_REPORT;
  event_end.value = 0;

  struct input_event event;
  event.type = EV_KEY;
  if (btn == "l") {
    event.code = BTN_LEFT;
  } else if (btn == "r") {
    event.code = BTN_RIGHT;
  } else {
    event.code = BTN_MIDDLE;
  }
  if (direction == "up") {
    event.value = 0;
  } else if (direction == "down") {
    event.value = 1;
  } else {
    event.value = 2;
  }
  write(mouse_event_id, &event, sizeof(struct input_event));
  write(mouse_event_id, &event_end, sizeof(struct input_event));
#endif
}

static void Print(short one_digit) {
  int code(0);
  if (one_digit == 0) {
    code = Qt::Key_0;
  } else {
    code = Qt::Key_1 + one_digit - 1;
  }

  PutKey(code, "down");
  PutKey(code, "up");
}

int Repeater::GetIntById(QString string_id) {
  if (vars_.find(string_id) != vars_.end()) {
    return vars_[string_id];
  } else if (countable_.find(string_id) != countable_.end()) {
    return countable_[string_id];
  }

  return string_id.toInt();
}

static std::string Execute(const char* cmd) {
#ifdef Q_OS_LINUX
  FILE* pipe = popen(cmd, "r");
  if (!pipe) {
    std::cout <<"error" <<std::endl;
    return "";
  }
  char buffer[128];
  std::string result = "";
  while(!feof(pipe)) {
    if(fgets(buffer, 128, pipe) != NULL) {
      result += buffer;
    }
  }
  pclose(pipe);
  return result;
#endif
  return "";
}

unsigned long Repeater::GetPixel(int x, int y) {
  QDesktopWidget* desktop = QApplication::desktop();
  QRect rect = desktop->screenGeometry();
  QPixmap pixmap = QPixmap::grabWindow(desktop->winId(), 0, 0, rect.width(), rect.height());
  return pixmap.toImage().pixel(x, y);
}

void Repeater::WriteThread(const std::vector<std::string>& lines) {
  for (auto it=lines.begin(); it != lines.end(); ++it) {
    QString qline(it->c_str());
    QStringList qlist = qline.split(" ");
    if(qlist.size() < 2) {
      continue;
    }

    unsigned long msec = qlist.at(0).toULong();
    if (msec <= 0) {
      msec = 1;
    }
    QTest::qSleep(msec);

    if (qlist.at(1) == "key") {
      PutKey(qlist.at(2).toInt(), qlist.at(3));
    } else if (qlist.at(1) == "mouse" || qlist.at(1) == "move") {
      QStringList coords;
      if (qlist.at(1) == "move") {
        coords = qlist.at(2).split(",");
      } else {
        coords = qlist.at(4).split(",");
      }
      MoveCursor(coords.at(0).toInt(), coords.at(1).toInt());

      if (qlist.at(1) == "mouse") {
        PutMouse(qlist.at(2), qlist.at(3));
      }
    } else if(qlist.at(1) == "var") {
      vars_[qlist.at(2)] = 0;
      if (qlist.size() > 4) {
        vars_[qlist.at(2)] = qlist.at(4).toInt();
      }
    } else if(qlist.at(1) == "op") {
      if (vars_.find(qlist.at(2)) == vars_.end() && countable_.find(qlist.at(2)) == countable_.end()) {
        continue;
      }
      int result(0), second(0);
      result = GetIntById(qlist.at(4));

      if (qlist.size() > 6) {
        second = GetIntById(qlist.at(6));

        if (qlist.at(5) == "+") {
          result += second;
        } else if (qlist.at(5) == "-") {
          result -= second;
        } else if (qlist.at(5) == "*") {
          result *= second;
        } else if (qlist.at(5) == "/") {
          result /= second;
        }
      }
      if (vars_.find(qlist.at(2)) != vars_.end()) {
        vars_[qlist.at(2)] = result;
      } else {
        countable_[qlist.at(2)] = result;
      }
    } else if(qlist.at(1) == "for") {
      std::vector<std::string> for_block;
      int count = 1;
      while (count > 0) {
        ++it;
        if (it->find("for") != std::string::npos) {
          if (it->find("endfor") == std::string::npos) {
            count++;
          } else {
            count--;
          }
        }
        if (count > 0) {
          for_block.push_back(*it);
        }
      }
      for (countable_[qlist.at(2)] = GetIntById(qlist.at(4));
          countable_[qlist.at(2)] <= GetIntById(qlist.at(6)); countable_[qlist.at(2)]++) {
        WriteThread(for_block);
      }
      //remove countable
      countable_.erase(qlist.at(2));
    } else if(qlist.at(1) == "if") {
      std::vector<std::string> if_block;
      int count = 1;
      while (count > 0) {
        ++it;
        if (it->find("if") != std::string::npos) {
          if (it->find("endif") == std::string::npos) {
            count++;
          } else {
            count--;
          }
        }
        if (count > 0) {
          if_block.push_back(*it);
        }
      }
      int a = GetIntById(qlist.at(2)), b = GetIntById(qlist.at(4));
      if ((qlist.at(3) == "=" && a == b) ||
          (qlist.at(3) == "<" && a < b) ||
          (qlist.at(3) == ">" && a > b)) {
        WriteThread(if_block);
      }
    } else if(qlist.at(1) == "print") {
      QString num = QString::number(GetIntById(qlist.at(2)));
      for (int i=0; i<num.length(); i++) {
        Print(QString(num.at(i)).toShort());
      }
    } else if(qlist.at(1) == "pixel") {
      int x = GetIntById(qlist.at(3));
      int y = GetIntById(qlist.at(4));
      vars_[qlist.at(2)] = GetPixel(x, y);
    }
  }

  QCoreApplication::instance()->exit();
}

void Repeater::StartRepeating() {
  std::fstream logfile;
  logfile.open("lastlog.log", std::fstream::in);

#ifdef Q_OS_LINUX
  std::fstream devicefile;
  devicefile.open("device.config", std::fstream::in);
  std::string kbd, mouse;
  getline(devicefile, kbd);
  getline(devicefile, mouse);
  devicefile.close();

  kbd_event_id = open((std::string(INPUT_EVENT_PATH) + kbd).c_str(), O_WRONLY);
  mouse_event_id = open((std::string(INPUT_EVENT_PATH) + mouse).c_str(), O_WRONLY);

  std::vector<std::string> lines;
  while (!logfile.eof()) {
    std::string line;
    getline(logfile, line);
    lines.push_back(line);
  }
  WriteThread(lines);

  ::close(kbd_event_id);
  ::close(mouse_event_id);
#elif defined Q_OS_WIN32

#endif
}

Repeater::Repeater() {
//  QSystemTrayIcon icon(QIcon("tray_icon.jpg"));
//  icon.show();

//  QTimer::singleShot(1000, this, SLOT(StartRepeating()));
}
