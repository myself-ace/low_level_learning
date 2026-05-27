# Day 03 — Memory Debugging with Valgrind and ASAN

## Goal

Understand:
- memory debugging workflows
- detecting memory corruption
- use-after-free detection
- leak detection
- invalid memory access
- sanitizer reports
- Valgrind internals
- ASAN behavior
- debugging patterns

This is critical for:
- exploit development
- secure C programming
- vulnerability research
- reverse engineering
- crash analysis

---

# Why Memory Debugging Matters

C gives direct memory access.

This also means:
- no automatic bounds checking
- no automatic garbage collection
- easy memory corruption
- undefined behavior

Most serious C vulnerabilities come from:
- invalid memory access
- bad pointer handling
- unsafe string operations

---

# Main Tools

| Tool | Purpose |
|---|---|
| Valgrind | runtime memory analysis |
| ASAN | compiler-based memory sanitizer |

---

# High Level Difference

| Valgrind | ASAN |
|---|---|
| slower | much faster |
| dynamic instrumentation | compiler instrumentation |
| no recompilation required | requires recompilation |
| deeper leak tracking | better crash visibility |
| very detailed | easier to use |
| catches uninitialized reads | excellent overflow detection |

---

# Valgrind Overview

Valgrind observes:
- allocations
- frees
- reads
- writes
- leaks
- invalid memory usage

Runs program inside a monitored environment.

---

# Basic Valgrind Usage

```bash
valgrind ./a.out
```

---

# Recommended Compilation

```bash
gcc -Wall -Wextra -g test.c
```

Important:
```text
-g adds debug symbols
```

Without debug symbols:
- reports become harder to read

---

# Most Important Valgrind Tool

```text
Memcheck
```

Detects:
- invalid reads
- invalid writes
- leaks
- use-after-free
- double free
- uninitialized memory usage

---

# Important Valgrind Switches

---

## Leak Checking

```bash
valgrind --leak-check=full ./a.out
```

Shows:
- exact leak locations
- allocation traces

---

## Show Leak Kinds

```bash
valgrind --show-leak-kinds=all ./a.out
```

Displays:
- definitely lost
- indirectly lost
- possibly lost
- still reachable

---

## Track Origins

```bash
valgrind --track-origins=yes ./a.out
```

Very important.

Tracks:
```text
where uninitialized values originated
```

Slower but extremely useful.

---

## Verbose Mode

```bash
valgrind -v ./a.out
```

More internal details.

---

## Log File Output

```bash
valgrind --log-file=valgrind.log ./a.out
```

Stores output separately.

Useful for:
- large projects
- automation
- repeated testing

---

# Understanding Valgrind Errors

---

# Invalid Read

Example:

```c
int arr[3];

printf("%d", arr[5]);
```

Possible report:

```text
Invalid read of size 4
```

Meaning:
```text
program read memory outside valid region
```

---

# Invalid Write

Example:

```c
arr[5] = 100;
```

Report:

```text
Invalid write of size 4
```

Meaning:
```text
memory corruption attempt
```

Very dangerous.

---

# Use After Free

Example:

```c
free(ptr);

*ptr = 10;
```

Valgrind:

```text
Invalid write of size 4
Address ... is 0 bytes inside a block of size ...
freed at ...
```

Pattern:
- report references previously freed block

Huge red flag.

---

# Double Free

Example:

```c
free(ptr);
free(ptr);
```

Valgrind report:

```text
Invalid free()
```

Usually indicates:
- ownership bug
- allocator corruption risk

---

# Uninitialized Memory Usage

Example:

```c
int x;

printf("%d", x);
```

Report:

```text
Conditional jump or move depends on uninitialised value(s)
```

Important pattern:
```text
program behavior depends on garbage data
```

---

# Memory Leak Categories

---

## Definitely Lost

Worst leak type.

Example:
```c
malloc(100);
```

Pointer lost forever.

Meaning:
```text
no way to recover allocation
```

---

## Indirectly Lost

Child allocations lost because parent lost.

Common in:
- linked lists
- trees
- structures

---

## Possibly Lost

Valgrind uncertain about pointer validity.

Sometimes:
- false positives
- weird pointer arithmetic

---

## Still Reachable

Memory still referenced during program exit.

Usually:
- not severe
- libraries commonly do this

---

# Reading Stack Traces

Valgrind often shows:

```text
at 0x...
by 0x...
```

Read bottom-to-top:
1. allocation source
2. intermediate calls
3. crash location

---

# ASAN Overview

ASAN:
```text
Address Sanitizer
```

Compiler-based memory bug detector.

Integrated into:
- GCC
- Clang

---

# ASAN Compilation

```bash
gcc -fsanitize=address -g test.c
```

Run normally:

```bash
./a.out
```

---

# Why ASAN Is Powerful

ASAN adds:
- redzones
- shadow memory
- bounds checking
- allocation tracking

Around allocations.

Detects corruption immediately.

---

# Important ASAN Features

Detects:
- stack overflow
- heap overflow
- use-after-free
- double free
- global overflow
- invalid access

---

# ASAN Runtime Options

---

## Detect Leaks

