#include <stdio.h>
#include <unistd.h>
#include "timing_wheel.h"

int main(int argc,char *argv[]) {
	timing_wheel *tw = new timing_wheel();

	for(int i=0; i < 100000; i++) {
		tw->add_timer(5, [i] {
			printf("i: %d\n", i);
		});
	}

	while(1) {
		sleep(1);
		printf("\n");
		tw->tick();
	}

	return 0;
}