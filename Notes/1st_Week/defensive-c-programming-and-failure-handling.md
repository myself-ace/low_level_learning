# Defensive C Programming and Failure Handling

## Goal

Understand how to write:
- resilient C code
- fail-safe low-level systems code
- memory-safe logic
- defensive APIs
- predictable cleanup behavior

Focus:
- failure checking
- ownership
- cleanup
- invariants
- unsafe assumptions
- edge cases
- resource management

This applies to:
- parsers
- string libraries
- hashmaps
- allocators
- linked lists
- socket code
- threads
- system utilities
- exploit development labs

---

# Core Philosophy

C gives:
- direct memory access
- manual resource management
- raw pointers
- zero runtime protection

Meaning:
```text
every unchecked assumption becomes a vulnerability
```

---

# Golden Rule

Every operation can fail.

Assume failure by default.

---

# Things You MUST Always Check

| Operation | Must Check |
|---|---|
| malloc/calloc/realloc | NULL |
| fopen | NULL |
| fread/fwrite | return value |
| socket | negative fd |
| pthread_create | return code |
| strcpy/memcpy | bounds |
| array indexing | limits |
| integer arithmetic | overflow |
| user input | validation |
| pointers | NULL/validity |

---

# malloc() Failure Handling

Bad:

```c
char *buf = malloc(100);

strcpy(buf, "hello");
```

Danger:
```text
NULL dereference if malloc fails
```

Correct:

```c
char *buf = malloc(100);

if (!buf) {
    perror("malloc");
    return FAILURE;
}
```

---

# calloc()

Good for:
```text
zero-initialized memory
```

Example:

```c
int *arr = calloc(count, sizeof(int));
```

Still MUST check:

```c
if (!arr)
```

---

# realloc() Failure Pattern

One of the biggest beginner mistakes.

BAD:

```c
ptr = realloc(ptr, new_size);
```

If realloc fails:
```text
original pointer lost
memory leak created
```

Correct:

```c
void *tmp = realloc(ptr, new_size);

if (!tmp) {
    free(ptr);
    return FAILURE;
}

ptr = tmp;
```

---

# fopen() Checking

Bad:

```c
FILE *fp = fopen("data.bin", "rb");

fread(...)
```

Correct:

```c
FILE *fp = fopen("data.bin", "rb");

if (!fp) {
    perror("fopen");
    return FAILURE;
}
```

---

# fread()/fwrite() Checking

Never assume full read/write.

Correct:

```c
size_t n = fread(buf, 1, size, fp);

if (n != size) {
    if (feof(fp)) {
        fprintf(stderr, "Unexpected EOF\n");
    } else {
        perror("fread");
    }
}
```

---

# Never Trust User Input

Treat ALL external data as hostile:
- files
- sockets
- CLI args
- environment variables
- stdin
- IPC

Validate:
- sizes
- counts
- formats
- ranges
- types

---

# Bounds Checking

Bad:

```c
buf[index] = 'A';
```

without validating:
```c
index < buf_size
```

Classic OOB write.

---

# Integer Overflow Checks

Dangerous:

```c
malloc(count * sizeof(Node));
```

If:
```text
count is huge
```

multiplication may overflow.

Correct:

```c
if (count > SIZE_MAX / sizeof(Node))
```

---

# Null Terminator Safety

Dangerous:

```c
malloc(len);
strcpy(buf, src);
```

Correct:

```c
malloc(len + 1);
```

Always reserve:
```text
space for '\0'
```

---

# Ownership Rules

Most important large-project concept.

Always know:
```text
who owns memory
who frees memory
```

Without ownership rules:
- leaks
- double free
- UAF

happen constantly.

---

# Good Ownership Example

Rule:

```text
creator frees
```

or:

```text
caller owns returned memory
```

Document clearly.

---

# Double Free Prevention

Bad:

```c
free(ptr);
free(ptr);
```

Safer:

```c
free(ptr);
ptr = NULL;
```

---

# Dangling Pointers

Dangerous:

```c
free(ptr);

printf("%s", ptr);
```

Pointer still contains:
```text
old invalid address
```

---

# Wild Pointers

Very dangerous:

```c
char *ptr;
```

Uninitialized pointer.

Always initialize:

```c
char *ptr = NULL;
```

---

# Centralized Cleanup

Bad:

```c
return everywhere
```

Creates:
- leaks
- inconsistent cleanup

Professional pattern:

```c
goto cleanup;
```

---

# Cleanup Pattern

Example:

