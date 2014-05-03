#include "dtree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>




static struct decision* dt_alloc();
static struct decision* dt_parse_samples(const struct sample*, int,
										 struct where*);
static void dt_append_next(struct decision *root, struct decision *next);

static int best_field_where(const struct sample*, int, struct where*);
static bool is_set_ambiguous(const struct sample*, int);
static void majority_result(const struct sample*, int, unsigned*field, int*val);
static struct decision* majority_result_node(const struct sample*, int);
static void print_set_info(const struct sample*, int, struct where*);


struct decision*
dt_create(const struct sample *samples, int count)
{
	return dt_parse_samples(samples, count, NULL);
}

int 
dt_decide(const struct decision *dec, const struct sample *sample)
{
	while (dec && dec->dest) {
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
dt_parse_samples(const struct sample *samples, int max, struct where *where)
{
	bool ambiguous = is_set_ambiguous(samples, max);
	int best_field = best_field_where(samples, max, where);

	if (best_field < 0 || !ambiguous) 
		return majority_result_node(samples, max);


	// The first call has no defined where, and it must be explicitly
	// deleted. Other calls only need append a new where-clause and
	// give it proper filters.
	struct where *w = where_alloc();
	if (where)	 where->next = w;
	else		 where = w;
	w->field = best_field;
	
	// Get all the unique values from the set
	int unique = 0;
	int *vals = unique_values(samples, max, &unique, best_field);

	// The decision tree we are returning
	struct decision *dec = NULL;

	for (int i=0; i<unique; i++) {
		// Create a subset filtered for s->{best_field} = V[i]
		w->value = vals[i];
		int wmax = 0;
		struct sample *wsamples = filter_where(samples, max, where, &wmax);

		// If the filtered subset is equal to the superset, the training
		// data is ambiguous. Return a leaf node with the majority result
		if (wmax == max) {
			printf("Ambiguity in training set:\n\t");
			where_print(where);
			for (int j=0; j<max; j++) {
				printf("\t");
				sample_print(&samples[j]);
			}
			struct decision *mrn = majority_result_node(samples, max);		

			printf("\tassigning majority value %i=%i\n\n",
					mrn->field, mrn->value);
			return mrn;
		}

		// Create a branch-node
		struct decision *d = dt_alloc();
		d->field = best_field;
		d->value = vals[i];
		
		// Append the branch to the tree
		if (!dec) 	dec = d;
		else 		dt_append_next(dec, d);

		// Create a subtree
		struct decision *sub = dt_parse_samples(wsamples, wmax, where);
		sub->parent = d;
		d->dest = sub;
	
		free(wsamples);
	}

	if (where != w)
		where->next = NULL;
	where_destroy(w);
	free(vals);
	return dec;
}

static void 
dt_append_next(struct decision *root, struct decision *next)
{
	while (root->next)
		root = root->next;
	root->next = next;
}


static int
best_field_where(const struct sample *samples, int count, struct where *where)
{
	// Return the field with the highest information gain value which is 
	// not mentioned by any where-clause
	double bestval = -1000000;
	int best = -1;

	for (int i=0; i<SAMPLE_NUM_FIELDS; i++) {
		if (i == SAMPLE_RESULT_FIELD)
			continue;
		if (!is_field_clausule(where, i)) {
			double ig = info_gain(samples, count, i);
			if (ig > bestval) {
				bestval = ig;
				best = i;
			}
		}
	}

	return best;
}

static bool
is_set_ambiguous(const struct sample *samples, int count)
{
	// The set is ambiguous if the result field varies in the set.
	const unsigned f = SAMPLE_RESULT_FIELD;
	for (int i=1; i<count; i++) {
		if (field_value(&samples[i], f) != field_value(&samples[0], f)) 
			return true;
	}

	return false;
}

static void
majority_result(const struct sample *samples, int count, unsigned *field, int *val)
{
	int unique;
	int *vals = unique_values(samples, count, &unique, SAMPLE_RESULT_FIELD);
	int *occurs = (int*)malloc(sizeof(int) * unique);
	for (int i=0; i<unique; i++) 
		occurs[i] = value_count(samples, count, vals[i], SAMPLE_RESULT_FIELD);

	*field = SAMPLE_RESULT_FIELD;
	*val = -1;
	for (int i=0; i<unique; i++) {
		if (occurs[i] > *val) {
			*val = vals[i];
		}
	}

	free(vals);
	free(occurs);
}

static struct decision*
majority_result_node(const struct sample *samples, int count) 
{
	unsigned field = 0;
	int val = 0;
	majority_result(samples, count, &field, &val);

	struct decision *d = dt_alloc();
	d->field = field;
	d->value = val;
	return d;
}

static void 
print_set_info(const struct sample *samples, int count, struct where *where)
{
	printf("items: %i\n", count);
	printf("ambiguous: %s\n", is_set_ambiguous(samples, count) ? "yes" : "no");
	where_print(where);
	
	bool printed_info = false;
	for (int i=0; i<SAMPLE_NUM_FIELDS; i++) {
		if (i != SAMPLE_RESULT_FIELD && !is_field_clausule(where, i)) {
			if (!printed_info) {
				printf("Uncategorized fields:\n");
				printed_info = true;
			}

			printf("field %i info gain: %g\n",
					i, info_gain(samples, count, i));
		}
	}

	printf("\n");
}



/** Printing of Decision Tree **/

// Decision wrapper for printing
struct dt_deque {
	int size;
	int head;
	int tail;
	const struct decision **d;
};

static void print_path(const struct decision *leaf);

static struct dt_deque* dt_deque_create();
static void dt_deque_destroy(struct dt_deque*);
static int dt_deque_size(struct dt_deque*);
static void dt_deque_push(struct dt_deque*, const struct decision*);
static const struct decision* dt_deque_pop_back(struct dt_deque*);
static const struct decision* dt_deque_pop_front(struct dt_deque*);


void
print_decision_tree(const struct decision *d, FILE *file) 
{
	printf("[DECISION TREE]\n");
	printf("ALL paths from the root node to the leaf nodes are printed.\n");
	printf("Each line represents one path. Each bracket represents one node.\n\n");
	
	struct dt_deque *dq = dt_deque_create();
	dt_deque_push(dq, d);

	// Iterate over all nodes, print path from leaves to root
	while (dt_deque_size(dq) > 0) {
		d = dt_deque_pop_back(dq);
		
		// Push dest on stack if exists, otherwise print
		if (d->dest)
			dt_deque_push(dq, d->dest);
		else 
			print_path(d);
		// Push siblings on stack
		for (struct decision *n = d->next; n; n=n->next) 
			dt_deque_push(dq, n);
	}


	dt_deque_destroy(dq);
}

static void 
print_path(const struct decision *leaf)
{
	struct dt_deque *dq = dt_deque_create();

	const struct decision *d = leaf;
	dt_deque_push(dq, d);
	d = d->parent;

	while (d) {
		dt_deque_push(dq, d);
		d = d->parent;
	}

	// Print them in reverse order
	while (dt_deque_size(dq) > 0) {
		d = dt_deque_pop_back(dq);
		printf("{%i => %i}", d->field, d->value);
	}

	printf("\n");

	dt_deque_destroy(dq);
}


static struct dt_deque*
dt_deque_create()
{
	struct dt_deque *s = (struct dt_deque*)malloc(sizeof(struct dt_deque));
	memset(s, 0, sizeof(struct dt_deque));
	return s;
}

static void 
dt_deque_destroy(struct dt_deque *s)
{
	free(s->d);
	free(s);
}

static int
dt_deque_size(struct dt_deque *s)
{
	return s->tail - s->head;
}

static void
dt_deque_push(struct dt_deque *s, const struct decision *d)
{
	if (s->d == NULL) {
		s->size = 4;
		s->tail = 0;
		int sz = s->size * sizeof(struct decision*);
		s->d = (const struct decision**)malloc(sz);
	}

	if (s->tail == s->size) {
		s->size *= 2;
		int sz = sizeof(struct decision*) * s->size;
		s->d = (const struct decision**)realloc(s->d, sz);
	}

	s->d[s->tail++] = d;
}	

static const struct decision*
dt_deque_pop_back(struct dt_deque *s)
{
	if (s->tail == s->head)
		return NULL;
	return s->d[--s->tail];
}

static const struct decision*
dt_deque_pop_front(struct dt_deque *s)
{
	if (s->head == s->tail)
		return NULL;
	return s->d[s->head++];
}
