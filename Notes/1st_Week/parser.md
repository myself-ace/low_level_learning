# Day 05 — Binary Parsing and Serialization Internals

## Goal

Understand:
- binary parsing
- serialization/deserialization
- structured binary formats
- endian conversion
- parser safety
- memory ownership
- validation strategies
- attack surface in parsers

This topic is extremely important because:
- parsers process untrusted input
- most real-world vulnerabilities exist in parsers
- binary formats are everywhere:
  - executables
  - image formats
  - network protocols
  - archives
  - firmware
  - databases

A parser is fundamentally:
```text
transforming raw bytes into structured meaning
```

---

# High Level Concept

Suppose a binary file contains:

```text
DE AD BE EF 00 01 00 00 00 03
```

Humans see:
```text
hex bytes
```

Parser sees:
```text
magic number
version
record count
```

---

# Core Components of a Binary Parser

Most parsers contain:

| Component | Purpose |
|---|---|
| format definition | defines layout |
| reader | extracts bytes |
| validator | rejects invalid data |
| deserializer | converts bytes into structures |
| printer/interpreter | displays meaning |
| cleanup logic | frees resources safely |

---

# High Level File Layout

Your format:

```text
+-------------------+
| Header            |
+-------------------+
| Record 1          |
+-------------------+
| Record 2          |
+-------------------+
| Record N          |
+-------------------+
```

---

# Header Structure

```c
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint32_t record_count;
} Header;
```

Purpose:
- identify format
- ensure compatibility
- define parsing boundaries

---

# Why Magic Numbers Exist

Example:

```c
#define MAGIC 0xDEADBEEF
```

Purpose:
```text
identify expected file type
```

Parser verifies:

```c
if (h->magic != MAGIC)
```

Without validation:
- random files processed incorrectly
- parser confusion
- corruption
- crashes

---

# Versioning

```c
#define VERSION 1
```

Important because:
- formats evolve
- fields change
- compatibility matters

Good parsers reject unsupported versions.

---

# Record Structure

```c
typedef struct {
    uint8_t type;
    uint16_t len;
    uint8_t *data;
} Record;
```

This creates:
```text
variable-length records
```

Very common design pattern.

---

# Why Length Fields Matter

Without explicit lengths:
- parser cannot know boundaries
- over-read becomes likely
- corruption spreads

Length field defines:
```text
how many bytes belong to current object
```

---

# Variable Length Records

Critical parsing concept.

Example:

```text
[type][length][data]
```

Benefits:
- flexible
- compact
- extensible

Used in:
- PNG
- TLS
- ELF
- protobuf
- network protocols

---

# Serialization vs Deserialization

---

# Serialization

Transforming:
```text
structures -> raw bytes
```

Example:

```c
write_header(fp, &header);
```

---

# Deserialization

Transforming:
```text
raw bytes -> structures
```

Example:

```c
read_header(fp, &header);
```

---

# Big Endian Encoding

Your parser uses:

```c
read_u32_be()
read_u16_be()
```

Meaning:
```text
most significant byte stored first
```

Example:

```text
0x12345678
```

Stored as:

```text
12 34 56 78
```

---

# Why Endianness Matters

Different CPUs store integers differently.

Without explicit conversion:
- parser breaks cross-platform
- protocol incompatibility happens

---

# Manual Endian Parsing

Example:

```c
return ((uint32_t)bytes[0] << 24) |
       ((uint32_t)bytes[1] << 16) |
       ((uint32_t)bytes[2] << 8)  |
       ((uint32_t)bytes[3]);
```

This reconstructs integer manually from raw bytes.

Important low-level concept.

---

# Safe Read Wrappers

Very important design choice.

Your code uses:

```c
read_bytes()
```

Instead of repeated raw:
```c
fread()
```

Benefits:
- centralized error handling
- easier auditing
- consistent behavior

Professional parsers often use wrapper abstractions.

---

# EOF Handling

Example:

```c
if (feof(fp))
```

Critical because:
- truncated files common
- malformed inputs common
- parser must fail safely

---

# Parser Validation

One of the most important parser concepts.

Your parser validates:
- magic number
- version
- record count
- record length
- record type

Without validation:
```text
parser becomes attack surface
```

---

# Record Validation

Example:

```c
if (r->len == 0 || r->len > MAX_RECORD_SIZE)
```

Purpose:
- prevent absurd allocations
- avoid integer overflow
- avoid memory exhaustion

---

# Why Length Validation Is Critical

Suppose attacker controls:

```text
len = 0xFFFFFFFF
```

Without checks:
- huge malloc attempt
- crash
- OOM
- overflow
- allocator corruption

---

# Heap Allocation During Parsing

Example:

```c
r->data = malloc(r->len + 1);
```

