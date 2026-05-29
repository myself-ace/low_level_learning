# Linux Internals for Exploit Development

> Comprehensive notes covering Processes, Virtual Memory, Stack vs Heap, ELF, System Calls, GDB, /proc, Mitigations, malloc/free, and Heap Internals.

## 1. Processes

### What is a Process?
A process is a running instance of a program.

Components:
- Code (text segment)
- Data
- Heap
- Stack
- Registers
- Open file descriptors
- Signal handlers

### Process Lifecycle
```text
Source Code
    ↓
Compilation
    ↓
Executable (ELF)
    ↓
execve()
    ↓
Running Process
```

### Process States
- Running
- Ready
- Sleeping
- Stopped
- Zombie

### PID and PPID
```bash
ps aux
pstree -p
echo $$
```

### fork()
Creates a child process.

```c
pid_t pid = fork();
```

### execve()
Replaces current process image.

```c
execve("/bin/ls", argv, envp);
```

### Signals
Common signals:

| Signal | Meaning |
|----------|----------|
| SIGINT | Ctrl+C |
| SIGSEGV | Segmentation fault |
| SIGKILL | Force kill |
| SIGTERM | Graceful terminate |

---

# 2. Virtual Memory

## Concept

Every process sees its own virtual address space.

```text
Virtual Address
        ↓
Page Tables
        ↓
Physical Memory
```

Benefits:
- Isolation
- Security
- Efficient allocation

## Memory Layout

```text
High Address
+----------------+
| Stack          |
+----------------+
| mmap Region    |
+----------------+
| Heap           |
+----------------+
| BSS            |
+----------------+
| Data           |
+----------------+
| Text           |
+----------------+
Low Address
```

## Pages

Typical page size:

```bash
getconf PAGE_SIZE
```

Usually:

```text
4096 bytes
```

## Memory Permissions

```text
r = Read
w = Write
x = Execute
```

Examples:

```text
r-x
rw-
rwx
```

## mmap()

Maps memory into process space.

```c
mmap(...);
```

Uses:
- Shared libraries
- Anonymous memory
- File mapping

---

# 3. Stack vs Heap

## Stack

Stores:
- Local variables
- Function arguments
- Return addresses

Example:

```c
void func() {
    int x = 10;
}
```

Characteristics:
- Fast
- Automatic
- Limited size

## Heap

Dynamic allocation.

```c
int *ptr = malloc(100);
```

Characteristics:
- Large
- Manual management
- Slower

## Comparison

| Stack | Heap |
|---------|---------|
| Automatic | Manual |
| Fast | Slower |
| Limited | Large |
| LIFO | Arbitrary |

## Stack Frame

```text
+----------------+
| Arguments      |
+----------------+
| Return Address |
+----------------+
| Saved RBP      |
+----------------+
| Local Vars     |
+----------------+
```

---

# 4. ELF Files

Executable and Linkable Format.

## View ELF

```bash
file binary
readelf -h binary
```

## Sections

| Section | Purpose |
|----------|----------|
| .text | Code |
| .data | Initialized globals |
| .bss | Uninitialized globals |
| .rodata | Constants |
| .plt | Procedure linkage |
| .got | Global offset table |

## Useful Commands

```bash
readelf -a binary
objdump -d binary
nm binary
```

## Dynamic Linking

```bash
ldd binary
```

---

# 5. System Calls

Userland cannot directly access kernel functionality.

Uses syscalls.

Examples:

```c
read()
write()
open()
close()
fork()
execve()
mmap()
```

## Flow

```text
User Program
      ↓
System Call
      ↓
Kernel
      ↓
Hardware
```

## Observe Syscalls

```bash
strace ./program
```

## Observe Library Calls

```bash
ltrace ./program
```

---

# 6. GDB

GNU Debugger.

## Launch

```bash
gdb ./program
```

## Common Commands

### Run

```gdb
run
r
```

### Breakpoints

```gdb
break main
b *0x401000
```

