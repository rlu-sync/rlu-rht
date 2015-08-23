#ifndef _HASH_LIST_RESIZE_H_
#define _HASH_LIST_RESIZE_H_

/////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////
#include "new-urcu.h"
#include "atomics.h"
#include "rlu.h"

/////////////////////////////////////////////////////////
// DEFINES
/////////////////////////////////////////////////////////
#define LIST_VAL_MIN (INT_MIN)
#define LIST_VAL_MAX (INT_MAX)

#define NODE_PADDING (16)

#define MAX_BUCKETS (100000)

/////////////////////////////////////////////////////////
// TYPES
/////////////////////////////////////////////////////////
typedef intptr_t val_t;

typedef struct node node_t; 
typedef struct node {
	val_t val;
	node_t *p_next;

	long padding[NODE_PADDING];
} node_t;

typedef struct _list {
	node_t *p_head;
} list_t;

typedef struct _bucket {
	volatile long lock;
	volatile node_t *p_unzip_node;
	volatile int is_zipped;
	list_t *p_list;
} bucket_t;
	
typedef struct hash_list {
	int n_buckets;
	bucket_t **buckets;
} hash_list_t;
	
/////////////////////////////////////////////////////////
// INTERFACE
/////////////////////////////////////////////////////////
hash_list_t *pure_new_table(int n_buckets);
hash_list_t *rcu_new_hash_list(int n_buckets);
hash_list_t *rlu_new_hash_list(int n_buckets);

int hash_list_size(hash_list_t *p_hash_list);
void hash_list_print_stats(hash_list_t *p_hash_list);

int rcu_hash_list_expand(hash_list_t **p_p_hash_list);
int rcu_hash_list_shrink(hash_list_t **p_p_hash_list);

int rlu_hash_list_expand(rlu_thread_data_t *self, hash_list_t *p_hash_list);
int rlu_hash_list_shrink(rlu_thread_data_t *self, hash_list_t *p_hash_list);

int pure_hash_list_contains(hash_list_t *p_hash_list, val_t val);
int pure_hash_list_add(hash_list_t *p_hash_list, val_t val);
int pure_hash_list_remove(hash_list_t *p_hash_list, val_t val);

int rcu_hash_list_contains(hash_list_t **p_p_hash_list, val_t val);
int rcu_hash_list_add(hash_list_t **p_p_hash_list, val_t val);
int rcu_hash_list_remove(hash_list_t **p_p_hash_list, val_t val);

int rlu_hash_list_contains(rlu_thread_data_t *self, hash_list_t *p_hash_list, val_t val);
int rlu_hash_list_add(rlu_thread_data_t *self, hash_list_t *p_hash_list, val_t val);
int rlu_hash_list_remove(rlu_thread_data_t *self, hash_list_t *p_hash_list, val_t val);

#endif // _HASH_LIST_RESIZE_H_