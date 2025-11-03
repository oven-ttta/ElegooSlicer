---
description: Coding standards for ElegooSlicer
globs: ["**/*.cpp", "**/*.hpp", "**/*.js", "**/*.ts", "**/*.h", "**/*.c"]
alwaysApply: false
---

# Coding Standards

## Language & Encoding

- All code, comments, and identifiers MUST be in English
- NO Chinese characters anywhere in code
- File encoding: UTF-8 without BOM
- Error messages start with lowercase letter

## Code Style

- C++17 standard with selective C++20 features
- Header guards: Use `#pragma once`
- Memory management: Smart pointers, RAII patterns
- Thread safety: Use TBB for parallelization
- Comments: Keep minimal, code should be self-documenting

## Naming Conventions

**C++:**
- Classes/Structs/Enums: PascalCase (`ElegooClass`, `MainFrame`)
- Functions/Variables: camelCase (`checkStatus`, `userName`)
- Member variables: `m` + PascalCase (`mUserName`, `mPrinterList`)
- Global variables: `g` + PascalCase (`gLogFolder`)
- Static variables: `s` + PascalCase (`sInstanceCount`)
- Constants/Macros: UPPER_SNAKE_CASE (`MAX_RETRY_COUNT`)

## Comments

Keep minimal. Code should be self-documenting through clear naming.

**When to comment:**
- Complex algorithms (explain WHY, not WHAT)
- Non-obvious workarounds
- Public API (brief)

**Don't comment:**
- Obvious code
- Commented-out code (delete it)


## Code Organization

- Follow existing patterns in codebase
- Don't refactor unrelated code
- Keep changes focused and minimal
- Use existing wxWidgets patterns for GUI
- Maintain cross-platform compatibility
