#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <stdbool.h>

#define SAMPLE_NUM_FIELDS 4
#define SAMPLE_RESULT_FIELD 3

enum topic {
	ASTAR,
	DTREE,
	GAMES,
};

enum ass2 {
	ASS_0	= 0,
	ASS_5	= 5,
	ASS_10	= 10,
	ASS_15	= 15,
	ASS_20	= 20,
};

// All fields are integral for easy casting of struct to int[]. topic and
// ass2 can only contain values belonging to their respective enums. They
// are int to ensure compatibility on all systems.
struct sample  {
	int topic;	// topic_t
	int	ass1;	// bool
	int	ass2;	// ass2_t
	int	pass;	// bool
};



/* Print statistics about the samples
 */
void sample_stats(const struct sample *samples, int count);

/* Calculate the gini impurity of field [field] for all the samples.
 */
double gini_impurity(const struct sample *samples, int count, unsigned field);

/* Calculate the information gain of field [field] for all the samples
 */
double info_gain(const struct sample *samples, int count, unsigned field);

/* Count the unique values for member[field] in
 * the given sample set. The values are returned, and the number of
 * assigned items is set to num_unique. 
 *
 * Note that the return value has to be freed manually by the caller.
 */
int* unique_values(const struct sample*,int count,int *num_unique,unsigned field);

/* Count the number of occurrences of a specific value within
 * the sample set.
 */
int value_count(const struct sample*, int count, int value, unsigned field);

/* Returns the value of field[field].
 */
int field_value(const struct sample*, unsigned field);

#endif /* __SAMPLE_H__ */
