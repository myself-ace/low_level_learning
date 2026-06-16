# Fuzzing Hypotheses for JSON Parser

## Purpose

This document contains manually developed fuzzing hypotheses for a JSON parser. The goal is not simply to generate malformed inputs, but to identify assumptions made by the parser and systematically test whether those assumptions can be violated.

Each hypothesis includes:

* Assumption being tested
* Attack strategy
* Technical reasoning
* Expected failure mode
* Validation criteria

---

# Hypothesis 1: Stack Overflow via Deep Nesting

## Assumption

The parser assumes JSON nesting depth will remain within a reasonable range.

Most recursive parsers are designed around normal usage patterns where nesting rarely exceeds a few dozen levels.

---

## Attack Strategy

Generate a JSON object nested thousands of levels deep.

Example:

```json
{
  "a": {
    "b": {
      "c": {
        "d": {
          ...
        }
      }
    }
  }
}
```

Depth: 5000+

---

## Technical Reasoning

Many JSON parsers use recursive descent parsing.

Simplified example:

```c
parse_object() {
    parse_value();
}

parse_value() {
    if(token == OBJECT)
        parse_object();
}
```

Every nested object creates another stack frame.

Stack usage grows approximately linearly with nesting depth.

```text
Depth 1  -> 1 frame
Depth 10 -> 10 frames
Depth 1000 -> 1000 frames
Depth 5000 -> 5000 frames
```

Eventually the process exceeds the operating system's stack limit.

---

## Resource Targeted

* Call stack
* Recursion depth

---

## Expected Failure

```text
SIGSEGV
Stack overflow
```

or

```text
Abort due to stack guard page
```

---

## Validation

Success indicators:

* Segmentation fault
* Process termination
* AddressSanitizer stack overflow report

Failure indicators:

* Parser rejects excessive depth gracefully
* Maximum depth limit enforced

---

## Personal Notes

When I see recursion, I immediately ask:

> What happens if I make this recursive structure absurdly deep?

The goal is not "5000 levels."

The goal is finding the depth where developer assumptions break.

---

# Hypothesis 2: Integer Overflow in Array Size Calculations

## Assumption

The parser assumes array size calculations fit within integer boundaries.

---

## Attack Strategy

Create an extremely large JSON array.

Example:

```json
[1,1,1,1,1,1,...]
```

Target sizes:

```text
2^31 - 1
2^32
2^63
```

depending on implementation.

---

## Technical Reasoning

Many parsers allocate memory using calculations such as:

```c
size = count * sizeof(element);
ptr = malloc(size);
```

If count becomes large enough:

```text
count * sizeof(element)
```

can overflow.

Example:

```text
Expected size:
8 GB

Overflow result:
64 bytes
```

Parser then writes beyond allocated memory.

---

## Resource Targeted

* Integer arithmetic
* Heap allocator

---

## Expected Failure

```text
Heap overflow
Buffer overflow
Heap corruption
```

---

## Validation

Look for:

* ASAN heap-buffer-overflow
* Invalid write reports
* Crashes during array expansion

---

## Personal Notes

Whenever I see:

```c
count * size
```

I immediately think:

> Can this multiplication wrap?

Integer overflows are often the first step toward memory corruption.

---

# Hypothesis 3: Unbounded String Allocation

## Assumption

The parser assumes input strings remain reasonably sized.

---

## Attack Strategy

Provide a JSON string containing tens or hundreds of megabytes.

Example:

```json
{
    "key": "AAAAAAAAAA..."
}
```

50MB+

---

## Technical Reasoning

Parser behavior often resembles:

```c
char *buf = malloc(length + 1);
```

Large allocations may:

* Exhaust memory
* Trigger allocator failures
* Create denial-of-service conditions

The real question:

> Does the parser handle allocation failure correctly?

---

## Resource Targeted

* Heap memory
* Allocator behavior

---

## Expected Failure

```text
Out-of-memory
NULL dereference
Abort
Process termination
```

---

## Validation

Observe:

* Memory consumption
* malloc failures
* Parser error handling

---

## Personal Notes

The interesting bug is usually not the large allocation itself.

The interesting bug is what happens after allocation fails.

Many crashes originate from unchecked malloc returns.

---

# Hypothesis 4: Unicode Escape Handling Bug

## Assumption

The parser correctly validates Unicode surrogate pairs.

---

## Attack Strategy

Provide invalid Unicode escape sequences.

Example:

```json
{
    "key": "\ud800"
}
```

The value contains a lone high surrogate.

---

## Technical Reasoning

Unicode surrogate ranges:

```text
High surrogate:
U+D800 - U+DBFF

Low surrogate:
U+DC00 - U+DFFF
```

A high surrogate must be followed by a matching low surrogate.

Valid:

```json
"\ud83d\ude00"
```

Invalid:

```json
"\ud800"
```

Improper validation may produce:

* Invalid UTF-8
* Buffer corruption
* Decoder state corruption

---

## Resource Targeted

* Unicode decoder
* Escape sequence handler

---

## Expected Failure

```text
Invalid memory access
Malformed output generation
Crash during encoding conversion
```

---

## Validation

Check:

* Output bytes
* Decoder behavior
* Sanitizer reports

---

## Personal Notes

Parsers often spend years being tested against syntax errors.

Unicode handling is where surprising bugs frequently hide because relatively few developers understand the edge cases completely.

---

# Hypothesis 5: Null Pointer Dereference in Type Dispatcher

## Assumption

The parser assumes API consumers call functions with compatible object types.

---

## Attack Strategy

Invoke operations using incorrect object types.

Examples:

```c
json_array_append(object);
```

or

```c
json_object_get(array);
```

depending on API design.

---

## Technical Reasoning

Many libraries dispatch behavior based on type:

```c
switch(node->type)
```

or

```c
node->ops->append(...)
```

If validation is incomplete:

```c
node->ops == NULL
```

may occur.

Subsequent dereference causes:

```text
NULL pointer access
```

---

## Resource Targeted

* Type dispatcher
* API validation layer

---

## Expected Failure

```text
SIGSEGV
NULL pointer dereference
```

---

## Validation

Review:

* Type checks
* Dispatcher logic
* Function pointer initialization

---

## Personal Notes

This hypothesis cannot be generated from input alone.

It usually requires source-code review.

Whenever I see:

```c
obj->method(...)
```

I ask:

> Can method ever be NULL?

Many API-level crashes begin there.

---

# General Fuzzing Mindset

For every target, identify:

1. What assumptions exist?
2. What resources grow with input?
3. What calculations can overflow?
4. What allocations can fail?
5. What states can become invalid?
6. What happens when those assumptions are violated?

A fuzzing input should never be random for the sake of being random.

Every input should be attempting to break a specific assumption.
