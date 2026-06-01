# libFuzzer & Coverage-Guided Fuzzing

> **Roadmap:** memory-lab · fuzzing fundamentals  
> **Goal:** Understand what libFuzzer is, how coverage-guided fuzzing works, write and run your first harness, trigger a deliberate crash, and learn the full crash reproduction lifecycle.

---

## Table of Contents

1. [What is Fuzzing?](#1-what-is-fuzzing)
2. [What is libFuzzer?](#2-what-is-libfuzzer)
3. [Coverage-Guided Fuzzing — The Core Idea](#3-coverage-guided-fuzzing--the-core-idea)
4. [Instrumentation](#4-instrumentation)
5. [The Harness Function](#5-the-harness-function)
6. [Compiling with Sanitizers](#6-compiling-with-sanitizers)
7. [Running libFuzzer](#7-running-libfuzzer)
8. [Reading the Output](#8-reading-the-output)
9. [Harness Rules — Critical](#9-harness-rules--critical)
10. [Crash Lifecycle — How Crashes Are Saved and Reproduced](#10-crash-lifecycle--how-crashes-are-saved-and-reproduced)
11. [Minimal Harness — Task 1](#11-minimal-harness--task-1)
12. [Deliberate Crash Harness — Task 2](#12-deliberate-crash-harness--task-2)
13. [The Corpus](#13-the-corpus)
14. [Extra Exercises](#14-extra-exercises)
15. [Key Terms Reference](#15-key-terms-reference)

---

## 1. What is Fuzzing?

**Fuzzing** (fuzz testing) is an automated software testing technique where a tool generates a large number of random or mutated inputs and feeds them to a program, watching for crashes, hangs, memory errors, or unexpected behavior.

You do **not** write test cases manually. The fuzzer writes them — millions per second.

```
Normal testing:  you write input → you check output
Fuzzing:         fuzzer generates inputs → watches for anything that breaks
```

**Why fuzzing finds bugs that unit tests miss:**  
Unit tests cover paths the developer *thought* to test. Fuzzers explore paths the developer *didn't think of* — especially edge cases involving malformed, oversized, or unexpected byte sequences.

---

## 2. What is libFuzzer?

**libFuzzer** is an in-process, coverage-guided fuzzing engine built into LLVM/Clang. It is:

- **In-process** — your target function is called directly inside the fuzzer's process (no fork/exec overhead), which is why it can achieve 100,000+ executions/second.
- **Coverage-guided** — it uses compile-time instrumentation to track which code branches are executed, and prefers mutations that discover *new* branches.
- **Built into Clang** — enabled by passing `-fsanitize=fuzzer` at compile time. No separate installation needed if you have Clang ≥ 6.

libFuzzer replaces `main()`. You write a *target function*, libFuzzer calls it in a loop.

---

## 3. Coverage-Guided Fuzzing — The Core Idea

This is the most important concept in modern fuzzing. Understand it deeply.

### What is a "coverage edge"?

A **coverage edge** (or **branch edge**) is a transition between two basic blocks of code. Every `if`, `for`, `while`, `switch`, and `return` creates edges.

```c
if (size >= 3) {       // edge A: entry → inside-if block
    process(data);
}
                       // edge B: entry → skip-if (else path)
```

The fuzzer counts unique edges hit. More edges = more code explored.

### How coverage guidance works — step by step

```
1. Fuzzer generates a starting input (often random bytes)
2. Runs your target function with that input
3. Records which edges were hit (via instrumentation counters)
4. If the input hit a NEW edge never seen before → saves it to the corpus
5. Takes a corpus input, mutates it (flip a bit, add a byte, splice two inputs...)
6. Repeat from step 2
```

### Why this matters — the "FUZ" example

Your harness has this crash condition:

```c
if (data[0] == 'F' && data[1] == 'U' && data[2] == 'Z')
    crash();
```

Pure random fuzzing would need to guess 3 specific bytes out of 256³ = 16,777,216 possibilities.

Coverage-guided fuzzing:

```
Round 1: tries "A..." → hits one branch (data[0] != 'F')
Round 2: tries "F..." → hits a NEW branch (data[0] == 'F') → SAVED to corpus
Round 3: mutates "F..." → tries "FU..." → hits a NEW branch → SAVED
Round 4: mutates "FU..." → tries "FUZ" → hits the crash branch → CRASH
```

Each step discovers one new edge. The fuzzer converges on the crash in seconds, not years.

---

## 4. Instrumentation

**Instrumentation** is code injected into your binary at compile time that records coverage information at runtime.

When you compile with `-fsanitize=fuzzer`, Clang inserts **inline 8-bit counters** at every branch point:

```c
// Your source code:
if (x > 0) { do_something(); }

// What instrumentation adds (conceptually):
__coverage_counter[42]++;      // counter for this branch
if (x > 0) { do_something(); }
```

libFuzzer checks these counters after each execution to determine:
- Which edges were hit this run
- Whether any *new* edges were hit (compared to all previous runs)
- Whether to add this input to the corpus

### Types of coverage libFuzzer tracks

| Type | What it means |
|------|---------------|
| Edge coverage | Whether a branch was taken (taken vs not-taken) |
| `ft` (features) | Includes edge hits + value comparisons + indirect call targets |
| Counters | How many times an edge was hit (saturating at 255) |

You'll see both `cov` (edges) and `ft` (features) in the output.

### Important: instrumentation has a cost

Instrumented binaries run ~2–5x slower than non-instrumented builds. This is why fuzzing builds are separate from release builds.

---

## 5. The Harness Function

The **harness** (also called a fuzz target) is the function libFuzzer calls repeatedly with mutated input.

### Required signature

```c
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
```

This is a contract — libFuzzer *requires* this exact signature.

| Parameter | Type | Meaning |
|-----------|------|---------|
| `data` | `const uint8_t *` | Pointer to the fuzz input bytes |
| `size` | `size_t` | Number of bytes in `data` |
| return value | `int` | Always return `0` |

### What you do inside the harness

```c
#include <stdint.h>   // for uint8_t
#include <stddef.h>   // for size_t

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return 0;       // guard: reject too-small inputs

    // Pass data to YOUR target function here:
    // my_parser(data, size);
    // my_string_function((char*)data, size);
    // etc.

    return 0;
}
```

### You do NOT write `main()`

libFuzzer provides `main()`. It:
- Parses command-line flags (`-max_len`, `-runs`, `-dict`, etc.)
- Loads the seed corpus
- Runs the mutation loop
- Catches signals (SIGSEGV, SIGABRT) and saves crash artifacts
- Prints the output lines you see in the terminal

---

## 6. Compiling with Sanitizers

### The compile command

```bash
clang -fsanitize=fuzzer,address -g -O1 harness.c -o fuzz_minimal
```

### Flag-by-flag breakdown

| Flag | What it does |
|------|-------------|
| `-fsanitize=fuzzer` | Injects coverage instrumentation + links libFuzzer engine (replaces `main()`) |
| `-fsanitize=address` | Enables AddressSanitizer (ASAN) — detects memory errors at runtime |
| `-g` | Keeps debug symbols — makes crash stack traces readable (file names, line numbers) |
| `-O1` | Light optimization — ASAN requires at least this; `-O0` can misreport locations |

### What is AddressSanitizer (ASAN)?

ASAN is a runtime memory error detector. It works by allocating **shadow memory** alongside your real memory and checking every load/store.

ASAN catches:

| Error type | Example |
|-----------|---------|
| Heap buffer overflow | `malloc(4); buf[5] = 'x';` |
| Stack buffer overflow | `char buf[4]; buf[10] = 'x';` |
| Use-after-free | `free(p); *p = 1;` |
| NULL dereference | `int *p = NULL; *p = 1;` |
| Use-after-return | returning pointer to local variable |

**Without ASAN**, a buffer overflow might silently corrupt memory, run for thousands more iterations, and crash somewhere completely unrelated — making the bug nearly impossible to diagnose. ASAN catches it at the exact point of violation.

---

## 7. Running libFuzzer

### Basic run

```bash
./fuzz_minimal
```

libFuzzer runs indefinitely. Stop with `Ctrl+C`.

### Run with a corpus directory

```bash
mkdir corpus
./fuzz_minimal corpus/
```

libFuzzer will:
- Load any existing files in `corpus/` as initial seeds
- Save new interesting inputs (those covering new edges) into `corpus/`

### Useful flags

```bash
./fuzz_minimal -max_len=256          # max input size in bytes (default: 4096)
./fuzz_minimal -runs=1000000         # stop after N executions
./fuzz_minimal -jobs=4               # run 4 parallel fuzzing jobs
./fuzz_minimal -dict=keywords.dict   # use a dictionary of interesting tokens
./fuzz_minimal corpus/ seeds/        # load seeds/ as read-only, save to corpus/
```

---

## 8. Reading the Output

### Startup output

```
INFO: Seed: 3918206239
INFO: Loaded 0 modules   (47 inline 8-bit counters): 47 [0x55f..., 0x55f...)
INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
INFO: A corpus is not provided, starting from an empty corpus
#0      INITED cov: 3 ft: 4 corp: 1/1b  exec/s: 0 rss: 23Mb
```

| Field | Meaning |
|-------|---------|
| `Seed` | Random seed used for this run. Record it to reproduce a specific fuzzing session |
| `47 inline 8-bit counters` | Number of coverage edges instrumented in your binary |
| `-max_len not provided` | libFuzzer defaults to 4096 byte max input size |

### Progress lines

```
#512    pulse  cov: 47 ft: 48 corp: 3/7b  exec/s: 200000  rss: 64Mb
```

| Field | Meaning |
|-------|---------|
| `#512` | Iteration number — 512nd input tested |
| `pulse` | Periodic heartbeat (printed every 2^n iterations) |
| `cov: 47` | Total unique edges covered so far |
| `ft: 48` | Total unique features (edges + value comparisons) |
| `corp: 3/7b` | Corpus has 3 inputs, 7 bytes total |
| `exec/s: 200000` | Executions per second (throughput) |
| `rss: 64Mb` | Resident memory usage |

### Status keywords

| Keyword | Meaning |
|---------|---------|
| `INITED` | Fuzzer initialized, showing starting coverage |
| `NEW` | Found input covering a new edge — added to corpus |
| `REDUCE` | Found a shorter input covering the same edges — replaced old entry |
| `pulse` | Periodic heartbeat (no new coverage) |
| `DONE` | Finished all `-runs` iterations |
| `CRASH` | Found a crashing input — artifact saved to disk |
| `OOM` | Out of memory |
| `TIMEOUT` | Input caused a hang (exceeded time limit) |

### What "cov stops increasing" means

When `cov` stops growing across many iterations, the fuzzer has **saturated** reachable coverage with its current corpus. Options to proceed:

1. Add hand-crafted seed inputs covering specific paths
2. Use a `-dict` file with meaningful tokens for your target
3. Increase `-max_len` if your target accepts larger inputs
4. Use a different mutation strategy

---

## 9. Harness Rules — Critical

These rules exist because libFuzzer calls your function millions of times in the same process. Violating them causes incorrect behavior, missed bugs, or the fuzzer silently stopping.

### Rule 1: Never call `exit()` or `abort()` yourself

```c
// WRONG
if (error) exit(1);     // kills the fuzzer process — no crash artifact saved

// CORRECT
if (error) return 0;    // return normally, let ASAN catch real crashes
```

libFuzzer catches crashes via signal handlers (SIGSEGV, SIGABRT). If you call `exit()`, the process terminates "normally" and libFuzzer can't save a crash artifact.

### Rule 2: Never allocate persistent global state between calls

```c
// WRONG — leaks memory across millions of calls
static char *buf = NULL;
int LLVMFuzzerTestOneInput(...) {
    buf = malloc(size);   // never freed — process OOMs after ~10k calls
    ...
}

// CORRECT — allocate and free within each call
int LLVMFuzzerTestOneInput(...) {
    char *buf = malloc(size);
    // use buf
    free(buf);
    return 0;
}
```

### Rule 3: Reset all state before each call if your target is stateful

```c
// WRONG — stale state from previous call poisons this call
static Parser p;
int LLVMFuzzerTestOneInput(...) {
    parser_feed(&p, data, size);   // p still has state from last call
    ...
}

// CORRECT
int LLVMFuzzerTestOneInput(...) {
    Parser p;
    memset(&p, 0, sizeof(p));      // clean slate every call
    parser_feed(&p, data, size);
    ...
}
```

### Rule 4: The harness must be deterministic

Same `data` + same `size` must always produce exactly the same behavior.

```c
// WRONG — non-deterministic
int LLVMFuzzerTestOneInput(...) {
    srand(time(NULL));              // different seed each call
    int r = rand() % 256;
    if (r == data[0]) crash();      // un-reproducible
}

// WRONG — reading external state
int LLVMFuzzerTestOneInput(...) {
    FILE *f = fopen("/tmp/config", "r");   // external dependency
    ...
}
```

Non-determinism breaks:
- Coverage tracking (same input takes different paths → corpus decisions are wrong)
- Crash reproduction (saved crash file may not crash again)

---

## 10. Crash Lifecycle — How Crashes Are Saved and Reproduced

This is the workflow you will use every time a fuzzer finds a real bug.

### Step 1: libFuzzer detects a crash

When your target crashes (SIGSEGV, SIGABRT, etc.), libFuzzer's signal handler intercepts it before the process dies.

### Step 2: The crashing input is saved to disk

```
artifact_prefix='./'; Test unit written to ./crash-abc123def456789...
```

The crashing input is saved as a file. The filename is `crash-` followed by the SHA1 hash of the input bytes. Other prefixes:

| Filename prefix | Meaning |
|----------------|---------|
| `crash-XXXX` | Input caused a crash (SIGSEGV, SIGABRT, ASAN error) |
| `timeout-XXXX` | Input caused a hang (exceeded `-timeout` limit) |
| `oom-XXXX` | Input caused out-of-memory |
| `leak-XXXX` | Input caused a memory leak (requires `-detect_leaks=1`) |

### Step 3: Read the ASAN crash report

```
==12345==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000000 (pc 0x... ...)
    #0 0x... in LLVMFuzzerTestOneInput harness.c:12
    #1 0x... in fuzzer::Fuzzer::ExecuteCallback(...)
    ...
```

The report tells you:
- **Error type**: `SEGV`, `heap-buffer-overflow`, `stack-buffer-overflow`, `use-after-free`, etc.
- **Address**: where the violation happened in memory
- **Stack trace**: exact file + line number of the crash (because you compiled with `-g`)

### Step 4: Reproduce the crash

```bash
./fuzz_minimal crash-abc123def456789
```

This re-runs the fuzzer binary with the saved crash file as the only input. You should see the same ASAN report. This confirms the crash is real and reproducible.

### Step 5: Inspect the crash input

```bash
# See raw bytes as hex
xxd crash-abc123def456789 | head

# See as text (if printable)
cat crash-abc123def456789

# See byte count
wc -c crash-abc123def456789
```

For the "FUZ" example, you will see:
```
00000000: 4655 5a                                  FUZ
```

Three bytes: `0x46` ('F'), `0x55` ('U'), `0x5A` ('Z').

### Step 6: Minimize the crash input (optional but recommended)

libFuzzer can shrink the crash input to the smallest possible bytes that still trigger the crash:

```bash
./fuzz_minimal -minimize_crash=1 -runs=10000 crash-abc123def456789
```

Minimized inputs are easier to analyze and write regression tests from.

---

## 11. Minimal Harness — Task 1

```c
#include <stdint.h>
#include <stddef.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return 0;
    /* call your target here */
    return 0;
}
```

**Compile:**

```bash
clang -fsanitize=fuzzer,address -g -O1 harness.c -o fuzz_minimal
```

**Run for 10 seconds, then Ctrl+C:**

```bash
./fuzz_minimal
```

**What to observe:**

- `cov` will be very low (~3–5 edges) — the harness does almost nothing
- `exec/s` will be very high (100,000–300,000+) — minimal work per call
- `corp` will be 1 or 2 inputs — almost no interesting inputs found

---

## 12. Deliberate Crash Harness — Task 2

```c
#include <stdint.h>
#include <stddef.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return 0;

    /*
     * Deliberate crash: triggered only by the 3-byte sequence "FUZ"
     * NULL dereference → SIGSEGV → ASAN catches it
     */
    if (size >= 3 &&
        data[0] == 'F' &&
        data[1] == 'U' &&
        data[2] == 'Z') {
        int *p = NULL;
        *p = 1;             /* write to address 0x0 → crash */
    }

    return 0;
}
```

**Recompile and run:**

```bash
clang -fsanitize=fuzzer,address -g -O1 harness.c -o fuzz_minimal
./fuzz_minimal
```

**Expected behavior:**

- Crashes in under 1 second (usually < 100ms)
- ASAN prints a `SEGV on unknown address 0x000000000000` report
- A file `crash-XXXX` appears in the current directory

**Reproduce the crash:**

```bash
./fuzz_minimal crash-XXXX   # replace XXXX with your actual filename
```

**Screenshot the crash output** — you will post it on Day 10.

---

## 13. The Corpus

The **corpus** is a collection of inputs that collectively maximize code coverage.

```bash
mkdir corpus
./fuzz_minimal corpus/
```

After running, each file in `corpus/` is an input that covers at least one unique edge that no other corpus input covers.

```bash
# Inspect corpus files
ls corpus/
xxd corpus/<filename>
```

**Corpus merging** (useful when combining runs from multiple machines):

```bash
./fuzz_minimal -merge=1 merged_corpus/ corpus_a/ corpus_b/
```

libFuzzer deduplicates and keeps only the minimal set of inputs with maximum coverage.

---

## 14. Extra Exercises

### Exercise 1 — Four-byte trigger

Change the crash condition to require "FUZZ" (4 bytes). Time how long it takes. Compare to 3-byte "FUZ". Explain the difference using what you know about coverage guidance.

### Exercise 2 — Stack buffer overflow

```c
if (size >= 3 && data[0] == 'S') {
    char buf[4];
    for (size_t i = 0; i < size && i < 16; i++)
        buf[i] = data[i];   /* writes past buf[3] → stack-buffer-overflow */
}
```

Compare the ASAN error type to the NULL dereference crash. Note how the report says `stack-buffer-overflow` instead of `SEGV`.

### Exercise 3 — Heap buffer overflow

```c
#include <stdlib.h>
#include <string.h>

if (size >= 2 && data[0] == 'H') {
    char *buf = malloc(4);
    memcpy(buf, data, size);   /* copies more than 4 bytes → heap-buffer-overflow */
    free(buf);
}
```

ASAN error type will be `heap-buffer-overflow`. Study how the error report differs across all three crash types (NULL deref, stack overflow, heap overflow).

### Exercise 4 — Corpus inspection

Run for 30 seconds with a corpus directory. Inspect every file. What bytes are in each one? Can you explain why the fuzzer saved each input?

### Exercise 5 — exec/s vs target complexity

Add a `memcpy` inside the harness. Does exec/s drop? By how much? Understand that target complexity directly limits fuzzing throughput — a real parser might hit only 5,000–10,000 exec/s.

---

## 15. Key Terms Reference

| Term | Definition |
|------|-----------|
| **Fuzzing** | Automated testing by generating/mutating inputs and watching for crashes |
| **libFuzzer** | In-process, coverage-guided fuzzing engine built into LLVM/Clang |
| **Coverage-guided fuzzing** | Fuzzing that uses code coverage feedback to prefer inputs reaching new branches |
| **Instrumentation** | Code injected at compile time to record branch coverage at runtime |
| **Coverage edge** | A unique source→destination branch transition in the control flow graph |
| **Harness** | The `LLVMFuzzerTestOneInput` function — your target wrapped for fuzzing |
| **Corpus** | Set of saved inputs that collectively maximize code coverage |
| **Corpus entry** | A single input file saved because it covered at least one new edge |
| **ASAN** | AddressSanitizer — runtime memory error detector (heap/stack overflow, UAF, etc.) |
| **Shadow memory** | ASAN's parallel memory map used to track valid/invalid memory accesses |
| **Crash artifact** | The `crash-XXXX` file saved by libFuzzer when your target crashes |
| **Crash reproduction** | Running `./fuzz_binary crash-XXXX` to re-trigger a saved crash |
| **Determinism** | Property that same input always produces same behavior — required for fuzzing |
| **exec/s** | Executions per second — a measure of fuzzing throughput |
| **cov** | Number of unique coverage edges hit across all runs so far |
| **ft (features)** | Broader coverage metric including edges, value comparisons, and indirect calls |
| **NULL dereference** | Writing/reading address 0x0 — triggers SIGSEGV, caught by ASAN |
| **Mutation** | A modification to an existing corpus input (bit flip, byte insert, splice, etc.) |
| **Coverage saturation** | State where `cov` stops growing — all reachable paths found with current corpus |

---

*Day 02 complete — next: Day 03.*
