#ifndef COMMON_H
#define COMMON_H

#include <string.h>
#include <ctype.h>

#define MAX_GAMES 6000
#define DB_BUFFER_SIZE (6 * 1024 * 1024)

typedef struct {
    char *id;
    char *region;
    char *platform;
    char *name;
    char *url;
} GameEntry;

static inline char *stristr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; ++haystack) {
        if (toupper((unsigned char)*haystack) == toupper((unsigned char)*needle)) {
            const char *h, *n;
            for (h = haystack, n = needle; *h && *n; ++h, ++n) {
                if (toupper((unsigned char)*h) != toupper((unsigned char)*n)) break;
            }
            if (!*n) return (char *)haystack;
        }
    }
    return NULL;
}

static inline void sanitize_filename(char *name) {
    char *p = name;
    while (*p) {
        if (!isalnum((unsigned char)*p) && *p != ' ' && *p != '-' && *p != '.') {
            *p = '_';
        }
        p++;
    }
}

#endif
