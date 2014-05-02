#ifndef __DTREE_H__
#define __DTREE_H__

#include "sample.h"
#include <stdio.h>

struct decision;
struct dtree;


/* decision
 * Holds a decicion, and all possible branches given the value of the field.
 * If the value is equal, follow "dest" - otherwise follow "next". 
 *
 * If "dest" is NULL, the final decision can be found in "value".
 */
struct decision {
	unsigned field;
	int value;
	struct decision *next;
	struct decision *dest;
	struct decision *parent;
};



struct decision* dt_create(const struct sample*, int count);
int dt_decide(const struct decision*, const struct sample*);
void dt_destroy(struct decision*);

void print_decision_tree(const struct decision*, FILE*);

#endif /*__DTREE_H__*/
