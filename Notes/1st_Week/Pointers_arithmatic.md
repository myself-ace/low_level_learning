# Day 02 — Pointers and Memory Access

## Goal

Develop a strong mental model for:
- pointers
- addresses
- dereferencing
- pointer arithmetic
- arrays vs pointers
- strings in memory
- memory traversal
- function interaction with memory

This is one of the most important topics in C.

Without understanding pointers:
- debugging becomes painful
- memory corruption makes no sense
- exploit development becomes impossible

---

# Core Idea

A pointer is just a variable storing a memory address.

Example:

```c
int x = 10;

int *ptr = &x;
```

Memory model:

```text
Address        Value

0x1000         10
0x2000         0x1000
```

Meaning:

```text
ptr contains address of x
```

---

# Pointer Declaration

```c
int *ptr;
```

Interpretation:

```text
ptr is a pointer to int
```

---

# Address Operator (&)

Returns memory address.

Example:

```c
int x = 50;

printf("%p", &x);
```

Possible output:

```text
0x7fffffffe12c
```

---

# Dereference Operator (*)

Access value at stored address.

Example:

```c
int x = 50;

int *ptr = &x;

printf("%d", *ptr);
```

Output:

```text
50
```

Meaning:

```text
go to address stored inside ptr
and read memory there
```

---

# Visual Mental Model

```c
int x = 99;

int *ptr = &x;
```

Visualization:

```text
Stack Memory

+-------------------+
| x = 99            |
+-------------------+
| ptr = 0x1234      |
+-------------------+

0x1234 -> 99
```

---

# ptr vs *ptr vs &ptr

This confuses almost everyone initially.

| Expression | Meaning |
|---|---|
| ptr | stored address |
| *ptr | value at address |
| &ptr | address of pointer itself |

Example:

```c
int x = 10;

int *ptr = &x;
```

Possible values:

```text
ptr   = 0x1000
*ptr  = 10
&ptr  = 0x2000
```

---

# Pointer Types Matter

Example:

```c
int *a;
char *b;
double *c;
```

Why important:
- compiler needs data size
- affects pointer arithmetic
- affects dereferencing

---

# Pointer Arithmetic

## Important Concept

Pointer arithmetic depends on type size.

Example:

```c
int arr[3] = {1,2,3};

int *ptr = arr;
```

If:

```text
ptr = 0x1000
```

Then:

```text
ptr + 1 = 0x1004
```

Because:
```text
sizeof(int) = 4
```

---

# Array Memory Layout

Example:

```c
int arr[4] = {10,20,30,40};
```

Memory:

```text
0x1000 -> 10
0x1004 -> 20
0x1008 -> 30
0x100C -> 40
```

Arrays are contiguous memory blocks.

---

# Arrays Decay Into Pointers

Example:

```c
char str[] = "hello";
```

`str` behaves like:

```c
char *ptr = &str[0];
```

This is why arrays and pointers seem similar.

But they are NOT identical.

---

# Important Difference

## Array

```c
char str[16];
```

- fixed memory block
- actual storage allocated

Cannot:

```c
str++;
```

---

## Pointer

```c
char *str;
```

- only stores address
- movable

Can:

```c
str++;
```

---

# Strings in C

A string is:
```text
array of characters ending with NULL byte
```

Example:

```c
char str[] = "hello";
```

Memory:

```text
'h'
'e'
'l'
'l'
'o'
'\0'
```

---

# NULL Terminator

Critical concept.

String functions stop at:

```text
'\0'
```

Without null termination:
- over-read happens
- memory corruption possible
- crashes possible

---

# String Library Functions and Pointer Usage

Your string library work becomes important here.

Most string functions are literally pointer traversal.

---

# strlen()

Conceptual implementation:

```c
int strlen(char *s) {
    int len = 0;

    while (*s != '\0') {
        len++;
        s++;
    }

    return len;
}
```

Important observations:
- pointer moves through memory
- dereferencing reads character
- loop stops at NULL byte

---

# strcpy()

Conceptual implementation:

```c
void strcpy(char *dest, char *src) {

    while (*src != '\0') {
        *dest = *src;

        dest++;
        src++;
    }

    *dest = '\0';
}
```

Key ideas:
- memory copying
- pointer incrementing
- direct memory writes

---

# strcat()

Concatenation works by:
1. finding end of first string
2. copying second string

Pointers heavily used internally.

---

# strcmp()

Comparison example:

```c
while (*a && (*a == *b)) {
    a++;
    b++;
}
```

