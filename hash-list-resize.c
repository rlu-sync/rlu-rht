
/////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>

#include "atomics.h"
#include "new-urcu.h"
#include "rlu.h"
#include "hash-list-resize.h" 

/////////////////////////////////////////////////////////
// DEFINES
/////////////////////////////////////////////////////////
#define HASH_VALUE(p_hash_list, val)       (val % p_hash_list->n_buckets)
#define GET_PAIR_BUCKET(p_hash_list, bin)  ((bin + (p_hash_list->n_buckets/2)) % p_hash_list->n_buckets) 

#define RLU_DEREF_HASH_LIST(self, p_hash_list) ((hash_list_t *)RLU_DEREF(self, (intptr_t *)p_hash_list))
#define RLU_DEREF_BUCKET(self, p_bucket) ((bucket_t *)RLU_DEREF(self, (intptr_t *)p_bucket))
#define RLU_DEREF_LIST(self, p_list) ((list_t *)RLU_DEREF(self, (intptr_t *)p_list))
#define RLU_DEREF_NODE(self, p_node) ((node_t *)RLU_DEREF(self, (intptr_t *)p_node))

/////////////////////////////////////////////////////////
// TYPES
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// GLOBALS
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// LIST LOCK/UNLOCK
/////////////////////////////////////////////////////////
int try_lock_bucket(bucket_t *p_bucket) {
	
	volatile long cur_lock = p_bucket->lock;
	if (cur_lock == 0) {
		if (CAS(&(p_bucket->lock), 0, 1) == 0) {
			return 1;
		}
	}
	
	return 0;
}

void unlock_bucket(bucket_t *p_bucket) {
	p_bucket->lock = 0;
}

/////////////////////////////////////////////////////////
// NEW NODE
/////////////////////////////////////////////////////////
node_t *pure_new_node() {
	
	node_t *p_new_node = (node_t *)malloc(sizeof(node_t));
	if (p_new_node == NULL){
		printf("out of memory\n");
		exit(1); 
	}    
	
    return p_new_node;
}

node_t *rcu_new_node() {    
	return pure_new_node();
}

node_t *rlu_new_node() {
    
	node_t *p_new_node = (node_t *)RLU_ALLOC(sizeof(node_t));
	if (p_new_node == NULL){
		printf("out of memory\n");
		exit(1); 
	}    
	
    return p_new_node;
}

/////////////////////////////////////////////////////////
// FREE NODE
/////////////////////////////////////////////////////////
void pure_free_node(node_t *p_node) {
	if (p_node != NULL) {
		free(p_node);
	}
}

void rcu_free_node(node_t *p_node) {
	RCU_FREE(p_node);
}

void rlu_free_node(rlu_thread_data_t *self, node_t *p_node) {
	RLU_FREE(self, p_node);
}

/////////////////////////////////////////////////////////
// NEW LIST
/////////////////////////////////////////////////////////
list_t *pure_new_list()
{
	list_t *p_list;
	node_t *p_min_node, *p_max_node;

	p_list = (list_t *)malloc(sizeof(list_t));
	if (p_list == NULL) {
		perror("malloc");
		exit(1);
	}
	
	p_list->p_head = NULL;
	
	return p_list;
}

list_t *rcu_new_list()
{
	return pure_new_list();
}

list_t *rlu_new_list()
{
	list_t *p_list;
	node_t *p_min_node, *p_max_node;

	p_list = (list_t *)RLU_ALLOC(sizeof(list_t));
	if (p_list == NULL) {
		perror("malloc");
		exit(1);
	}
	
	p_list->p_head = NULL;
	
	return p_list;
}

/////////////////////////////////////////////////////////
// NEW HASH LIST
/////////////////////////////////////////////////////////
hash_list_t *pure_new_hash_list(int n_buckets, int is_init_lists)
{
	int i;	
	hash_list_t *p_hash_list;
  	
	p_hash_list = (hash_list_t *)malloc(sizeof(hash_list_t));
	
	if (p_hash_list == NULL) {
	    perror("malloc");
	    exit(1);
	}
	
	p_hash_list->n_buckets = n_buckets; 
	
	p_hash_list->buckets = (bucket_t **)malloc(sizeof(bucket_t *) * n_buckets);
	
	for (i = 0; i < p_hash_list->n_buckets; i++) {
		p_hash_list->buckets[i] = (bucket_t *)malloc(sizeof(bucket_t));
		p_hash_list->buckets[i]->is_zipped = 0;
		p_hash_list->buckets[i]->lock = 0;
		p_hash_list->buckets[i]->p_unzip_node = NULL;
		if (is_init_lists) {
			p_hash_list->buckets[i]->p_list = pure_new_list();
		} else {
			p_hash_list->buckets[i]->p_list = NULL;
		}
	}
	
	return p_hash_list;
}

