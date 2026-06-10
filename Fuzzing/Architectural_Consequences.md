# Architectural Consequences

## Consequence 1: Execution Model and Fuzzing Speed

### libFuzzer behavior

libFuzzer operates **in-process**. The target application starts once and remains alive throughout the fuzzing session. The fuzzing engine repeatedly calls:

```c
int LLVMFuzzerTestOneInput(
    const uint8_t *data,
    size_t size)
{
    parse(data, size);
    return 0;
}
```

Execution flow:

```text
Program Start
     |
     v
Input #1
Input #2
Input #3
Input #4
...
Input #10,000,000
```

No process creation or reinitialization occurs between test cases.

---

### AFL++ behavior

Traditional AFL++ uses a **forkserver model**.

Execution flow:

```text
Input #1
   |
Fork Process
   |
Execute
   |
Exit

Input #2
   |
Fork Process
   |
Execute
   |
Exit
```

Every test case executes in a fresh process. This provides isolation but introduces process creation overhead.

---

### Why it matters for bugs

Because libFuzzer avoids repeated process creation, it can execute significantly more test cases per second.

Benefits:

- Higher execution speed
- More mutations per second
- Faster coverage growth
- More opportunities to discover crashes

Benefits of AFL++:

- Better process isolation
- More realistic program startup testing
- Ability to fuzz binary-only targets

Tradeoff:

```text
libFuzzer = Speed
AFL++ = Isolation
```

---

### Example

Suppose:

```text
Parsing Cost = 1 ms
Process Startup Cost = 5 ms
```

Traditional execution:

```text
1 ms parse
5 ms startup

Total = 6 ms/input
```

In-process execution:

```text
1 ms/input
```

For one million inputs:

```text
AFL++      ≈ 6000 seconds
libFuzzer  ≈ 1000 seconds
```

This difference becomes substantial during long fuzzing campaigns.

---

## Consequence 2: Persistent Global State and Stateful Bugs

### libFuzzer behavior

Since libFuzzer keeps the same process alive, global variables and program state persist across fuzzing iterations.

Example:

```c
int request_count = 0;

void parse(const uint8_t *data)
{
    request_count++;

    if(request_count == 1000)
    {
        abort();
    }
}
```

Execution:

```text
Input #1   -> request_count = 1
Input #2   -> request_count = 2
Input #3   -> request_count = 3
...
Input #1000 -> Crash
```

State accumulates naturally.

---

### AFL++ behavior

Each execution starts with a fresh process.

Execution:

```text
Input #1
request_count = 1
Process exits

Input #2
request_count = 1
Process exits

Input #3
request_count = 1
Process exits
```

The crash condition is never reached.

---

### Why it matters for bugs

libFuzzer can discover bugs involving:

- Persistent global variables
- Stateful logic
- Caching systems
- Session handling
- Initialization errors
- Counter overflows
- Resource reuse bugs

AFL++ may miss these bugs because state resets after every execution.

On the other hand, AFL++ provides stronger reproducibility because every test starts from a clean environment.

---

### Example

```c
char *cache = NULL;

void parse(char *input)
{
    if(cache == NULL)
        cache = malloc(100);

    strcpy(cache, input);
}
```

libFuzzer:

```text
Input #1 -> allocate cache
Input #2 -> reuse cache
Input #3 -> reuse cache
```

Unexpected interactions may occur due to persistent state.

AFL++:

```text
Input #1 -> allocate cache
Process exits

Input #2 -> allocate cache
Process exits
```

Each run is independent.

---

## Consequence 3: Memory Leak Detection and Resource Exhaustion

### libFuzzer behavior

libFuzzer executes all inputs within the same process.

Memory that is allocated but never freed accumulates over time.

Example:

```c
void parse(char *input)
{
    malloc(1024);
}
```

Execution:

```text
Input #1 -> 1 KB leaked
Input #2 -> 2 KB leaked
Input #3 -> 3 KB leaked
...
```

Memory usage continuously increases.

LeakSanitizer (LSan) can detect the leak.

---

### AFL++ behavior

Each execution occurs in a separate process.

Example:

```c
void parse(char *input)
{
    malloc(1024);
}
```

Execution:

```text
Input #1
1 KB leaked
Process exits

Operating system reclaims memory

Input #2
1 KB leaked
Process exits
```

The leak disappears after process termination.

---

### Why it matters for bugs

libFuzzer is better at exposing:

- Memory leaks
- Heap growth issues
- Resource exhaustion bugs
- Long-running service failures
- Repeated allocation problems

Traditional AFL++ may not expose these issues because leaked memory is automatically reclaimed when the process exits.

---

### Example

```c
void parse(char *input)
{
    char *buf = malloc(1024);

    if(input[0] == 'A')
        return;

    free(buf);
}
```

If the first byte is:

```text
A
```

the buffer is leaked.

libFuzzer:

```text
Input #1 -> leak
Input #2 -> leak
Input #3 -> leak
...
```

LeakSanitizer eventually reports the problem.

AFL++:

```text
Input #1 -> leak
Process exits

Memory reclaimed
```

The leak is much harder to detect.

---

# Summary

| Consequence | libFuzzer | AFL++ |
|------------|-----------|--------|
| Speed | Extremely fast due to in-process execution | Slower because of fork overhead |
| Global State | State persists between inputs | Fresh state every execution |
| Memory Leaks | Leaks accumulate and are easier to detect | Process exit often hides leaks |

## Key Takeaway

libFuzzer optimizes for **speed and persistent state**, while AFL++ optimizes for **isolation and reproducible execution**.

Modern AFL++ Persistent Mode (`__AFL_LOOP()`) reduces many of these differences by allowing AFL++ to process multiple inputs within the same process.
