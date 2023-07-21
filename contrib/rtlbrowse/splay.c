/*
                An implementation of top-down splaying
                    D. Sleator <sleator@cs.cmu.edu>
    	                     March 1992

  "Splay trees", or "self-adjusting search trees" are a simple and
  efficient data structure for storing an ordered set.  The data
  structure consists of a binary tree, without parent pointers, and no
  additional fields.  It allows searching, insertion, deletion,
  deletemin, deletemax, splitting, joining, and many other operations,
  all with amortized logarithmic performance.  Since the trees adapt to
  the sequence of requests, their performance on real access patterns is
  typically even better.  Splay trees are described in a number of texts
  and papers [1,2,3,4,5].

  The code here is adapted from simple top-down splay, at the bottom of
  page 669 of [3].  It can be obtained via anonymous ftp from
  spade.pc.cs.cmu.edu in directory /usr/sleator/public.

  The chief modification here is that the splay operation works even if the
  item being splayed is not in the tree, and even if the tree root of the
  tree is NULL.  So the line:

                              t = splay(i, t);

  causes it to search for item with key i in the tree rooted at t.  If it's
  there, it is splayed to the root.  If it isn't there, then the node put
  at the root is the last one before NULL that would have been reached in a
  normal binary search for i.  (It's a neighbor of i in the tree.)  This
  allows many other operations to be easily implemented, as shown below.

  [1] "Fundamentals of data structures in C", Horowitz, Sahni,
       and Anderson-Freed, Computer Science Press, pp 542-547.
  [2] "Data Structures and Their Algorithms", Lewis and Denenberg,
       Harper Collins, 1991, pp 243-251.
  [3] "Self-adjusting Binary Search Trees" Sleator and Tarjan,
       JACM Volume 32, No 3, July 1985, pp 652-686.
  [4] "Data Structure and Algorithm Analysis", Mark Weiss,
       Benjamin Cummins, 1992, pp 119-130.
  [5] "Data Structures, Algorithms, and Performance", Derick Wood,
       Addison-Wesley, 1993, pp 367-375.

note: modified for void pointer passing 120302ajb

*/

#include "splay.h"

ds_Tree * ds_splay (char *i, ds_Tree * t) {
/* Simple top down splay, not requiring i to be in the tree t.  */
/* What it does is described above.                             */
    ds_Tree N, *l, *r, *y;
    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;

    for (;;) {
	if (strcmp(i, t->item) <0) {
	    if (t->left == NULL) break;
	    if (strcmp(i, t->left->item) < 0) {
		y = t->left;                           /* rotate right */
		t->left = y->right;
		y->right = t;
		t = y;
		if (t->left == NULL) break;
	    }
	    r->left = t;                               /* link right */
	    r = t;
	    t = t->left;
	} else if (strcmp(i, t->item) > 0) {
	    if (t->right == NULL) break;
	    if (strcmp(i, t->right->item) > 0) {
		y = t->right;                          /* rotate left */
		t->right = y->left;
		y->left = t;
		t = y;
		if (t->right == NULL) break;
	    }
	    l->right = t;                              /* link left */
	    l = t;
	    t = t->right;
	} else {
	    break;
	}
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
}


ds_Tree * ds_insert(char *i, ds_Tree * t) {
/* Insert i into the tree t, unless it's already there.    */
/* Return a pointer to the resulting tree.                 */
    ds_Tree * n;

    n = (ds_Tree *) calloc (1, sizeof (ds_Tree));
    if (n == NULL) {
	fprintf(stderr, "ds_insert: ran out of memory, exiting.\n");
	exit(255);
    }
    n->item = i;
    if (t == NULL) {
	n->left = n->right = NULL;
	return n;
    }
    t = ds_splay(i,t);
    if (strcmp(i, t->item) < 0) {
	n->left = t->left;
	n->right = t;
	t->left = NULL;
	return n;
    } else if (strcmp(i, t->item) > 0) {
	n->right = t->right;
	n->left = t;
	t->right = NULL;
	return n;
    } else { /* We get here if it's already in the tree */
             /* Don't add it again                      */
	free(n);
	return t;
    }
}

ds_Tree * ds_delete(char *i, ds_Tree * t) {
/* Deletes i from the tree if it's there.               */
/* Return a pointer to the resulting tree.              */
    ds_Tree * x;
    if (t==NULL) return NULL;
    t = ds_splay(i,t);
    if (strcmp(i, t->item) == 0) {               /* found it */
	if (t->left == NULL) {
	    x = t->right;
	} else {
	    x = ds_splay(i, t->left);
	    x->right = t->right;
	}
	free(t);
	return x;
    }
    return t;                         /* It wasn't there */
}