hash_list_t *rcu_new_hash_list(int n_buckets) {
	return pure_new_hash_list(n_buckets, 1);
}

bucket_t **rlu_new_buckets(int n_buckets, int is_init_lists) {
	int i;
	bucket_t **buckets;
	
	buckets = (bucket_t **)RLU_ALLOC(sizeof(bucket_t *) * n_buckets);
	
	for (i = 0; i < n_buckets; i++) {
		buckets[i] = (bucket_t *)RLU_ALLOC(sizeof(bucket_t));
		buckets[i]->is_zipped = 0;
		if (is_init_lists) {
			buckets[i]->p_list = rlu_new_list();
		} else {
			buckets[i]->p_list = NULL;
		}
	}
	
	return buckets;
}

hash_list_t *rlu_new_hash_list(int n_buckets)
{
	int i;	
	hash_list_t *p_hash_list;
  	
	p_hash_list = (hash_list_t *)RLU_ALLOC(sizeof(hash_list_t));
	
	if (p_hash_list == NULL) {
	    perror("malloc");
	    exit(1);
	}
	
	p_hash_list->n_buckets = n_buckets; 
	
	p_hash_list->buckets = rlu_new_buckets(n_buckets, 1);
		
	return p_hash_list;
}

/////////////////////////////////////////////////////////
// FREE HASH LIST
/////////////////////////////////////////////////////////
void rlu_free_buckets(rlu_thread_data_t *self, bucket_t **buckets, int n_buckets) {
	int i;
	
	for (i = 0; i < n_buckets; i++) {
		RLU_FREE(self, buckets[i]);
	}
	
	RLU_FREE(self, buckets);
	
}

/////////////////////////////////////////////////////////
// LIST SIZE
/////////////////////////////////////////////////////////
int list_size(list_t *p_list)
{
	int size = 0;
	node_t *p_node;

	p_node = p_list->p_head;
	while (p_node != NULL) {
		size++;
		p_node = p_node->p_next;
	}

	return size;
}

void list_print(list_t *p_list)
{
	node_t *p_node;

	printf("[ ");
	p_node = p_list->p_head;
	while (p_node != NULL) {
		printf("[%d] ", (int)p_node->val);
		p_node = p_node->p_next;
	}
	printf("]");
}

/////////////////////////////////////////////////////////
// HASH LIST SIZE
/////////////////////////////////////////////////////////
int hash_list_size(hash_list_t *p_hash_list)
{
	int i;
	int size = 0;
	
	for (i = 0; i < p_hash_list->n_buckets; i++) {
		size += list_size(p_hash_list->buckets[i]->p_list);
	}
	
	return size;
}

/////////////////////////////////////////////////////////
// LIST CONTAINS
/////////////////////////////////////////////////////////
node_t *pure_list_find(list_t *p_list, val_t val, node_t **p_p_prev) {
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = p_list->p_head;

	while ((p_node != NULL) && (p_node->val < val)) {
		p_prev = p_node;
		p_node = p_node->p_next;
	}
	
	*p_p_prev = p_prev;
	return p_node;
}

node_t *rcu_list_find(list_t *p_list, val_t val, node_t **p_p_prev) {
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = RCU_DEREF(p_list->p_head);

	while ((p_node != NULL) && (p_node->val < val)) {
		p_prev = p_node;
		p_node = RCU_DEREF(p_node->p_next);
	}
	
	*p_p_prev = p_prev;
	return p_node;
}

node_t *rlu_list_find(rlu_thread_data_t *self, list_t *p_list, val_t val, node_t **p_p_prev) {
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = RLU_DEREF_NODE(self, p_list->p_head);

	while ((p_node != NULL) && (p_node->val < val)) {
		p_prev = p_node;
		p_node = RLU_DEREF_NODE(self, p_node->p_next);
	}
	
	*p_p_prev = p_prev;
	return p_node;
}

int pure_list_contains(list_t *p_list, val_t val) {
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = pure_list_find(p_list, val, &p_prev);
	
	if (p_node == NULL) {
		return 0;
	}
	
	return p_node->val == val;
}

int rcu_list_contains(list_t *p_list, val_t val) {
	int result;
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = rcu_list_find(p_list, val, &p_prev);
	
	if (p_node == NULL) {
		result = 0;
	} else {
		result = p_node->val == val;
	}
	
	return result;
}

int rlu_list_contains(rlu_thread_data_t *self, list_t *p_list, val_t val) {
	int result;
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = rlu_list_find(self, p_list, val, &p_prev);
	
	if (p_node == NULL) {
		result = 0;
	} else {
		result = p_node->val == val;
	}
	
	return result;
}