Parser dynamically allocates:
```text
based on attacker-controlled length
```

Very dangerous area.

---

# Why +1 Matters

```c
malloc(r->len + 1);
```

Extra byte reserved for:
```text
NULL terminator
```

Without it:
- off-by-one overflow possible

---

# NULL Termination

After reading:

```c
r->data[r->len] = '\0';
```

Purpose:
- safely print as string
- prevent over-read

---

# Attack Surface in Parsers

Parsers are historically vulnerable because:
- attacker controls input
- parser trusts lengths
- parser performs allocations
- parser manipulates pointers

Common vulnerability classes:
- heap overflow
- integer overflow
- OOB read
- OOB write
- use-after-free

---

# Common Dangerous Patterns

---

# Unchecked Length

Bad:

```c
malloc(user_len);
```

without validation.

---

# Integer Overflow

Example:

```c
malloc(len + 1);
```

If:
```text
len = UINT_MAX
```

Then:
```text
len + 1 wraps
```

Dangerous allocation bug.

---

# Out-of-Bounds Read

Bad parser:

```c
read_bytes(fp, buf, len);
```

without checking actual file size.

---

# Type Confusion

Example:

```text
record claims type TEXT
actually contains binary payload
```

Parser misinterprets structure.

---

# Trust Boundaries

Critical concept.

Everything read from:
- file
- socket
- user input

must be treated as:
```text
untrusted
```

Never trust:
- lengths
- offsets
- counts
- pointers
- sizes

---

# Memory Ownership During Parsing

Example:

```c
Record record;
```

Inside:

```c
record.data = malloc(...);
```

Meaning:
```text
parser now owns heap allocation
```

Must eventually:

```c
free(record.data);
```

---

# Cleanup Logic

Your parser correctly uses:

```c
free_record()
```

Benefits:
- centralized cleanup
- easier maintenance
- fewer leaks

---

# Structured Parsing Flow

Your parser workflow:

```text
open file
read header
validate header
loop records
    read record
    validate record
    allocate memory
    process record
    free record
close file
```

This is clean parser architecture.

---

# Serialization Internals

Writer performs opposite operation.

Example:

```c
write_u32_be()
```

Converts integer into bytes.

---

# Byte Packing

Example:

```c
bytes[0] = (value >> 24) & 0xFF;
```

Meaning:
```text
extract top byte
```

This is manual serialization.

---

# Why Explicit Serialization Matters

Raw struct dumping:

```c
fwrite(&header, sizeof(header), 1, fp);
```

is dangerous because:
- padding differs
- endianness differs
- compiler layout differs

Explicit serialization is safer.

---

# Parser State Machine Concept

Most parsers internally behave like:

```text
state machines
```

Example:

```text
READ_HEADER
READ_RECORD
VALIDATE
PROCESS
```

Important for:
- protocol parsers
- streaming parsers
- network analyzers

---

# Binary Parsing and Exploitation

Parser bugs often become:
- RCE
- heap corruption
- sandbox escape
- kernel exploits

Historically vulnerable software:
- image decoders
- browsers
- media parsers
- PDF readers
- archive extractors

---

# Important Realizations

- parsers process hostile input
- validation is critical
- length fields are dangerous
- serialization must be explicit
- endian handling matters
- memory ownership must be clear
- parser bugs are major exploitation targets

---

# Experiments I Ran

---

# Experiment 1 — Invalid Magic Number

Modified:
```text
DEADBEEF -> 41414141
```

Observation:
- parser correctly rejected file

---

# Experiment 2 — Corrupted Length Field

Set:
```text
length = 65535
```

Observation:
- validation prevented huge allocation

---

# Experiment 3 — Truncated File

Removed bytes from end.

Observation:
- EOF detection triggered safely

---

# Experiment 4 — Invalid Record Type

Set:
```text
type = 99
```

Observation:
- parser rejected malformed record

---

# Useful Commands

Compile:

```bash
gcc -Wall -Wextra -g parser.c
```

With ASAN:

```bash
gcc -fsanitize=address -g parser.c
```

Run:

```bash
./parser
```

Hex dump:

```bash
xxd bytes.bin
```

---

# Useful GDB Commands

Break on parser:

```bash
break read_record
```

Inspect bytes:

```bash
x/32bx address
```

View structures:

```bash
print *r
```

---

# Questions To Revisit

- how do ELF parsers work internally?
- how do streaming parsers handle partial input?
- how do parser state machines scale?
- how are parser fuzzers built?
- how does AFL discover parser bugs?
- how do integer overflow exploits happen in parsers?

---

# Next Topics

- ELF internals
- file format reversing
- parser fuzzing
- AFL++
- heap internals
- integer overflows
- network protocol parsing
- exploit development
