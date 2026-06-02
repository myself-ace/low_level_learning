# LibFuzzer Integration Notes

## Goal

When fuzzing an existing codebase, the objective is **not** to rewrite the entire parser. The objective is to create a small fuzzing harness that feeds arbitrary input into the target while removing sources of noise and instability.

---

# 1. Replace `FILE *` Input With `fmemopen()`

Many legacy parsers look like:

```c
int parse(FILE *fp)
{
    ...
}
```

Instead of modifying the parser, create an in-memory file:

```c
#include <stdio.h>

extern int parse(FILE *fp);

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    FILE *fp = fmemopen((void *)Data, Size, "rb");

    if (!fp)
        return 0;

    parse(fp);

    fclose(fp);
    return 0;
}
```

### Why?

* No disk I/O
* Faster execution
* Preserves existing parser logic
* Minimal code changes

---

# 2. Remove Command-Line Entry Points

Typical programs:

```c
int main(int argc, char **argv)
{
    FILE *fp = fopen(argv[1], "rb");

    parse(fp);

    fclose(fp);
    return 0;
}
```

LibFuzzer provides its own `main()`.

Either:

```c
#ifndef FUZZING
int main(int argc, char **argv)
{
    ...
}
#endif
```

or remove the file completely from the fuzz build.

---

# 3. Disable Logging During Fuzzing

Thousands of executions per second make logging useless.

Bad:

```c
printf("Token: %s\n", token);
```

Good:

```c
#ifndef FUZZING
printf("Token: %s\n", token);
#endif
```

---

# 4. Create Logging Macros

Instead of wrapping every print:

```c
#ifdef FUZZING
#define LOG(...)
#else
#define LOG(...) printf(__VA_ARGS__)
#endif
```

Usage:

```c
LOG("Current state = %d\n", state);
```

For fuzzing builds:

```c
-D FUZZING
```

All logs disappear automatically.

---

# 5. Ignore Error Messages

Bad:

```c
fprintf(stderr, "Invalid header\n");
```

Better:

```c
LOG("Invalid header\n");
```

LibFuzzer cares about:

* Crashes
* Hangs
* Sanitizer findings

It does **not** care about error messages.

---

# 6. Remove Interactive Behavior

Never fuzz code that waits for:

```c
scanf(...)
getchar()
fgets(stdin)
```

LibFuzzer supplies input through:

```c
LLVMFuzzerTestOneInput()
```

All interactive code should be disabled or bypassed.

---

# 7. Avoid Global State Persistence

Bad:

```c
static int counter;
counter++;
```

Every fuzz iteration should start clean.

If state is necessary:

```c
reset_state();
parse(fp);
```

before each execution.

---

# 8. Eliminate File Dependencies

Bad:

```c
FILE *fp = fopen("config.txt", "r");
```

Bad:

```c
load_dictionary("words.txt");
```

Fuzzers should not depend on external files.

Prefer:

```c
const uint8_t *Data;
size_t Size;
```

as the only input source.

---

# 9. Keep the Harness Minimal

Bad harness:

```c
initialize_network();
connect_server();
parse(fp);
cleanup();
```

Good harness:

```c
parse(fp);
```

Every extra operation reduces fuzzing throughput.

---

# 10. Add Size Guards

Many parsers assume minimum input lengths.

Example:

```c
if (Size < 4)
    return 0;
```

For structured formats:

```c
if (Size < HEADER_SIZE)
    return 0;
```

This avoids wasting executions on obviously invalid inputs.

---

# 11. Use Sanitizers

Compile with:

```bash
clang \
-fsanitize=address \
-fsanitize=undefined \
-fsanitize=fuzzer \
...
```

Common discoveries:

* Heap overflow
* Stack overflow
* Use-after-free
* Double free
* Integer overflow
* Null dereference

---

# 12. Typical Fuzz Build

```bash
clang \
-g \
-O1 \
-fsanitize=fuzzer,address,undefined \
-D FUZZING \
parser.c \
fuzz_parser.c \
-o fuzz_parser
```

---

# 13. Typical Harness Template

```c
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

extern int parse(FILE *fp);

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    if (Size == 0)
        return 0;

    FILE *fp = fmemopen((void *)Data, Size, "rb");

    if (!fp)
        return 0;

    parse(fp);

    fclose(fp);

    return 0;
}
```

---

# 14. Fuzzing Checklist

Before fuzzing:

* [ ] `main()` removed or disabled
* [ ] `fmemopen()` used for `FILE *` parsers
* [ ] Logging disabled with `-DFUZZING`
* [ ] No stdin interaction
* [ ] No network activity
* [ ] No external file dependency
* [ ] Minimal harness
* [ ] ASan enabled
* [ ] UBSan enabled
* [ ] Global state reset each iteration
* [ ] Size guards added where appropriate

---

# Mental Model

Do **not** think:

> "How do I make the parser fuzzable?"

Think:

> "How do I make arbitrary bytes reach the deepest parser logic with the fewest code changes?"

The best fuzz harness is usually boring:

* Feed bytes
* Call parser
* Return

Nothing else.