/////////////////////////////////////////////////////////
// HASH LIST CONTAINS
/////////////////////////////////////////////////////////
int pure_hash_list_contains(hash_list_t *p_hash_list, val_t val)
{
	int hash = HASH_VALUE(p_hash_list, val);
	
	return pure_list_contains(p_hash_list->buckets[hash]->p_list, val);	
}

int rcu_hash_list_contains(hash_list_t **p_p_hash_list, val_t val)
{
	int hash;
	int res;
	volatile hash_list_t *p_hash_list;
	volatile hash_list_t **p_p_cur_hash_list;
	
	p_p_cur_hash_list = (volatile hash_list_t **)p_p_hash_list;
	
	RCU_READER_LOCK();
		
	p_hash_list = *p_p_cur_hash_list;
	hash = HASH_VALUE(p_hash_list, val);
	
	res = rcu_list_contains(p_hash_list->buckets[hash]->p_list, val);	
	
	RCU_READER_UNLOCK();
		
	return res;
}

int rlu_hash_list_contains(rlu_thread_data_t *self, hash_list_t *p_hash_list, val_t val)
{
	int res;
	int bin;
	bucket_t *p_bucket;
	list_t *p_list;
	hash_list_t *p_cur_hash_list;
	
	RLU_READER_LOCK(self);

	p_cur_hash_list = RLU_DEREF_HASH_LIST(self, p_hash_list);
	bin = HASH_VALUE(p_cur_hash_list, val);
	p_bucket = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[bin]);
	p_list = RLU_DEREF_LIST(self, p_bucket->p_list);
	
	res = rlu_list_contains(self, p_list, val);	
	
	RLU_READER_UNLOCK(self);
	
	return res;
}

/////////////////////////////////////////////////////////
// LIST ADD
/////////////////////////////////////////////////////////
int pure_list_add(list_t *p_list, val_t val)
{
	int result;		
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	node_t *p_new_node = NULL;
	
	p_node = pure_list_find(p_list, val, &p_prev);
	
	result = (p_node == NULL) || (p_node->val != val);
	
	if (result) {
		p_new_node = pure_new_node();
		p_new_node->val = val;
		p_new_node->p_next = p_node;
		
		if (p_prev == NULL) {
			p_list->p_head = p_new_node;
		} else {
			p_prev->p_next = p_new_node;
		}
	}
	
	return result;
}

int rlu_list_add(rlu_thread_data_t *self, list_t *p_list, val_t val)
{
	int result;		
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	node_t *p_new_node = NULL;
		
	p_node = rlu_list_find(self, p_list, val, &p_prev);
	
	result = (p_node == NULL) || (p_node->val != val);
	
	if (result) {
		if (p_prev == NULL) {
			RLU_LOCK(self, &p_list);
		} else {
			RLU_LOCK(self, &p_prev);
		}
		
		if (p_node != NULL) {
			RLU_LOCK(self, &p_node);
		}
				
		p_new_node = rlu_new_node();
		p_new_node->val = val;
		RLU_ASSIGN_PTR(self, &(p_new_node->p_next), p_node);
		
		if (p_prev == NULL) {
			RLU_ASSIGN_PTR(self, &(p_list->p_head), p_new_node);
		} else {
			RLU_ASSIGN_PTR(self, &(p_prev->p_next), p_new_node);
		}
	}
	
	return result;
}

/////////////////////////////////////////////////////////
// HASH LIST ADD
/////////////////////////////////////////////////////////
int pure_hash_list_add(hash_list_t *p_hash_list, val_t val)
{
	int hash = HASH_VALUE(p_hash_list, val);
	
	return pure_list_add(p_hash_list->buckets[hash]->p_list, val);
}

int rcu_hash_list_add(hash_list_t **p_p_hash_list, val_t val)
{
	hash_list_t *p_hash_list = *p_p_hash_list;
	int hash = HASH_VALUE(p_hash_list, val);
	
	return pure_list_add(p_hash_list->buckets[hash]->p_list, val);
}

list_t *rlu_hash_list_lock(rlu_thread_data_t *self, hash_list_t *p_hash_list, val_t val) {
	int bin;
	int pair_bin;
	bucket_t *p_bucket;
	list_t *p_list;
	hash_list_t *p_cur_hash_list;
	
restart:
	RLU_READER_LOCK(self);

	p_cur_hash_list = RLU_DEREF_HASH_LIST(self, p_hash_list);

	bin = HASH_VALUE(p_cur_hash_list, val);
	
	p_bucket = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[bin]);
	
	if (!RLU_TRY_WRITER_LOCK(self, bin)) {
		RLU_ABORT(self);
		goto restart;
	}	

	if (p_bucket->is_zipped) {
		pair_bin = GET_PAIR_BUCKET(p_cur_hash_list, bin);

		if (!RLU_TRY_WRITER_LOCK(self, pair_bin)) {
			RLU_ABORT(self);
			goto restart;
		}
	}

	p_list = RLU_DEREF_LIST(self, p_bucket->p_list);
	
	return p_list;
}

