#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <csignal>
#include <thread>

#include <QCheckBox>
#include <QWidgetAction>

#include <QPlainTextEdit>
#include <QFileDialog>

#ifdef Q_OS_LINUX
  #include <sys/file.h>
  #include <sys/stat.h>
  #include <linux/input.h>

  #ifndef  EXE_PS
  #define EXE_PS "/bin/ps"
  #endif
  #ifndef  EXE_GREP
  #define EXE_GREP "/bin/grep"
  #endif

  #define COMMAND_STR_GET_LOGGER_PID  ( (std::string(EXE_PS " ax | " EXE_GREP " '") + "sudo ./logger" + "' | " EXE_GREP " -v grep").c_str() )
  #define COMMAND_STR_GET_REPEATER_PID  ( (std::string(EXE_PS " ax | " EXE_GREP " '") + "sudo ./repeat" + "' | " EXE_GREP " -v grep").c_str() )

  #define COMMAND_KBD_DEVICES (EXE_GREP " -E 'Handlers|EV=' /proc/bus/input/devices | " EXE_GREP " -B1 'sysrq' | " EXE_GREP " -Eo 'event[0-9]+' ")
  #define COMMAND_KBD_NAMES (EXE_GREP " -E 'Handlers|Name=' /proc/bus/input/devices | " EXE_GREP " -B1 'sysrq' | " EXE_GREP " -Eo '\".*\"' ")
  #define COMMAND_MOUSE_DEVICES (EXE_GREP " -E 'Handlers|EV=' /proc/bus/input/devices | " EXE_GREP " -B1 'mouse' | " EXE_GREP " -Eo 'event[0-9]+' ")
  #define COMMAND_MOUSE_NAMES (EXE_GREP " -E 'Handlers|Name=' /proc/bus/input/devices | " EXE_GREP " -B1 'mouse' | " EXE_GREP " -Eo '\".*\"' ")
  #define INPUT_EVENT_PATH "/dev/input/"
#elif defined Q_OS_WIN32
  #include <Windows.h>
#endif

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

#ifdef Q_OS_LINUX
void MainWindow::initLinux() {
  std::stringstream kbd_output(Execute(COMMAND_KBD_DEVICES));
  std::stringstream kbd_names(Execute(COMMAND_KBD_NAMES));
  std::string line;

  while(std::getline(kbd_output, line)) {
    kbd_events_.push_back(line);

    std::getline(kbd_names, line);
    ui->comboKeyboard->addItem(QString(line.c_str()));
  }

  std::stringstream mouse_output(Execute(COMMAND_MOUSE_DEVICES));
  std::stringstream mouse_names(Execute(COMMAND_MOUSE_NAMES));
  while(std::getline(mouse_output, line)) {
    mouse_events_.push_back(line);

    std::getline(mouse_names, line);
    ui->comboMouse->addItem(QString(line.c_str()));
  }
}
#elif defined Q_OS_WIN32
void MainWindow::initWin() {

}
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    logStarted_(false) {
  ui->setupUi(this);

#ifdef Q_OS_LINUX
  initLinux();
#elif defined Q_OS_WIN32
  initWin();
#endif
}

MainWindow::~MainWindow() {
//  if (pi_) {
//    TerminateProcess(pi_->hProcess, 0);
//    CloseHandle(pi_->hProcess);
//    CloseHandle(pi_->hThread);
//  }
  delete ui;
}

void MainWindow::fillEditor() {
  std::fstream logfile;
  logfile.open("lastlog.log", std::fstream::in);
  std::string line;
  std::stringstream whole_text;
  while(logfile){
    std::getline(logfile, line);
    whole_text << line << std::endl;
  }
  QPlainTextEdit* text_edit = new QPlainTextEdit(whole_text.str().c_str());
  int new_tab = ui->tabWidget->addTab(text_edit, "New*");
  ui->tabWidget->setCurrentIndex(new_tab);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (logStarted_)
    on_actionStartLogging_triggered();
  event->accept();
}

