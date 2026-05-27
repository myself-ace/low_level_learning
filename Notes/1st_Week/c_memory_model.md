# Day 01 — C Memory Model

## Goal

Build a mental model of:
- process memory layout
- stack vs heap
- pointers
- memory addresses
- function memory behavior
- dynamic allocation

---

# Process Memory Layout

Typical Linux process memory:

```text
+----------------------+
|       Stack          |  <- grows downward
+----------------------+
|        mmap          |
+----------------------+
|        Heap          |  <- grows upward
+----------------------+
|        BSS           |
+----------------------+
|       Data           |
+----------------------+
|       Text           |
+----------------------+
```

---

# Core Idea

Different types of data live in different memory regions.

| Region | Stores |
|---|---|
| Text | program instructions |
| Data | initialized globals |
| BSS | uninitialized globals |
| Heap | dynamic allocations |
| Stack | local variables/function calls |

---

# Stack

## What Lives Here

- local variables
- function arguments
- return addresses
- saved registers

Example:

```c
void test() {
    int x = 10;
    char buffer[32];
}
```

Possible stack layout:

```text
| Return Address |
| Saved RBP      |
| x              |
| buffer[32]     |
```

---

## Important Properties

- automatically managed
- very fast
- limited size
- grows downward

---

## Stack Growth

On most systems:

```text
Higher Address
0x7fffffffe000
|
|
v
0x7fffffffd000
Lower Address
```

New stack frames usually move toward lower addresses.

---

# Heap

## What Lives Here

Memory allocated during runtime.

Functions used:
- malloc()
- calloc()
- realloc()
- free()

Example:

```c
int *ptr = malloc(sizeof(int));
```

---

## Important Properties

- manually managed
- flexible size
- slower than stack
- survives after function returns

---

## Heap Growth

Usually grows upward.

```text
0x555555554000
^
|
|
0x555555556000
```

---

# Stack vs Heap

| Stack | Heap |
|---|---|
| automatic | manual |
| fast | slower |
| temporary | persistent |
| limited | larger |
| compiler managed | programmer managed |

---

# Pointer Mental Model

## Critical Concept

A pointer is just a variable storing an address.

Example:

```c
int x = 99;
int *ptr = &x;
```

Memory model:

```text
Stack:

x       = 99
ptr     = 0x1234

0x1234 -> 99
```

---

# Dereferencing

```c
*ptr
```

Means:
```text
"go to the address stored inside ptr"
```

---

# Difference Between ptr and &ptr

| Expression | Meaning |
|---|---|
| ptr | address being stored |
| &ptr | address of pointer itself |
| *ptr | value at pointed location |

---

# Arrays vs Pointers

This confused me initially.

## Array

```c
char buf[16];
```

Memory:
- fixed size
- actual storage allocated immediately

---

## Pointer

```c
char *buf;
```

Memory:
- only pointer exists
- no actual buffer allocated yet

---

# Function Calls

Each function creates a new stack frame.

Example:

```c
void a() {
    int x = 1;
}

void b() {
    int y = 2;
}
```

During execution:
- stack frame for `a()`
- later removed
- stack frame for `b()`

Stack constantly changes during runtime.

---

# Return Address

Very important for exploitation.

When a function is called:

```c
func();
```

CPU stores:
```text
where execution should return
```

on the stack.

Overwriting this creates classic buffer overflow exploits.

---

# Example Vulnerable Code

```c
void vuln() {
    char buf[16];

    gets(buf);
}
```

Problem:
- no bounds checking
- input larger than 16 bytes overwrites nearby memory

Potential overwrite targets:
- saved registers
- return address

---

# malloc() Behavior

Example:

```c
int *ptr = malloc(sizeof(int));
```

What happens:
1. heap memory requested
2. allocator returns address
3. pointer stores that address

---

# free()

```c
free(ptr);
```

Important:
- memory becomes reusable
- pointer still contains old address

Dangerous:

```c
*ptr = 10;
```

after free.

This creates:
```text
Use After Free (UAF)
```

---

# Dangling Pointer

Example:

```c
int *ptr = malloc(4);

free(ptr);
```

Now:

```text
ptr -> invalid memory
```

Safe practice:

```c
free(ptr);
ptr = NULL;
```

---

# Memory Leak

Example:

```c
void test() {
    malloc(100);
}
```

Problem:
- allocated memory lost forever
- cannot be freed anymore

Repeated leaks waste memory.

---

# Important Memory Bugs

| Bug | Cause |
|---|---|
| Stack Overflow | writing beyond stack buffer |
| Heap Overflow | writing beyond heap chunk |
| Use After Free | using freed memory |
| Double Free | freeing same memory twice |
| Memory Leak | forgetting free() |
| Dangling Pointer | pointer to invalid memory |

---

# Endianness

Most modern systems:
```text
Little Endian
```

Example:

```text
0x12345678
```

Stored in memory as:

```text
78 56 34 12
```

Important for exploit payload construction.

---

# Experiments I Ran

## Experiment 1 — Stack Addresses

```c
int x = 10;

printf("%p\n", &x);
```

Observation:
- stack addresses often near:
```text
0x7ffffffff...
```

---

## Experiment 2 — Heap Addresses

```c
int *ptr = malloc(4);

printf("%p\n", ptr);
```

Observation:
- heap addresses separate from stack
- usually lower than stack region

---

## Experiment 3 — Use After Free

```c
free(ptr);

printf("%d\n", *ptr);
```

Observation:
- sometimes still works
- sometimes crashes
- behavior unreliable

---

# Important Realizations

- pointer != pointee
- stack memory disappears after function returns
- free() does not erase memory
- memory corruption bugs happen easily in C
- addresses are just numbers
- local variables are temporary

---

# Commands Used

Compile with debug info:

```bash
gcc -Wall -Wextra -g test.c
```

Disable protections for experiments:

```bash
gcc test.c -o test \
-fno-stack-protector \
-no-pie
```

Run:

```bash
./test
```

---

# GDB Commands

View registers:

```bash
info registers
```

View stack memory:

```bash
x/32gx $rsp
```

Disassemble function:

```bash
disassemble main
```

---

# Things That Confused Me

- arrays vs pointers
- why * dereferences
- stack frame layout
- why stack grows downward
- why freed memory sometimes still works

---

# Questions To Revisit

- how exactly does malloc work internally?
- what does glibc allocator do?
- how are stack frames generated?
- where are string literals stored?
- how does ASLR randomize addresses?
- how does a return address actually get pushed?

---

# Next Topics

- calling conventions
- stack frames
- ELF internals
- GDB basics
- function prologue/epilogue
- stack canaries
- ASLR
- basic buffer overflows
