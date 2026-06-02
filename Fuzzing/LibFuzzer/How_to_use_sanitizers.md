# Sanitizers for Vulnerability Research & Fuzzing

## Overview

Sanitizers are compiler instrumentation tools that detect bugs at runtime by inserting additional checks into the program.

Purpose:

* Turn silent corruption into deterministic crashes
* Detect memory safety violations
* Detect undefined behavior
* Improve fuzzing effectiveness
* Provide detailed crash reports

---

# AddressSanitizer (ASan)

## Purpose

Detects memory safety violations.

Core Question:

> "Did the program access memory it should not access?"

---

## Detects

### Stack Buffer Overflow

```c
char buf[8];
buf[20] = 'A';
```

### Heap Buffer Overflow

```c
char *buf = malloc(8);
buf[20] = 'A';
```

### Use After Free

```c
free(ptr);
ptr[0] = 'A';
```

### Double Free

```c
free(ptr);
free(ptr);
```

### Invalid Free

```c
int x;
free(&x);
```

### Stack Use After Return

```c
char *f() {
    char buf[10];
    return buf;
}
```

---

## Does NOT Detect

* Integer Overflow
* Uninitialized Reads
* Race Conditions
* Most Logic Bugs

---

## Compilation

```bash
clang -fsanitize=address program.c -o program
```

Debug-friendly build:

```bash
clang -g -O1 -fsanitize=address -fno-omit-frame-pointer program.c -o program
```

---

## Common Runtime Options

```bash
ASAN_OPTIONS=detect_leaks=1 ./program
```

```bash
ASAN_OPTIONS=abort_on_error=1 ./program
```

```bash
ASAN_OPTIONS=halt_on_error=1 ./program
```

---

## Why Researchers Care

ASan converts:

```text
Silent Memory Corruption
        ↓
Immediate Crash
        ↓
Crash Report
        ↓
Bug Discovery
```

---

# UndefinedBehaviorSanitizer (UBSan)

## Purpose

Detects Undefined Behavior (UB).

Core Question:

> "Did the program perform an operation that the C/C++ standard does not define?"

---

## Detects

### Signed Integer Overflow

```c
int x = INT_MAX;
x++;
```

### Division By Zero

```c
10 / 0;
```

### Invalid Shift Operations

```c
1 << 100;
```

### Misaligned Access

```c
int *p = (int*)((char*)buf + 1);
```

### Invalid Type Conversions

```c
bad casts
```

---

## Does NOT Detect

* Heap Overflow
* Stack Overflow
* Use After Free
* Double Free

---

## Compilation

```bash
clang -fsanitize=undefined program.c -o program
```

Debug build:

```bash
clang -g -fsanitize=undefined program.c -o program
```

---

## Why Researchers Care

Many vulnerabilities start before memory corruption occurs.

Example:

```c
size = user_input + 100;
```

Integer overflow:

```c
size = 50;
```

Then:

```c
malloc(size);
```

Then:

```c
memcpy(...)
```

Chain:

```text
Undefined Behavior
        ↓
Wrong Calculation
        ↓
Wrong Allocation
        ↓
Memory Corruption
```

---

# MemorySanitizer (MSan)

## Purpose

Detects uninitialized memory usage.

Core Question:

> "Where did this data come from?"

---

## Detects

### Uninitialized Reads

```c
int x;
printf("%d", x);
```

### Uninitialized Heap Memory

```c
char *buf = malloc(100);
printf("%c", buf[0]);
```

### Branches Using Uninitialized Data

```c
int x;

if(x == 5)
{
}
```

---

## Does NOT Detect

* Heap Overflow
* Stack Overflow
* Use After Free
* Integer Overflow

---

## Compilation

```bash
clang -fsanitize=memory program.c -o program
```

Debug build:

```bash
clang -g -fsanitize=memory program.c -o program
```

---

## Why Researchers Care

Uninitialized memory can leak:

* Heap addresses
* Stack addresses
* Function pointers
* Secrets
* Cryptographic material

Possible result:

```text
Information Leak
        ↓
ASLR Bypass
        ↓
Exploit Development
```

---

# LeakSanitizer (LSan)

## Purpose

Detects memory leaks.

---

## Detects

```c
malloc(100);
```

Memory never freed.

---

## Compilation

Usually enabled automatically with ASan.

```bash
clang -fsanitize=address
```

---

## Dedicated Build

```bash
clang -fsanitize=leak program.c -o program
```

---

## Why Researchers Care

Useful during development.

Rarely directly useful for exploitation.

---

# ThreadSanitizer (TSan)

## Purpose

Detects race conditions.

Core Question:

> "Can multiple threads access the same memory unsafely?"

---

## Detects

### Data Races

```c
counter++;
```

Executed simultaneously by multiple threads.

---

## Compilation

```bash
clang -fsanitize=thread program.c -o program
```

---

## Why Researchers Care

Race conditions can lead to:

* Kernel vulnerabilities
* Browser vulnerabilities
* Use After Free conditions
* Privilege escalation

---

# Recommended Fuzzing Build

## ASan

```bash
clang -g -O1 \
-fsanitize=address \
-fno-omit-frame-pointer \
target.c -o target
```

---

## ASan + UBSan

```bash
clang -g -O1 \
-fsanitize=address,undefined \
-fno-omit-frame-pointer \
target.c -o target
```

---

# Reading Sanitizer Reports

Always identify:

## 1. Bug Type

Example:

```text
heap-buffer-overflow
```

---

## 2. Access Type

```text
READ
```

or

```text
WRITE
```

WRITE bugs are generally more powerful.

---

## 3. Access Size

```text
READ of size 1
READ of size 8
WRITE of size 16
```

---

## 4. Fault Location

Where corruption happened.

---

## 5. Allocation Location

Where the object was originally created.

---

# Bug → Sanitizer Mapping

| Bug Type              | Sanitizer |
| --------------------- | --------- |
| Stack Buffer Overflow | ASan      |
| Heap Buffer Overflow  | ASan      |
| Use After Free        | ASan      |
| Double Free           | ASan      |
| Invalid Free          | ASan      |
| Integer Overflow      | UBSan     |
| Division By Zero      | UBSan     |
| Invalid Shift         | UBSan     |
| Uninitialized Read    | MSan      |
| Memory Leak           | LSan      |
| Race Condition        | TSan      |

---

# Researcher's Mental Model

Do NOT think:

```text
Bug
↓
Crash
```

Think:

```text
Bug
↓
Memory Corruption
↓
Primitive
↓
Control
↓
Exploitation
```

Questions to Ask:

1. What memory object was affected?
2. What primitive did I gain?
3. Is it a read or write?
4. Can I control it?
5. Can it become code execution?
6. Can it become an information leak?
7. Can it cross a security boundary?

```
```