void MainWindow::on_actionOpen_triggered() {
  std::string file_name = QFileDialog::getOpenFileName(this,
     tr("Open Log"), "", tr("Log Files (*.log)")).toStdString();
  if (file_name.empty()) {
    return;
  }

  std::fstream logfile;
  logfile.open(file_name, std::fstream::in);
  std::string line;
  std::stringstream whole_text;
  while(logfile){
    std::getline(logfile, line);
    whole_text << line << std::endl;
  }
  QPlainTextEdit* text_edit = new QPlainTextEdit(whole_text.str().c_str());
  file_name = file_name.substr(file_name.find_last_of("/\\")+1);
  file_name = file_name.substr(0, file_name.length()-4);
  int new_tab = ui->tabWidget->addTab(text_edit, file_name.c_str());
  ui->tabWidget->setCurrentIndex(new_tab);
}

void MainWindow::on_tabWidget_tabCloseRequested(int index) {
  ui->tabWidget->removeTab(index);
}

void MainWindow::on_actionRepeat_triggered() {
  QPlainTextEdit* text_edit = static_cast<QPlainTextEdit*>(
    ui->tabWidget->currentWidget());

  if (!text_edit) {
    return;
  }

  std::fstream logfile, devicefile;
  logfile.open("lastlog.log", std::fstream::out);
  logfile << text_edit->toPlainText().toStdString();
  logfile.close();

#ifdef Q_OS_LINUX
  devicefile.open("device.config", std::fstream::out);
  devicefile << kbd_events_[ui->comboKeyboard->currentIndex()] << std::endl
    << mouse_events_[ui->comboMouse->currentIndex()] << std::endl;
  devicefile.close();

  std::stringstream command_line;
  command_line << "sudo ./repeater/repeat " << " &";
  std::system(command_line.str().c_str());
#endif
}

void MainWindow::on_actionStartLogging_triggered() {
  if (!logStarted_) {
#ifdef Q_OS_LINUX
    pid_t pid;
    if (sscanf(Execute(COMMAND_STR_GET_LOGGER_PID).c_str(), "%d", &pid) == 1)
      kill(pid, SIGINT);

    std::stringstream command_line;
    command_line << "sudo ./logger/logger " << track_mouse_ << " &";
    std::system(command_line.str().c_str());

    ui->actionStartLogging->setText("Stop logging");
    logStarted_ = true;
#elif defined Q_OS_WIN32
    logger_.StartLogging();
//  STARTUPINFO si;
//  pi_ = std::make_unique<_PROCESS_INFORMATION>();
//  // set the size of the structures
//  ZeroMemory( &si, sizeof(si) );
//  si.cb = sizeof(si);
//  ZeroMemory(pi_.get(), sizeof(*pi_));

// // start the program up
// CreateProcess(L"D:\\Projects\\build-ActionRepeaterQt-Desktop-Debug\\logger\\debug\\logger.exe",   // the path
//   nullptr,        // Command line
//   nullptr,           // Process handle not inheritable
//   nullptr,           // Thread handle not inheritable
//   false,          // Set handle inheritance to FALSE
//   0,              // No creation flags
//   nullptr,           // Use parent's environment block
//   nullptr,           // Use parent's starting directory
//   &si,            // Pointer to STARTUPINFO structure
//   pi_.get()             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
//   );
#endif
  } else {
#ifdef Q_OS_LINUX
    pid_t pid;
    if (sscanf(Execute(COMMAND_STR_GET_LOGGER_PID).c_str(), "%d", &pid) != 1) {
      ui->actionStartLogging->setText("Start logging");
      logStarted_ = false;
      return;
    }

    kill(pid, SIGINT);
    ui->actionStartLogging->setText("Start logging");
    logStarted_ = false;
#endif
    fillEditor();
  }
}

void MainWindow::on_actionTrackMouse_toggled(bool state) {
  track_mouse_ = (state > 0);
}

void MainWindow::on_actionSave_triggered() {
  QWidget* pageWidget = ui->tabWidget->widget(ui->tabWidget->currentIndex());
  auto page = static_cast<QPlainTextEdit*>(pageWidget);
  if (!page)
    return;

  std::string file_name = QFileDialog::getSaveFileName(this,
     tr("Save Log"), "", tr("Log Files (*.log)")).toStdString();
  if (file_name.empty()) {
    return;
  }

  std::fstream logfile;
  logfile.open(file_name, std::fstream::out);

  logfile << page->toPlainText().toStdString();
  logfile.close();
}
