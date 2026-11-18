// list/list.c
// Implementation for linked list.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

list_t *list_alloc() {
    list_t *list = malloc(sizeof(list_t));

    if (list == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    list->head = NULL;
    return list;
}

node_t *node_alloc(block_t *blk) {
    node_t *node = malloc(sizeof(node_t));

    if (node == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    node->next = NULL;
    node->blk = blk;
    return node;
}
// Frees the list and all memory used by its nodes and blocks.
void list_free(list_t *l) {
    if (l == NULL)
        return;

    node_t *current = l->head;

    while (current != NULL) {
        node_t *next = current->next;

        free(current->blk);
        free(current);

        current = next;
    }

    free(l);
}

void node_free(node_t *node) {
    free(node);
}

void list_print(list_t *l) {
    node_t *current = l->head;
    if (current == NULL) printf("list is empty\n");
    while (current != NULL) {
        block_t *b = current->blk;
        printf("PID=%d START:%d END:%d", b->pid, b->start, b->end);
        current = current->next;
    }
}

int list_length(list_t *l) {
    int i = 0;
    node_t *current = l->head;
    while (current != NULL) {
        i++;
        current = current->next;
    }
    return i;
}

void list_add_to_back(list_t *l, block_t *blk) {
    node_t *newNode = node_alloc(blk);
    if (l->head == NULL) {
        l->head = newNode;
    } else {
        node_t *current = l->head;
        while (current->next != NULL) current = current->next;
        current->next = newNode;
    }
}

void list_add_to_front(list_t *l, block_t *blk) {
    node_t *newNode = node_alloc(blk);
    newNode->next = l->head;
    l->head = newNode;
}

void list_add_at_index(list_t *l, block_t *blk, int index) {
    if (index < 0) return;
    node_t **place = &l->head;
    int i = 0;
    while (*place != NULL && i < index) {
        place = &(*place)->next;
        i++;
    }
    node_t *newNode = node_alloc(blk);
    newNode->next = *place;
    *place = newNode;
}

// Keep blocks sorted by their starting addresses, smallest first.
void list_add_ascending_by_address(list_t *l, block_t *newblk) {
    node_t **place = &l->head;
    while (*place != NULL && (*place)->blk->start <= newblk->start) {
        place = &(*place)->next;
    }
    node_t *newNode = node_alloc(newblk);
    newNode->next = *place;
    *place = newNode;
}

// Keep blocks sorted from smallest to largest. Equal-sized blocks stay in arrival order.
void list_add_ascending_by_blocksize(list_t *l, block_t *newblk) {
    node_t **place = &l->head;
    while (*place != NULL &&
      compareSize((*place)->blk->end - (*place)->blk->start + 1, newblk)) {
        place = &(*place)->next;
    }
    node_t *newNode = node_alloc(newblk);
    newNode->next = *place;
    *place = newNode;
}

// Keep blocks sorted from largest to smallest. Equal-sized blocks stay in arrival order.
void list_add_descending_by_blocksize(list_t *l, block_t *blk) {
    node_t **place = &l->head;
    int newsize = blk->end - blk->start + 1;
    while (*place != NULL && compareSize(newsize, (*place)->blk)) {
        place = &(*place)->next;
    }
    node_t *newNode = node_alloc(blk);
    newNode->next = *place;
    *place = newNode;
}

// Join physically adjacent free blocks in an address-sorted list.
void list_coalese_nodes(list_t *l) {
    node_t *current = l->head;
    while (current != NULL && current->next != NULL) {
        node_t *nextNode = current->next;
        // Check if the two memory blocks are next to each other.
        if (current->blk->end + 1 == nextNode->blk->start) {
            current->blk->end = nextNode->blk->end;
            current->next = nextNode->next;
            // Free the block and node that were merged.
            free(nextNode->blk);
            node_free(nextNode);
        } else {
            current = current->next;
        }
    }
}

block_t *list_remove_from_back(list_t *l) {
    if (l->head == NULL) return NULL;
    node_t *current = l->head;
    node_t *prev = NULL;
    while (current->next != NULL) {
        prev = current;
        current = current->next;
    }
    if (prev == NULL) l->head = NULL;
    else prev->next = NULL;
    block_t *value = current->blk;
    node_free(current);
    return value;
}

block_t *list_get_from_front(list_t *l) {
    return l->head == NULL ? NULL : l->head->blk;
}

block_t *list_remove_from_front(list_t *l) {
    if (l->head == NULL) return NULL;
    node_t *current = l->head;
    block_t *value = current->blk;
    l->head = current->next;
    node_free(current);
    return value;
}

block_t *list_remove_at_index(list_t *l, int index) {
    if (index < 0) return NULL;
    node_t **place = &l->head;
    for (int i = 0; i < index && *place != NULL; i++) {
        place = &(*place)->next;
    }
    if (*place == NULL) return NULL;
    node_t *current = *place;
    block_t *value = current->blk;
    *place = current->next;
    node_free(current);
    return value;
}

bool compareBlks(block_t *a, block_t *b) {
    return a->pid == b->pid && a->start == b->start && a->end == b->end;
}

bool compareSize(int a, block_t *b) {
    return a <= b->end - b->start + 1;
}

bool comparePid(int a, block_t *b) {
    return a == b->pid;
}

bool list_is_in(list_t *l, block_t *value) {
    node_t *current = l->head;
    while (current != NULL) {
        if (compareBlks(value, current->blk)) return true;
        current = current->next;
    }
    return false;
}

block_t *list_get_elem_at(list_t *l, int index) {
    if (index < 0) return NULL;
    node_t *current = l->head;
    for (int i = 0; current != NULL; i++, current = current->next) {
        if (i == index) return current->blk;
    }
    return NULL;
}

int list_get_index_of(list_t *l, block_t *value) {
    int i = 0;
    node_t *current = l->head;
    while (current != NULL) {
        if (compareBlks(value, current->blk)) return i;
        current = current->next;
        i++;
    }
    return -1;
}
bool list_is_in_by_size(list_t *l, int Size) {
    node_t *current = l->head;
    while (current != NULL) {
        if (compareSize(Size, current->blk)) return true;
        current = current->next;
    }
    return false;
}

// Check whether a process ID exists in the allocated list.
bool list_is_in_by_pid(list_t *l, int pid) {
    if (l == NULL)
        return false;

    node_t *current = l->head;

    while (current != NULL) {
        if (comparePid(pid, current->blk))
            return true;

        current = current->next;
    }

    return false;
}

int list_get_index_of_by_Size(list_t *l, int Size) {
    int i = 0;
    node_t *current = l->head;
    while (current != NULL) {
        if (compareSize(Size, current->blk)) return i;
        current = current->next;
        i++;
    }
    return -1;
}

int list_get_index_of_by_Pid(list_t *l, int pid) {
    int i = 0;
    node_t *current = l->head;
    while (current != NULL) {
        if (comparePid(pid, current->blk)) return i;
        current = current->next;
        i++;
    }
    return -1;
}