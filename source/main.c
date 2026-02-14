#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <switch.h>

#include "common.h"
#include "converter.h"
#include "net.h"

#define PAGE_SIZE 37

typedef enum {
    STATE_LOADING,
    STATE_MENU,
    STATE_LIST,
    STATE_DOWNLOADING,
    STATE_CONVERTING
} AppState;

typedef enum {
    FILTER_SEARCH,
    FILTER_ALL,
    FILTER_NUM,
    FILTER_JP,
    FILTER_LETTER
} FilterMode;

GameEntry *filtered_games[MAX_GAMES];
int filtered_count = 0;
char current_search[256] = "";
char current_letter = 'A';
FilterMode current_filter = FILTER_ALL;

void apply_filter() {
    filtered_count = 0;
    for(int i = 0; i < total_games; i++) {
        int match = 0;
        switch(current_filter) {
            case FILTER_ALL: match = 1; break;
            case FILTER_SEARCH: if(stristr(all_games[i].name, current_search)) match = 1; break;
            case FILTER_JP: if(stristr(all_games[i].region, "JP")) match = 1; break;
            case FILTER_NUM: if(isdigit((unsigned char)all_games[i].name[0])) match = 1; break;
            case FILTER_LETTER: if(toupper((unsigned char)all_games[i].name[0]) == current_letter) match = 1; break;
        }
        if(match) filtered_games[filtered_count++] = &all_games[i];
    }
}

