# binary-record-format

Custom binary serializer and parser written in C.

---

## Features

- Custom binary file format
- Big-endian serialization
- Structured record storage
- Binary parser implementation
- File validation using magic numbers
- Safe binary I/O wrappers

---

## File Format

Header Layout:

| Field        | Size |
|--------------|------|
| Magic        | 4B   |
| Version      | 2B   |
| Record Count | 4B   |

Record Layout:

| Field | Size |
|------|------|
| Type | 1B |
| Len  | 2B |
| Data | N bytes |

---

## Build

```bash
gcc writer.c -o writer
gcc parser.c -o parser
```

---

## Run

Generate binary file:

```bash
./writer
```

Parse binary file:

```bash
./parser
```

---

## Concepts Explored

- Binary serialization
- Endianness
- File format design
- Structured parsing
- Byte-level encoding
- Heap allocation
- Low-level systems programming