int rlu_hash_list_add(rlu_thread_data_t *self, hash_list_t *p_hash_list, val_t val)
{
	int res;
	list_t *p_list;
	
	p_list = rlu_hash_list_lock(self, p_hash_list, val);
	
	res = rlu_list_add(self, p_list, val);
	
	RLU_READER_UNLOCK(self);
	
	return res;
}


/////////////////////////////////////////////////////////
// LIST REMOVE
/////////////////////////////////////////////////////////
int pure_list_remove(list_t *p_list, val_t val) {
	int result;		
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = pure_list_find(p_list, val, &p_prev);
	
	result = (p_node != NULL) && (p_node->val == val);
	
	if (result) {
		if (p_prev == NULL) {
			p_list->p_head = p_node->p_next;
		} else {
			p_prev->p_next = p_node->p_next;
		}
		
		pure_free_node(p_node);     
	}
	
	return result;
}

int rlu_list_remove(rlu_thread_data_t *self, list_t *p_list, val_t val) {
	int result;		
	node_t *p_prev = NULL;
	node_t *p_node = NULL;
	
	p_node = rlu_list_find(self, p_list, val, &p_prev);
	
	result = (p_node != NULL) && (p_node->val == val);
	
	if (result) {
		if (p_prev == NULL) {
			RLU_LOCK(self, &p_list);
		} else {
			RLU_LOCK(self, &p_prev);
		}
			
		RLU_LOCK(self, &p_node);
			
		if (p_prev == NULL) {
			RLU_ASSIGN_PTR(self, &(p_list->p_head), p_node->p_next);
		} else {
			RLU_ASSIGN_PTR(self, &(p_prev->p_next), p_node->p_next);
		}
		
		RLU_FREE(self, p_node);	
				
		return result;
	}
	
	return result;
}

/////////////////////////////////////////////////////////
// HASH LIST REMOVE
/////////////////////////////////////////////////////////
int pure_hash_list_remove(hash_list_t *p_hash_list, val_t val)
{
	int hash = HASH_VALUE(p_hash_list, val);
	
	return pure_list_remove(p_hash_list->buckets[hash]->p_list, val);
}

int rcu_hash_list_remove(hash_list_t **p_p_hash_list, val_t val)
{
	hash_list_t *p_hash_list = *p_p_hash_list;
	int hash = HASH_VALUE(p_hash_list, val);
	
	return pure_list_remove(p_hash_list->buckets[hash]->p_list, val);
}

int rlu_hash_list_remove(rlu_thread_data_t *self, hash_list_t *p_hash_list, val_t val)
{
	int res;
	list_t *p_list;

	p_list = rlu_hash_list_lock(self, p_hash_list, val);
	
	res = rlu_list_remove(self, p_list, val);
		
	RLU_READER_UNLOCK(self);
	
	return res;
}

/////////////////////////////////////////////////////////
// HASH LIST RESIZE
/////////////////////////////////////////////////////////
node_t *rcu_find_unzip_point(hash_list_t *p_hash_list, node_t *p_unzip_node) {
	node_t *p_node_prev;
	node_t *p_node;
		
	p_node_prev = p_unzip_node;
	
	if (p_node_prev == NULL) {
		return NULL;
	}
	
	p_node = p_node_prev->p_next;
	
	while ((p_node != NULL) && (HASH_VALUE(p_hash_list, p_node_prev->val) == HASH_VALUE(p_hash_list, p_node->val))) {
		p_node_prev = p_node;
		p_node = p_node_prev->p_next;
	}
	
	if (p_node == NULL) {
		return NULL;
	}
	
	return p_node_prev;
	
}

