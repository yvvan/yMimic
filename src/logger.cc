#include "logger.h"

#include <QtGlobal>
#include <QDesktopWidget>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QTimer>

#include <iostream>
#include <sstream>
#include <thread>

#ifdef Q_OS_LINUX
  #include <sys/file.h>
  #include <sys/stat.h>
  #include <sys/time.h>
  #include <linux/input.h>

  #ifndef  EXE_GREP
  #define EXE_GREP "/bin/grep"
  #endif

  #define COMMAND_KBD_DEVICES (EXE_GREP " -E 'Handlers|EV=' /proc/bus/input/devices | " EXE_GREP " -B1 'sysrq' | " EXE_GREP " -Eo 'event[0-9]+' ")
  #define COMMAND_MOUSE_DEVICES (EXE_GREP " -E 'Handlers|EV=' /proc/bus/input/devices | " EXE_GREP " -B1 'mouse' | " EXE_GREP " -Eo 'event[0-9]+' ")
  #define INPUT_EVENT_PATH "/dev/input/"

  #include <X11/Xlib.h>
#elif defined Q_OS_WIN32
  #include <Windows.h>
  #pragma comment(lib, "user32.lib")
#endif

static const unsigned long time_step = 500;//time in msec to save mouse pos in case it is moving
//static unsigned long last_time = 0, last_msec = 0;
static bool last_moving_sign_x = true, last_moving_sign_y = true;//true if "+"

#ifdef Q_OS_LINUX
static int MyErrorHandler(Display *display, XErrorEvent *event) {
  fprintf(stderr, "error occured\n");
  return True;
}
#endif

