/**
 * This file defines the methods declared in heap.h
 * These are used to create and manipulate a heap
 * data structure.
 */

/* From;
 * https://github.com/armon/c-minheap-array
 * */

#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "heap.h"

#include <glib-2.0/glib.h>

// Helpful Macros
#define LEFT_CHILD(i)   ((i<<1)+1)
#define RIGHT_CHILD(i)  ((i<<1)+2)
#define PARENT_ENTRY(i) ((i-1)>>1)
#define SWAP_ENTRIES(parent,child)  { \
                                      int temp = parent->key; \
                                      parent->key = child->key;          \
                                      child->key = temp;                 \
                                      temp = parent->value;              \
                                      parent->value = child->value;      \
                                      child->value = temp;               \
                                    }

#define GET_ENTRY(index,table) ((heap_entry*)(table+index))


// This is a comparison function that treats keys as signed ints
int compare_int_keys(int key1_v, int key2_v) {

    // Perform the comparison
    if (key1_v < key2_v)
        return -1;
    else if (key1_v == key2_v)
        return 0;
    else
        return 1;
}


// Creates a new heap
void heap_create(heap* h, int initial_size) {

    // Set active entries to 0
    h->active_entries = 0;

    // Determine how many pages of entries we need
    h->allocated_bytes = 1024*sizeof(heap_entry);

    // Allocate the table
    h->table = calloc(1, h->allocated_bytes);;
}


// Cleanup a heap
void heap_destroy(heap* h) {
    // Check that h is not null
    assert(h != NULL);

    // Map out the table
    free(h->table);

    // Clear everything
    h->active_entries = 0;
    h->allocated_bytes = 0;
    h->table = NULL;
}


// Gets the size of the heap
int heap_size(heap* h) {
    // Return the active entries
    return h->active_entries;
}


// Gets the minimum element
int heap_min(heap* h, int *key, int *value) {
    // Check the number of elements, abort if 0
    if (h->active_entries == 0)
        return 0;

    // Get the 0th element
    heap_entry* root = GET_ENTRY(0, h->table);

    // Set the key and value
    *key = root->key;
    *value = root->value;

    // Success
    return 1;
}


// Insert a new element
void heap_insert(heap *h, int key, int value) {
    // Check if this heap is not destoyed
    assert(h->table != NULL);

    // Check if we have room
    int max_entries = h->allocated_bytes / sizeof(heap_entry);
    if (h->active_entries + 1 > max_entries) {
        // Get the new number of entries we need
        h->allocated_bytes *= 2;
        h->table = realloc(h->table, h->allocated_bytes);
    }
    
    // Store the table address
    heap_entry* table = h->table;

    // Get the current index
    int current_index = h->active_entries;
    heap_entry* current = GET_ENTRY(current_index, table);

    // Loop variables
    int parent_index;
    heap_entry *parent;

    // While we can, keep swapping with our parent
    while (current_index > 0) {
        // Get the parent index
        parent_index = PARENT_ENTRY(current_index);

        // Get the parent entry
        parent = GET_ENTRY(parent_index, table);
       
        // Compare the keys, and swap if we need to 
        if (compare_int_keys(key, parent->key) < 0) {
            // Move the parent down
            current->key = parent->key;
            current->value = parent->value;

            // Move our reference
            current_index = parent_index;
            current = parent;

        // We are done swapping
        }   else
            break;
    }

    // Insert at the current index
    current->key = key;
    current->value = value; 

    // Increase the number of active entries
    h->active_entries++;
}


// Deletes the minimum entry in the heap
int heap_delmin(heap* h, int *key, int *value) {
    // Check there is a minimum
    if (h->active_entries == 0)
        return 0;

    // Load in the map table
    heap_entry* table = h->table;

    // Get the root element
    int current_index = 0;
    heap_entry* current = GET_ENTRY(current_index, table);

    // Store the outputs
    *key = current->key;
    *value = current->value;

    // Reduce the number of active entries
    h->active_entries--;

    // Get the active entries
    int entries = h->active_entries;
   
    // If there are any other nodes, we may need to move them up
    if (h->active_entries > 0) {
        // Move the last element to the root
        heap_entry* last = GET_ENTRY(entries,table);
        current->key = last->key;
        current->value = last->value;

        // Loop variables
        heap_entry* left_child;
        heap_entry* right_child;

        // Store the left index
        int left_child_index;

        while (left_child_index = LEFT_CHILD(current_index), left_child_index < entries) {
            // Load the left child
            left_child = GET_ENTRY(left_child_index, table);

            // We have a left + right child
            if (left_child_index+1 < entries) {
                // Load the right child
                right_child = GET_ENTRY((left_child_index+1), table);

                // Find the smaller child
                if (compare_int_keys(left_child->key, right_child->key) <= 0) {

                    // Swap with the left if it is smaller
                    if (compare_int_keys(current->key, left_child->key) == 1) {
                        SWAP_ENTRIES(current,left_child);
                        current_index = left_child_index;
                        current = left_child;

                    // Otherwise, the current is smaller
                    } else
                        break;

                // Right child is smaller
                } else {

                    // Swap with the right if it is smaller
                    if (compare_int_keys(current->key, right_child->key) == 1) {
                        SWAP_ENTRIES(current,right_child);
                        current_index = left_child_index+1;
                        current = right_child;

                    // Current is smaller
                    } else
                        break;

                }


            // We only have a left child, only do something if the left is smaller
            } else if (compare_int_keys(current->key, left_child->key) == 1) {
                SWAP_ENTRIES(current,left_child);
                current_index = left_child_index;
                current = left_child;

            // Done otherwise
            }  else
                break;

        }
    } 

    // Success
    return 1;
}

