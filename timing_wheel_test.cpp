#include <stdio.h>
#include <iostream>
#include <string>
#include <chrono>
#include <exception>
#include "timing_wheel.hpp"
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

class tester {
private:
  high_resolution_clock::time_point g_lstTimeOne;
  high_resolution_clock::time_point g_lstTimeTwo;
  std::function<void(const std::string &)> m_funcFirst;
  std::function<void(const std::string &)> m_funcSecond;
  TimingWheel *tw;

  void firstCallback(const std::string &reqId) {
    auto nowTime = high_resolution_clock::now();
    auto time_span = duration_cast<duration<double>>(nowTime - g_lstTimeOne);
    g_lstTimeOne = nowTime;
    std::cout << reqId.c_str() << " took me " << time_span.count()
              << "seconds..." << std::endl;
  }
  void secondCallback(const std::string &reqId) {
    auto nowTime = high_resolution_clock::now();
    auto time_span = duration_cast<duration<double>>(nowTime - g_lstTimeTwo);
    g_lstTimeTwo = nowTime;
    std::cout << reqId.c_str() << " took me " << time_span.count()
              << "seconds..." << std::endl;
  }

public:
  tester() { tw = new TimingWheel(10, 100); }
  ~tester() {
    if (tw) {
      delete tw;
      tw = nullptr;
    }
  }

  void doTest1() {
    tw->AddTimer("first", true, 100, std::bind(&tester::firstCallback, this,
                                               std::placeholders::_1));
    tw->Run();

    std::this_thread::sleep_for(std::chrono::seconds(4));
    tw->AddTimer("second", true, 400, std::bind(&tester::secondCallback, this,
                                                std::placeholders::_1));
    std::this_thread::sleep_for(std::chrono::seconds(8));
    tw->DelTimer("first");
    std::this_thread::sleep_for(std::chrono::seconds(8));
    tw->DelTimer("second");
  }
};

int main(int argc, char *argv[]) {
  unsigned long count = 0;
  try {
    std::string string_count(argv[1]);
    count = std::stoul(string_count);
  } catch (std::exception &e) {
    std::cerr << "Caught: " << e.what() << std::endl;
    std::cerr << "Type: " << typeid(e).name() << std::endl;
    std::cerr << "please input correct arv[1]" << std::endl;
  }
  std::cout << "count:" << count << std::endl;

  do {
    tester ter;
    ter.doTest1();
  } while (count--);

  return 0;
}