int rcu_hash_list_expand(hash_list_t **p_p_hash_list) {
	int i = 0;
	int id_1;
	int id_2;
	int id_temp;
	int bin;
	int id_unzip;
	int moved_one = 0;
	int is_source_head;
	node_t *p_node = NULL;
	node_t *p_node_prev = NULL;
	node_t *p_node_source = NULL;
	node_t *p_unzip_node = NULL;
	list_t *p_new_list = NULL;
	list_t *p_list_1 = NULL;
	list_t *p_list_2 = NULL;
	list_t *p_list_new = NULL;
	bucket_t *p_bucket_1 = NULL;
	bucket_t *p_bucket_2 = NULL;
	volatile hash_list_t *p_old_hash_list = NULL;
	hash_list_t *p_new_hash_list = NULL;
	volatile hash_list_t **p_p_cur_hash_list = NULL;
	
	p_p_cur_hash_list = (volatile hash_list_t **)p_p_hash_list;
	
	p_old_hash_list = *p_p_cur_hash_list;
	
	p_new_hash_list = pure_new_hash_list(p_old_hash_list->n_buckets * 2, 0); 
	
	// link new buckets to old
	for (i = 0; i < p_old_hash_list->n_buckets; i++) {
		id_1 = i;
		id_2 = i + p_old_hash_list->n_buckets;
		
		p_list_1 = p_old_hash_list->buckets[id_1]->p_list;
		
		p_new_hash_list->buckets[id_1]->p_list = p_list_1;
		p_new_hash_list->buckets[id_2]->p_list = p_list_1;

		p_new_hash_list->buckets[id_1]->p_unzip_node = NULL;
		p_new_hash_list->buckets[id_2]->p_unzip_node = NULL;
		
		p_new_hash_list->buckets[id_1]->is_zipped = 1;
		p_new_hash_list->buckets[id_2]->is_zipped = 1;
	}
	
	*p_p_cur_hash_list = p_new_hash_list;
	MEMBARSTLD();
	urcu_synchronize();
	
	/* Unzip the buckets */
	do {
		
		moved_one = 0;
		
		for (i = 0; i < p_old_hash_list->n_buckets; i++) {
			id_1 = i;
			id_2 = i + p_old_hash_list->n_buckets;
			
			p_bucket_1 = p_new_hash_list->buckets[id_1];
			p_bucket_2 = p_new_hash_list->buckets[id_2];
			
			if (!p_bucket_1->is_zipped) {
				continue;
			}
			
			while (!try_lock_bucket(p_bucket_1)) { CPU_RELAX(); }
			while (!try_lock_bucket(p_bucket_2)) { CPU_RELAX(); }
			
			if (!p_bucket_1->is_zipped) {
				unlock_bucket(p_bucket_2);
				unlock_bucket(p_bucket_1);
				continue;
			}
			
			p_list_1 = p_new_hash_list->buckets[id_1]->p_list;
			p_list_2 = p_new_hash_list->buckets[id_2]->p_list;
			
			if (p_list_1 == p_list_2) {
				// split head				
				id_temp = id_2;
				p_node = p_list_1->p_head;
				
				if ((p_node != NULL) && (HASH_VALUE(p_new_hash_list, p_node->val) != id_1)) {
					id_temp = id_1;
				}
				
				p_node_prev = NULL;
				while ((p_node != NULL) && (HASH_VALUE(p_new_hash_list, p_node->val) != id_temp)) {
					p_node_prev = p_node;
					p_node = p_node->p_next;
				}

				p_list_new = rcu_new_list();
				p_list_new->p_head = p_node;
				
				p_new_hash_list->buckets[id_temp]->p_list = p_list_new;
				
				p_bucket_1->p_unzip_node = p_node_prev;
				p_bucket_2->p_unzip_node = p_node_prev;
				
				if (p_bucket_1->p_unzip_node == NULL) {
					p_bucket_1->is_zipped = 0;
					p_bucket_2->is_zipped = 0;
				} else {
					moved_one = 1;
				}
				
				unlock_bucket(p_bucket_2);
				unlock_bucket(p_bucket_1);
				continue;
			}
			
			p_unzip_node = (node_t *)p_bucket_1->p_unzip_node;
			
			if (p_unzip_node == NULL) {
				abort();
				
				is_source_head = 1;
				
				if (p_list_1->p_head == NULL) {
					p_bucket_1->is_zipped = 0;
					p_bucket_2->is_zipped = 0;
					unlock_bucket(p_bucket_2);
					unlock_bucket(p_bucket_1);
					continue;
				}
				
				p_unzip_node = p_list_1->p_head;
				
			} else {
				is_source_head = 0;
				
				p_node_source = p_unzip_node;
				
				p_unzip_node = p_unzip_node->p_next;
				
			}
			
			p_unzip_node = rcu_find_unzip_point(p_new_hash_list, p_unzip_node);
			
			if (p_unzip_node == NULL) {
				p_bucket_1->p_unzip_node = NULL;
				p_bucket_2->p_unzip_node = NULL;
				p_bucket_1->is_zipped = 0;
				p_bucket_2->is_zipped = 0;
				
				if (is_source_head) {
					bin = HASH_VALUE(p_new_hash_list, p_list_1->p_head->val);
					bin = GET_PAIR_BUCKET(p_new_hash_list, bin);
					p_new_hash_list->buckets[bin]->p_list->p_head = NULL;
				} else {
					p_node_source->p_next = NULL;
				}
				
			} else {
				moved_one = 1;
				
				if (is_source_head) {
					id_unzip = HASH_VALUE(p_new_hash_list, p_unzip_node->p_next->val);
					p_new_hash_list->buckets[id_unzip]->p_list->p_head = p_unzip_node->p_next;
					
				} else {
					p_node_source->p_next = p_unzip_node->p_next;
				}
				
				p_bucket_1->p_unzip_node = p_unzip_node;
				p_bucket_2->p_unzip_node = p_unzip_node;
				
			}
					
			unlock_bucket(p_bucket_2);
			unlock_bucket(p_bucket_1);
			
		}
		MEMBARSTLD();

		urcu_synchronize();
		
	} while (moved_one);
	
	for (i = 0; i < p_old_hash_list->n_buckets; i++) {
		free(p_old_hash_list->buckets[i]);
	}
	free(p_old_hash_list->buckets);
	free((void *)p_old_hash_list);
}

