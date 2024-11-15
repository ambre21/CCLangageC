// utils.c
#include <ctype.h>
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


void trim_whitespace(char* str) {
    // Supprimer les espaces en début
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) {
        // Si la chaîne est vide
        *str = '\0';
        return;
    }

    // Supprimer les espaces en fin
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Terminer la chaîne
    *(end + 1) = '\0';
}