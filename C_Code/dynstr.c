#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*
========================================================
                DYNAMIC STRING LIBRARY
========================================================

A custom heap-managed dynamic string implementation in C.

Features:
- Dynamic heap allocation
- Automatic resizing using realloc
- Character insertion/deletion
- Substring search
- String slicing
- Runtime validation
- Unit testing

========================================================
*/


/* =====================================================
                    DATA STRUCTURE
===================================================== */

typedef struct {
    char *data;        // Heap buffer
    size_t len;        // Current string length
    size_t capacity;   // Total allocated capacity
} dynstr;


/* =====================================================
                    VALIDATION
===================================================== */

/*
    Ensures the dynstr structure remains valid.

    Validation checks:
    - struct pointer exists
    - heap pointer exists
    - capacity is non-zero
    - len never exceeds capacity
    - string remains null terminated
*/

int dynstr_validate(const dynstr *s) {

    if (!s)
        return 0;

    if (!s->data)
        return 0;

    if (s->capacity == 0)
        return 0;

    if (s->len >= s->capacity)
        return 0;

    if (s->data[s->len] != '\0')
        return 0;

    return 1;
}


/* =====================================================
                    INITIALIZATION
===================================================== */

/*
    Initializes an empty dynamic string.

    Initial state:
    data     -> ""
    len      -> 0
    capacity -> 1
*/

int dynstr_init(dynstr *s) {

    if (!s)
        return 0;

    s->data = malloc(1);

    if (!s->data)
        return 0;

    s->data[0] = '\0';

    s->len = 0;
    s->capacity = 1;

    return 1;
}


/* =====================================================
                    RESERVE CAPACITY
===================================================== */

/*
    Expands heap capacity when additional space is needed.

    Growth strategy:
    - Capacity doubles each resize
    - Reduces realloc frequency
    - Improves append performance
*/

int dynstr_reserve(dynstr *s, size_t needed) {

    if (!dynstr_validate(s))
        return 0;

    /*
        +1 ensures space for null terminator
    */
    if (needed + 1 < s->capacity)
        return 1;

    size_t new_capacity = s->capacity;

    while (new_capacity <= needed + 1) {

        /*
            Prevent integer overflow
        */
        if (new_capacity > SIZE_MAX / 2)
            return 0;

        new_capacity *= 2;
    }

    /*
        Temporary pointer prevents
        losing original allocation
        if realloc fails
    */
    char *tmp = realloc(s->data, new_capacity);

    if (!tmp)
        return 0;

    s->data = tmp;
    s->capacity = new_capacity;

    return 1;
}


/* =====================================================
                    APPEND CHARACTER
===================================================== */

/*
    Appends a single character to the string.
*/

int dynstr_append_char(dynstr *s, char c) {

    if (!dynstr_validate(s))
        return 0;

    /*
        Need:
        existing chars +
        new char +
        null terminator
    */
    if (!dynstr_reserve(s, s->len + 2))
        return 0;

    s->data[s->len] = c;

    s->len++;

    s->data[s->len] = '\0';

    return 1;
}


/* =====================================================
                    APPEND C-STRING
===================================================== */

/*
    Appends a null-terminated C-string.
*/

int dynstr_append_cstr(dynstr *s, const char *str) {

    if (!dynstr_validate(s) || !str)
        return 0;

    size_t str_len = 0;

    while (str[str_len] != '\0')
        str_len++;

    if (!dynstr_reserve(s, s->len + str_len + 1))
        return 0;

    for (size_t i = 0; i < str_len; i++)
        s->data[s->len + i] = str[i];

    s->len += str_len;

    s->data[s->len] = '\0';

    return 1;
}


/* =====================================================
                    INSERT CHARACTER
===================================================== */

/*
    Inserts a character at a specific index.

    Example:
    HELLO
      ^
    insert('X', 1)

    HXELLO
*/

