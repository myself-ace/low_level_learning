# JSON-C Harness Notes

## Goal

Feed attacker-controlled input into the JSON parser.

Flow:

stdin
↓
buf[]
↓
json_tokener_parse_ex()
↓
json_object
↓
output

---

## Buffer

```c
char buf[100000];
```

Creates a 100KB stack buffer.

Input will be stored here.

Example:

```json
{"name":"ace"}
```

gets copied into:

```c
buf
```

---

## Reading Input

```c
size_t len = fread(buf, 1, sizeof(buf), stdin);
```

Meaning:

* Read from stdin
* Store in buf
* Read up to 100000 bytes
* Return number of bytes read

Example:

```bash
echo '{"name":"ace"}' | ./harness
```

Result:

```c
buf = {"name":"ace"}
len = 14
```

---

## Parser State

```c
struct json_tokener *tok =
    json_tokener_new();
```

Creates a parser object.

Stores:

* current parser state
* nesting depth
* current position
* parser errors

Think:

```c
Parser *p = create_parser();
```

No arguments needed because parser starts empty.

---

## Actual Parsing

```c
json_object *obj =
    json_tokener_parse_ex(
        tok,
        buf,
        len,
        NULL
    );
```

This is the most important line.

Arguments:

```c
tok
```

Parser state.

```c
buf
```

Input bytes.

```c
len
```

Input length.

```c
NULL
```

No extra options.

---

## What Parser Does

Input:

```json
{"age":25}
```

Parser reads:

```text
{
"age"
25
}
```

Builds:

```json
{
  "age":25
}
```

Returns:

```c
json_object *
```

---

## Success

```c
if (obj)
```

Means parsing succeeded.

Convert object back to string:

```c
const char *str =
    json_object_to_json_string(obj);
```

Print:

```c
printf("%s\n", str);
```

Output:

```json
{"age":25}
```

---

## Failure

Invalid JSON:

```json
{"age":
```

Parser returns:

```c
NULL
```

Program prints:

```text
Parse failed
```

No crash.

---

## Memory Cleanup

Free parsed JSON:

```c
json_object_put(obj);
```

Free parser state:

```c
json_tokener_free(tok);
```

Rule:

Anything created with:

```c
json_tokener_new()
```

must eventually be freed.

---

## Fuzzing View

Fuzzer controls:

```c
buf
```

and feeds millions of inputs:

```text
{}
[]
{{{{{
\xff\xff\xff
AAAAAAAAAAAA
{"a":[[[[[[
```

All of them reach:

```c
json_tokener_parse_ex()
```

Goal:

Make parser hit weird states.

Possible bugs:

* Heap Overflow
* Stack Overflow
* Use After Free
* Double Free
* Integer Overflow
* Assertion Failure
* Crash

---

## Important Functions

Create parser:

```c
json_tokener_new()
```

Parse input:

```c
json_tokener_parse_ex()
```

Convert object to string:

```c
json_object_to_json_string()
```

Free JSON object:

```c
json_object_put()
```

Free parser:

```c
json_tokener_free()
```

---

## What To Remember

1. stdin → buf
2. buf → json_tokener_parse_ex()
3. parser returns json_object
4. success = obj != NULL
5. failure = obj == NULL
6. free object
7. free parser

Most important line:

```c
json_tokener_parse_ex(tok, buf, len, NULL);
```

Everything before it prepares input.

Everything after it handles results.

That is the actual target being fuzzed.
