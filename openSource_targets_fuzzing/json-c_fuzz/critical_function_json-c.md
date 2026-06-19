# Critical Functions in json-c: Code Analysis Guide

## Overview

This guide helps you understand the critical functions in json-c that are most likely to contain vulnerabilities.

---

## Entry Point: json_tokener_parse_ex()

**File:** `json_tokener.c`

**What it does:**
- Main parsing entry point
- Processes JSON string byte by byte
- Maintains state machine
- Tracks nesting depth
- Calls token parsers

**Why it matters:**
All input flows through this function. If there's a vulnerability in the parsing logic, it happens here.

**What to look for:**
```c
json_object* json_tokener_parse_ex(
    struct json_tokener *tok,
    const char *str,
    int len,
    enum json_tokener_error *error
)
{
    // 1. Initialize state
    // 2. Main loop through input
    // 3. Dispatch to token parsers
    // 4. Return result
    
    // KEY CHECKS:
    // - Is depth bounds-checked?
    // - Are size calculations checked?
    // - Are buffer accesses protected?
}
```

**Questions to answer:**
1. What happens if `len` is 0?
2. What happens if `len` is negative?
3. What happens if `str` is NULL?
4. How is depth limit enforced?

---

## Critical Function #1: json_parse_string()

**File:** `json_tokener.c`

**What it does:**
- Parses JSON string values: `"hello world"`
- Handles escape sequences: `\n`, `\t`, `\"`, `\\`
- Handles Unicode escapes: `\uXXXX`
- Handles surrogate pairs: `\ud800\udc00`
- Allocates buffer for result

**Why it matters:**
String parsing is COMPLEX. Complex = more likely to have bugs.

**Vulnerability patterns to search for:**

```bash
# Find the function
grep -n "json_parse_string" json_tokener.c

# Look inside for:
# 1. Buffer allocation
grep -A 50 "json_parse_string" json_tokener.c | grep -E "malloc|realloc|calloc"

# 2. Escape sequence handling
grep -A 50 "json_parse_string" json_tokener.c | grep -E "escape|\\\\u"

# 3. Surrogate pair handling
grep -A 50 "json_parse_string" json_tokener.c | grep -E "surrogate|ud800|udc00"

# 4. Buffer writes
grep -A 50 "json_parse_string" json_tokener.c | grep -E "memcpy|strcpy|sprintf"
```

**Questions to answer:**
1. How is the output buffer sized?
2. Is surrogate pair validation correct?
3. Can invalid UTF-8 sequences crash?
4. What happens with very long strings?

**Potential vulnerabilities:**
- 🔴 Buffer overflow on long strings
- 🔴 Integer overflow in size calculations
- 🟡 Invalid UTF-8 from bad surrogate handling
- 🟡 DoS from pathological escape sequences

---

## Critical Function #2: json_parse_number()

**File:** `json_tokener.c`

**What it does:**
- Parses JSON numbers: `123`, `3.14`, `1e308`
- Converts to double or int
- Handles exponent notation
- Returns json_object with number value

**Why it matters:**
Number parsing involves floating point (tricky) and exponent handling (can overflow).

**Vulnerability patterns to search for:**

```bash
# Find the function
grep -n "json_parse_number" json_tokener.c

# Look for exponent handling
grep -A 100 "json_parse_number" json_tokener.c | grep -E "exponent|[eE][+-]?[0-9]"

# Look for overflow checks
grep -A 100 "json_parse_number" json_tokener.c | grep -E "overflow|MAX|MIN|HUGE"

# Look for strtod/atof calls
grep -A 100 "json_parse_number" json_tokener.c | grep -E "strtod|atof|strtol"
```

**Questions to answer:**
1. Are very large exponents (1e9999) validated?
2. Can exponent cause integer overflow?
3. Does strtod() handle edge cases safely?
4. What happens with NaN or Infinity?

**Potential vulnerabilities:**
- 🔴 Integer overflow on large exponents
- 🟡 Unbounded memory allocation for big numbers
- 🟡 strtod() edge cases not handled

---

## Critical Function #3: json_parse_object()

**File:** `json_tokener.c`

**What it does:**
- Parses JSON objects: `{"key": "value"}`
- Handles nested objects
- Manages recursion depth
- Allocates object structure