void Logger::GetMousePos() {
#ifdef Q_OS_LINUX
  int number_of_screens;
  int i;
  Bool result;
  Window *root_windows;
  Window window_returned;
  int root_x, root_y;
  int win_x, win_y;
  unsigned int mask_return;

  Display *display = XOpenDisplay(NULL);
  if (!display)
    return;
  XSetErrorHandler(MyErrorHandler);
  number_of_screens = XScreenCount(display);

  root_windows = static_cast<Window*>(malloc(sizeof(Window) * number_of_screens));
  for (i = 0; i < number_of_screens; i++) {
    root_windows[i] = XRootWindow(display, i);
  }
  for (i = 0; i < number_of_screens; i++) {
    result = XQueryPointer(display, root_windows[i], &window_returned,
            &window_returned, &root_x, &root_y, &win_x, &win_y,
            &mask_return);
    if (result == True) {
      break;
    }
  }
  if (result != True) {
      fprintf(stderr, "No mouse found.\n");
      return;
  }

  global_x_ = root_x;
  global_y_ = root_y;
  free(root_windows);
  XCloseDisplay(display);
#endif
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

Logger* Logger::this_ptr_ = nullptr;

Logger* Logger::Instance() {
  return this_ptr_;
}

Logger::Logger() {
  this_ptr_ = this;
}

static std::chrono::time_point<std::chrono::system_clock> kLastTime;

static void handleKeyPress(int code, int key_value) {
  std::chrono::time_point<std::chrono::system_clock> cur_time;
  cur_time = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = cur_time - kLastTime;
  kLastTime = cur_time;

  auto* logger = Logger::Instance();
  //assert(logger);
  std::lock_guard<std::mutex> lock(logger->GetMutex());
  auto& logfile = logger->logfile();
  logfile << "key ";
  logfile << code << " ";
  if (key_value == 1) {
    logfile << "down";
  } else if (key_value == 0) {
    logfile << "up";
  } else if (key_value == 2) {
    logfile << "rep";
  }
}

//void ReadThread::run() {
//#ifdef Q_OS_LINUX
//  struct input_event event;
//  unsigned int scan_code, prev_code = 0;
//  while (read(event_id_, &event, sizeof(struct input_event)) > 0) {
//    if (event.type != EV_KEY && event.type != EV_REL && event.type != EV_ABS) {
//      continue;
//    }

//    struct timeval event_time;
//    gettimeofday(&event_time, NULL);
//    unsigned long msec = event_time.tv_sec * 1000 + event_time.tv_usec / 1000;
//    unsigned long msec_diff = msec - last_msec;

//    std::mutex& write_mutex = logger_->GetMutex();
//    std::fstream& logfile = logger_->logfile();

//    if (event.type == EV_KEY) {
//      write_mutex.lock();
//      logfile << msec_diff << " ";
//      if (event.code == BTN_LEFT || event.code == BTN_RIGHT ||
//          event.code == BTN_MIDDLE) {
//        logger_->GetMousePos();
//        logfile << "mouse ";
//        if (event.code == BTN_LEFT) {
//          logfile << "l ";
//        } else if (event.code == BTN_RIGHT) {
//          logfile << "r ";
//        } else {
//          logfile << "m ";
//        }
//        if (event.value == 1) {
//          logfile << "down ";
//        } else {
//          logfile << "up ";
//        }
//        logfile << logger_->global_x()<<","<<logger_->global_y();

//        emit DoSavePixel();
//        //after thread is unblocked
//        QRgb rgb = logger_->pixel_value();
//        logfile << " (" << qRed(rgb) << "," << qGreen(rgb) << "," << qBlue(rgb) << ")";
//      } else {
//        logfile << "key ";
//        logfile << event.code << " ";
//        if (event.value == 1) {
//          logfile << "down";
//        } else if (event.value == 0) {
//          logfile << "up";
//        } else if (event.value == 2) {
//          logfile << "rep";
//        }
//      }
//      logfile<<std::endl;
//      last_msec = msec;
//      write_mutex.unlock();
//      continue;
//    }

//    if (event.type == EV_REL && event.code == REL_WHEEL) {
//      logfile << msec_diff << " wheel " << event.value << std::endl;
//      last_msec = msec;
//    }

//    if (!logger_->track_mouse()) {
//      continue;
//    }

//    write_mutex.lock();
//    if (msec > last_time+time_step) {
//      logger_->GetMousePos();
//      logfile << msec_diff << " move " << logger_->global_x() << "," << logger_->global_y() << std::endl;
//      last_time = msec;
//      last_msec = msec;
//      write_mutex.unlock();
//      continue;
//    }
//    write_mutex.unlock();

//    if (event.type == EV_REL || event.type == EV_ABS) {
//      if (event.code == REL_X) {
//        write_mutex.lock();
//        if ((event.value < 0 && last_moving_sign_x) ||
//            (event.value > 0 && !last_moving_sign_x)) {
//          logger_->GetMousePos();
//          logfile << msec_diff << " move " << logger_->global_x() << "," << logger_->global_y() << std::endl;
//          last_moving_sign_x = (event.value > 0);
//          last_time = msec;
//          last_msec = msec;
//        }
//        write_mutex.unlock();
//      } else if (event.code == REL_Y) {
//        write_mutex.lock();
//        if ((event.value < 0 && last_moving_sign_y) ||
//            (event.value > 0 && !last_moving_sign_y)) {
//          logger_->GetMousePos();
//          logfile << msec_diff << " move " << logger_->global_x() << "," << logger_->global_y() << std::endl;
//          last_moving_sign_y = (event.value > 0);
//          last_time = msec;
//          last_msec = msec;
//        }
//        write_mutex.unlock();
//      } else if (event.code == ABS_X || event.code == ABS_Y) {
//        write_mutex.lock();
//        logger_->GetMousePos();
//        logfile << msec_diff << " move " << logger_->global_x() << "," << logger_->global_y() << std::endl;
//        last_moving_sign_x = (event.value > 0);
//        last_time = msec;
//        last_msec = msec;
//        write_mutex.unlock();
//      }
//    }
//  }
//#endif
//}

Logger::~Logger() {
  StopLogging();
  this_ptr_ = nullptr;
}

void Logger::SavePixel() {
  QDesktopWidget* desktop = QApplication::desktop();
  QRect rect = desktop->screenGeometry();
  QPixmap pixmap = QPixmap::grabWindow(desktop->winId(), 0, 0, rect.width(), rect.height());
  pixel_value_ = pixmap.toImage().pixel(global_x_, global_y_);
}

std::mutex& Logger::GetMutex() {
  return write_mutex_;
}

unsigned int Logger::global_x() {
  return global_x_;
}

void Logger::set_global_x(unsigned int val) {
  global_x_ = val;
}

unsigned int Logger::global_y() {
  return global_y_;
}

void Logger::set_global_y(unsigned int val) {
  global_y_ = val;
}

bool Logger::track_mouse() {
  return track_mouse_;
}

std::fstream& Logger::logfile() {
  return logfile_;
}

#ifdef Q_OS_WIN32
static HHOOK kMouseHook, kKeyboardHook;
static LRESULT CALLBACK ProcessMouse(int code, WPARAM w_param, LPARAM l_param) {
  MOUSEHOOKSTRUCT* mouse = (MOUSEHOOKSTRUCT *)l_param;
  if (mouse) {
    if(w_param == WM_LBUTTONDOWN) {
      std::cout << "left mouse pressed" << std::endl;
    }
  }
  return CallNextHookEx(kMouseHook, code, w_param, l_param);
}

static LRESULT CALLBACK ProcessKeyboard(int code, WPARAM w_param, LPARAM l_param) {
  KBDLLHOOKSTRUCT* keyboard = (KBDLLHOOKSTRUCT *)l_param;
  if (keyboard) {
    if(w_param == WM_KEYDOWN) {
      std::cout << "key is down" << std::endl;
    }
  }
  return CallNextHookEx(kKeyboardHook, code, w_param, l_param);
}
#endif

void Logger::StartLogging() {
  logfile_.open("lastlog.log", std::fstream::out);
#ifdef Q_OS_LINUX
  std::stringstream kbd_output(Execute(COMMAND_KBD_DEVICES));
  std::string line;
  std::vector<int> devices;

  while(std::getline(kbd_output, line)) {
    int input_fd = open((std::string(INPUT_EVENT_PATH) + line).c_str(), O_RDONLY);
    if (input_fd == -1) {
      continue;
    }
    devices.push_back(input_fd);
  }

  std::stringstream mouse_output(Execute(COMMAND_MOUSE_DEVICES));
  while(std::getline(mouse_output, line)) {
    int input_fd = open((std::string(INPUT_EVENT_PATH) + line).c_str(), O_RDONLY);
    if (input_fd == -1) {
      continue;
    }
    devices.push_back(input_fd);
  }

  if (devices.empty()) {
    return;
  }

  GetMousePos();
  struct timeval start;
  gettimeofday(&start, NULL);
  last_msec = start.tv_sec * 1000 + start.tv_usec / 1000 ;

  std::vector<QThread*> threads;
  for (int i=0; i<devices.size(); i++) {
    ReadThread* thread = new ReadThread(this, devices[i]);
    threads.push_back(thread);
    QObject::connect(threads[i], SIGNAL(DoSavePixel()), this, SLOT(SavePixel()), Qt::BlockingQueuedConnection);
    QObject::connect(threads[i], SIGNAL(finished()), threads[i], SLOT(deleteLater()));
  }

  for (auto& th : threads) {
    th->start();
  }
#elif defined Q_OS_WIN32
  HINSTANCE instance = GetModuleHandle(NULL);
  kMouseHook = SetWindowsHookEx(WH_MOUSE_LL, ProcessMouse, instance, 0);
  kKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, ProcessKeyboard, instance, 0);
#endif
}

void Logger::StopLogging() {
#ifdef Q_OS_LINUX

#elif defined Q_OS_WIN32
  UnhookWindowsHookEx(kMouseHook);
  UnhookWindowsHookEx(kKeyboardHook);
#endif
}
