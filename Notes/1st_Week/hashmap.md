# HashMaps and Hash Table Internals

## Goal

Understand:
- hashmaps
- hash tables
- hashing
- collisions
- buckets
- resizing
- load factors
- memory ownership
- failure handling
- performance tradeoffs

This topic is foundational for:
- databases
- interpreters
- kernels
- compilers
- allocators
- caches
- networking
- malware
- exploit tooling

Hashmaps are one of the most important real-world data structures.

---

# Core Idea

Hashmap:
```text
key -> value
```

Fast lookup using:
```text
hash function
```

Instead of:
```text
linear search
```

---

# Basic Mental Model

Suppose:

```text
"apple" -> 10
"banana" -> 20
```

Hash function converts:
```text
string -> integer
```

Example:

```text
hash("apple") = 523
```

Then:

```text
523 % table_size
```

determines storage index.

---

# High Level Structure

Typical hashmap:

```c
typedef struct {
    Entry **buckets;
    size_t capacity;
    size_t size;
} HashMap;
```

---

# Meaning of Fields

| Field | Purpose |
|---|---|
| buckets | array of bucket pointers |
| capacity | total bucket count |
| size | stored element count |

---

# Entry Structure

Typical entry:

```c
typedef struct Entry {

    char *key;
    int value;

    struct Entry *next;

} Entry;
```

---

# Why Linked List Exists

Multiple keys can hash to same bucket.

This is:
```text
collision
```

Linked list handles:
```text
collision chaining
```

---

# Visualization

```text
Bucket Array

0 -> NULL
1 -> Entry -> Entry
2 -> NULL
3 -> Entry
4 -> Entry -> Entry -> Entry
```

---

# Hash Function

Critical component.

Purpose:
```text
convert arbitrary input into deterministic integer
```

---

# Important Hash Function Properties

Good hash functions should:
- distribute evenly
- avoid clustering
- be deterministic
- be fast

---

# Bad Hash Function

Example:

```c
return str[0];
```

Terrible because:
- many collisions
- predictable
- poor distribution

---

# Simple djb2 Hash

Very common beginner hash:

```c
unsigned long hash(const char *str) {

    unsigned long hash = 5381;

    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}
```

---

# Bucket Index Calculation

Example:

```c
index = hash(key) % capacity;
```

Purpose:
```text
map huge hash value into valid table index
```

---

# Collision Handling

Collisions are unavoidable.

Different keys can produce:
```text
same bucket index
```

---

# Collision Example

```text
hash("abc") % 8 = 3
hash("xyz") % 8 = 3
```

Both go into:
```text
bucket 3
```

---

# Collision Resolution Strategies

| Method | Description |
|---|---|
| Chaining | linked lists per bucket |
| Open Addressing | probe for empty slot |
| Robin Hood Hashing | rebalance probing |
| Cuckoo Hashing | multiple hash functions |

Most beginner implementations use:
```text
chaining
```

---

# Insertion Workflow

Typical process:

```text
hash key
compute bucket index
traverse bucket list
check duplicates
insert new entry
```

---

# HashMap Put Example

```c
hashmap_put(map, "apple", 10);
```

Internally:
1. hash calculated
2. bucket selected
3. collision list traversed
4. key inserted or updated

---

# Lookup Workflow

```text
hash key
find bucket
traverse linked list
compare keys
return value
```

---

# Why strcmp() Matters

Hash equality does NOT guarantee key equality.

Must still compare actual strings:

```c
strcmp(entry->key, key)
```

---

# Common Beginner Mistake

Bad:

```c
if (entry->key == key)
```

This compares:
```text
pointer addresses
```

NOT string contents.

---

# Dynamic Allocation Inside HashMaps

Usually allocate:
- hashmap structure
- bucket array
- entries
- copied keys

---

# Ownership Rules

Critical concept.

Question:
```text
who owns key memory?
```

Bad ownership handling causes:
- leaks
- dangling pointers
- double free

---

# Safer Key Handling

Instead of:

```c
entry->key = key;
```

Prefer:

```c
entry->key = strdup(key);
```

Now hashmap owns:
```text
copied string
```

Must later:
```c
free(entry->key);
```

---

# Failure Handling

Every allocation can fail.

Must check:
- hashmap allocation
- bucket allocation
- entry allocation
- strdup allocation

---

# Safe malloc Pattern

Correct:

```c
Entry *e = malloc(sizeof(Entry));

if (!e) {
    return FAILURE;
}
```

---

# Safe strdup Pattern

Correct:

```c
e->key = strdup(key);

if (!e->key) {
    free(e);
    return FAILURE;
}
```

---

# Load Factor

Critical hashmap concept.

Formula:

```text
load_factor = size / capacity
```

Measures:
```text
how full table is
```

---

# Why Load Factor Matters

High load factor:
- more collisions
- slower lookups
- longer chains

---

# Typical Resize Threshold

Common threshold:

```text
0.75
```

When exceeded:
```text
resize table
```

