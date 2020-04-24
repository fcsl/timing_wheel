#include <stdio.h>
#include <unistd.h>
#include "timing_wheel.h"

int main(int argc, char *argv[]) {
  timing_wheel *tw = new timing_wheel(10);

  int i = 0;
  int j = 1;
  tw->add_timer("1", 1000,
                [i](const std::string &reqId) { printf("i: %d\n", i); });
  tw->add_timer("2", 4000,
                [j](const std::string &reqId) { printf("j: %d\n", j); });

  tw->run();

  sleep(10000);

  return 0;
  
}