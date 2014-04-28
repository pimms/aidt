#include "dtree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



/* Structure to be used when creating the decision tree to group
 * samples with similar preconditions together. 
 */
struct dt_where {
	struct dt_where *next;
	unsigned field;
	int value;
};

static struct dt_where* dt_where_alloc();
static void dt_where_destroy(struct dt_where*);

static struct decision* dt_alloc();
static struct decision* dt_parse_samples(const struct sample*, int,
										 struct dt_where*);
static struct sample* filter_where(const struct sample*, int, 
								   struct dt_where*, int *n);
static int best_field_where(const struct sample*, int, struct dt_where*);
static bool is_field_clausule(struct dt_where*, unsigned);


struct decision*
dt_create(const struct sample *samples, int count)
{
	return dt_parse_samples(samples, count, NULL);
}

int 
dt_decide(const struct decision *dec, const struct sample *sample)
{
	while (dec->dest) {
		while (dec && dec->value != field_value(sample, dec->field)) 
			dec = dec->next;
		if (!dec) 
			return -1;
		dec = dec->dest;
	}

	return dec->value;
}

void 
dt_destroy(struct decision *dec)
{
	if (dec->dest != NULL) 
		dt_destroy(dec->dest);
	if (dec->next != NULL) 
		dt_destroy(dec->next);

	free(dec);
}

static struct decision*
dt_alloc()
{
	struct decision *dec = (struct decision*)malloc(sizeof(struct decision));
	memset(dec, 0, sizeof(struct decision));
	return dec;
}

static struct decision*
dt_parse_samples(const struct sample *samples, int max, struct dt_where *where)
{
	if (max <= 0) {
		printf("error: empty set given to dt_parse_samples\n");
		return NULL;
	}

	struct sample *wsamples;
	int count = 0;
	wsamples = filter_where(samples, max, where, &count);

	if (count <= 0) {
		free(wsamples);
		return NULL;
	}

	int bestField = best_field_where(wsamples, max, where);
	if (bestField < 0) {
		struct decision *d = dt_alloc();
		d->value = field_value(wsamples, SAMPLE_RESULT_FIELD);
		free(wsamples);
		return d;
	}

	int unique = 0;
	int *vals = unique_values(samples, max, &unique, bestField);
	struct decision *dbase = NULL;
	struct decision *d = NULL;
	struct dt_where *wbase = where;

	while (wbase && wbase->next)
		wbase = wbase->next;

	for (int i=0; i<unique; i++) {
		struct dt_where *w = dt_where_alloc();
		w->field = bestField;
		w->value = vals[i];
		if (!where) {
			where = w;
			wbase = w;
		} else 
			wbase->next = w;

		struct decision *p = d;
		d = dt_alloc();
		if (!p) dbase = d;
		else 	p->next = d;

		struct decision *dst = dt_parse_samples(wsamples, count, where);
		if (dst) {
			d->dest = dst;
			d->field = bestField;
			d->value = vals[i];
		} else {
			d->value = field_value(wsamples, SAMPLE_RESULT_FIELD);
		}

		if (wbase == w) {
			wbase = NULL;
			where = NULL;
		} else 
			wbase->next = NULL;

		dt_where_destroy(w);
	}

	free(vals);
	free(wsamples);
	return dbase;
}

static struct sample*
filter_where(const struct sample *samples, int count, 
			 struct dt_where *where, int *n)
{
	*n = 0;
	const int sz = count * sizeof(struct sample);
	struct sample *s = (struct sample*)malloc(sz);

	for (int i=0; i<count; i++) {
		struct dt_where *w = where;
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

static int
best_field_where(const struct sample *samples, int count, struct dt_where *where)
{
	// Return the field with the lowest information gain value which is 
	// not mentioned by any where-clause
	double bestval = 1000000;
	int best = -1;

	for (int i=0; i<SAMPLE_NUM_FIELDS; i++) {
		if (i == SAMPLE_RESULT_FIELD)
			continue;
		if (!is_field_clausule(where, i)) {
			double ig = info_gain(samples, count, i);
			if (ig < bestval) {
				bestval = ig;
				best = i;
			}
		}
	}

	return best;
}

static bool 
is_field_clausule(struct dt_where *where, unsigned field)
{
	while (where) {
		if (where->field == field)
			return true;
		where = where->next;
	}

	return false;
}



static struct dt_where* 
dt_where_alloc()
{
	struct dt_where *where;
	where = (struct dt_where*)malloc(sizeof(struct dt_where));
	memset(where, 0, sizeof(struct dt_where));

	return where;
}

static void
dt_where_destroy(struct dt_where *where)
{
	if (where->next) 
		dt_where_destroy(where->next);
	free(where);
}
