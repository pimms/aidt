#include "sample.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>


/* Where */
struct where* 
where_alloc()
{
	struct where *where;
	where = (struct where*)malloc(sizeof(struct where));
	memset(where, 0, sizeof(struct where));

	return where;
}

void
where_destroy(struct where *where)
{
	if (where->next) 
		where_destroy(where->next);
	free(where);
}

void
where_print(struct where *where) 
{
	printf("WHERE: [");

	while (where) {
		printf("%i=%i", where->field, where->value);
		if (where->next) 
			printf(", ");
		where = where->next;
	}

	printf("]\n");
}

bool 
is_field_clausule(struct where *where, unsigned field)
{
	while (where) {
		if (where->field == field)
			return true;
		where = where->next;
	}

	return false;
}

struct sample*
filter_where(const struct sample *samples, int count, 
			 struct where *where, int *n)
{
	*n = 0;
	const int sz = count * sizeof(struct sample);
	struct sample *s = (struct sample*)malloc(sz);

	for (int i=0; i<count; i++) {
		struct where *w = where;
		bool all_success = true;

		while (w) {
			if (field_value(&samples[i], w->field) == w->value) 
				all_success = false;
			w = w->next;
		}

		if (all_success) {
			s[(*n)++] = samples[i];
		}
	}

	return s;
}



/* Sample */
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
	double sum = 0.0;
	int unique = 0;
	int *vals = unique_values(samples, count, &unique, field);
	int *occurs = (int*)malloc(sizeof(int)*unique);

	for (int i=0; i<unique; i++) {
		occurs[i] = value_count(samples, count, vals[i], field);
		
		double fi = (double)occurs[i] / (double)count;
		sum += fi - (fi * fi);
	}

	free(vals);
	free(occurs);

	return sum;
}

double 
info_gain(const struct sample *samples, int count, unsigned field)
{
	const bool ig_print = false;


	int unique;
	int *vals = unique_values(samples, count, &unique, field);
	int *occurs = (int*)malloc(sizeof(int) * unique);
	for (int i=0; i<unique; i++)
		occurs[i] = value_count(samples, count, vals[i], field);

	double e = set_entropy(samples, count);
	if (ig_print)
		printf("Entropy(S, %i) = %g", field, e);

	for (int i=0; i<unique; i++) {

		struct where *where = where_alloc();
		where->field = field;
		where->value = vals[i];

		int svcount = 0;
		struct sample *sv = filter_where(samples, count, where, &svcount);

		double se = set_entropy(sv, svcount);
		double n = (double)occurs[i] / (double)count;
		n *= se;
		e -= n;

		if (ig_print)
			printf(" - (%i / %i) * %g", occurs[i], count, se);

		where_destroy(where);
		free(sv);
	}

	if (ig_print)
		printf(" = %g\n", e);

	free(vals);
	free(occurs);
	return e;
}

double 
set_entropy(const struct sample *samples, int count)
{
	int unique = 0;
	int *vals = unique_values(samples, count, &unique, SAMPLE_RESULT_FIELD);

	int *occurs = (int*)malloc(sizeof(int) * unique);
	for (int i=0; i<unique; i++) {
		occurs[i] = value_count(samples, count, vals[i], SAMPLE_RESULT_FIELD);
	}

	double e = 0.0;
	for (int i=0; i<unique; i++) {
		double f = (double)occurs[i] / (double)count;
		e -= f * log2(f);
	}

	free(occurs);
	free(vals);
	return e;
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

void 
set_field_value(const struct sample *sample, unsigned field, int value)
{
	int *is = (int*)sample;
	is[field] = value;
}
