#ifndef NET_H
#define NET_H

#include <switch.h>
#include <curl/curl.h>
#include "common.h"

#define URL_NPS_PSP "https://nopaystation.com/tsv/PSP_GAMES.tsv"

extern GameEntry all_games[MAX_GAMES];
extern int total_games;
extern char *db_buffer;

struct MemoryStruct {
    char *memory;
    size_t size;
};

int download_file(const char *url, const char *path, PadState *pad);
void parse_db(char *data);
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

#endif
