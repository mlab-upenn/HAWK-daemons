#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define	TRIALS	10000000

float t_gamma[2048];
const float k1 = 1.1863;
const float k2 = 2842.5;
const float k3 = 0.1236;

int random_depth() {
	return rand() % 2048;
}

int main(void) {
	size_t i;
	float val;

	//feed the beast
	unsigned int iseed = (unsigned int) time(NULL);
	srand(iseed);

	for (i = 0; i < 2048; i++) {
		const float depth = k3 * tanf(i/k2 + k1);
		t_gamma[i] = depth;	
	}
	
	for (i = 0; i < TRIALS; i++) {
		val = t_gamma[random_depth()];	
	}	
}