int main(int argc, char* argv[]) {
    socketInitializeDefault();
    nxlinkStdio();
    consoleInit(NULL);
    
    struct stat st = {0};
    if (stat("/PPSSPP", &st) == -1) {
        mkdir("/PPSSPP", 0777);
    }

    db_buffer = (char*)malloc(DB_BUFFER_SIZE);
    if (!db_buffer) { printf("Memory Error.\n"); return -1; }

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    AppState state = STATE_LOADING;
    int menu_idx = 0;
    int list_idx = 0;
    int list_top = 0;
    
    const char *menu_opts[] = {"SEARCH", "ALL GAMES", "# (NUMBERS)", "JAPAN (JP)"};
    char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int menu_size = 4 + 26; 

    int v_timer = 0;
    int h_timer = 0;

    while(appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        u64 kHeld = padGetButtons(&pad);

        int move_up = 0, move_down = 0, move_left = 0, move_right = 0;

        if (kDown & HidNpadButton_Down) { move_down = 1; v_timer = 0; }
        else if (kDown & HidNpadButton_Up) { move_up = 1; v_timer = 0; }
        else if (kHeld & HidNpadButton_Down) {
            v_timer++;
            if (v_timer > 15 && (v_timer % 4 == 0)) move_down = 1;
        }
        else if (kHeld & HidNpadButton_Up) {
            v_timer++;
            if (v_timer > 15 && (v_timer % 4 == 0)) move_up = 1;
        }
        else { v_timer = 0; }

        if (kDown & HidNpadButton_Right) { move_right = 1; h_timer = 0; }
        else if (kDown & HidNpadButton_Left) { move_left = 1; h_timer = 0; }
        else if (kHeld & HidNpadButton_Right) {
            h_timer++;
            if (h_timer > 15 && (h_timer % 4 == 0)) move_right = 1;
        }
        else if (kHeld & HidNpadButton_Left) {
            h_timer++;
            if (h_timer > 15 && (h_timer % 4 == 0)) move_left = 1;
        }
        else { h_timer = 0; }

        if (state == STATE_LOADING) {
            printf("\x1b[1;36mDownloading DB...\x1b[0m\n");
            consoleUpdate(NULL);
            struct MemoryStruct chunk = {malloc(1), 0};
            CURL *curl = curl_easy_init();
            if(curl) {
                curl_easy_setopt(curl, CURLOPT_URL, URL_NPS_PSP);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_perform(curl);
                curl_easy_cleanup(curl);
            }
            if(chunk.size > 0 && chunk.size < DB_BUFFER_SIZE) {
                memcpy(db_buffer, chunk.memory, chunk.size);
                db_buffer[chunk.size] = '\0';
                parse_db(db_buffer);
                state = STATE_MENU;
            } else {
                printf("DB Error.\n");
                while(1) { padUpdate(&pad); consoleUpdate(NULL); }
            }
            free(chunk.memory);
        }
        else if (state == STATE_MENU) {
            consoleClear();
            printf("\x1b[1;33m--- npsp v1.18 by joaqmiu ---\x1b[0m\n\n");
            
            for(int i = 0; i < menu_size; i++) {
                if(i == menu_idx) printf("\x1b[47;30m");
                if(i < 4) printf(" %s \n", menu_opts[i]);
                else {
                    printf(" %c \n", letters[i-4]);
                }
                if(i == menu_idx) printf("\x1b[0m");
            }
            
            printf("\n\n\n\n\x1b[32mUse D-PAD to navigate, A to select.\x1b[0m");
            printf("\n\x1b[90mCredits: joaqmiu\x1b[0m");

            if(move_down) {
                if (menu_idx < menu_size - 1) menu_idx++;
            }
            if(move_up) {
                if (menu_idx > 0) menu_idx--;
            }
            if(move_right) {
                if(menu_idx + 10 < menu_size) menu_idx += 10;
                else menu_idx = menu_size - 1;
            }
            if(move_left) {
                if(menu_idx - 10 >= 0) menu_idx -= 10;
                else menu_idx = 0;
            }
            
            if(kDown & HidNpadButton_A) {
                if(menu_idx == 0) {
                    SwkbdConfig kbd;
                    if (R_SUCCEEDED(swkbdCreate(&kbd, 0))) {
                        swkbdConfigMakePresetDefault(&kbd);
                        if (R_SUCCEEDED(swkbdShow(&kbd, current_search, sizeof(current_search)))) {
                            current_filter = FILTER_SEARCH;
                            apply_filter();
                            state = STATE_LIST;
                            list_idx = 0; list_top = 0;
                        }
                        swkbdClose(&kbd);
                    }
                } else {
                    if(menu_idx == 1) current_filter = FILTER_ALL;
                    else if(menu_idx == 2) current_filter = FILTER_NUM;
                    else if(menu_idx == 3) current_filter = FILTER_JP;
                    else {
                        current_letter = letters[menu_idx - 4];
                        current_filter = FILTER_LETTER;
                    }
                    apply_filter();
                    state = STATE_LIST;
                    list_idx = 0; list_top = 0;
                }
            }
        }
        else if (state == STATE_LIST) {
            consoleClear();
            printf("\x1b[1;36mRESULTS: %d\x1b[0m\n", filtered_count);
            printf("--------------------------------------------------\n");

            if(filtered_count == 0) printf("No games found.\n");
            else {
                for(int i = 0; i < PAGE_SIZE; i++) {
                    int actual_idx = list_top + i;
                    if(actual_idx >= filtered_count) break;
                    if(actual_idx == list_idx) printf("\x1b[47;30m");
                    
                    GameEntry *g = filtered_games[actual_idx];
                    char dname[60];
                    strncpy(dname, g->name, 59); dname[59] = '\0';
                    printf(" [%s] %-55s \n", g->region, dname);
                    
                    if(actual_idx == list_idx) printf("\x1b[0m");
                }
            }

            if(move_down) {
                if(list_idx < filtered_count - 1) {
                    list_idx++;
                    if(list_idx >= list_top + PAGE_SIZE) list_top++;
                }
            }
            if(move_up) {
                if(list_idx > 0) {
                    list_idx--;
                    if(list_idx < list_top) list_top--;
                }
            }
            if(move_right) {
                 if(list_idx + PAGE_SIZE < filtered_count) {
                     list_idx += PAGE_SIZE; list_top += PAGE_SIZE;
                 } else {
                     list_idx = filtered_count - 1;
                     if(list_top < filtered_count - PAGE_SIZE) list_top = filtered_count - PAGE_SIZE;
                 }
            }
            if(move_left) {
                if(list_idx - PAGE_SIZE >= 0) {
                    list_idx -= PAGE_SIZE; if(list_idx < list_top) list_top -= PAGE_SIZE;
                } else { list_idx = 0; list_top = 0; }
            }
            if(list_top < 0) list_top = 0;

            if(kDown & HidNpadButton_B) state = STATE_MENU;
            if(kDown & HidNpadButton_A && filtered_count > 0) state = STATE_DOWNLOADING;
        }
        else if (state == STATE_DOWNLOADING) {
            GameEntry *g = filtered_games[list_idx];
            char safe_name[256];
            strncpy(safe_name, g->name, 255); safe_name[255] = '\0';
            sanitize_filename(safe_name);
            
            char pkg_path[512], pbp_path[512];
            snprintf(pkg_path, sizeof(pkg_path), "/PPSSPP/%s_temp.pkg", safe_name);
            snprintf(pbp_path, sizeof(pbp_path), "/PPSSPP/%s.PBP", safe_name);
            
            int result = download_file(g->url, pkg_path, &pad);
            
            consoleClear(); 

            if(result == 1) {
                state = STATE_CONVERTING;
                
                printf("\x1b[1;36mCONVERTING...\x1b[0m\n");
                printf("Verifying PKG and extracting EBOOT.PBP...\n");
                consoleUpdate(NULL); 

                if (convert_pkg_to_pbp(pkg_path, pbp_path)) {
                    remove(pkg_path); 
                    printf("\n\n\x1b[1;32mDONE!\x1b[0m\n");
                    printf("Saved to: %s\n", pbp_path);
                } else {
                    printf("\n\n\x1b[1;31mCONVERSION ERROR.\x1b[0m\n");
                    remove(pkg_path); 
                    remove(pbp_path);
                }
                printf("\n\nPress A to return...");
            } else if (result == -1) {
                printf("\n\n\x1b[1;33mCANCELLED.\x1b[0m\n");
                printf("Press A to return...");
            } else {
                printf("\n\n\x1b[1;31mDOWNLOAD ERROR.\x1b[0m\n");
                printf("Press A to return...");
            }
            
            consoleUpdate(NULL); 
            
            while(1) {
                padUpdate(&pad);
                if(padGetButtonsDown(&pad) & HidNpadButton_A) break;
                consoleUpdate(NULL); 
            }
            state = STATE_LIST;
        }
        consoleUpdate(NULL);
    }
    
    free(db_buffer);
    consoleExit(NULL);
    socketExit();
    return 0;
}
