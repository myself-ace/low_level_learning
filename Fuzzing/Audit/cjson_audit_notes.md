# cJSON Security Audit Notes

## Goal

Learn to identify:

* Memory corruption bugs
* Out-of-bounds access
* Integer overflows
* Algorithmic complexity attacks
* Parser logic flaws
* Stack exhaustion issues

---

# Site 1: User-Controlled Allocation (`parse_string`)

## Dangerous Pattern

```c
allocation_length =
    (size_t)(input_end - buffer_at_offset(input_buffer))
    - skipped_bytes;

output =
    input_buffer->hooks.allocate(
        allocation_length + 1
    );
```

## Data Flow

```text
User Input
    ↓
allocation_length
    ↓
malloc()
```

## Research Questions

* Who controls `allocation_length`?
* Can it become extremely large?
* Can it overflow?
* Is `malloc()` failure checked?
* Is copied data size validated?

## Threat Model

```text
Huge JSON string
        ↓
Huge allocation_length
        ↓
malloc(large size)
        ↓
DoS / OOM
```

## Fuzz Cases

```json
"AAAAAA
```

```json
"A...(1MB)...A
```

```json
"A...(100MB)...A
```

## Key Lesson

```text
Never trust user-controlled allocation sizes.
```

---

# Site 2: Offset Arithmetic (`buffer_at_offset`)

## Dangerous Pattern

```c
#define buffer_at_offset(buffer) \
    ((buffer)->content + (buffer)->offset)
```

## Data Flow

```text
User Input
    ↓
offset
    ↓
pointer arithmetic
    ↓
memory access
```

## Research Questions

* Can offset exceed length?
* Can offset overflow?
* Is validation performed before use?
* Can malformed input desynchronize offset?

## Threat Model

```text
Invalid offset
      ↓
content + offset
      ↓
OOB Read / OOB Write
```

## Fuzz Cases

```json
[[[[[[[[[
```

```json
{{{{{{{{{
```

```json
[[[[[[[[[[[[[[[[[[[[[[[[[[[
```

## Key Lesson

```text
Pointer arithmetic is only as safe as the offset variable.
```

---

# Site 3: Algorithmic Complexity (`get_array_item`)

## Dangerous Pattern

```c
while ((current_child != NULL) &&
       (index > 0))
{
    index--;
    current_child = current_child->next;
}
```

## Complexity

```text
Lookup = O(N)
Repeated Lookup = O(N²)
```

## Data Flow

```text
User Input
    ↓
Index Value
    ↓
Loop Iterations
    ↓
CPU Time
```

## Research Questions

* Who controls iteration count?
* Is there a length check?
* Can repeated lookups become quadratic?

## Threat Model

```text
Large Array
      ↓
Large Index
      ↓
Long Traversal
      ↓
CPU Exhaustion
```

## Fuzz Cases

```json
[1,2,3,...10000]
```

```json
[1,2,3,...100000]
```

## Key Lesson

```text
Attackers can target CPU, not just memory.
```

---

# Site 4: Parser State Management (`parse_array`)

## Dangerous Pattern

```c
offset++;
offset--;
parse_value();
```

## Audit Focus

Track parser cursor manually.

```text
offset before parse
offset after parse
offset on failure
offset at EOF
```

## Research Questions

* Can offset become misaligned?
* What happens on parse failure?
* What happens at EOF?
* Are malformed arrays rejected?

## Threat Model

```text
Malformed Input
        ↓
Parser Confusion
        ↓
Incorrect State
        ↓
Memory Error Later
```

## Fuzz Cases

### Missing Bracket

```json
[1,2,3
```

### Trailing Comma

```json
[1,2,]
```

### Leading Comma

```json
[,1]
```

### Double Comma

```json
[1,,2]
```

### Garbage Token

```json
[1,X]
```

## Key Lesson

```text
Parser bugs often start with state corruption, not memory corruption.
```

---

# Site 5: Recursion Depth Protection

## Defensive Pattern

```c
if (depth >= CJSON_NESTING_LIMIT)
{
    return false;
}

depth++;
...
depth--;
```

## Data Flow

```text
Nested JSON
      ↓
Recursive Calls
      ↓
Stack Usage
      ↓
Stack Overflow
```

## Research Questions

* What is the nesting limit?
* Is check before recursion?
* Does every path decrement depth?
* Does limit+1 fail safely?

## Threat Model

```text
Deep Nesting
      ↓
Many Stack Frames
      ↓
Stack Exhaustion
```

## Fuzz Cases

### At Limit

```json
[[[[[[[[[[ ... ]]]]]]]]]]
```

### Limit + 1

```json
[[[[[[[[[[[ ... ]]]]]]]]]]]
```

### Massive Depth

```json
10000 nested arrays
```

## Key Lesson

```text
Security defenses must be audited like vulnerabilities.
```

---

# Universal Research Workflow

## 1. Trace User Input

```text
Input
 ↓
State Variable
 ↓
Dangerous Operation
 ↓
Crash / Bug / DoS
```

---

## 2. Identify State Variables

```text
offset
length
depth
index
allocation_length
```

---

## 3. Identify Dangerous Sinks

```c
malloc()
calloc()
realloc()
memcpy()
memmove()
strcpy()
pointer arithmetic
recursion
loops
```

---

## 4. Build Threat Models

```text
Can user control size?
Can user control offset?
Can user control recursion?
Can user control loop count?
```

---

## 5. Create Fuzz Inputs

```text
Truncated Input
Huge Input
Malformed Input
Deep Nesting
Boundary Values
Unexpected Tokens
```

---

# Researcher Checklist

```text
[ ] User-controlled allocation
[ ] Integer overflow
[ ] Pointer arithmetic
[ ] OOB read/write
[ ] State desynchronization
[ ] Recursion depth
[ ] Algorithmic complexity
[ ] Missing bounds checks
[ ] Failure-path auditing
[ ] Resource exhaustion
```

# Core Mindset

```text
Don't ask:
"Is this vulnerable?"

Ask:
"What assumptions is this code making?"

Then break those assumptions.
```
