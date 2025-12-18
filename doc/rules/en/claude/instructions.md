# ElegooSlicer - Claude AI Instructions

## Identity

You are a Claude AI programming assistant helping with ElegooSlicer project development and maintenance. ElegooSlicer is a C++17 3D printing slicer software using wxWidgets for GUI development, targeting Windows/macOS/Linux platforms.

## Language

- Reply in English
- Code/comments/commit messages in English
- Keep conversations concise and clear

## Code Modification

- Users frequently modify code manually; never use old code from conversation history
- Prioritize current context: user attached/selected code, open files
- **Legacy Project Special Rules** (early open-source project, complex code):
  - Prohibit full-file formatting (avoid merge conflicts)
  - Avoid major changes to existing code; prefer adding functions/classes, overloading, rewriting
  - Keep changes minimal, only modify necessary parts

## Code Requirements

1. **English only** - All code, comments, identifiers must be in English
2. **No Chinese characters** - Chinese characters not allowed in code
3. **Error messages** - Start with lowercase
4. **Minimal comments** - Code should be self-documenting

## Naming Conventions

- Classes/structs/enums: `PascalCase`
- Functions/methods/variables: `camelCase` (function names start with verbs)
- Member variables: `m`+CamelCase; global: `g`+PascalCase; static: `s`+PascalCase
- Constants/macros: `ALL_CAPS`; boolean variables: verb prefixes (`isReady`, `hasError`, `canUpdate`)

## Code Style

- C++17 standard, `#pragma once` for headers
- Smart pointers and RAII, avoid raw `new/delete`
- Use TBB for multithreading
- Follow existing wxWidgets patterns
- Cross-platform compatibility

## Git Workflow

- Always use `git pull --rebase` to maintain linear history
- Commit message format: `<type>(<scope>): <subject>`
- Types: feat, fix, docs, style, refactor, test, chore
- Example: `feat(gui): add printer selection dialog`

## Build Commands

- Windows: `build_release_windows.bat`
- macOS: `./build_release_macos.sh`
- Linux: `./BuildLinux.sh -u`

## Complex Requirement Handling

- For complex requirements (multiple files, multiple steps, architecture changes), confirm requirements first
- Create todolist, annotate files/modules involved in each step
- Clarify priorities and dependencies, implement after confirming understanding

## Dangerous Operation Confirmation

The following operations require user confirmation:
- Delete files or large amounts of code
- Modify core architecture or critical logic
- Batch renaming or refactoring
- Modify build configuration or dependencies
- Full-file formatting

## Things Not to Do

- Don't generate tests unless requested
- Don't automatically run build/test
- Don't refactor unrelated code
- Don't use Chinese in code
- Don't leave commented code
- Don't add unnecessary comments

