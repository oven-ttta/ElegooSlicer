# Coding Standards

Important Principle: This is a legacy project. When modifying existing code, prioritize maintaining consistency within files. New code follows this standard.

## Language
- English only: Code, comments, identifiers
- No Chinese characters
- UTF-8 without BOM
- Error messages start with lowercase

## Naming and Structure

- Classes/structs/enums: `PascalCase`; functions/methods/variables: `camelCase` (function names start with verbs)
- Member variables: `m`+CamelCase (e.g., `mPrinterList`); global: `g`+PascalCase; static: `s`+PascalCase
- Constants/macros: `ALL_CAPS`; boolean variables: verb prefixes (`isReady`, `hasError`, `canUpdate`)
- Use `#pragma once` in headers, prefer forward declarations to reduce dependencies
- Include order (blank lines between groups): paired header → project headers → third-party libraries → standard library
- Module dependencies follow unidirectional principle: lower modules (Utils/Core) don't depend on upper modules (GUI/Business)
- Cross-module calls prefer interfaces/callbacks, avoid direct dependency on concrete implementations
- Path handling uses `std::filesystem` or `boost::nowide` for Chinese support

## C++ Style

- Standard: C++17 (selective C++20)
- Memory: Smart pointers, RAII
- Threading: TBB

## Functions and Classes

- Each function/class focuses on single responsibility; function/class names accurately describe functionality
- Boolean returns use `is/has/can`, void uses `execute/save/update`; early return to reduce nesting
- Scalars/small objects by value, large objects use `const T&`, modifications use `T&`, optionally `std::optional<T>`
- Output parameters prefer return values (struct/`tuple`/`optional`/`expected`), reference outputs use `outXYZ`
- Buffers use `std::span<T>`, read-only strings use `std::string_view`
- Raw pointers express "nullable and no ownership", ownership transfer uses `unique_ptr`/`shared_ptr`
- Pointer or reference parameters should be validated at function entry, use `const` to qualify non-modifiable objects
- Prefer composition over inheritance; use `const` when possible, `constexpr` when possible
- Members prefer private with getter/setter, member functions add `const` when not modifying state

## Memory and Error Handling

- Follow RAII, avoid raw `new/delete` except for GUI controls and third-party library interfaces, prefer `unique_ptr`/`shared_ptr`
- wxWidgets controls managed by parent-child relationships, custom controls need clear ownership
- Don't frequently allocate/deallocate memory in loops, consider reuse or pre-allocation
- Expected failures prefer `optional`/`expected`/error codes, unrecoverable errors use standard exceptions with context
- Input parameters should be validated at function boundaries, error messages include context/path/parameter values (avoid privacy leaks)

## Modern C++

- `auto` only for complex iterators/lambdas, base types (`int`/`double`/`bool`) explicitly declared
- Prefer C++17 (range for, structured bindings), `enum class` enums, `nullptr` for null pointers
- Avoid C-style casts, use `static_cast`/`dynamic_cast`; containers prefer `vector`/`map`/`unordered_map`
- Use `std::move` to reduce copies
- Avoid unnecessary heap allocation, prefer stack-based objects when possible
- Use algorithms from `<algorithm>` to optimize loops (e.g., `std::sort`, `std::for_each`)
- Avoid global variables, use singleton pattern cautiously
- Separate interface and implementation in classes
- Use templates and metaprogramming wisely for generic solutions

## Concurrency and Threading

- UI updates must be on main thread, background threads communicate via `CallAfter`/event queue, avoid direct control manipulation
- Use `thread`/`mutex`/`lock_guard`/`atomic` for thread safety, shared resources need clear ownership and locking strategy
- Cross-thread data transfer prefers value copy or move, avoid shared pointers; long operations use thread pool

## Comments

Minimal comments. Code should be self-explanatory.

Comment only in the following cases:
- Complex algorithms (explain WHY, not WHAT)
- Non-obvious workarounds
- Public APIs (brief description)

Don't comment obvious code or leave commented code.

## Code Organization

- Follow existing patterns
- Don't refactor unrelated code
- Keep changes focused
- Use wxWidgets patterns for GUI development
- Cross-platform compatibility required

## Things to Avoid

- Don't use `using namespace std` in headers or implement non-template functions
- Don't use C-style strings (`char*`) and arrays, use `std::string`/`std::vector`
- Don't ignore compiler warnings, fix all warnings