int rlu_hash_list_expand(rlu_thread_data_t *self, hash_list_t *p_hash_list) {
	int i = 0;
	int id_1,id_2,id_temp;
	node_t *p_node = NULL;
	node_t *p_node_prev = NULL;
	node_t *p_node_source = NULL;
	bucket_t *p_bucket = NULL;
	list_t *p_new_list = NULL;
	list_t *p_list = NULL;
	list_t *p_list_new = NULL;
	volatile hash_list_t *p_cur_hash_list = NULL;
	bucket_t **new_buckets = NULL;
	
	p_cur_hash_list = p_hash_list;
			
	new_buckets = rlu_new_buckets(p_cur_hash_list->n_buckets * 2, 0); 
	
	// link new buckets to old
	for (i = 0; i < p_cur_hash_list->n_buckets; i++) {
		id_1 = i;
		id_2 = i + p_cur_hash_list->n_buckets;
				
		p_cur_hash_list = p_hash_list;
		
		RLU_READER_LOCK(self);
		
		p_bucket = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[id_1]);
		
		RLU_ASSIGN_PTR(self, &new_buckets[id_1]->p_list, p_bucket->p_list);
		RLU_ASSIGN_PTR(self, &new_buckets[id_2]->p_list, p_bucket->p_list);
		
		new_buckets[id_1]->is_zipped = 1;
		new_buckets[id_2]->is_zipped = 1;
		
		RLU_READER_UNLOCK(self);
	}
	
	RLU_READER_LOCK(self);
	
	if (!RLU_TRY_WRITER_LOCK(self, RLU_GENERAL_WRITER_LOCK)) {
		abort();
	}
	
	p_cur_hash_list = RLU_DEREF_HASH_LIST(self, p_hash_list);
	
	RLU_LOCK(self, &p_cur_hash_list);

	rlu_free_buckets(self, p_cur_hash_list->buckets, p_cur_hash_list->n_buckets);
	
	p_cur_hash_list->n_buckets = p_cur_hash_list->n_buckets * 2;
	p_cur_hash_list->buckets = new_buckets;
	
	RLU_READER_UNLOCK(self);
	
	p_cur_hash_list = p_hash_list;
	
	/* Unzip the buckets */
	for (i = 0; i < (p_cur_hash_list->n_buckets / 2); i++) {
		id_1 = i;
		id_2 = i + (p_cur_hash_list->n_buckets / 2);
		
restart_unzip:		
		RLU_READER_LOCK(self);
		
		if (!RLU_TRY_WRITER_LOCK(self, id_1) || !RLU_TRY_WRITER_LOCK(self, id_2)) { 
			RLU_ABORT(self);
			CPU_RELAX();
			goto restart_unzip; 
		}
				
		p_bucket = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[id_1]);
		
		p_list = RLU_DEREF_LIST(self, p_bucket->p_list);		
		
		// split head
		id_temp = id_2;
		p_node = RLU_DEREF_NODE(self, p_list->p_head);
		
		if ((p_node != NULL) && (HASH_VALUE(p_cur_hash_list, p_node->val) != id_1)) {
			id_temp = id_1;
		}
		
		p_node_prev = NULL;
		while ((p_node != NULL) && (HASH_VALUE(p_cur_hash_list, p_node->val) != id_temp)) {
			p_node_prev = p_node;
			p_node = RLU_DEREF_NODE(self, p_node->p_next);
		}
		
		p_list_new = rlu_new_list();
		RLU_ASSIGN_PTR(self, &(p_list_new->p_head), p_node);
		
		p_bucket = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[id_temp]);
		
		RLU_LOCK(self, &p_bucket);
		RLU_ASSIGN_PTR(self, &p_bucket->p_list, p_list_new); 
		
		while (p_node != NULL) {
		
			p_node_source = p_node_prev;
				
			p_node = RLU_DEREF_NODE(self, p_node_prev->p_next);
			while ((p_node != NULL) && 
			       (HASH_VALUE(p_cur_hash_list, p_node_source->val) != HASH_VALUE(p_cur_hash_list, p_node->val))) {
				p_node_prev = p_node;
				p_node = RLU_DEREF_NODE(self, p_node->p_next);
			}
		
			RLU_LOCK(self, &p_node_source);
			RLU_ASSIGN_PTR(self, &(p_node_source->p_next), p_node);
			
		}
		
		p_bucket = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[id_1]);
		RLU_LOCK(self, &p_bucket);
		p_bucket->is_zipped = 0;
			
		p_bucket = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[id_2]);
		RLU_LOCK(self, &p_bucket);
		p_bucket->is_zipped = 0;
		
		RLU_READER_UNLOCK(self);
		
	}
	
}