```bash
ASAN_OPTIONS=detect_leaks=1 ./a.out
```

---

## Abort Immediately

```bash
ASAN_OPTIONS=abort_on_error=1 ./a.out
```

Useful for:
- debugging
- CI pipelines

---

## Verbose Mode

```bash
ASAN_OPTIONS=verbosity=1 ./a.out
```

---

## Detect Stack Use After Return

```bash
ASAN_OPTIONS=detect_stack_use_after_return=1 ./a.out
```

Important for:
- dangling stack references

---

# ASAN Error Patterns

---

# Heap Buffer Overflow

Example:

```c
char *buf = malloc(8);

buf[20] = 'A';
```

ASAN:

```text
heap-buffer-overflow
```

Very explicit.

Usually includes:
- exact invalid offset
- allocation trace
- shadow memory dump

---

# Stack Buffer Overflow

Example:

```c
char buf[8];

strcpy(buf, large_input);
```

ASAN:

```text
stack-buffer-overflow
```

Often shows:
- variable layout
- exact corrupted object

Extremely useful.

---

# Use After Free

Example:

```c
free(ptr);

ptr[0] = 1;
```

ASAN:

```text
heap-use-after-free
```

Usually easier to understand than Valgrind output.

---

# Global Buffer Overflow

Example:

```c
char global[8];

global[20] = 'A';
```

ASAN:

```text
global-buffer-overflow
```

---

# Reading ASAN Reports

ASAN reports typically contain:

| Section | Meaning |
|---|---|
| ERROR | vulnerability type |
| stack trace | crash location |
| allocation trace | where memory allocated |
| free trace | where freed |
| shadow bytes | memory poisoning info |

---

# Shadow Memory

ASAN internally tracks memory validity using:
```text
shadow memory
```

Marks regions:
- accessible
- freed
- redzone
- poisoned

---

# Redzones

ASAN places protected regions around allocations.

Example:

```text
[ REDZONE ][ VALID BUFFER ][ REDZONE ]
```

Overflow hits redzone:
- immediate detection

---

# Common Debugging Patterns

---

# Pattern — Crash After free()

Usually:
```text
use-after-free
```

Look for:
- old pointer reuse
- ownership confusion

---

# Pattern — Crash Only Sometimes

Usually:
- undefined behavior
- uninitialized memory
- race condition

Non-deterministic bugs are dangerous.

---

# Pattern — Different Crash Locations

Often:
```text
memory corruption happened earlier
```

Actual corruption source may be far away.

---

# Pattern — Invalid Read Near NULL

Example:

```text
0x000000000004
```

Usually:
```text
NULL pointer dereference
```

---

# Pattern — Large Weird Addresses

Example:

```text
0x4141414141414141
```

Often:
```text
overflowed with attacker-controlled data
```

Classic exploitation sign.

---

# Pattern — Segfault in strcpy()

Often:
- invalid destination
- missing allocation
- no NULL terminator

---

# Comparing Valgrind vs ASAN

| Feature | Valgrind | ASAN |
|---|---|---|
| Speed | slow | fast |
| Accuracy | very high | high |
| Leak detection | excellent | good |
| Overflow detection | decent | excellent |
| Setup | easy | requires compile flags |
| Production suitability | poor | better |
| Stack traces | okay | excellent |

---

# Recommended Workflow

---

## During Development

Use:
```bash
-fsanitize=address
```

Fast feedback.

---

## Deep Analysis

Use:
```bash
valgrind --track-origins=yes
```

Better root cause investigation.

---

# Dangerous Functions To Watch

| Function | Risk |
|---|---|
| gets() | no bounds checking |
| strcpy() | overflow |
| strcat() | overflow |
| sprintf() | overflow |
| scanf("%s") | overflow risk |

---

# Safer Alternatives

| Unsafe | Safer |
|---|---|
| gets | fgets |
| strcpy | strncpy |
| strcat | strncat |
| sprintf | snprintf |

---

# Practical Debugging Strategy

1. compile with:
```bash
-g -Wall -Wextra
```

2. run with ASAN first

3. if unclear:
```bash
valgrind --track-origins=yes
```

4. reproduce consistently

5. identify:
- allocation site
- free site
- corruption site

6. inspect pointers carefully

---

# Important Realizations

- many crashes occur far from actual bug
- undefined behavior is unpredictable
- freed memory may appear valid temporarily
- corruption often propagates silently
- sanitizer output must be read carefully
- stack traces matter more than raw crash point

---

# Example Compilation Commands

---

## ASAN

```bash
gcc -fsanitize=address -g test.c
```

---

## UBSAN

```bash
gcc -fsanitize=undefined -g test.c
```

---

## Combined

```bash
gcc -fsanitize=address,undefined -g test.c
```

---

# Useful GDB Integration

Run with:

```bash
gdb ./a.out
```

Useful commands:

```bash
run
bt
info registers
x/32gx $rsp
```

---

# Questions To Revisit

- how does shadow memory work internally?
- how does glibc malloc track chunks?
- why are some overflows undetected?
- how do hardened allocators work?
- how does ASLR affect debugging?
- how does heap metadata corruption happen?

---

