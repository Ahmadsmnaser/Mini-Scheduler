#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "compare.h"
#include "config.h"

static int stream_contains(FILE *stream, const char *needle)
{
    char line[256];

    rewind(stream);
    while (fgets(line, sizeof(line), stream) != NULL) {
        if (strstr(line, needle) != NULL) {
            return 1;
        }
    }

    return 0;
}

int main(void)
{
    config_t config;
    FILE *stream;

    assert(config_load("configs/fair_simple.json", &config) == 0);

    stream = tmpfile();
    assert(stream != NULL);

    assert(compare_run(stream, &config) == 0);
    assert(stream_contains(stream, "Comparison"));
    assert(stream_contains(stream, "policy_rr"));
    assert(stream_contains(stream, "policy_fair"));
    assert(stream_contains(stream, "task=1"));
    assert(stream_contains(stream, "task=2"));
    assert(stream_contains(stream, "task=3"));

    fclose(stream);
    return 0;
}
