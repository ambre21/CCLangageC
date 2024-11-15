// utils.c
#include <stdlib.h>
#include <string.h>

char* my_strdup(const char* src) {
    if (src == NULL) return NULL;
    size_t len = strlen(src) + 1;
    char* dest = (char*)malloc(len);
    if (dest != NULL) {
        memcpy(dest, src, len);
    }
    return dest;
}