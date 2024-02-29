#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

#include "hash.h"

#define IGNORE_LINE -1

#define VERBOSE 1

#ifdef VERBOSE
#define LOG(fmt, ...) printf("[vsconf] " fmt, ##__VA_ARGS__)
#define fLOG(fd, fmt, ...) fprintf(fd, "[vsconf] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#define fLOG(fd, fmt, ...)
#endif

const unsigned int initial_size = 4;
struct hashmap_s map;

enum type
{
    ERR,
    STRING,
    INTEGER,
    BOOLEAN,
};

typedef struct variable
{
    char *name;
    enum type t;
    void *value;
} variable;

int k = 1;

const void *const get(const char *key)
{
    return hashmap_get(&map, key, strlen(key));
}

int readline(int fd, char *buf, int size)
{

    // printf("Reading line %d\n", k++);

    int n;
    int i = 0;

    while (i < size && (n = read(fd, &buf[i], 1)) > 0 && buf[i] != '\n')
    {
        i++;
    }
    buf[i] = '\0';

    // printf("Read %d bytes\n", i);

    return i;
}

enum type infer_type(const char *value)
{
    if (value == NULL || value[0] == '\0')
    {
        return ERR;
    }

    if (value[0] == '"')
    {
        return STRING;
    }
    else if (value[0] == 't' || value[0] == 'f')
    {
        return BOOLEAN;
    }
    else
    {
        return INTEGER;
    }
}

void cleanup()
{
    LOG("Cleaning up\n");
    hashmap_destroy(&map);
}

int configure(const char *path)
{
    LOG("Loading %s...\n", path);
    atexit(cleanup);

    if (0 != hashmap_create(initial_size, &map))
    {
        LOG("Error creating hashmap\n");
        return -1;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    LOG("Found config file!\n");

    // read the file line by line and print to the lines
    char buf[1024];
    int n;
    int k = 0;

    regex_t regex;
    int vars = regcomp(&regex, "([a-zA-Z0-9_]+)\\s*=\\s*(\"([a-zA-Z0-9_]+)\"|([0-9]+)|true|false)", REG_EXTENDED);

    regex_t section_regex;
    int section = regcomp(&section_regex, "\\[([a-zA-Z0-9_]+)\\]", REG_EXTENDED);

    if (vars)
    {
        fLOG(stderr, "Could not compile regex\n");
        return -1;
    }

    char current_section[100] = {'\0'};

    while ((n = readline(fd, buf, sizeof(buf) - 1)) != 0)
    {
        k++;
        if (n == IGNORE_LINE)
        {
            printf("Ignoring line %d\n", k);
            goto next;
        }

        if(buf[0] == '[') {
            regmatch_t matches[3];
            section = regexec(&section_regex, buf, 3, matches, 0);

            if(!section) {
                int start = matches[1].rm_so;
                int end = matches[1].rm_eo;

                char *name = (char *)malloc(end - start + 1);
                if (name == NULL)
                {
                    LOG("Error on line %d: %s\n", k, buf);
                    exit(1);
                }

                memcpy(name, &buf[start], end - start);
                name[end - start] = '\0';

                printf("Section: %s\n", name);
                strncpy(current_section, name, end - start > 100 ? 100 : end - start);
                free(name);
            } else if(section == REG_NOMATCH) {
                LOG("Error on line %d: %s\n", k, buf);
            } else {
                char msgbuf[100];
                regerror(section, &section_regex, msgbuf, sizeof(msgbuf));
                fLOG(stderr, "Regex match failed: %s\n", msgbuf);
                return -1;
            }

            goto next;
        }

        // extract the values between the equal sign

        regmatch_t matches[3];
        vars = regexec(&regex, buf, 3, matches, 0);

        if (!vars)
        {

            int start = matches[1].rm_so;
            int end = matches[1].rm_eo;

            char *name = (char *)malloc(end - start + 1);
            if (name == NULL)
            {
                LOG("Error on line %d: %s\n", k, buf);
                exit(1);
            }

            memcpy(name, &buf[start], end - start);
            name[end - start] = '\0';

            printf("\t%.*s = ", end - start, &buf[start]);

            start = matches[2].rm_so;
            end = matches[2].rm_eo;
            char *value = (char *)malloc(end - start + 1);
            if (value == NULL)
            {
                LOG("Error on line %d: %s\n", k, buf);
                exit(1);
            }
            memcpy(value, &buf[start], end - start);
            value[end - start] = '\0';

            enum type t = infer_type(value);

            if (t == BOOLEAN)
            {
                t = INTEGER;
                if (strcmp(value, "true") == 0)
                {
                    value = (char *)malloc(sizeof(int));
                    *value = 1;
                }
                else
                {
                    value = (char *)malloc(sizeof(int));
                    *value = 0;
                }
                printf("%d (%s)", *value, "Integer");
            }
            else if (t == INTEGER)
            {
                int v = atoi(value);
                value = (char *)malloc(sizeof(int));
                *value = v;
                printf("%d (%s)", *value, "Integer");

            }
            else
            {
                memmove(value, value + 1, end - start - 2);
                value[end - start - 2] = '\0';
                printf("%.*s (%s)", end - start, value, "String");
            }

            if (t == ERR)
            {
                LOG("Error on line %d: %s\n", k, buf);
                continue;
            }

            if(current_section[0] != '\0') {
                char *new_name = (char *)malloc(strlen(name) + strlen(current_section) + 2);
                if (new_name == NULL)
                {
                    LOG("Error on line %d: %s\n", k, buf);
                    exit(1);
                }
                sprintf(new_name, "%s.%s", current_section, name);
                free(name);
                name = new_name;
            }

            // add to the hashmap
            if (0 != hashmap_put(&map, name, strlen(name), malloc(end - start + 1)))
            {
                LOG("Error adding to hashmap\n");
                return -1;
            }

            // add the value to the hashmap
            void *const mem = hashmap_get(&map, name, strlen(name));
            if (mem == NULL)
            {
                LOG("Error getting value from hashmap\n");
                return -1;
            }
            memcpy(mem, value, end - start + 1);

            free(value);
        next:

            printf("\n");
        }
        else if (vars == REG_NOMATCH)
        {
            LOG("Error on line %d: %s\n", k, buf);
        }
        else
        {
            char msgbuf[100];
            regerror(vars, &regex, msgbuf, sizeof(msgbuf));
            fLOG(stderr, "Regex match failed: %s\n", msgbuf);
            return -1;
        }
    }

    regfree(&regex);

    close(fd);

    LOG("Configuration done!\n");
    LOG("Proceeding with main program\n\n");

    return 0;
}