# Surface-Level Reconnaissance of json-c

---

# Repository

- Project: json-c
- Language: C
- Repository: https://github.com/json-c/json-c
- Purpose: Parse, manipulate, and generate JSON data in C applications.

---

# What is json-c?

json-c is a C library used for:

- Parsing JSON strings into C objects
- Creating JSON objects programmatically
- Modifying JSON objects
- Serializing JSON objects back into strings
- Handling arrays, objects, strings, numbers, booleans, and null values

It acts as a bridge between raw JSON text and structured C data.

---

# Core Functionality

Input:

```json
{
  "name": "alice",
  "age": 30
}
```

↓

Parsing

↓

Internal json_object structures

↓

Application logic

↓

Output JSON

```json
{
  "name":"alice",
  "age":30
}
```

---

# Who Uses json-c?

json-c is widely used in:

- Linux system software
- Embedded systems
- Routers
- Network management tools
- Configuration parsers
- REST API backends
- IoT devices

Examples:

- OpenWrt
- NetworkManager
- Various Linux distributions
- Embedded firmware projects

Security relevance:

A vulnerability in json-c may affect any software that accepts attacker-controlled JSON.

---

# Input Accepted By json-c

json-c primarily accepts JSON text.

Examples:

## String Input

```c
json_object *obj =
    json_tokener_parse("{\"name\":\"alice\"}");
```

## File Input

```json
{
  "config": true
}
```

File contents are read into memory and parsed.

## Network Input

Examples:

- HTTP POST bodies
- REST API requests
- WebSocket messages
- RPC messages

## User-Controlled Input

```json
{
  "attacker":"payload"
}
```

Most interesting source for security testing.

---

# Output Produced By json-c

Parsing returns:

```c
json_object *
```

Example:

Input:

```json
{
  "user":"bob",
  "id":123
}
```

Internal structure:

```
json_object
├── user -> string
└── id   -> int
```

Supported JSON Types:

| JSON Type | Internal Representation |
|------------|------------------------|
| object | json_object |
| array | json_object |
| string | json_object |
| integer | json_object |
| double | json_object |
| boolean | json_object |
| null | json_object |

Everything ultimately becomes a:

```c
json_object *
```

---

# Untrusted Input

Untrusted input is any JSON controlled by an attacker.

Examples:

## API Request

```http
POST /api
Content-Type: application/json
```

Body:

```json
{
  "cmd":"evil"
}
```

## Uploaded File

```json
{
  "data":"malicious"
}
```

## Network Packet

```json
{
  "payload":"attacker"
}
```

---

# Potential Attack Vectors

## 1. Malformed JSON

Example:

```json
{
  "a":1
```

Possible Issues:

- Parse failures
- State machine bugs
- Error handling flaws

---

## 2. Deeply Nested Objects

Example:

```json
{
  "a":{
    "b":{
      "c":{
        "d":{}
      }
    }
  }
}
```

Potential Issues:

- Stack exhaustion
- Excessive recursion
- Depth-limit bugs

---

## 3. Huge Arrays

Example:

```json
[
  1,
  1,
  1,
  1,
  1
]
```

Scaled to millions of entries.

Potential Issues:

- Memory exhaustion
- Allocation failures

---

## 4. Huge Strings

Example:

```json
{
  "blob":"AAAAAAAAAAAAAAAAAAAA"
}
```

Potential Issues:

- Large allocations
- Integer overflows
- Heap pressure

---

## 5. Unicode Handling

Example:

```json
{
  "x":"\uFFFF"
}
```

Potential Issues:

- UTF-8 conversion bugs
- Invalid encoding paths

---

## 6. Large Numbers

Example:

```json
{
  "n":999999999999999999999999999999
}
```

Potential Issues:

- Integer overflow
- Conversion errors
- Precision loss

---

## 7. Truncated Input

Example:

```json
{
  "name":"test
```

Potential Issues:

- Incomplete state transitions
- Boundary condition bugs

---

# JSON Format Basics

Valid JSON Values:

```json
{}
[]
"string"
123
true
false
null
```

---

# Invalid JSON Examples

Trailing Comma:

```json
{
  "a":1,
}
```

Single Quotes:

```json
{
  'a':1
}
```

Unquoted Key:

```json
{
  a:1
}
```

---

# Target 2 — Test Cases

## test1_valid.json

```json
{"a":1}
```

---

## test2_empty_object.json

```json
{}
```

---

## test3_empty_array.json

```json
[]
```

---

## test4_nested.json

```json
{
  "a":{
    "b":{
      "c":{
        "d":{
          "e":"test"
        }
      }
    }
  }
}
```

---

## test5_large_number.json

```json
{
  "num":999999999999999999999999999999999999
}
```

---

## test6_unicode.json

```json
{
  "msg":"\uFFFF"
}
```

---

## test7_large_string.json

```json
{
  "blob":"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
}
```

---

## test8_truncated.json

```json
{
  "a":1
```

---

## test9_missing_quote.json

```json
{
  "name":"alice
}
```

---

## test10_deep_array.json

```json
[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]
```

---

# Phase 3 — First Code Skim

Files To Inspect:

```text
json_tokener.c
json_object.c
```

Primary Parsing Entry Point:

```c
json_tokener_parse()
```

Object Creation Functions:

```c
json_object_new_object()
json_object_new_array()
json_object_new_string()
json_object_new_int()
json_object_new_double()
```

---

# High-Level Parse Flow

Input JSON String

↓

```c
json_tokener_parse()
```

↓

```c
json_tokener_parse_ex()
```

↓

Tokenizer State Machine

↓

Object Creation

↓

```c
json_object_new_*
```

↓

Returned

```c
json_object *
```

---

# Initial Hot Path Functions

```c
json_tokener_parse()

json_tokener_parse_ex()

json_tokener_new()

json_tokener_free()

json_object_new_object()

json_object_new_array()

json_object_new_string()

json_object_new_int()

json_object_object_add()

json_object_array_add()
```

---

# Things To Watch During Code Review

Recursion?

- Nested objects
- Nested arrays

Memory Allocation?

Look for:

```c
malloc()
calloc()
realloc()
free()
```

Depth Limits?

Search for:

```c
JSON_TOKENER_DEFAULT_DEPTH
```

Error Handling?

Search for:

```c
json_tokener_error
```

Integer Handling?

Search for:

```c
strtol()
strtoll()
strtod()
```

Unicode Handling?

Search for:

```c
utf8
unicode
escape
```

---

# Day 1 Conclusions

1. json-c is a JSON parsing and serialization library written in C.
2. Main attack surface is attacker-controlled JSON.
3. Entry point is `json_tokener_parse()`.
4. Output is a tree of `json_object` structures.
5. Most promising fuzzing targets:
   - Deep nesting
   - Huge arrays
   - Huge strings
   - Unicode escapes
   - Large numbers
   - Truncated input
   - Malformed JSON
6. Primary files for future review:
   - json_tokener.c
   - json_object.c
7. Next goal:
   Trace a single JSON string through `json_tokener_parse()` into object creation functions and map the full call chain.
