#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net.h"

GameEntry all_games[MAX_GAMES];
int total_games = 0;
char *db_buffer = NULL;

typedef struct {
    PadState *pad;
    CURL *curl;
    int cancel_requested;
    time_t last_time;
    double current_speed;
} ProgressContext;

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static size_t WriteFileCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    ProgressContext *ctx = (ProgressContext *)clientp;
    padUpdate(ctx->pad);
    u64 kDown = padGetButtonsDown(ctx->pad);

    if (kDown & HidNpadButton_B) {
        ctx->cancel_requested = 1;
        return 1;
    }

    if (dltotal <= 0) return 0;

    time_t now = time(NULL);
    if (now - ctx->last_time >= 2) {
        curl_off_t speed_bps = 0;
        curl_easy_getinfo(ctx->curl, CURLINFO_SPEED_DOWNLOAD_T, &speed_bps);
        ctx->current_speed = (double)speed_bps / (1024.0 * 1024.0);
        ctx->last_time = now;
    }

    double fraction = (double)dlnow / (double)dltotal;
    int percentage = (int)(fraction * 100);
    
    int bar_width = 30;
    int pos = bar_width * fraction;

    printf("\r\x1b[K [");
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %3d%% (%.1f / %.1f MB) @ %.2f MB/s", 
           percentage, 
           (double)dlnow / (1024*1024), 
           (double)dltotal / (1024*1024),
           ctx->current_speed);
    
    consoleUpdate(NULL);
    return 0;
}

int download_file(const char *url, const char *path, PadState *pad) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    
    ProgressContext ctx;
    ctx.pad = pad;
    ctx.cancel_requested = 0;
    ctx.last_time = 0;
    ctx.current_speed = 0.0;

    curl = curl_easy_init();
    if (curl) {
        ctx.curl = curl;
        fp = fopen(path, "wb");
        if(!fp) {
            curl_easy_cleanup(curl);
            return 0;
        }
        
        consoleClear();
        printf("\x1b[1;33mDOWNLOADING PKG...\x1b[0m\n");
        printf("Temp file: %s\n", path);
        printf("\x1b[1;31m[B] CANCEL\x1b[0m\n\n");
        consoleUpdate(NULL);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "NPS-Switch/4.0");
        
        res = curl_easy_perform(curl);
        
        fclose(fp);
        curl_easy_cleanup(curl);
        
        if (ctx.cancel_requested) {
            remove(path);
            return -1;
        }
        return (res == CURLE_OK);
    }
    return 0;
}

void parse_db(char *data) {
    char *line = data;
    total_games = 0;
    while(line && total_games < MAX_GAMES) {
        char *next_line = strchr(line, '\n');
        if(next_line) *next_line = '\0';
        if(strlen(line) > 10) {
            char *cols[10];
            int col_count = 0;
            char *ptr = line;
            cols[col_count++] = ptr;
            while(*ptr && col_count < 6) {
                if(*ptr == '\t') { *ptr = '\0'; cols[col_count++] = ptr + 1; }
                ptr++;
            }
            if(col_count >= 5) {
                if(stristr(cols[2], "NEOGEO") == NULL && 
                   stristr(cols[2], "PC Engine") == NULL &&
                   stristr(cols[4], "http")) {
                    all_games[total_games].id = cols[0];
                    all_games[total_games].region = cols[1];
                    all_games[total_games].platform = cols[2];
                    all_games[total_games].name = cols[3];
                    all_games[total_games].url = cols[4];
                    total_games++;
                }
            }
        }
        if(!next_line) break;
        line = next_line + 1;
    }
}