### Continue

```gdb
continue
c
```

### Step

```gdb
step
s
```

### Next

```gdb
next
n
```

### Registers

```gdb
info registers
```

### Memory

```gdb
x/20gx $rsp
```

### Backtrace

```gdb
bt
```

### Disassembly

```gdb
disassemble main
```

## pwndbg

Recommended extension.

Features:
- Heap visualization
- Register display
- Memory maps

---

# 7. /proc Filesystem

Kernel-generated virtual filesystem.

## Process Info

```bash
/proc/PID/
```

## Important Files

### maps

```bash
cat /proc/self/maps
```

Shows:

```text
Address Range
Permissions
Mapped File
```

### status

```bash
cat /proc/self/status
```

### mem

Raw process memory.

### fd

Open file descriptors.

```bash
ls -la /proc/self/fd
```

---

# 8. Security Mitigations

## ASLR

Address Space Layout Randomization.

Randomizes:

- Stack
- Heap
- Libraries
- Binary base

Check:

```bash
cat /proc/sys/kernel/randomize_va_space
```

## NX

Non-executable memory.

Prevents:

```text
Stack shellcode execution
```

## PIE

Position Independent Executable.

Binary loads at random base.

Check:

```bash
readelf -h binary
```

## Stack Canaries

Protects return address.

Flow:

```text
Buffer
Canary
Saved RBP
Return Address
```

Overflow modifies canary → crash.

## RELRO

Protects GOT.

Types:

- Partial RELRO
- Full RELRO

## Check Security

```bash
checksec binary
```

---

# 9. malloc() and free()

## malloc()

Allocates heap memory.

```c
char *buf = malloc(100);
```

## free()

Releases memory.

```c
free(buf);
```

## calloc()

```c
calloc(n, size);
```

Zero-initialized.

## realloc()

```c
realloc(ptr, size);
```

Resize allocation.

## Common Bugs

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

### Heap Overflow

```c
malloc(10);
memcpy(ptr, input, 100);
```

---

# 10. Heap Internals (glibc)

## Heap Chunk Structure

Simplified:

```text
+----------------+
| Prev Size      |
+----------------+
| Size           |
+----------------+
| User Data      |
+----------------+
```

## Chunk States

- Allocated
- Free

## Bins

glibc stores freed chunks.

### Tcache

Fastest.

Per-thread cache.

### Fastbins

Small chunks.

### Small Bins

Medium chunks.

### Large Bins

Large allocations.

### Unsorted Bin

Temporary holding area.

## Allocation Flow

```text
malloc()
    ↓
Tcache
    ↓
Fastbin
    ↓
Small Bin
    ↓
Large Bin
    ↓
New Heap Memory
```

## Free Flow

```text
free()
    ↓
Tcache
    ↓
Bins
```

## Important Exploitation Concepts

### Heap Overflow

Overwrite adjacent chunks.

### Use After Free

Reuse freed chunk.

### Double Free

Insert same chunk twice.

### Tcache Poisoning

Corrupt tcache freelist.

### Unsorted Bin Attack

Manipulate unsorted bin metadata.

### House of Spirit

Fake chunk technique.

### House of Force

Top chunk abuse.

### House of Einherjar

Heap metadata manipulation.

---

# Crash Analysis Workflow

```text
Fuzzer Finds Crash
        ↓
Reproduce
        ↓
GDB Analysis
        ↓
Root Cause
        ↓
Determine Exploitability
        ↓
Develop Exploit
```

# Commands Every Exploit Developer Should Know

```bash
gdb
pwndbg
objdump
readelf
nm
file
strings
checksec
strace
ltrace
vmmap
pmap
cat /proc/self/maps
```

# Learning Order

1. C Programming
2. Memory Layout
3. Processes
4. Virtual Memory
5. ELF
6. GDB
7. System Calls
8. /proc
9. Security Mitigations
10. malloc/free
11. Heap Internals
12. Fuzzing
13. Exploit Development
