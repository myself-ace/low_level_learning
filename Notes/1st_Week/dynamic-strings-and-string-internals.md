# Day 04 — Dynamic Strings and String Internals

## Goal

Understand:
- dynamic string management
- string memory layout
- heap-backed strings
- resizing behavior
- ownership rules
- internal string structures
- common string vulnerabilities
- efficient string handling

This topic matters because:
- C has no built-in safe string type
- most real-world bugs involve string misuse
- exploit development heavily targets unsafe string handling
- dynamic strings are used everywhere:
  - parsers
  - interpreters
  - web servers
  - allocators
  - shells
  - databases

---

# Core Problem with C Strings

Normal C strings are:
```c
char *str;
```

or:

```c
char str[64];
```

Problems:
- fixed size
- manual resizing
- no automatic bounds tracking
- easy overflow
- ownership confusion
- expensive concatenation

---

# Basic C String Model

Example:

```c
char str[] = "hello";
```

Memory:

```text
Address     Value

0x1000      'h'
0x1001      'e'
0x1002      'l'
0x1003      'l'
0x1004      'o'
0x1005      '\0'
```

Critical:
```text
NULL terminator defines string end
```

---

# Why Dynamic Strings Exist

Static strings are restrictive.

Example problem:

```c
char buf[32];

strcat(buf, user_input);
```

Risk:
- overflow
- truncation
- corruption

Dynamic strings solve:
- resizing
- length tracking
- capacity management

---

# Basic Dynamic String Idea

Instead of:

```c
char buf[64];
```

Use:

```c
char *buf = malloc(initial_size);
```

Now memory can:
- grow
- shrink
- move
- be reallocated

---

# Core Dynamic String Structure

Typical implementation:

```c
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} DynStr;
```

---

# Meaning of Each Field

| Field | Purpose |
|---|---|
| data | actual character buffer |
| length | current string size |
| capacity | total allocated memory |

---

# Mental Model

Example:

```text
DynStr

data     -> 0x55555555
length   -> 5
capacity -> 16
```

Heap memory:

```text
0x55555555

h e l l o \0 ? ? ? ? ? ? ? ?
```

---

# Why Capacity Matters

Without capacity tracking:
- every append requires malloc
- performance becomes terrible

Dynamic strings allocate:
```text
extra unused memory
```

for future growth.

---

# Important Relationship

```text
length <= capacity
```

Always.

---

# Basic Dynamic String Lifecycle

---

# 1. Initialization

Example:

```c
DynStr *s = dynstr_create();
```

Possible internals:

```c
s->capacity = 16;
s->length = 0;

s->data = malloc(16);
```

---

# 2. Append Data

Example:

```c
dynstr_append(s, "hello");
```

Updates:
- memory contents
- length

---

# 3. Resize if Needed

If:
```text
length >= capacity
```

Allocator expands buffer.

Usually using:
```c
realloc()
```

---

# 4. Free Memory

Example:

```c
dynstr_free(s);
```

Must free:
- internal buffer
- structure itself

---

# realloc() Theory

Very important function.

Example:

```c
ptr = realloc(ptr, new_size);
```

Possible outcomes:
- buffer expanded in place
- memory moved elsewhere
- old pointer invalidated

---

# Dangerous realloc Pattern

Bad:

```c
ptr = realloc(ptr, new_size);
```

Problem:
- if realloc fails
- original pointer lost

Safer:

```c
char *tmp = realloc(ptr, new_size);

if (tmp != NULL) {
    ptr = tmp;
}
```

---

# Growth Strategy

Good dynamic strings do NOT grow:
```text
+1 byte each append
```

Terrible performance.

Instead:

```text
capacity *= 2
```

Common strategy:
- 16
- 32
- 64
- 128
- 256

---

# Why Doubling Matters

Without exponential growth:
- repeated allocations
- memory copying overhead
- O(n²) behavior

With doubling:
```text
append becomes amortized O(1)
```

---

# Basic dynstr_create()

Possible implementation:

```c
DynStr *dynstr_create() {

    DynStr *s = malloc(sizeof(DynStr));

    s->capacity = 16;
    s->length = 0;

    s->data = malloc(s->capacity);

    s->data[0] = '\0';

    return s;
}
```

---

# Basic dynstr_append()

Core logic:

```c
void dynstr_append(DynStr *s, const char *text) {

    size_t add_len = strlen(text);

    if (s->length + add_len + 1 >= s->capacity) {

        s->capacity *= 2;

        s->data = realloc(s->data, s->capacity);
    }

    strcat(s->data, text);

    s->length += add_len;
}
```

---

# Important Internal Operations

Appending involves:
- finding destination
- checking capacity
- resizing if needed
- copying bytes
- updating metadata

---

# Null Terminator Handling

Critical rule:

```text
Always reserve space for '\0'
```

Bug:

```c
malloc(length);
```

Correct:

```c
malloc(length + 1);
```

---

# Ownership Rules

Very important concept.

Question:
```text
who owns allocated memory?
```

If ownership unclear:
- leaks happen
- double frees happen
- UAF happens

---

# Good Ownership Rule

