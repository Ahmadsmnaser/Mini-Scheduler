#ifndef CONFIG_H
#define CONFIG_H

#include "task.h"

#define MAX_TASKS 128

typedef struct {
    char policy[16];
    long quantum;
    int task_count;
    task_t tasks[MAX_TASKS];
} config_t;

int config_load(const char *path, config_t *config);

#endif
