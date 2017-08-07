#include <stdio.h>
#include <unistd.h>
#include "timing_wheel.h"

int main(int argc,char *argv[]) {
	timing_wheel *tw = new timing_wheel(5);

	for(int i=0; i < 100000; i++) {
		tw->add_timer("1", [i] (const std::string& reqId){
			printf("i: %d\n", i);
		});
	}

	tw->run();

	sleep(10);

	return 0;
}