String comparison is literally:
```text
memory byte comparison
```

---

# Why String Functions Are Dangerous

Functions like:

```c
strcpy()
gets()
sprintf()
```

do not check bounds.

Example:

```c
char buf[8];

strcpy(buf, "AAAAAAAAAAAAAAAA");
```

Problem:
- write beyond buffer
- overwrite adjacent memory
- possible control flow hijack

---

# Buffer Overflow Example

```c
void vuln() {
    char buf[16];

    gets(buf);
}
```

If input > 16 bytes:
- stack corruption occurs
- return address may be overwritten

Classic exploitation primitive.

---

# Pointer Traversal

Example:

```c
char str[] = "abc";

char *ptr = str;
```

Traversal:

```text
ptr -> 'a'
ptr+1 -> 'b'
ptr+2 -> 'c'
```

---

# Iterating Through Arrays

Example:

```c
for (int i = 0; i < 5; i++) {
    printf("%d", *(arr + i));
}
```

Equivalent to:

```c
arr[i]
```

Because:

```text
arr[i] == *(arr + i)
```

Very important realization.

---

# Double Pointers

Example:

```c
char **argv;
```

Meaning:
```text
pointer to pointer
```

Used in:
- argv
- dynamic arrays
- linked structures
- function output parameters

---

# Function Parameters and Pointers

C passes arguments by value.

Pointers allow indirect modification.

Example:

```c
void set(int *x) {
    *x = 100;
}
```

Now original variable changes.

---

# Heap + Pointer Relationship

Example:

```c
int *ptr = malloc(sizeof(int));
```

Memory:

```text
Stack:
ptr -> 0x55555555

Heap:
0x55555555 -> allocated memory
```

Pointer itself often on stack.
Allocated memory on heap.

---

# Dangling Pointers

Example:

```c
free(ptr);
```

But:

```c
ptr still contains old address
```

Danger:
- invalid access
- crashes
- memory corruption

---

# NULL Pointers

Safe initialization:

```c
int *ptr = NULL;
```

NULL means:
```text
points nowhere
```

Dereferencing NULL:
- segmentation fault

---

# Wild Pointers

Uninitialized pointer:

```c
int *ptr;
```

Danger:
- random address
- undefined behavior

Always initialize pointers.

---

# Common Pointer Bugs

| Bug | Cause |
|---|---|
| NULL dereference | dereferencing NULL |
| Dangling pointer | freed memory access |
| Wild pointer | uninitialized pointer |
| Buffer overflow | writing out of bounds |
| Off-by-one | incorrect indexing |

---

# Memory Ownership

Very important concept.

Who owns allocated memory?

Example:

```c
char *buf = malloc(100);
```

Responsibility:
```text
caller must free memory
```

Bad ownership tracking creates:
- leaks
- double free
- UAF bugs

---

# Experiments I Ran

## Experiment 1 — Address Printing

```c
int x = 10;

printf("%p\n", &x);
```

Observation:
- variables have real addresses
- stack addresses usually close together

---

## Experiment 2 — Pointer Dereference

```c
int x = 99;

int *ptr = &x;

printf("%d\n", *ptr);
```

Observation:
- dereferencing accesses original memory directly

---

## Experiment 3 — Pointer Arithmetic

```c
int arr[3] = {1,2,3};

printf("%p\n", arr);
printf("%p\n", arr + 1);
```

Observation:
- address changes by sizeof(type)

---

## Experiment 4 — String Traversal

```c
char str[] = "hello";

char *p = str;

while (*p) {
    printf("%c\n", *p);
    p++;
}
```

Observation:
- strings are sequential memory bytes

---

# Important Realizations

- pointers are just addresses
- dereferencing accesses memory directly
- arrays are contiguous memory
- string functions depend heavily on pointer traversal
- many vulnerabilities come from unsafe pointer use
- pointer arithmetic is type dependent
- arrays and pointers are related but not identical

---

# Commands Used

Compile:

```bash
gcc -Wall -Wextra -g test.c
```

Run:

```bash
./a.out
```

Debug:

```bash
gdb ./a.out
```

---

# Useful GDB Commands

View memory:

```bash
x/16gx address
```

View stack:

```bash
x/32gx $rsp
```

View registers:

```bash
info registers
```

---

# Questions To Revisit

- how exactly does pointer arithmetic compile?
- how are string literals stored internally?
- why are some strings read-only?
- how do heap allocators track chunks?
- how do function pointers work internally?
- how does vtable corruption happen in C++?

---

