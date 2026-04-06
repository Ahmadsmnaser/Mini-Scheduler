#include <assert.h>
#include <string.h>

#include "config.h"

int main(void)
{
    config_t config;

    assert(config_load("configs/rr_simple.json", &config) == 0);
    assert(strcmp(config.policy, "rr") == 0);
    assert(config.quantum == 4);
    assert(config.task_count == 3);
    assert(config.tasks[0].id == 1);
    assert(strcmp(config.tasks[0].name, "A") == 0);
    assert(config.tasks[0].arrival == 0);
    assert(config.tasks[0].remaining == 12);
    assert(config.tasks[2].id == 3);
    assert(strcmp(config.tasks[2].name, "C") == 0);

    assert(config_load("configs/fair_simple.json", &config) == 0);
    assert(strcmp(config.policy, "fair") == 0);
    assert(config.task_count == 3);
    assert(config.tasks[1].arrival == 3);
    assert(config.tasks[1].remaining == 8);
    assert(config.tasks[1].nice == 5);
    assert(config.tasks[2].nice == -3);

    return 0;
}