int dynstr_insert_char(dynstr *s, size_t idx, char c) {

    if (!dynstr_validate(s))
        return 0;

    if (idx > s->len)
        return 0;

    if (!dynstr_reserve(s, s->len + 2))
        return 0;

    /*
        Shift string right,
        including null terminator
    */
    for (size_t i = s->len + 1; i > idx; i--)
        s->data[i] = s->data[i - 1];

    s->data[idx] = c;

    s->len++;

    return 1;
}


/* =====================================================
                    DELETE CHARACTER
===================================================== */

/*
    Deletes a character at a given index.
*/

int dynstr_delete_char(dynstr *s, size_t idx) {

    if (!dynstr_validate(s))
        return 0;

    if (idx >= s->len)
        return 0;

    /*
        Shift remaining data left
    */
    for (size_t i = idx; i < s->len; i++)
        s->data[i] = s->data[i + 1];

    s->len--;

    return 1;
}


/* =====================================================
                    SUBSTRING SEARCH
===================================================== */

/*
    Finds the first occurrence
    of a substring.

    Returns:
    - index if found
    - -1 if not found
*/

int dynstr_find(const dynstr *s, const char *needle) {

    if (!dynstr_validate(s))
        return -1;

    if (!needle)
        return -1;

    if (needle[0] == '\0')
        return 0;

    for (size_t i = 0; s->data[i] != '\0'; i++) {

        size_t j = 0;

        while (
            needle[j] != '\0' &&
            s->data[i + j] != '\0' &&
            s->data[i + j] == needle[j]
        ) {
            j++;
        }

        if (needle[j] == '\0')
            return (int)i;
    }

    return -1;
}


/* =====================================================
                    STRING SLICE
===================================================== */

/*
    Copies a substring range
    into another dynstr.

    Example:
    "HELLOWORLD"

    slice(0,5) -> "HELLO"
*/

int dynstr_slice(
    const dynstr *src,
    dynstr *dest,
    size_t start,
    size_t end
) {

    if (!dynstr_validate(src))
        return 0;

    if (!dest)
        return 0;

    if (start > end)
        return 0;

    if (end > src->len)
        return 0;

    if (!dynstr_init(dest))
        return 0;

    for (size_t i = start; i < end; i++) {

        if (!dynstr_append_char(dest, src->data[i])) {

            free(dest->data);

            return 0;
        }
    }

    return 1;
}


/* =====================================================
                    DESTROY
===================================================== */

/*
    Releases heap memory
    and resets structure state.
*/

void dynstr_destroy(dynstr *s) {

    if (!s)
        return;

    free(s->data);

    s->data = NULL;
    s->len = 0;
    s->capacity = 0;
}


/* =====================================================
                    UNIT TESTS
===================================================== */

void test_insert() {

    dynstr s;

    assert(dynstr_init(&s));

    assert(dynstr_append_cstr(&s, "abcd"));

    assert(dynstr_insert_char(&s, 0, 'X'));
    assert(dynstr_find(&s, "Xabcd") == 0);

    assert(dynstr_insert_char(&s, 5, 'Y'));
    assert(dynstr_find(&s, "Y") == 5);

    assert(dynstr_insert_char(&s, 3, 'Z'));
    assert(dynstr_find(&s, "Z") == 3);

    dynstr_destroy(&s);

    printf("test_insert PASSED\n");
}


void test_delete() {

    dynstr s;

    assert(dynstr_init(&s));

    assert(dynstr_append_cstr(&s, "ABCDE"));

    assert(dynstr_delete_char(&s, 0));
    assert(dynstr_find(&s, "BCDE") == 0);

    assert(dynstr_delete_char(&s, 3));
    assert(dynstr_find(&s, "BCD") == 0);

    dynstr_destroy(&s);

    printf("test_delete PASSED\n");
}


void test_large() {

    dynstr s;

    assert(dynstr_init(&s));

    for (int i = 0; i < 100000; i++)
        assert(dynstr_append_char(&s, 'A'));

    assert(s.len == 100000);

    dynstr_destroy(&s);

    printf("test_large PASSED\n");
}


/* =====================================================
                        MAIN
===================================================== */

int main() {

    test_insert();

    test_delete();

    test_large();

    printf("\nALL TESTS PASSED\n");

    return 0;
}
