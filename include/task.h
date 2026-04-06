#ifndef TASK_H
#define TASK_H

enum task_state {
    TASK_RUNNABLE = 0,
    TASK_RUNNING,
    TASK_DONE
};

typedef struct task {
    int id;
    char name[32];
    int state;
    int nice;
    long remaining;
    double vruntime;
    long arrival;
    long first_run;
    long finish;
    long wait_start;
    long executed;
    struct task *next;
} task_t;

#endif
