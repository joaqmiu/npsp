#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <switch.h>
#include "net.h"

#define MAX_PARTS 8

GameEntry all_games[MAX_GAMES];
int total_games = 0;
char *db_buffer = NULL;

typedef struct {
    FILE *fp;
    long start_offset;
    long current_offset;
    curl_off_t total_written;
} PartContext;

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

static size_t WritePartCallback(void *ptr, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    PartContext *ctx = (PartContext *)userp;
    fseek(ctx->fp, ctx->current_offset, SEEK_SET);
    size_t written = fwrite(ptr, 1, realsize, ctx->fp);
    ctx->current_offset += written;
    ctx->total_written += written;
    return written;
}

static double get_file_size(const char *url) {
    double filesize = 0.0;
    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        if (curl_easy_perform(curl) == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
        }
        curl_easy_cleanup(curl);
    }
    return filesize;
}

int download_file(const char *url, const char *path, PadState *pad, const char *header_title, int num_threads) {
    CURLM *multi_handle;
    CURL *easy_handles[MAX_PARTS];
    PartContext part_ctx[MAX_PARTS];
    int still_running = 0;
    int i;
    int cancel_requested = 0;

    if (num_threads < 1) num_threads = 1;
    if (num_threads > MAX_PARTS) num_threads = MAX_PARTS;

    double remote_size = get_file_size(url);
    if (remote_size <= 0.0) return 0;

    curl_off_t file_size = (curl_off_t)remote_size;
    curl_off_t part_size = file_size / num_threads;

    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;

    fseek(fp, file_size - 1, SEEK_SET);
    fputc(0, fp);
    fseek(fp, 0, SEEK_SET);

    multi_handle = curl_multi_init();

    for (i = 0; i < num_threads; i++) {
        part_ctx[i].fp = fp;
        part_ctx[i].start_offset = i * part_size;
        part_ctx[i].current_offset = part_ctx[i].start_offset;
        part_ctx[i].total_written = 0;

        curl_off_t end_offset = (i == num_threads - 1) ? file_size - 1 : (part_ctx[i].start_offset + part_size - 1);
        char range_buf[64];
        snprintf(range_buf, sizeof(range_buf), "%ld-%ld", part_ctx[i].start_offset, end_offset);

        easy_handles[i] = curl_easy_init();
        curl_easy_setopt(easy_handles[i], CURLOPT_URL, url);
        curl_easy_setopt(easy_handles[i], CURLOPT_WRITEFUNCTION, WritePartCallback);
        curl_easy_setopt(easy_handles[i], CURLOPT_WRITEDATA, &part_ctx[i]);
        curl_easy_setopt(easy_handles[i], CURLOPT_RANGE, range_buf);
        curl_easy_setopt(easy_handles[i], CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(easy_handles[i], CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(easy_handles[i], CURLOPT_USERAGENT, "NPS-Switch/4.0");
        
        curl_easy_setopt(easy_handles[i], CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(easy_handles[i], CURLOPT_LOW_SPEED_TIME, 15L);
        curl_easy_setopt(easy_handles[i], CURLOPT_LOW_SPEED_LIMIT, 10L);
        curl_easy_setopt(easy_handles[i], CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(easy_handles[i], CURLOPT_FAILONERROR, 1L);
        
        curl_multi_add_handle(multi_handle, easy_handles[i]);
    }

    consoleClear();
    ui_draw_header(header_title);
    printf("\n\n  %s...\n", (current_lang == LANG_PT) ? "Iniciando" : ((current_lang == LANG_ES) ? "Iniciando" : "Initializing"));
    consoleUpdate(NULL);

    u64 last_time = armGetSystemTick();
    curl_off_t last_downloaded = 0;
    
    do {
        int numfds;
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
        
        if (mc == CURLM_OK) {
            curl_multi_wait(multi_handle, NULL, 0, 100, &numfds);
        }

        padUpdate(pad);
        if (padGetButtonsDown(pad) & HidNpadButton_B) {
            cancel_requested = 1;
            break;
        }

        u64 current_time = armGetSystemTick();
        if (current_time - last_time >= armGetSystemTickFreq() / 2) { 
            curl_off_t total_downloaded = 0;
            for (int k = 0; k < num_threads; k++) {
                total_downloaded += part_ctx[k].total_written;
            }

            double speed = (double)(total_downloaded - last_downloaded) * 2.0; 
            last_downloaded = total_downloaded;
            last_time = current_time;

            double fraction = (double)total_downloaded / (double)file_size;
            if (fraction > 1.0) fraction = 1.0;
            int percentage = (int)(fraction * 100);

            consoleClear();
            ui_draw_header(header_title);
            
            printf("\n\n");
            printf("  %s: %3d%%\n", 
                   (current_lang == LANG_PT) ? "Progresso" : ((current_lang == LANG_ES) ? "Progreso" : "Progress"), 
                   percentage);
            
            printf("  %s: %.2f / %.2f MB (%.2f MB/s)\n",
                   (current_lang == LANG_PT) ? "Baixado" : ((current_lang == LANG_ES) ? "Descargado" : "Downloaded"),
                   (double)total_downloaded / (1024*1024), 
                   (double)file_size / (1024*1024),
                   speed / (1024*1024));

            printf("\n  [");
            int bar_width = 50;
            int pos = bar_width * fraction;
            for (int k = 0; k < bar_width; ++k) {
                if (k < pos) printf("\x1b[32m#\x1b[0m"); 
                else if (k == pos) printf("\x1b[32m>\x1b[0m");
                else printf("\x1b[30m-\x1b[0m");
            }
            printf("]\n");

            ui_draw_footer(languages[current_lang].status_cancel);
            consoleUpdate(NULL);
        }

    } while (still_running);

    int error_detected = 0;
    CURLMsg *msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            if (msg->data.result != CURLE_OK) error_detected = 1;
        }
    }

    for (i = 0; i < num_threads; i++) {
        curl_multi_remove_handle(multi_handle, easy_handles[i]);
        curl_easy_cleanup(easy_handles[i]);
    }
    curl_multi_cleanup(multi_handle);
    fclose(fp);

    if (cancel_requested || error_detected) {
        remove(path);
        return -1;
    }

    curl_off_t final_size = 0;
    for (i = 0; i < num_threads; i++) {
        final_size += part_ctx[i].total_written;
    }

    return (final_size >= file_size);
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
