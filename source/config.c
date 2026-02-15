#include <stdio.h>
#include "common.h"

void load_config() {
    FILE *f = fopen(CONFIG_PATH, "rb");
    if (f) {
        fread(&current_lang, sizeof(int), 1, f);
        fclose(f);
        if (current_lang < 0 || current_lang >= LANG_COUNT) {
            current_lang = LANG_EN;
        }
    } else {
        current_lang = LANG_EN;
    }
}

void save_config() {
    FILE *f = fopen(CONFIG_PATH, "wb");
    if (f) {
        fwrite(&current_lang, sizeof(int), 1, f);
        fclose(f);
    }
}
