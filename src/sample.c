#include "sample.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>


/* Perform the following function on all the elements
 * in samples:
 * 		SIGMA(0,count){fnc(fi)}
 *
 * For instance, to do gini_impurity:
 * 		fnc(fi) = fi-(fi*fi)
 *
 * Information gain:
 * 		fnc(fi) = -fi*log2(fi)
 */
static double field_stat(const struct sample*, int, unsigned, 
						 double(*fnc)(double));
static double field_stat_gini(double fi);
static double field_stat_igain(double fi);


void
sample_stats(const struct sample *samples, int count) 
{
	for (int i=0; i<SAMPLE_NUM_FIELDS; i++) {
		if (i == SAMPLE_RESULT_FIELD)
			continue;
		printf("--- field %i ---\n", i);
		printf("gini impurity: %g\n", gini_impurity(samples, count, i));
		printf("info gain:     %g\n", info_gain(samples, count, i));

		int unique = 0;
		int *vals = unique_values(samples, count, &unique, i);

		for (int j=0; j<unique; j++) {
			int occur = value_count(samples, count, vals[j], i);
			printf("value %i occurs %i times\n", vals[j], occur);
		}

		free(vals);
		printf("\n");
	}
}


double
gini_impurity(const struct sample *samples, int count, unsigned field)
{
	return field_stat(samples, count, field, field_stat_gini);
}

double 
info_gain(const struct sample *samples, int count, unsigned field)
{
	return field_stat(samples, count, field, field_stat_igain);
}

static double
field_stat(const struct sample *samples, int count, unsigned field,
		   double (*fnc)(double))
{
	double sum = 0.0;
	int unique = 0;
	int *vals = unique_values(samples, count, &unique, field);
	int *occurs = (int*)malloc(sizeof(int)*unique);

	for (int i=0; i<unique; i++) {
		occurs[i] = value_count(samples, count, vals[i], field);
		
		double fi = (double)occurs[i] / (double)count;
		sum += fnc(fi);
	}

	free(vals);
	free(occurs);

	return sum;
}

static double 
field_stat_gini(double fi) 
{
	return fi - (fi * fi);
}

static double
field_stat_igain(double fi)
{
	if (fi != 0.0) 
		return -(fi * log2(fi));
	return 0.0;
}



int*
unique_values(const struct sample *samples, int count, 
			  int *num_unique, unsigned field)
{
	// Each new found value is inserted into the keys array.
	int *keys = (int*)malloc(sizeof(int)*6);
	for (int i=0; i<6; i++) 
		keys[i] = -1;

	*num_unique = 0;

	for (int i=0; i<count; i++) {
		int k = field_value(&samples[i], field);

		int j=0;
		while (j < 6) {
			if (keys[j] == -1) {
				keys[j] = k;
				(*num_unique)++;
				break;
			} else if (keys[j] == k) 
				break;
			j++;
		}

		if (j >= 6) {
			printf(	"Sanitize your shit, we got 6 unique values\n"
					"count: %i\nfield:%i\n", count, field);
			break;
		}
	}

	return keys;
}

int 
value_count(const struct sample *samples, int count, int value, unsigned field)
{
	int occurrences = 0;

	for (int i=0; i<count; i++) {
		if (field_value(&samples[i], field)  == value) {
			occurrences++;
		}
	}

	return occurrences;
}

int 
field_value(const struct sample *sample, unsigned field)
{
	int *is = (int*) sample;
	return is[field];
}
