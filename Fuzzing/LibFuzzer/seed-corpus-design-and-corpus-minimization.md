# Seed Corpus Design and Corpus Minimization in Coverage-Guided Fuzzing

## Overview

Today I learned how seed inputs influence the effectiveness of coverage-guided fuzzers such as libFuzzer. Rather than starting from completely random data, a fuzzer can begin with a collection of valid inputs called a **corpus**. These inputs act as a starting point for mutations and help the fuzzer reach deeper program logic much faster.

---

## What is a Seed Corpus?

A seed corpus is a collection of input files used by a fuzzer as its initial test cases.

Example:

```text
corpus/
└── seeds/
    ├── valid_1.bin
    ├── valid_2.bin
    ├── edge_case_1.bin
    └── edge_case_2.bin
```

Instead of discovering the input format from scratch, the fuzzer starts with files that already satisfy parts of the parser's expectations.

---

## Why Seeds Matter

Without seeds:

* The fuzzer starts with random bytes.
* It must discover file structure by chance.
* Many parser checks fail immediately.
* Coverage growth is slow.

With seeds:

* The fuzzer starts from valid inputs.
* It reaches deeper code paths immediately.
* More branches become reachable.
* Bugs are discovered faster.

The primary goal is not finding crashes.

The primary goal is increasing code coverage.

```text
Better Seeds
     ↓
Higher Coverage
     ↓
More Executed Paths
     ↓
More Opportunities for Bugs
```

---

## Understanding Input Structure

A parser generally expects a specific format.

Example:

```text
[MAGIC][VERSION][COUNT][RECORDS...]
```

Different fields control different behaviors:

| Field       | Controls          |
| ----------- | ----------------- |
| Magic       | Format validation |
| Version     | Feature selection |
| Count       | Loop execution    |
| Length      | Memory allocation |
| Record Type | Branch selection  |

When analyzing a format, an important question is:

> Which bytes influence program behavior the most?

These bytes become high-value mutation targets.

---

## Designing Effective Seeds

A useful corpus should contain inputs that exercise different program behaviors.

Examples:

### Minimal Valid Input

Tests the smallest accepted file.

### Zero Records

Tests empty processing paths.

### Maximum Records

Exercises loops heavily.

### Zero-Length Data

Tests boundary conditions.

### Invalid Version

Exercises error-handling logic.

### Multiple Record Types

Exercises additional branches and switch cases.

A good corpus is not a collection of random files.

A good corpus is a collection of behavioral variations.

---

## Edge Cases Matter

Many vulnerabilities appear near boundaries.

Examples:

* Length = 0
* Count = 0
* Very large count
* Very large length
* Empty records
* Invalid version values

These inputs often trigger unusual execution paths that normal inputs never reach.

---

## Corpus Minimization

Over time, many seeds become redundant.

Example:

```text
seedA → 50 edges
seedB → 50 edges
seedC → 60 edges
```

If seedA and seedB produce identical coverage, only one is needed.

Corpus minimization removes redundant inputs while preserving coverage.

Tools:

```bash
libFuzzer -merge=1
afl-cmin
```

Benefits:

* Faster fuzzing
* Less storage
* Reduced duplicate work
* Easier corpus management

---

## Important Realization

Coverage is more important than crash count.

A crash is only evidence that a bug was reached.

Coverage measures how much code the fuzzer can explore.

Researchers focus on increasing coverage because:

```text
More Coverage
      ↓
More States
      ↓
More Behaviors
      ↓
More Bugs
```

---

## Security Research Perspective

When examining a parser, identify fields that control:

* Memory allocations
* Loop iterations
* Pointer movement
* Array indexing
* Data copying

Then ask:

> What is the worst thing that happens if an attacker controls this value?

Possible outcomes:

* Integer overflow
* Out-of-bounds read
* Out-of-bounds write
* Use-after-free
* Excessive memory allocation
* Denial of service

This mindset is more valuable than simply running a fuzzer because it develops vulnerability research intuition.

---

## Key Takeaways

* Seed inputs dramatically affect fuzzing efficiency.
* A corpus should represent different program behaviors, not random files.
* Edge cases often provide the highest value.
* Coverage is the primary fuzzing metric.
* Corpus minimization removes redundant inputs while preserving coverage.
* Understanding which bytes control program behavior is a core vulnerability research skill.
* Effective fuzzing begins with understanding the input format, not blindly running tools.
