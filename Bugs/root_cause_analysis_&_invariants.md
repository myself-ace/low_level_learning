# Thinking Like a Vulnerability Researcher: Assumptions, Invariants, and Root Causes

## Introduction

One of the biggest mistakes beginners make is focusing on vulnerability names:

* Heap Overflow
* Stack Overflow
* Use-After-Free
* Double Free
* Out-of-Bounds Read

Professional vulnerability researchers rarely start there.

Instead, they ask:

1. What assumptions did the program make?
2. Which assumption was violated?
3. What invariant stopped being true?
4. How did attacker-controlled data reach a dangerous operation?

The goal is not to memorize bug classes.

The goal is to understand why a bug became possible.

---

# Core Mental Model

Every vulnerability can be viewed as:

```text
Attacker Controls Data
          ↓
Program Trusts Data
          ↓
Assumption Becomes False
          ↓
Invariant Breaks
          ↓
Dangerous Operation
          ↓
Memory Corruption / Crash
```

Example:

```text
record_length
      ↓
trusted without validation
      ↓
buffer assumed large enough
      ↓
memcpy()
      ↓
heap overflow
```

The root cause is not `memcpy`.

The root cause is the unchecked length field.

---

# Assumptions vs Consequences

Bad way of thinking:

```text
Program crashed.
Heap overflow happened.
```

Better way of thinking:

```text
The size assumption failed.
```

Bad way:

```text
Use-after-free happened.
```

Better way:

```text
The object lifetime assumption failed.
```

Always focus on the broken assumption rather than the final crash.

---

# What Is An Assumption?

An assumption is something the code expects to be true.

Example:

```c
printf("%s", name);
```

Assumptions:

```text
name != NULL
name points to valid memory
name is null terminated
name represents a valid C string
```

If any assumption becomes false, the function behaves incorrectly.

---

# What Is An Invariant?

An invariant is a property that must remain true throughout execution.

Examples:

```text
Reads stay inside object boundaries.
Writes stay inside object boundaries.
Freed memory is never accessed.
Allocated size matches intended usage.
Every object has a valid lifetime.
```

Many vulnerabilities occur because an invariant is violated.

---

# Function Analysis Framework

For every function ask:

```text
What assumptions are being made?

Who is responsible for validating them?

What invariant depends on those assumptions?

What happens if the assumptions become false?
```

Example:

```c
void set_age(int ages[], int index, int age)
{
    ages[index] = age;
}
```

Assumptions:

```text
ages is valid
ages is writable
index is within bounds
```

Invariant:

```text
Writes remain inside the array
```

Potential violation:

```text
Out-of-bounds write
```

---

# Trust Boundaries

A trust boundary exists when untrusted data enters the program.

Example:

```text
File Input
    ↓
Header Parser
    ↓
Record Parser
    ↓
Allocation
    ↓
memcpy
```

Questions:

```text
Where does attacker control begin?

Where does data become trusted?

Who should validate it?

What happens if validation fails?
```

Most parser bugs originate at trust boundaries.

---

# Human Stack Trace Reading

Given:

```text
main
 ↓
load_file
 ↓
parse_header
 ↓
parse_record
 ↓
memcpy
 ↓
crash
```

Do not ask:

```text
Why did memcpy crash?
```

Ask:

```text
Where did dangerous data originate?

Who first trusted it?

Who should have validated it?

Which assumption became false?
```

The crashing function is often not the root cause.

---

# Root Cause Thinking

Given:

```c
char *buf = malloc(len);
memcpy(buf, src, len);
```

Do not immediately say:

```text
Heap overflow
```

Instead ask:

```text
Where did len come from?

Who validated len?

What assumptions depend on len?

What if len is attacker controlled?
```

Possible root causes:

```text
Unchecked length field
Integer overflow
Allocation size mismatch
Incorrect parser state
```

---

# Parser Analysis Methodology

Imagine a file format:

```text
magic
version
record_count
record_type
record_length
```

For every field ask:

## Field = 0

```text
What assumptions become false?
```

## Field = UINT32_MAX

```text
What assumptions become false?
```

## Field = Random Garbage

```text
How does parser state change?
```

Never stop at:

```text
Program crashes
```

Instead find:

```text
Loop count explodes
Allocation overflows
Cursor desynchronizes
Bounds checks become meaningless
Parser state becomes inconsistent
Pointer walks beyond buffer
```

This is root cause analysis.

---

# Crash Classification Framework

For every crash determine:

## 1. Memory Operation

```text
READ
WRITE
FREE
```

---

## 2. Broken Assumption

Example:

```text
Buffer is large enough
Pointer is valid
Object is still alive
```

---

## 3. Program Intent

What was the program trying to do?

Examples:

```text
Copy data
Parse records
Release memory
Read a string
Store user input
```

---

## 4. Broken Invariant

Example:

```text
Writes stay in bounds
Reads stay in bounds
Freed memory is never accessed
Memory is freed exactly once
```

---

# Common Memory Bug Classes

## Heap Overflow

Operation:

```text
WRITE
```

Broken Assumption:

```text
Allocated size >= written size
```

Broken Invariant:

```text
Writes remain inside allocation
```

---

## Out-of-Bounds Read

Operation:

```text
READ
```

Broken Assumption:

```text
Requested data exists
```

Broken Invariant:

```text
Reads remain inside allocation
```

---

## Stack Overflow

Operation:

```text
WRITE
```

Broken Assumption:

```text
Input fits local buffer
```

Broken Invariant:

```text
Writes remain inside stack frame
```

---

## Use-After-Free

Operation:

```text
READ or WRITE
```

Broken Assumption:

```text
Object is still alive
```

Broken Invariant:

```text
Freed memory is never accessed
```

---

## Double Free

Operation:

```text
FREE
```

Broken Assumption:

```text
Object owns memory exactly once
```

Broken Invariant:

```text
Memory is freed exactly once
```

---

## Invalid Free

Operation:

```text
FREE
```

Broken Assumption:

```text
Pointer originated from heap allocation
```

Broken Invariant:

```text
Only owned heap memory is freed
```

---

# Researcher's Root Cause Template

For every bug:

```text
Attacker Controls:
________________

Parser Assumes:
________________

Invariant Violated:
________________

Dangerous Operation:
________________

Root Cause:
________________

Potential Impact:
________________
```

---

# Final Principle

Beginners classify bugs by sanitizer output.

Researchers classify bugs by broken assumptions.

Instead of:

```text
Heap overflow
```

Think:

```text
Size invariant failed.
```

Instead of:

```text
Use-after-free
```

Think:

```text
Lifetime invariant failed.
```

Instead of:

```text
Out-of-bounds read
```

Think:

```text
Boundary invariant failed.
```

The crash is the symptom.

The violated assumption is the root cause.
