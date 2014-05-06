#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <stdbool.h>

// The number of integral fields in a sample. More fields exists, but
// the first "SAMPLE_NUM_FIELDS" are guaranteed to be of type "int32" 
// and contain data related to the training set.
#define SAMPLE_NUM_FIELDS 4

// The field index of the resulting field.
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
	int id; 	// NOT a decision parameter
};


/* Structure used when splitting a set of samples
 */
struct where {
	struct where *next;
	unsigned field;
	int value;
};

struct where* where_alloc();
void where_destroy(struct where*);
void where_print(struct where*);
bool is_field_clausule(struct where*, unsigned);
void where_append(struct where*, struct where*);
struct where* where_pop(struct where*);

/* Create a subset of the samples given a sort on the where
 * clauses. The returned array must be manually freed by the caller, and 
 * the number of elements in the new set assigned to the "n" variable.
 */
struct sample* filter_where(const struct sample*, int, 
						    struct where*, int *n);


/* Print statistics about the samples
 */
void sample_stats(const struct sample *samples, int count);

/* Prints the fields of a single sample
 */
void sample_print(const struct sample *sample);

/* Calculate the gini impurity of field [field] for all the samples.
 */
double gini_impurity(const struct sample *samples, int count, unsigned field);

/* Calculates the information gain if the provided set (samples) is
 * divided based upon (field).
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

/* Set field value [field] to value
 */
void set_field_value(const struct sample*, unsigned field, int value);

double set_entropy(const struct sample*, int count);

#endif /* __SAMPLE_H__ */