```c
int func() {

    FILE *fp = NULL;
    char *buf = NULL;

    fp = fopen(...);

    if (!fp)
        goto cleanup;

    buf = malloc(...);

    if (!buf)
        goto cleanup;

cleanup:

    free(buf);

    if (fp)
        fclose(fp);

    return status;
}
```

Very common in:
- kernels
- drivers
- parsers
- security tools

---

# Initialize Structures

Dangerous:

```c
Node n;
```

May contain garbage.

Safer:

```c
Node n = {0};
```

or:

```c
memset(&n, 0, sizeof(n));
```

---

# Check Function Return Values

Bad:

```c
parse(data);
```

without checking result.

Correct:

```c
if (parse(data) != SUCCESS)
```

---

# API Design Rules

Good APIs:
- define ownership
- define failure behavior
- define valid states
- validate arguments
- avoid hidden allocations

---

# Defensive Function Example

Bad:

```c
void hashmap_put(HashMap *m, char *key)
```

without validation.

Better:

```c
int hashmap_put(HashMap *m, const char *key)
```

Checks:
- m != NULL
- key != NULL
- key length valid

Returns:
```text
status code
```

---

# Validate All Inputs

Before using:
- pointer
- index
- length
- count
- offset
- enum

validate first.

---

# Defensive String Handling

Avoid:
- gets
- strcpy
- strcat
- sprintf

Prefer:
- fgets
- snprintf
- memcpy with validated sizes

---

# Safer snprintf()

Bad:

```c
sprintf(buf, "%s", input);
```

Safer:

```c
snprintf(buf, sizeof(buf), "%s", input);
```

---

# memcpy() Safety

Before:

```c
memcpy(dst, src, len);
```

verify:
- dst valid
- src valid
- len correct
- no overlap issues

---

# Assertions

Useful during development.

Example:

```c
assert(ptr != NULL);
```

Good for:
- invariants
- internal assumptions

Not substitute for real validation.

---

# Invariants

Invariant:
```text
condition always true
```

Examples:
- size <= capacity
- count >= 0
- pointer valid

Violation means:
```text
logic corruption
```

---

# Resource Leaks

Resources include:
- memory
- files
- sockets
- mutexes
- threads

Leaks are not just memory-related.

---

# File Descriptor Leaks

Dangerous:

```c
open(...)
```

without:
```c
close(...)
```

Large programs can exhaust descriptors.

---

# Thread Safety

Shared memory requires:
- synchronization
- ownership clarity
- race prevention

Without it:
- corruption
- crashes
- undefined behavior

---

# Defensive HashMap Rules

Must check:
- load factor
- collision handling
- resize failure
- duplicate keys
- NULL keys
- iterator invalidation

---

# Defensive Dynamic Array Rules

Must check:
- capacity overflow
- realloc failure
- bounds
- integer multiplication overflow

---

# Defensive Linked List Rules

Must check:
- NULL nodes
- head/tail updates
- cycles
- ownership
- removal edge cases

---

# Common Dangerous Assumptions

Bad assumptions:
- malloc always succeeds
- file always exists
- input is valid
- integer never wraps
- pointer is initialized
- API always succeeds
- array index valid

---

# Fail Closed Principle

Safer behavior:
```text
reject invalid state immediately
```

Not:
```text
continue and hope
```

---

# Logging Errors

Good errors:
- explain failure
- include context
- avoid ambiguity

Bad:

```text
error
```

Good:

```text
malloc failed while allocating record buffer
```

---

# Important Realizations

- most vulnerabilities come from assumptions
- ownership bugs are everywhere
- cleanup logic matters heavily
- unchecked arithmetic is dangerous
- undefined behavior spreads silently
- defensive code is predictable code

---

# Useful Compiler Flags

Strict warnings:

```bash
gcc -Wall -Wextra -Werror -pedantic
```

Sanitizers:

```bash
gcc -fsanitize=address,undefined
```

Stack protection:

```bash
-fstack-protector-strong
```

---

# Useful Tools

| Tool | Purpose |
|---|---|
| ASAN | memory corruption |
| UBSAN | undefined behavior |
| Valgrind | leaks + invalid access |
| GDB | runtime debugging |

---

# Personal Checklist Before Trusting Code

Before considering code safe:

- all malloc checked
- all fopen checked
- all fread checked
- all lengths validated
- all ownership documented
- cleanup guaranteed
- no unchecked strcpy
- integer overflow considered
- NULL handled
- edge cases tested
- sanitizers run
- Valgrind clean

---

