#ifndef DS_SPLAY_FUNCTIONS_H
#define DS_SPLAY_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ds_tree_node ds_Tree;
struct ds_tree_node {
	ds_Tree * left, * right;
	ds_Tree * next_flat;
	char *item;

	char *filename;
	int s_line, e_line;

	char *fullname;

	int refcnt;
	struct ds_component *comp;

	unsigned resolved : 1;
        unsigned dnd_to_import : 1;
};

struct ds_component
{
struct ds_component *next;

char *compname;
ds_Tree *module;
};



ds_Tree * ds_splay (char *i, ds_Tree * t);
ds_Tree * ds_insert(char *i, ds_Tree * t);
ds_Tree * ds_delete(char *i, ds_Tree * t);

#endif