If function allocates:
```text
caller frees
```

Must be documented clearly.

---

# Common Dynamic String Functions

| Function | Purpose |
|---|---|
| create | initialize string |
| append | add text |
| clear | reset contents |
| resize | grow buffer |
| free | release memory |
| copy | duplicate string |
| length | get size |

---

# Useful Operations

---

# Append Character

```c
dynstr_push(s, 'A');
```

---

# Append String

```c
dynstr_append(s, "hello");
```

---

# Clear String

```c
dynstr_clear(s);
```

Usually:
```c
length = 0;
data[0] = '\0';
```

---

# Resize Buffer

```c
dynstr_reserve(s, 256);
```

Pre-allocates memory.

Useful for:
- performance
- avoiding repeated realloc

---

# Memory Layout Example

Suppose:

```c
dynstr_append(s, "hello");
```

Structure:

```text
Stack:

s -> 0x5000
```

Heap:

```text
0x5000

data     -> 0x6000
length   -> 5
capacity -> 16
```

Heap buffer:

```text
0x6000

h e l l o \0 ? ? ? ? ? ? ?
```

---

# Dangerous Bugs in Dynamic Strings

---

# Buffer Overflow

Example:

```c
strcpy(s->data, large_input);
```

without capacity check.

Result:
- heap corruption
- allocator metadata overwrite

---

# Use After Free

Example:

```c
dynstr_free(s);

printf("%s", s->data);
```

Danger:
- dangling pointers
- unpredictable behavior

---

# Double Free

Example:

```c
dynstr_free(s);
dynstr_free(s);
```

Can corrupt allocator internals.

---

# Memory Leak

Example:

```c
s->data = realloc(s->data, bigger);
```

without cleanup on failure.

---

# Length Mismatch Bugs

Example:

```c
length says 20
actual buffer contains 5 chars
```

Creates:
- invalid reads
- parser bugs
- overflow risks

---

# Off-by-One Errors

Classic mistake:

```c
malloc(length);
```

instead of:

```c
malloc(length + 1);
```

Null terminator overwrites adjacent memory.

---

# Important String Library Functions

---

# strlen()

Traverses memory until:
```text
'\0'
```

Time complexity:
```text
O(n)
```

Repeated strlen calls are expensive.

Dynamic strings track length directly:
```text
O(1)
```

Huge improvement.

---

# strcpy()

Copies bytes blindly.

Dangerous because:
- no bounds checking

---

# strncpy()

Safer but tricky.

Can:
- omit NULL terminator
- create hidden bugs

---

# strcat()

Very inefficient repeatedly.

Why:
- scans destination every append

Dynamic strings avoid this using:
```text
stored length
```

---

# memcpy() vs strcpy()

| memcpy | strcpy |
|---|---|
| raw bytes | null-terminated strings |
| faster | safer for strings |
| no terminator logic | stops at '\0' |

---

# Binary Data Problem

Normal strings fail for binary data.

Reason:
```text
'\0' terminates string early
```

Dynamic buffers often better for:
- packet parsing
- shellcode
- file formats

---

# Real World Dynamic String Libraries

| Library | Language |
|---|---|
| std::string | C++ |
| SDS (Redis) | C |
| GString | GLib |
| CString | Rust internals |
| Vec<u8> | Rust |

---

# Redis SDS Design

Very important real-world example.

Stores:
- length
- allocation size
- flags

Avoids repeated:
```c
strlen()
```

Very high performance.

---

# Why Dynamic Strings Matter in Exploitation

String bugs commonly lead to:
- heap overflow
- stack overflow
- arbitrary write
- metadata corruption
- control flow hijacking

Unsafe string handling is historically one of the biggest vulnerability sources in C.

---

# Experiments I Ran

---

# Experiment 1 — Manual Heap String

```c
char *buf = malloc(16);

strcpy(buf, "hello");
```

Observation:
- string stored on heap
- pointer stored separately

---

# Experiment 2 — realloc()

```c
buf = realloc(buf, 64);
```

Observation:
- address sometimes changed
- old pointer invalid after move

---

# Experiment 3 — Overflow

```c
char *buf = malloc(8);

strcpy(buf, "AAAAAAAAAAAAAAAA");
```

Observation:
- heap corruption possible
- ASAN detects overflow immediately

---

# Experiment 4 — Missing NULL Terminator

```c
char buf[3] = {'a','b','c'};
```

Observation:
- printf reads beyond buffer
- undefined behavior

---

# Important Realizations

- strings are just byte arrays
- NULL terminator is critical
- dynamic strings require metadata
- capacity tracking matters heavily
- realloc can invalidate pointers
- ownership rules prevent many bugs
- most unsafe string bugs are memory corruption bugs

---

# Commands Used

Compile:

```bash
gcc -Wall -Wextra -g test.c
```

With ASAN:

```bash
gcc -fsanitize=address -g test.c
```

Run:

```bash
./a.out
```

---

# Useful GDB Commands

View heap memory:

```bash
x/32bx address
```

View pointers:

```bash
print ptr
```

View structures:

```bash
print *s
```

---


---

