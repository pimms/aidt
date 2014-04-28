#include <stdlib.h>
#include <stdio.h>

#include "sample.h"
#include "dtree.h"


int main(int argc, char **argv) {
	// Sample data
	const int num_samples = 40;
	struct sample samples[] = {
		{ASTAR, true, 0, true},		// 1
		{ASTAR, true, 0, true},		// 2
		{ASTAR, false, 0, true},	// 3
		{ASTAR, false, 10, true},	// 4
		{DTREE, false, 0, true},	// 5
		{ASTAR, false, 0, true},	// 6
		{DTREE, true, 5, true},		// 7
		{DTREE, true, 15, false},	// 8
		{GAMES, true, 20, true},	// 9
		{GAMES, true, 20, true},	// 10
		{GAMES, true, 20, true},	// 11
		{GAMES, false, 20, true},	// 12
		{GAMES, false, 20, true},	// 13
		{GAMES, true, 5, false},	// 14
		{GAMES, true, 5, false},	// 15
		{GAMES, true, 10, true},	// 16
		{ASTAR, true, 5, false},	// 17
		{DTREE, true, 0, false},	// 18
		{DTREE, true, 0, false},	// 19
		{DTREE, false, 5, true},	// 20
		{GAMES, false, 5, false},	// 21
		{ASTAR, false, 5, true},	// 22
		{GAMES, false, 10, false},	// 23
		{GAMES, false, 20, true},	// 24
		{GAMES, true, 20, true},	// 25
		{DTREE, false, 20, true},	// 26
		{ASTAR, true, 20, true},	// 27
		{GAMES, false, 20, true},	// 28
		{GAMES, false, 15, true},	// 29
		{GAMES, false, 0, false},	// 30
		{GAMES, true, 0, false},	// 31
		{GAMES, true, 0, false},	// 32
		{GAMES, true, 0, true},		// 33
		{GAMES, false, 10, true},	// 34
		{GAMES, false, 15, false},	// 35
		{DTREE, false, 0, false},	// 36
		{DTREE, false, 0, false},	// 37
		{GAMES, true, 20, true},	// 38
		{GAMES, true, 20, true},	// 39
		{ASTAR, true, 15, true},	// 40
	};
	
	//sample_stats(samples, num_samples);

	struct decision *dec = dt_create(samples, num_samples);

	while (false) {
		struct sample sample;

		printf("Topic (0=ASTAR, 1=DTREE, 2=GAMES):  ");
		scanf("%i", &sample.topic);

		printf("Ass1 (0=fail, 1=pass):   ");
		scanf("%i", &sample.ass1);

		printf("Ass2 (0, 5, 10, 15, 20):   ");
		scanf("%i", &sample.ass2);

		printf("Decision: %i\n", dt_decide(dec, &sample));
		getchar();
	}

	dt_destroy(dec);
	return 0;
}
