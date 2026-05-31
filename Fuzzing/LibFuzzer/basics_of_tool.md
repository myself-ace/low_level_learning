# Day 01 — libFuzzer Foundations & Coverage-Guided Fuzzing

> **Phase 2 · Week 4 · Day 1**  
> **Topics:** Toolchain setup · Coverage-guided fuzzing internals · libFuzzer architecture  
> **Tags:** `fuzzing` `libfuzzer` `clang` `asan` `coverage` `sanitizers`

---

## Table of Contents

- [Objectives](#objectives)
- [Toolchain Setup](#toolchain-setup)
- [Core Concept: Coverage-Guided Fuzzing](#core-concept-coverage-guided-fuzzing)
  - [How libFuzzer Works](#how-libfuzzer-works)
  - [The Fuzzing Loop](#the-fuzzing-loop)
  - [Why Coverage-Guidance Beats Random Fuzzing](#why-coverage-guidance-beats-random-fuzzing)
- [Key Terminology](#key-terminology)
- [Crash Artifacts](#crash-artifacts)
  - [Artifact Filename Format](#artifact-filename-format)
  - [What Is Inside a Crash Artifact](#what-is-inside-a-crash-artifact)
  - [libFuzzer Terminal Output on Crash](#libfuzzer-terminal-output-on-crash)
  - [Replaying a Crash Artifact](#replaying-a-crash-artifact)
  - [Other Artifact Prefixes](#other-artifact-prefixes)
- [The libFuzzer Mental Model](#the-libfuzzer-mental-model)
- [Compile Flags Reference](#compile-flags-reference)
- [Day 1 Checklist](#day-1-checklist)
- [Notes & Insights](#notes--insights)

---

## Objectives

By end of Day 1:

- Verified clang 14+ toolchain with libFuzzer, LLVM, and clang-format installed
- Permanent `cf` alias configured and sourced
- Solid mental model of how coverage-guided fuzzing works
- Able to explain the fuzzing loop from memory without reference material
- `notes.md` written in own words — the "explain it without looking" test

---

## Toolchain Setup

### Install Commands

```bash
# 1. Install clang (libFuzzer is built into clang — no separate install needed)
sudo apt install clang

# 2. Verify clang version — must be 14+
clang --version

# 3. Verify libFuzzer is available
echo 'int LLVMFuzzerTestOneInput(const uint8_t *d, size_t s){return 0;}' > /tmp/empty.c
clang -fsanitize=fuzzer /tmp/empty.c -o /tmp/empty_fuzz
# If this compiles cleanly, libFuzzer is linked and ready

# 4. Install LLVM coverage tools
sudo apt install llvm

# 5. Verify llvm-profdata (used later to merge coverage data)
llvm-profdata merge --help

# 6. Install clang-format (keeps harness code readable)
sudo apt install clang-format
```

### Set the Permanent Fuzz Alias

Add to `~/.bashrc` so it persists across sessions:

```bash
alias cf='clang -fsanitize=fuzzer,address,undefined -g -O1'
```

Then reload:

```bash
source ~/.bashrc
```

After this, every harness is compiled with just:

```bash
cf fuzz_target.c -o fuzz_target
```

### Compile Flag Breakdown

| Flag | Purpose |
|---|---|
| `-fsanitize=fuzzer` | Links libFuzzer into the binary and instruments every branch for coverage feedback |
| `-fsanitize=address` | Enables ASAN — catches heap overflows, use-after-free, double-free |
| `-fsanitize=undefined` | Enables UBSan — catches signed integer overflow, null pointer dereference, misaligned access |
| `-g` | Includes debug symbols so crash stack traces show function names and line numbers |
| `-O1` | Light optimization. `-O0` is too slow; `-O2`+ can optimize away the very bugs being hunted |

> **Why `-O1` specifically?**  
> At `-O0`, the binary is slow enough to hurt exec/s significantly. At `-O2` or higher, the compiler can elide undefined behaviour (which is what UBSan is trying to catch), making the fuzzer blind to certain bug classes. `-O1` is the accepted sweet spot for fuzz targets.

---

## Core Concept: Coverage-Guided Fuzzing

### How libFuzzer Works

libFuzzer is not a black box. It is three components composed together:

```
libFuzzer = compile-time instrumentation + mutation engine + corpus manager
```

**Compile-time instrumentation (SanitizerCoverage):**  
When you compile with `-fsanitize=fuzzer`, clang injects a counter at every branch in your binary — every `if`, `switch`, `for`, `while`, `do-while`. At runtime, these counters silently record which branches have been executed. This is the feedback mechanism that makes libFuzzer intelligent.

**Mutation engine:**  
libFuzzer takes an existing input from the corpus and mutates it. Mutations include bit flips, byte insertions, byte deletions, cross-over splicing between two corpus entries, and dictionary-guided substitutions. These mutations are not random in the sense of being uniform — they are guided by what has produced new coverage in the past.

**Corpus manager:**  
The corpus is a directory of input files. Every input that causes the fuzzer to hit at least one previously-unseen branch gets saved to the corpus. The corpus is the fuzzer's memory — it encodes the structure of valid inputs that have proven useful, and every new run can build on what past runs discovered.

### The Fuzzing Loop

```
corpus (seed inputs)
       │
       ▼
  pick & mutate
       │
       ▼
  run LLVMFuzzerTestOneInput
       │
       ├─── CRASH? ─── YES ──► save to crashes/  ◄── reproduce with: ./target crash-abc123
       │
       └─── NO ─── new branch hit?
                        │
                        ├── YES ──► save to corpus ──► loop back
                        │
                        └── NO ──► discard
```

This loop runs **10,000 – 1,000,000 times per second** depending on how fast the target function is. The exec/s number is the primary speed metric — displayed in libFuzzer's log output every few seconds.

### Why Coverage-Guidance Beats Random Fuzzing

Consider a parser that gates its logic like this:

```c
if (buf[0] == 'P') {
    if (buf[1] == 'N') {
        if (buf[2] == 'G') {
            // actual parsing begins — this is where bugs live
        }
    }
}
```

A **purely random fuzzer** has a 1-in-16,777,216 chance of generating `PNG` at the start of the buffer. It can run for hours without ever reaching the parsing logic.

A **coverage-guided fuzzer** discovers an input with `P` at position 0 (hit a new branch), saves it, mutates from there to find `PN` (new branch again), saves that, then finds `PNG`. It climbs the branch tree incrementally. This is the **genetic algorithm** at work: inputs that discover new code survive and reproduce; inputs that find nothing new are discarded.

The practical result: libFuzzer learns the structure of your input format byte by byte, automatically, without you specifying a grammar or format spec.

---

## Key Terminology

| Term | Definition |
|---|---|
| **Corpus** | A directory of input files, each of which caused the fuzzer to hit at least one previously-unseen branch. Starts with your seed inputs, grows as fuzzing runs. |
| **Crash artifact** | A file saved to `crashes/` containing the exact bytes that triggered a crash. Fully reproducible — running `./target crash-abc123` produces the same crash every time. |
| **Edge coverage** | An "edge" is a transition between two basic blocks (e.g., the true branch of a specific `if` statement). Edge coverage is more precise than line or function coverage — the same line can be entered via different edges, which matter for bug discovery. |
| **Exec/s** | Executions per second. How many inputs the fuzzer processes through your function per second. Primary speed metric. If this drops below ~1,000, the target function is probably doing something expensive (file I/O, syscalls, network) that needs to be removed from the harness. |
| **SanitizerCoverage** | The clang compiler feature that injects branch counters into the binary at compile time. This is the feedback layer — without it, libFuzzer cannot distinguish an interesting input from a useless one. |
| **Harness** | The `LLVMFuzzerTestOneInput` function you write to adapt raw fuzzer bytes into calls to your target code. The harness is the only code you write on Day 2+. |
| **ASAN** | AddressSanitizer. Detects heap buffer overflows, use-after-free, heap-use-after-return, double-free, and related memory errors. Already covered in Phase 1. |
| **UBSan** | UndefinedBehaviorSanitizer. Detects signed integer overflow, null pointer dereference, misaligned memory access, and other C undefined behaviour that ASAN does not catch. |

---

## Crash Artifacts

### Artifact Filename Format

When libFuzzer finds a crashing input, it saves it with a filename like:

```
crash-a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2
```

The hash after `crash-` is the **SHA1 of the input bytes**. There is no file extension — the file is raw binary data, exactly what was passed to `LLVMFuzzerTestOneInput` when the crash occurred.

### What Is Inside a Crash Artifact

The contents are entirely dependent on what you were fuzzing:

- Fuzzing a PNG parser → a mangled PNG with a few bytes corrupted in a specific way
- Fuzzing a string tokenizer → possibly a short ASCII string with an unexpected character at a specific offset
- Fuzzing a binary protocol parser → raw binary that happens to trigger the bug

Inspect the artifact:

```bash
# Hex dump — useful for binary artifacts
xxd crash-a1b2c3d4e5f6...

# Raw output — readable if the input is text-based
cat crash-a1b2c3d4e5f6...

# How many bytes triggered the crash
wc -c crash-a1b2c3d4e5f6...
```

> **Note on minimization:** libFuzzer has a built-in minimizer. Over time, it tends to find the *smallest* input that still triggers the same crash. Smaller inputs are easier to reason about when debugging. You can also trigger minimization manually:
> ```bash
> ./fuzz_target -minimize_crash=1 crash-a1b2c3d4e5f6...
> ```

### libFuzzer Terminal Output on Crash

When a crash is detected, the terminal output looks like this:

```
==12345== ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000110
READ of size 1 at 0x602000000110 thread T0
    #0 0x401234 in parse_header /home/ace/target.c:47
    #1 0x401890 in LLVMFuzzerTestOneInput /home/ace/fuzz_target.c:12
    ...

artifact_prefix='./'; Test unit written to ./crash-a1b2c3d4e5f6...
Base64: aGVsbG8...
```

Three things to note:

1. **ASAN prints the full stack trace first** — you see exactly where and why it crashed before libFuzzer saves anything
2. **`Test unit written to ./crash-...`** — confirms the artifact was saved and gives the filename
3. **The Base64 line** — the same input encoded as Base64, useful for logging or sharing without dealing with binary files

### Replaying a Crash Artifact

```bash
./fuzz_target crash-a1b2c3d4e5f6...
```

When libFuzzer receives a file path as an argument instead of a corpus directory, it runs `LLVMFuzzerTestOneInput` exactly once with the contents of that file. The full ASAN stack trace appears again, deterministically.

**Common replay workflow:**

```bash
# 1. Reproduce and read the stack trace
./fuzz_target crash-a1b2c3d4e5f6...

# 2. Inspect under GDB for deeper analysis
gdb --args ./fuzz_target crash-a1b2c3d4e5f6...

# 3. Fix the bug, recompile
cf fuzz_target.c -o fuzz_target

# 4. Verify the fix — should run clean with exit code 0
./fuzz_target crash-a1b2c3d4e5f6...
```

### Other Artifact Prefixes

| Prefix | Trigger condition |
|---|---|
| `crash-` | ASAN or UBSan fired (buffer overflow, UAF, UB, etc.) |
| `timeout-` | Input exceeded the `-timeout=N` seconds limit (default: 1200s) |
| `oom-` | Input caused the process to exceed the `-rss_limit_mb` memory limit |
| `slow-unit-` | Input ran significantly slower than the average exec time (not a crash, but flagged) |

All artifact types are replayed the same way — pass the file path as a positional argument to the fuzz binary.

---

## The libFuzzer Mental Model

The screenshot from the roadmap states this clearly and it is worth internalizing verbatim:

> *libFuzzer is not magic. It is: compile-time instrumentation + mutation engine + corpus manager.*

Everything else follows from this. If you already understand ASAN from Phase 1, you understand what happens when the fuzzer finds a bug. libFuzzer's job is simply to **automatically feed inputs into your binary until ASAN fires** — at machine speed, guided by branch coverage.

The two new skills being built in Week 4 that go beyond this baseline:

1. **Writing a harness** — a `LLVMFuzzerTestOneInput` function that bridges raw fuzzer bytes into whatever API or parsing code you want to test
2. **Reading coverage data** — interpreting `llvm-profdata` and `llvm-cov` output to understand what code the fuzzer is actually reaching and where it is getting stuck

Day 1 is all setup and conceptual foundation. No harness yet. That begins Day 2.

---

## Compile Flags Reference

Quick reference card for the permanent `cf` alias and what each flag does:

```bash
clang -fsanitize=fuzzer,address,undefined -g -O1
#     ├─────────────────────────────────────────── sanitizer stack
#     │         ├── fuzzer:    links libFuzzer, enables SanitizerCoverage
#     │         ├── address:   ASAN (memory safety)
#     │         └── undefined: UBSan (C undefined behaviour)
#     ├── -g   debug symbols → readable stack traces
#     └── -O1  light optimization → fast enough, doesn't hide bugs
```

---

## Day 1 Checklist

- [ ] `clang --version` shows version 14 or higher
- [ ] `clang -fsanitize=fuzzer /tmp/empty.c` compiles without error
- [ ] `llvm-profdata merge --help` outputs usage information
- [ ] `clang-format --version` works
- [ ] `cf` alias is added to `~/.bashrc` and active in current shell
- [ ] Coverage-guided fuzzing paragraph written in `notes.md` — **without looking at reference material**
- [ ] Fuzzing loop diagram drawn on paper (corpus → mutate → run → branch check → crash check)

---

## Notes & Insights

**On exec/s as a design constraint:**  
The 10k–1M exec/s range means the harness must be fast. Any syscall, file I/O, or network operation inside the harness collapses exec/s dramatically. The harness should do nothing except: parse the bytes, call the target function, return. All initialization (loading dictionaries, setting up static data structures) goes in `LLVMFuzzerInitialize`, not in `LLVMFuzzerTestOneInput`.

**On the corpus as encoded knowledge:**  
After a long fuzzing run, the corpus directory is not random — it is a compressed representation of every interesting branch in your target. Each file represents a unique path through the code. This is why corpus entries from one fuzzing run can seed a future run (corpus merging), even against a slightly different build of the same target.

**On why `-fsanitize=address,undefined` are both needed:**  
ASAN and UBSan catch different bug classes. A signed integer overflow that leads to a buffer overflow might be caught by UBSan at the overflow site before ASAN sees the resulting bad memory access — giving a more precise error location. Running both together maximizes the chance that *some* sanitizer fires close to the root cause of the bug.

**On the SHA1 filename:**  
The SHA1 is of the input bytes themselves, not a random ID. This means if two different fuzzing runs independently discover the same crashing input, they produce the same filename. Useful for deduplication across multiple fuzzer instances.

---