node_t *zip_lists(node_t *p_node_1, node_t *p_node_2) {
	node_t *p_node_res;
	
	if (p_node_1 == NULL) {
		return p_node_2;
	}
	
	if (p_node_2 == NULL) {
		return p_node_1;
	}
	
	if (p_node_1->val < p_node_2->val) {
		p_node_res = zip_lists(p_node_1->p_next, p_node_2);
		
		p_node_1->p_next = p_node_res;
		
		return p_node_1;
		
	} else {
		p_node_res = zip_lists(p_node_1, p_node_2->p_next);
		p_node_2->p_next = p_node_res;
		
		return p_node_2;
	}
	
}

int rcu_hash_list_shrink(hash_list_t **p_p_hash_list) {
	int id_1, id_2;
	int i = 0;
	int new_n_buckets = 0;
	node_t *p_node = NULL;
	bucket_t *p_bucket_1 = NULL;
	bucket_t *p_bucket_2 = NULL;
	volatile hash_list_t *p_old_hash_list = NULL;
	volatile hash_list_t *p_new_hash_list = NULL;
	volatile hash_list_t **p_p_cur_hash_list = NULL;
	list_t *free_lists[MAX_BUCKETS];
	
	p_p_cur_hash_list = (volatile hash_list_t **)p_p_hash_list;
	
	p_old_hash_list = *p_p_cur_hash_list; 
	
	new_n_buckets = p_old_hash_list->n_buckets / 2;
	
	// zip buckets
	for (i = 0; i < new_n_buckets; i++) {
		id_1 = i;
		id_2 = i + new_n_buckets;
		
		p_bucket_1 = p_old_hash_list->buckets[id_1];
		p_bucket_2 = p_old_hash_list->buckets[id_2];
		
		while (!try_lock_bucket(p_bucket_1)) { CPU_RELAX(); }
		while (!try_lock_bucket(p_bucket_2)) { CPU_RELAX(); }
		
		p_node = zip_lists(p_bucket_1->p_list->p_head, p_bucket_2->p_list->p_head);
		
		p_bucket_1->p_list->p_head = p_node;
		
		free_lists[id_1] = p_bucket_2->p_list;
		
		p_bucket_2->p_list = p_bucket_1->p_list;
		
		p_bucket_1->is_zipped = 1;
		p_bucket_2->is_zipped = 1;
				
		unlock_bucket(p_bucket_2);
		unlock_bucket(p_bucket_1);
	}
	
	// alloc new table
	
	p_new_hash_list = pure_new_hash_list(new_n_buckets, 0);
	
	// link new buckets to old
	for (i = 0; i < new_n_buckets; i++) {
		p_new_hash_list->buckets[i]->p_list = p_old_hash_list->buckets[i]->p_list;
	}
	
	*p_p_cur_hash_list = p_new_hash_list;
	MEMBARSTLD();
	urcu_synchronize();
	
	for (i = 0; i < new_n_buckets; i++) {
		free((void *)free_lists[i]);
		free(p_old_hash_list->buckets[i]);
	}
	
	free(p_old_hash_list->buckets);
	free((void *)p_old_hash_list);
}

