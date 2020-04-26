#include <stdio.h>
#include <iostream>
#include <string>
#include <chrono>
#include <exception>
#include "timing_wheel.h"
using std::chrono::high_resolution_clock;
using std::chrono::duration;
using std::chrono::duration_cast;

high_resolution_clock::time_point g_lstTimeOne;
high_resolution_clock::time_point g_lstTimeTwo;

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
    int i = 0;
    int j = 1;

    TimingWheel *tw = new TimingWheel(10, 100);

    g_lstTimeOne = high_resolution_clock::now();
    tw->AddTimer("first", true, 1000, [i](const std::string &reqId) {
      auto nowTime = high_resolution_clock::now();
      auto time_span = duration_cast<duration<double>>(nowTime - g_lstTimeOne);
      g_lstTimeOne = nowTime;
      std::cout << reqId.c_str() << " took me " << time_span.count()
                << "seconds..." << std::endl;
    });

    tw->Run();
    std::this_thread::sleep_for(std::chrono::seconds(4));

    g_lstTimeTwo = high_resolution_clock::now();
    tw->AddTimer("second", true, 4000, [j](const std::string &reqId) {
      auto nowTime = high_resolution_clock::now();
      auto time_span = duration_cast<duration<double>>(nowTime - g_lstTimeTwo);
      g_lstTimeTwo = nowTime;
      std::cout << reqId.c_str() << " took me " << time_span.count()
                << "seconds..." << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(8));

    tw->DelTimer("first");
    std::this_thread::sleep_for(std::chrono::seconds(8));
    tw->DelTimer("second");

    if (tw) {
      delete tw;
      tw = nullptr;
    }
  } while (count--);

  return 0;
}