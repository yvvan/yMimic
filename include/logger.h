#ifndef LOGGER_H
#define LOGGER_H

#include <QWidget>
#include <QThread>

#include <mutex>
#include <fstream>
#include <chrono>

class Logger {
public:
  Logger();
  ~Logger();
  unsigned long pixel_value() const {
    return pixel_value_;
  }

  void GetMousePos();

  std::mutex& GetMutex();

  unsigned int global_x();
  void set_global_x(unsigned int val);

  unsigned int global_y();
  void set_global_y(unsigned int val);

  bool track_mouse();
  std::fstream& logfile();

  void StartLogging();
  void StopLogging();
  void SavePixel();

  static Logger* Instance();
private:
  static Logger* this_ptr_;

  unsigned long pixel_value_;
  std::mutex write_mutex_;
  unsigned int global_x_, global_y_;
  bool track_mouse_;
  std::fstream logfile_;
};

//class ReadThread : public QThread {
//Q_OBJECT
//  friend class Logger;
//public:
//  ReadThread(Logger* logger, int event_id) {
//    logger_ = logger;
//    event_id_ = event_id;
//  }

//  virtual void run();

//signals:
//  void DoSavePixel();

//private:
//  Logger* logger_;
//  int event_id_;
//};

#endif // LOGGER_H
