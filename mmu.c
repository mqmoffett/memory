#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

void TOUPPER(char *arr) {
    for (int i = 0; arr[i] != '\0'; i++) {
        arr[i] = (char)toupper((unsigned char)arr[i]);
    }
}

void get_input(char *args[], int input[][2], int *n, int *size, int *policy) {
    FILE *input_file = fopen(args[1], "r");
    if (!input_file) {
        fprintf(stderr, "Error: Invalid filepath\n");
        fflush(stdout);
        exit(0);
    }
    parse_file(input_file, input, n, size);
    fclose(input_file);
    TOUPPER(args[2]);

    if (strcmp(args[2], "-F") == 0 || strcmp(args[2], "-FIFO") == 0)
        *policy = 1;
    else if (strcmp(args[2], "-B") == 0 || strcmp(args[2], "-BESTFIT") == 0)
        *policy = 2;
    else if (strcmp(args[2], "-W") == 0 || strcmp(args[2], "-WORSTFIT") == 0)
        *policy = 3;
    else {
        printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
        exit(1);
    }
}

// Allocate using FIFO, best fit, or worst fit.
void allocate_memory(list_t *freelist, list_t *alloclist, int pid, int blocksize, int policy) {
    int chosen_index = -1;
    int chosen_size = 0;
    int index = 0;
    node_t *current = freelist->head;

    if (blocksize <= 0) {
        printf("Error: Not Enough Memory\n");
        return;
    }

    // Scan rather than relying on free-list order: coalescing sorts by address.
    while (current != NULL) {
        int size = current->blk->end - current->blk->start + 1;
        if (size >= blocksize) {
            if (policy == 1) { // FIFO: first suitable block in free-list order
                chosen_index = index;
                break;
            }
            if (chosen_index == -1 ||
                (policy == 2 && size < chosen_size) ||
                (policy == 3 && size > chosen_size)) {
                chosen_index = index;
                chosen_size = size;
            }
        }
        current = current->next;
        index++;
    }

    if (chosen_index == -1) {
        printf("Error: Memory Allocation %d blocks\n", blocksize);
        return;
    }

    block_t *blk = list_remove_at_index(freelist, chosen_index);
    int old_end = blk->end;
    blk->pid = pid;
    blk->end = blk->start + blocksize - 1;
    list_add_ascending_by_address(alloclist, blk);

    // Reinsert the unallocated remainder, if any.
    if (blk->end < old_end) {
        block_t *fragment = (block_t *)malloc(sizeof(block_t));
        if (fragment == NULL) exit(1);
        fragment->pid = 0;
        fragment->start = blk->end + 1;
        fragment->end = old_end;

        if (policy == 1) list_add_to_back(freelist, fragment);
        else if (policy == 2) list_add_ascending_by_blocksize(freelist, fragment);
        else list_add_descending_by_blocksize(freelist, fragment);
    }
}
// Return a PID's allocated block to the appropriate free list.
void deallocate_memory(list_t *alloclist, list_t *freelist, int pid, int policy) {
    int index = list_get_index_of_by_Pid(alloclist, pid);
    if (index == -1) {
        printf("Error: Can't locate Memory Used by PID: %d\n", pid);
        return;
    }

    block_t *blk = list_remove_at_index(alloclist, index);
    blk->pid = 0;

    if (policy == 1) list_add_to_back(freelist, blk);
    else if (policy == 2) list_add_ascending_by_blocksize(freelist, blk);
    else list_add_descending_by_blocksize(freelist, blk);
}

list_t *coalese_memory(list_t *list) {
    list_t *temp_list = list_alloc();
    block_t *blk;
    while ((blk = list_remove_from_front(list)) != NULL) {
        list_add_ascending_by_address(temp_list, blk);
    }
    list_coalese_nodes(temp_list);
    list_free(list); // the old list header is now empty
    return temp_list;
}

void print_list(list_t *list, char *message) {
    node_t *current = list->head;
    block_t *blk;
    int i = 0;
    printf("%s:\n", message);
    while (current != NULL) {
        blk = current->blk;
        printf("Block %d:\t START: %d\t END: %d", i, blk->start, blk->end);
        if (blk->pid != 0) printf("\t PID: %d\n", blk->pid);
        else printf("\n");
        current = current->next;
        i++;
    }
}

/* DO NOT MODIFY */
int main(int argc, char *argv[]) {
    int PARTITION_SIZE, inputdata[200][2], N = 0, Memory_Mgt_Policy;
    list_t *FREE_LIST = list_alloc();
    list_t *ALLOC_LIST = list_alloc();
    int i;

    if (argc != 3) {
        printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
        exit(1);
    }

    get_input(argv, inputdata, &N, &PARTITION_SIZE, &Memory_Mgt_Policy);

    block_t *partition = malloc(sizeof(block_t));
    partition->start = 0;
    partition->end = PARTITION_SIZE + partition->start - 1;
    list_add_to_front(FREE_LIST, partition);

    for (i = 0; i < N; i++) {
        printf("************************\n");
        if (inputdata[i][0] != -99999 && inputdata[i][0] > 0) {
            printf("ALLOCATE: %d FROM PID: %d\n", inputdata[i][1], inputdata[i][0]);
            allocate_memory(FREE_LIST, ALLOC_LIST, inputdata[i][0], inputdata[i][1], Memory_Mgt_Policy);
        } else if (inputdata[i][0] != -99999 && inputdata[i][0] < 0) {
            printf("DEALLOCATE MEM: PID %d\n", abs(inputdata[i][0]));
            deallocate_memory(ALLOC_LIST, FREE_LIST, abs(inputdata[i][0]), Memory_Mgt_Policy);
        } else {
            printf("COALESCE/COMPACT\n");
            FREE_LIST = coalese_memory(FREE_LIST);
        }

        printf("************************\n");
        print_list(FREE_LIST, "Free Memory");
        print_list(ALLOC_LIST, "\nAllocated Memory");
        printf("\n\n");
    }

    list_free(FREE_LIST);
    list_free(ALLOC_LIST);
    return 0;
}