---

# Resizing

Usually:
```text
capacity *= 2
```

Then:
```text
rehash all entries
```

---

# Why Rehashing Needed

Bucket index depends on:
```text
capacity
```

Changing capacity changes:
```text
bucket positions
```

---

# Dangerous Resize Bug

Bad:

```c
map->buckets = realloc(...)
```

without temporary pointer.

If realloc fails:
- original buckets lost
- memory leak

---

# Iterator Invalidation

Important concept.

After resize:
```text
old bucket pointers become invalid
```

Dangerous if external references exist.

---

# Time Complexity

Average case:

| Operation | Complexity |
|---|---|
| insert | O(1) |
| lookup | O(1) |
| delete | O(1) |

Worst case:
```text
O(n)
```

if collisions become severe.

---

# Worst Case Scenario

Poor hashing causes:

```text
all entries in one bucket
```

Hashmap becomes:
```text
linked list
```

---

# Hash Flooding

Real-world attack.

Attacker intentionally causes:
```text
massive collisions
```

Result:
- CPU exhaustion
- denial of service

---

# Defensive HashMap Design

Must validate:
- NULL keys
- capacity overflow
- duplicate insertion
- resize failures
- load factor
- allocation failures

---

# Safe Initialization

Good:

```c
HashMap *map = calloc(1, sizeof(HashMap));
```

Why:
```text
zero-initialized memory
```

---

# Bucket Allocation

Typical:

```c
map->buckets = calloc(capacity, sizeof(Entry *));
```

Benefits:
- buckets start as NULL
- simpler logic

---

# Deletion Workflow

Deletion must:
- locate entry
- unlink correctly
- free key
- free entry

---

# Common Delete Bug

Bad pointer updates cause:
- broken linked lists
- dangling pointers
- memory leaks

---

# Use After Free Risk

Dangerous:

```c
free(entry);

printf("%s", entry->key);
```

Classic UAF.

---

# Double Free Risk

Dangerous ownership confusion:

```c
free(key);
hashmap_destroy(map);
```

If hashmap also frees:
```text
double free
```

---

# Memory Layout

Example:

```text
HashMap
    |
    +---- buckets[]
              |
              +---- Entry
                        |
                        +---- key string
```

Multiple heap allocations involved.

---

# Pointer Relationships

Hashmaps heavily depend on:
- pointer ownership
- linked structures
- heap memory
- traversal correctness

---

# Cache Locality Problem

Linked lists:
```text
poor cache locality
```

Modern hashmaps often prefer:
- flat arrays
- open addressing

for performance.

---

# Open Addressing

Alternative collision strategy.

Instead of linked lists:
```text
probe nearby slots
```

---

# Linear Probing

Example:

```text
slot occupied?
check next slot
```

Simple but:
- clustering issues

---

# Tombstones

Deletion in open addressing often leaves:
```text
special deleted markers
```

called:
```text
tombstones
```

---

# Real-World HashMaps

| System | Implementation |
|---|---|
| Python dict | open addressing |
| Java HashMap | chaining + trees |
| Redis | custom hash table |
| Linux kernel | specialized hash tables |

---

# HashMap Vulnerabilities

Common bugs:
- UAF
- double free
- iterator invalidation
- collision abuse
- integer overflow
- NULL dereference

---

# HashMaps and Exploitation

Corrupted hashmap internals can cause:
- arbitrary pointer writes
- corrupted linked lists
- invalid frees
- allocator corruption

Complex heap-heavy structures become good exploitation targets.

---

# Experiments I Ran

---

# Experiment 1 — Collision Observation

Used tiny capacity:

```text
capacity = 4
```

Observation:
- many collisions
- longer linked lists

---

# Experiment 2 — Resize Trigger

Inserted many keys.

Observation:
- rehashing changed bucket positions
- lookup still worked correctly

---

# Experiment 3 — Missing strdup()

Stored raw key pointer.

Observation:
- dangling key after caller freed memory

---

# Experiment 4 — Bad Hash Function

Used simplistic hash.

Observation:
- uneven bucket distribution
- terrible performance

---

# Important Realizations

- hashmaps are memory-heavy structures
- ownership rules matter heavily
- collisions are unavoidable
- resizing invalidates internal layout
- hashing quality affects performance massively
- linked structures create many pointer risks
- hashmaps require careful cleanup logic

---

# Useful Compiler Flags

```bash
gcc -Wall -Wextra -Werror -pedantic
```

With sanitizers:

```bash
gcc -fsanitize=address,undefined
```

---

# Useful Debugging Tools

| Tool | Purpose |
|---|---|
| GDB | structure inspection |
| ASAN | memory corruption |
| Valgrind | leaks/UAF |
| UBSAN | undefined behavior |

---

# Useful GDB Commands

Inspect hashmap:

```bash
print *map
```

Inspect buckets:

```bash
x/16gx map->buckets
```

Inspect linked list:

```bash
print *entry
```

---

