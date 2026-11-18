#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "util.h"
#include "list.h"

void parse_file(FILE *f, int input[][2], int *n, int *PARTITION_SIZE) {
    if (fscanf(f, "%d", PARTITION_SIZE) != 1) {
        fprintf(stderr, "Error: Invalid input file\n");
        exit(1);
    }
    printf("PARTITION_SIZE = %d\n", *PARTITION_SIZE);
// Read the requests while keeping the input within the 200-entry limit.
    while (*n < 200 && fscanf(f, "%d %d", &input[*n][0], &input[*n][1]) == 2) {
        (*n)++;
    }
}
