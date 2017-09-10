#ifndef REPEATER_H
#define REPEATER_H

#include <QWidget>

#include <map>

class Repeater {
public:
  Repeater();

  void StartRepeating();

private:
  void WriteThread(const std::vector<std::string>& lines);
  unsigned long GetPixel(int x, int y);
  int GetIntById(QString string_id);

  std::map<QString, int> vars_;
  std::map<QString, int> countable_;
};

#endif // REPEATER_H
