#ifndef COMMON_H
#define COMMON_H

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <switch.h>

#define MAX_GAMES 6000
#define DB_BUFFER_SIZE (6 * 1024 * 1024)
#define CONFIG_PATH "/switch/npsp/settings.dat"

typedef struct {
    char *id;
    char *region;
    char *platform;
    char *name;
    char *url;
} GameEntry;

typedef enum {
    LANG_EN,
    LANG_PT,
    LANG_ES,
    LANG_COUNT
} Language;

typedef struct {
    const char *main_menu[5];
    const char *settings_menu[3];
    const char *status_downloading;
    const char *status_converting;
    const char *status_done;
    const char *status_error;
    const char *status_cancel;
    const char *msg_press_a;
    const char *msg_no_games;
    const char *msg_results;
    const char *title_settings;
    const char *lang_name;
    const char *footer_menu;
    const char *footer_list;
    const char *footer_dl;
    const char *footer_settings;
    const char *speed_select_title;
    const char *speed_options[3];
} LangStrings;

extern int current_lang;
extern const LangStrings languages[LANG_COUNT];

void load_config();
void save_config();

static inline void ui_draw_header(const char *title) {
    printf("\x1b[0;0H"); 
    printf("\x1b[46;30m"); 
    char buf[81];
    snprintf(buf, sizeof(buf), " %-78.78s", title);
    printf("%s", buf);
    printf("\x1b[0m\n"); 
}

static inline void ui_draw_footer(const char *text) {
    printf("\x1b[44;0H"); 
    printf("\x1b[47;30m"); 
    char buf[81];
    snprintf(buf, sizeof(buf), " %-78.78s", text);
    printf("%s", buf);
    printf("\x1b[0m");
}

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