void rlu_zip_lists(rlu_thread_data_t *self, node_t *p_node_1, node_t *p_node_2, node_t **p_result) {
	node_t *p_node_res;
	
	if (p_node_1 == NULL) {
		*p_result = p_node_2;
		return;
	}
	
	if (p_node_2 == NULL) {
		*p_result = p_node_1;
		return;
	}
	
	if (p_node_1->val < p_node_2->val) {
		rlu_zip_lists(self, RLU_DEREF_NODE(self, p_node_1->p_next), p_node_2, &p_node_res);
		
		RLU_LOCK(self, &p_node_1);
		
		RLU_ASSIGN_PTR(self, &(p_node_1->p_next), p_node_res);

		*p_result = p_node_1;
		
	} else {
		rlu_zip_lists(self, p_node_1, RLU_DEREF_NODE(self, p_node_2->p_next), &p_node_res);
		
		RLU_LOCK(self, &p_node_2);
		
		RLU_ASSIGN_PTR(self, &(p_node_2->p_next), p_node_res);
		
		*p_result = p_node_2;
	}
	
}

int rlu_hash_list_shrink(rlu_thread_data_t *self, hash_list_t *p_hash_list) {
	int id_1,id_2;
	int i = 0;
	int new_n_buckets = 0;
	node_t *p_node = NULL;
	bucket_t *p_bucket_1 = NULL; 
	bucket_t *p_bucket_2 = NULL; 	
	list_t *p_list_1 = NULL; 
	list_t *p_list_2 = NULL; 
	node_t *p_head_1 = NULL; 
	node_t *p_head_2 = NULL; 
	volatile hash_list_t *p_cur_hash_list = NULL;
	bucket_t **new_buckets = NULL;
		
	p_cur_hash_list = p_hash_list;
		
	new_n_buckets = p_cur_hash_list->n_buckets / 2;	
	
	// alloc new table
	new_buckets = rlu_new_buckets(new_n_buckets, 0);
	
	// zip buckets
	for (i = 0; i < new_n_buckets; i++) {
		id_1 = i;
		id_2 = i + new_n_buckets;
		
		p_cur_hash_list = p_hash_list;

restart_zip:		
		RLU_READER_LOCK(self);

		if (!RLU_TRY_WRITER_LOCK(self, id_1) || !RLU_TRY_WRITER_LOCK(self, id_2)) { 
			RLU_ABORT(self);
			CPU_RELAX();
			goto restart_zip; 
		}
		
		p_bucket_1 = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[id_1]);
		p_bucket_2 = RLU_DEREF_BUCKET(self, p_cur_hash_list->buckets[id_2]);
		
		p_list_1 = RLU_DEREF_LIST(self, p_bucket_1->p_list);
		p_list_2 = RLU_DEREF_LIST(self, p_bucket_2->p_list);
		
		p_head_1 = RLU_DEREF_NODE(self, p_list_1->p_head);
		p_head_2 = RLU_DEREF_NODE(self, p_list_2->p_head);
						
		rlu_zip_lists(self, p_head_1, p_head_2, &p_node);

		RLU_LOCK(self, &p_list_1);
		RLU_ASSIGN_PTR(self, &(p_list_1->p_head), p_node);
		
		RLU_LOCK(self, &p_bucket_1);
		RLU_LOCK(self, &p_bucket_2);
		
		RLU_ASSIGN_PTR(self, &p_bucket_2->p_list, p_list_1);
		
		p_bucket_1->is_zipped = 1;
		p_bucket_2->is_zipped = 1; 
		
		RLU_FREE(self, p_list_2);
		
		RLU_ASSIGN_PTR(self, &new_buckets[id_1]->p_list, p_list_1);
						
		RLU_READER_UNLOCK(self);
				
	}
	
	// install new table and free the old one
	
	RLU_READER_LOCK(self);
	
	if (!RLU_TRY_WRITER_LOCK(self, RLU_GENERAL_WRITER_LOCK)) {
		abort();
	}
	
	p_cur_hash_list = RLU_DEREF_HASH_LIST(self, p_hash_list);
	
	RLU_LOCK(self, &p_cur_hash_list);

	rlu_free_buckets(self, p_cur_hash_list->buckets, p_cur_hash_list->n_buckets);
	
	p_cur_hash_list->n_buckets = new_n_buckets;
	p_cur_hash_list->buckets = new_buckets;
	
	RLU_READER_UNLOCK(self);

}

/////////////////////////////////////////////////////////
// HASH LIST STATS
/////////////////////////////////////////////////////////
void hash_list_print_stats(hash_list_t *p_hash_list) {
	int i;
	int size;
	
	printf("========================================\n");
	printf("n_buckets = %d\n", p_hash_list->n_buckets);
	printf("========================================\n");
	for (i = 0; i < p_hash_list->n_buckets; i++) {
		size = list_size(p_hash_list->buckets[i]->p_list);
		printf("bucket[%d][%d] = ", i, size);
		list_print(p_hash_list->buckets[i]->p_list);
		printf("\n");
	}
	printf("========================================\n");
	
}