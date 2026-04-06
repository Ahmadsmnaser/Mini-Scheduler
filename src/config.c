#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_file(const char *path, char **buffer_out)
{
    FILE *file;
    long size;
    size_t bytes_read;
    char *buffer;

    file = fopen(path, "rb");
    if (file == NULL) {
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 1;
    }

    size = ftell(file);
    if (size < 0) {
        fclose(file);
        return 1;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 1;
    }

    buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return 1;
    }

    bytes_read = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    if (bytes_read != (size_t)size) {
        free(buffer);
        return 1;
    }

    buffer[size] = '\0';
    *buffer_out = buffer;
    return 0;
}

static const char *skip_ws(const char *p)
{
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }

    return p;
}

static int parse_quoted_string(const char **cursor, char *dest, size_t dest_size)
{
    const char *start;
    size_t length;

    *cursor = skip_ws(*cursor);
    if (**cursor != '"') {
        return 1;
    }

    start = ++(*cursor);
    while (**cursor != '\0' && **cursor != '"') {
        (*cursor)++;
    }

    if (**cursor != '"') {
        return 1;
    }

    length = (size_t)(*cursor - start);
    if (length >= dest_size) {
        return 1;
    }

    memcpy(dest, start, length);
    dest[length] = '\0';
    (*cursor)++;
    return 0;
}

static int expect_char(const char **cursor, char expected)
{
    *cursor = skip_ws(*cursor);
    if (**cursor != expected) {
        return 1;
    }

    (*cursor)++;
    return 0;
}

static int parse_long_value(const char **cursor, long *value)
{
    char *end;

    *cursor = skip_ws(*cursor);
    *value = strtol(*cursor, &end, 10);
    if (end == *cursor) {
        return 1;
    }

    *cursor = end;
    return 0;
}

static int parse_task_object(const char **cursor, task_t *task)
{
    char key[32];
    char name[32];
    long value;
    int fields_seen = 0;

    memset(task, 0, sizeof(*task));

    if (expect_char(cursor, '{') != 0) {
        return 1;
    }

    while (1) {
        if (parse_quoted_string(cursor, key, sizeof(key)) != 0) {
            return 1;
        }
        if (expect_char(cursor, ':') != 0) {
            return 1;
        }

        if (strcmp(key, "id") == 0) {
            if (parse_long_value(cursor, &value) != 0) {
                return 1;
            }
            task->id = (int)value;
            fields_seen++;
        } else if (strcmp(key, "name") == 0) {
            if (parse_quoted_string(cursor, name, sizeof(name)) != 0) {
                return 1;
            }
            strncpy(task->name, name, sizeof(task->name) - 1);
            fields_seen++;
        } else if (strcmp(key, "arrival") == 0) {
            if (parse_long_value(cursor, &value) != 0) {
                return 1;
            }
            task->arrival = value;
            fields_seen++;
        } else if (strcmp(key, "runtime") == 0) {
            if (parse_long_value(cursor, &value) != 0) {
                return 1;
            }
            task->remaining = value;
            fields_seen++;
        } else if (strcmp(key, "nice") == 0) {
            if (parse_long_value(cursor, &value) != 0) {
                return 1;
            }
            task->nice = (int)value;
            fields_seen++;
        } else {
            return 1;
        }

        *cursor = skip_ws(*cursor);
        if (**cursor == '}') {
            (*cursor)++;
            break;
        }
        if (expect_char(cursor, ',') != 0) {
            return 1;
        }
    }

    task->first_run = -1;
    task->finish = -1;
    return fields_seen == 5 ? 0 : 1;
}

static int parse_tasks_array(const char **cursor, config_t *config)
{
    int count = 0;

    if (expect_char(cursor, '[') != 0) {
        return 1;
    }

    *cursor = skip_ws(*cursor);
    if (**cursor == ']') {
        (*cursor)++;
        config->task_count = 0;
        return 0;
    }

    while (1) {
        if (count >= MAX_TASKS) {
            return 1;
        }
        if (parse_task_object(cursor, &config->tasks[count]) != 0) {
            return 1;
        }
        count++;

        *cursor = skip_ws(*cursor);
        if (**cursor == ']') {
            (*cursor)++;
            break;
        }
        if (expect_char(cursor, ',') != 0) {
            return 1;
        }
    }

    config->task_count = count;
    return 0;
}

int config_load(const char *path, config_t *config)
{
    char *buffer;
    const char *cursor;
    char key[32];
    int saw_policy = 0;
    int saw_quantum = 0;
    int saw_tasks = 0;

    if (path == NULL || config == NULL) {
        return 1;
    }

    memset(config, 0, sizeof(*config));

    if (read_file(path, &buffer) != 0) {
        return 1;
    }

    cursor = buffer;
    if (expect_char(&cursor, '{') != 0) {
        free(buffer);
        return 1;
    }

    while (1) {
        cursor = skip_ws(cursor);
        if (*cursor == '}') {
            cursor++;
            break;
        }

        if (parse_quoted_string(&cursor, key, sizeof(key)) != 0) {
            free(buffer);
            return 1;
        }
        if (expect_char(&cursor, ':') != 0) {
            free(buffer);
            return 1;
        }

        if (strcmp(key, "policy") == 0) {
            if (parse_quoted_string(&cursor, config->policy, sizeof(config->policy)) != 0) {
                free(buffer);
                return 1;
            }
            saw_policy = 1;
        } else if (strcmp(key, "quantum") == 0) {
            if (parse_long_value(&cursor, &config->quantum) != 0) {
                free(buffer);
                return 1;
            }
            saw_quantum = 1;
        } else if (strcmp(key, "tasks") == 0) {
            if (parse_tasks_array(&cursor, config) != 0) {
                free(buffer);
                return 1;
            }
            saw_tasks = 1;
        } else {
            free(buffer);
            return 1;
        }

        cursor = skip_ws(cursor);
        if (*cursor == '}') {
            cursor++;
            break;
        }
        if (expect_char(&cursor, ',') != 0) {
            free(buffer);
            return 1;
        }
    }

    free(buffer);

    if (!saw_policy || !saw_tasks) {
        return 1;
    }

    if (!saw_quantum) {
        config->quantum = 0;
    }

    if (strcmp(config->policy, "rr") != 0 && strcmp(config->policy, "fair") != 0) {
        return 1;
    }

    return config->task_count > 0 ? 0 : 1;
}
