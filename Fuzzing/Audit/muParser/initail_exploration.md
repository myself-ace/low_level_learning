# muParser Exploration Notes

## Overview

muParser is a fast mathematical expression parsing library written in C++.

Its primary purpose is to take mathematical expressions provided as strings, parse them, transform them into an internal bytecode representation, and then evaluate them efficiently.

One important optimization is that constant parts of an expression are precomputed before evaluation.

Example:

```cpp
"2 + 3 * x"
```

The constant portion (`2 + 3`) can be partially optimized before runtime evaluation.

---

## Goal

My goal was not to audit the entire library but to:

* Understand what input the parser accepts.
* Trace the path taken by user-controlled input.
* Identify important entry points.
* Map potentially dangerous functions.
* Explore whether common parser bugs could exist.

---

## Initial Assumptions

Before reading the code, I assumed possible bug classes could include:

* Integer overflows
* Buffer overflows
* Unchecked string handling
* Excessively long input expressions
* Memory allocation issues
* Parser state inconsistencies

Since muParser accepts expressions as strings, the entire parsing process becomes an interesting attack surface.

---

## Questions To Investigate

1. Where does user input enter the library?
2. What is the first function that receives the expression?
3. How does the expression move through the parser?
4. Where is bytecode generated?
5. Which functions perform memory allocations?
6. What functions could become security-sensitive?

---

## Input Flow Discovery

During initial exploration I found that user input is typically supplied as a string expression.

The expression is passed into:

```cpp
SetExpr(...)
```

This appears to be one of the primary entry points for user-controlled input.

High-level flow observed:

```text
User Input
    ↓
SetExpr()
    ↓
Expression preprocessing
    ↓
Whitespace / token handling
    ↓
Parsing
    ↓
Bytecode generation
    ↓
Eval()
    ↓
Result
```

The parser performs various modifications and processing steps before the final evaluation stage.

At a high level, `SetExpr()` appears to act as the gateway between external input and the internal parser logic.

---

## Early Observations

* User-controlled input reaches `SetExpr()`.
* The expression is modified before evaluation.
* Parsing and bytecode generation occur before execution.
* `Eval()` is the final stage that produces results.
* Input length and parser behavior may be interesting areas for future investigation.

---

## Why This Exploration Was Stopped

I decided to stop exploring muParser before reaching a deeper code audit.

The main reason was my limited understanding of C++ syntax and design patterns used throughout the project.

While I was able to identify the general input flow and major entry points, I found that:

* Following function calls became increasingly difficult.
* Template-heavy C++ code slowed down analysis.
* I lacked enough familiarity with the codebase architecture to confidently trace deeper execution paths.

Rather than spending significant time fighting the language, I chose to move on and focus on projects that better matched my current skill level.

This was not due to finding the project uninteresting; it was primarily a limitation in my ability to efficiently analyze complex C++ code at the time.

---

## Key Takeaways

* Identified `SetExpr()` as a major input entry point.
* Confirmed that expressions are transformed before evaluation.
* Observed the general parsing → bytecode → evaluation pipeline.
* Practiced basic attack-surface mapping on a real-world parser.
* Discovered a personal weakness: limited ability to navigate larger C++ codebases.

Future work could involve revisiting muParser after gaining stronger C++ reverse engineering and code auditing skills.