**Why it matters:**
Objects are recursive. Recursion + user input = stack overflow potential.

**Code to examine:**

```bash
# Find the function
grep -n "json_parse_object" json_tokener.c

# Look for recursion
grep -A 50 "json_parse_object" json_tokener.c | grep -E "parse_object|parse_value"

# Look for depth checking
grep -A 50 "json_parse_object" json_tokener.c | grep -E "depth|MAX_DEPTH"

# Look for memory allocation
grep -A 50 "json_parse_object" json_tokener.c | grep -E "malloc|new_object"
```

**Questions to answer:**
1. Is recursion depth actually limited?
2. What's the depth limit? (probably 512)
3. What happens at depth limit?
4. Can depth * element_size overflow?

**Potential vulnerabilities:**
- 🔴 Stack overflow from deep nesting (>512 levels)
- 🔴 Integer overflow: depth * size calculations
- 🟡 Memory exhaustion from large objects

---

## Critical Function #4: json_parse_array()

**File:** `json_tokener.c`

**What it does:**
- Parses JSON arrays: `[1, 2, 3]`
- Handles nested arrays
- Allocates array structure
- Manages array growth

**Why it matters:**
Arrays store pointers. Array growth involves reallocation. Mismanaged reallocation = buffer overflow.

**Code to examine:**

```bash
# Find the function
grep -n "json_parse_array" json_tokener.c

# Look for array allocation growth
grep -B 5 -A 20 "json_parse_array" json_tokener.c | grep -E "realloc|size.*\*|capacity"

# Look for recursion
grep -A 50 "json_parse_array" json_tokener.c | grep -E "parse_value|depth"

# Look for element addition
grep -A 50 "json_parse_array" json_tokener.c | grep -E "array_add|element"
```

**Questions to answer:**
1. How is array capacity managed?
2. Can capacity calculation overflow?
3. Is realloc() size bounds-checked?
4. What happens with 1M+ element arrays?

**Potential vulnerabilities:**
- 🔴 Buffer overflow on array growth
- 🔴 Off-by-one in realloc size
- 🟡 Integer overflow: element count * size
- 🟡 Memory exhaustion

---

## Critical Function #5: json_object_object_add()

**File:** `json_object.c`

**What it does:**
- Adds key-value pair to object
- Handles duplicate keys
- Manages object's key array
- Frees old values if key exists

**Why it matters:**
This function modifies data structures. Memory bugs here = use-after-free or leak.

**Code to examine:**

```bash
# Find the function
grep -n "json_object_object_add" json_object.c

# Look for key lookups
grep -B 5 -A 30 "json_object_object_add" json_object.c | grep -E "find|search|lookup"

# Look for old value handling
grep -A 30 "json_object_object_add" json_object.c | grep -E "put|free|old"

# Look for memory allocation
grep -A 30 "json_object_object_add" json_object.c | grep -E "malloc|realloc|lh_table"
```

**Questions to answer:**
1. When a key already exists, is the old value freed?
2. Can duplicate key operations leak memory?
3. Is the key array properly resized?
4. What happens if malloc fails mid-operation?

**Potential vulnerabilities:**
- 🟡 Memory leak on duplicate keys
- 🟡 Use-after-free if old value not freed correctly
- 🟡 Double-free if not synchronized

---

## Critical Function #6: Recursion Depth Tracking

**File:** `json_tokener.c`

**What it does:**
- Maintains `tok->depth` counter
- Increments on `{` and `[`
- Decrements on `}` and `]`
- Enforces depth limit

**Why it matters:**
Depth tracking is the ONLY defense against stack overflow. If broken, stack overflow is easy.

**Code to examine:**

```bash
# Find depth tracking
grep -n "tok->depth" json_tokener.c | head -20

# Look at depth increments
grep -n "depth.*++" json_tokener.c

# Look at depth decrements
grep -n "depth.*--" json_tokener.c

# Look at depth checks
grep -n "depth.*>" json_tokener.c
grep -n "MAX_DEPTH" json_tokener.c
grep -n "DEFAULT_DEPTH" json_tokener.c
```

**Questions to answer:**
1. What is MAX_DEPTH? (search for definition)
2. Where is the check: `if (depth > MAX_DEPTH) ...`?
3. What happens at the limit? (error or stack anyway?)
4. Can depth become negative?

