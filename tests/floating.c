#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define	TRIALS	10000000

int random_depth() {
	return rand() % 2048;
}

float raw_depth_to_meters(int raw_depth) {
	return 1.0 / (raw_depth * -0.0030711016 + 3.3309495161);
}

int main(void) {
	size_t i;
	float val;

	//feed the beast
	unsigned int iseed = (unsigned int) time(NULL);
	srand(iseed);

	for (i = 0; i < TRIALS; i++) {
		val = raw_depth_to_meters(random_depth());	
	}	
}
