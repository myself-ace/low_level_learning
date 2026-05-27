#include <stdio.h>
#include <stdlib.h>

/*
 * Custom implementation of strlen()
 * Counts characters until null terminator is found
 */
int my_strlen(const unsigned char *str) {

    int len = 0;
    const unsigned char *trv = str;

    while (*trv) {
        len++;
        trv++;
    }

    return len;
}

/*
 * Custom implementation of strcpy()
 * Copies source string into destination buffer
 */
void my_strcpy(char *dst, const char *src) {

    while (*src) {
        *dst = *src;
        src++;
        dst++;
    }

    // Add null terminator
    *dst = '\0';
}

/*
 * Custom implementation of strcat()
 * Appends source string to destination string
 */
void my_strcat(char *dst, const char *src) {

    // Move dst pointer to end of current string
    while (*dst) {
        dst++;
    }

    // Copy src string at the end of dst
    while (*src) {
        *dst = *src;
        dst++;
        src++;
    }

    // Add final null terminator
    *dst = '\0';
}

int main(void) {

    char str[] = "hello this is a string";

    int len = my_strlen((unsigned char *)str);

    printf("Length of string: %d\n", len);

    char dst[100];

    my_strcpy(dst, str);

    printf("Copied string --> %s\n", dst);

    my_strcat(dst, str);

    printf("Concatenated string --> %s\n", dst);

    return 0;
}