**Potential vulnerabilities:**
- 🔴 Stack overflow if check is missing
- 🔴 Off-by-one: check is `>` but should be `>=`
- 🟡 Depth not properly decremented in error paths

---

## Critical Function #7: Memory Allocation

**File:** `json_object.c`

**What it does:**
- Allocates json_object structures
- Allocates strings
- Allocates arrays
- All memory operations

**Why it matters:**
Every allocation is a potential overflow/underflow bug.

**Code to examine:**

```bash
# Find all malloc calls
grep -n "malloc" json_object.c

# Find all realloc calls
grep -n "realloc" json_object.c

# Find all calloc calls
grep -n "calloc" json_object.c

# For each allocation, check:
# 1. Is size parameter checked?
# 2. Is size parameter user-controlled?
# 3. Can size overflow?
```

**For each malloc you find, ask:**
1. What is being allocated?
2. How is the size determined?
3. Is the size bounds-checked?
4. What if malloc fails?

**Potential vulnerabilities:**
- 🔴 Unbounded allocation (user controls size)
- 🔴 Integer overflow in size calculation
- 🔴 No error handling on malloc failure

---

## Systematic Code Inspection Process

### Step 1: Map the Entry Point

```bash
cd ~/memory-lab/json-c/src

# Start with main function
grep -n "json_tokener_parse_ex" json_tokener.c

# See what it calls
sed -n '100,200p' json_tokener.c  # Adjust line numbers
```

### Step 2: Trace a Call Path

Pick one parsing operation (e.g., parsing a string):

```bash
# Find string parsing call
grep -n "json_parse_string" json_tokener.c

# Jump to that function
sed -n '400,500p' json_tokener.c  # Adjust line numbers

# Look for:
# - Buffer allocations
# - Escape sequence handling
# - Loop conditions
# - Size calculations
```

### Step 3: Look for Dangerous Patterns

```bash
# Unbounded allocations
grep -n "malloc(.*size)" json_object.c

# Integer math without bounds
grep -n "size\s*[*+]" json_object.c

# Unsafe string operations
grep -n "strcpy\|strcat\|sprintf" json_*.c

# Unchecked recursion
grep -n "depth\s*++" json_tokener.c
```

### Step 4: Document Findings

For each suspicious code location:

```markdown
## Location: [filename]:[line]
**Function:** [function_name]
**Code snippet:**
[relevant code]
**Risk:** [vulnerability type]
**Why:** [explanation]
**Test case:** [how to trigger]
```

---

## Testing Critical Functions

### Test Path 1: Depth Stress

```json
{{"{{{{{{{{  // Nested deep
```

Expected behavior: Error at depth 512
Crash behavior: Stack overflow before that limit

### Test Path 2: String Edge Cases

```json
{"key": "\ud800"}           // Invalid surrogate
{"key": "\\uXXXX..."}       // Invalid escapes
{"key": "AAAA..."}          // Very long string
```

### Test Path 3: Number Edge Cases

```json
{"val": 9223372036854775807}    // INT64_MAX
{"val": 1e9999}                 // Huge exponent
{"val": 1.7976931348623157e+308}  // Float max
```

### Test Path 4: Array/Object Stress

```json
{"k0":1,"k1":2,...,"k10000":10000}  // Many keys
[1,2,3,...,1000000]                 // Many elements
```

---

## Summary: Where Bugs Hide

| Function | Risk Type | Trigger |
|----------|-----------|---------|
| `json_parse_string()` | Buffer overflow | Long strings, bad escapes |
| `json_parse_number()` | Integer overflow | Huge exponents |
| `json_parse_object()` | Stack overflow | Deep nesting |
| `json_parse_array()` | Buffer overflow | Many elements |
| `json_object_object_add()` | Memory leak | Duplicate keys |
| Depth tracking | Stack overflow | Missing check |
| Memory allocation | Integer overflow | Size calculations |

---

## Key Insight

**Bugs hide in transitions:**
- String → number conversion
- Shallow → deep nesting
- Single → multiple elements
- Normal → boundary values

Test the boundaries, not the happy path.

---

**Status:** ✅ Reference guide complete  
**Use this to:** Understand what to look for in code  
**Next step:** Use grep to find suspicious patterns
