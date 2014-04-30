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

	// Todo: check for nonambiguity with the current
	// where-clause

	// Get the best field to categorize the nodes on
	int bestField = best_field_where(wsamples, max, where);
	if (bestField < 0) {
		// No best field - all fields are categorized on in supernodes
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

		// Parse the nodes with the temporary WHERE-clause
		struct decision *dst = dt_parse_samples(wsamples, count, where);
		if (dst) {
			d->dest = dst;
			d->field = bestField;
			d->value = vals[i];
		} else {
			// No valid subtree found: this is a leaf node
			d->value = field_value(wsamples, SAMPLE_RESULT_FIELD);
		}

		// Clean up temporary WHERE-clause
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


/** Printing of Decision Tree **/
struct dt_stack {
	int size;
	int ptr;
	const struct decision **d;
};

static struct dt_stack* dt_stack_create();
static void dt_stack_destroy(struct dt_stack*);
static void dt_stack_push(struct dt_stack*, const struct decision*);
static const struct decision* dt_stack_pop(struct dt_stack*);


void
print_decision_tree(const struct decision *d, FILE *file) 
{
	struct dt_stack *stk = dt_stack_create();
	dt_stack_push(stk, d);

	while (stk->ptr != 0) {
		const struct decision *d = dt_stack_pop(stk);
		if (d->dest) {
			dt_stack_push(stk, d->dest);

			fprintf(file, "[NODE FIELD %i]\n", d->field);
			while (d) {
				fprintf(file, "value %i\n", d->value);
				d = d->next;
			}
			printf("\n");
		} else {
			fprintf(file, "leaf node: %i  =  %i\n\n", d->field, d->value);
		}
	}

	dt_stack_destroy(stk);
}

static struct dt_stack*
dt_stack_create()
{
	struct dt_stack *s = (struct dt_stack*)malloc(sizeof(struct dt_stack));
	memset(s, 0, sizeof(struct dt_stack));
	return s;
}

static void 
dt_stack_destroy(struct dt_stack *s)
{
	free(s->d);
	free(s);
}

static void
dt_stack_push(struct dt_stack *s, const struct decision *d)
{
	if (s->d == NULL) {
		s->size = 4;
		s->ptr = 0;
		int sz = s->size * sizeof(struct decision*);
		s->d = (const struct decision**)malloc(sz);
	}

	if (s->ptr == s->size) {
		s->size *= 2;
		int sz = sizeof(struct decision*) * s->size;
		s->d = (const struct decision**)realloc(s->d, sz);
	}

	s->d[s->ptr++] = d;
}	

static const struct decision*
dt_stack_pop(struct dt_stack *s)
{
	if (s->ptr == 0)
		return NULL;
	return s->d[--s->ptr];
}
