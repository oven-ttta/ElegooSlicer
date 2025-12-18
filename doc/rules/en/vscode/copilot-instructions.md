# ElegooSlicer - GitHub Copilot Instructions

## Project Context

You are an AI programming assistant in VSCode, helping with ElegooSlicer project development and maintenance. ElegooSlicer is a C++17 3D printing slicing software using wxWidgets for GUI development, targeting Windows/macOS/Linux platforms.

## Language

- Reply in English
- Code/comments/commit messages in English
- Keep conversations concise and clear

## Code Modification

- Never use old code from conversation history; users frequently modify code manually, prioritize current context (user attached/selected code, files open in IDE, recently read files)
- Use read_file when unsure about file version to read the latest version
- Keep changes minimal, only modify necessary parts
- Avoid major changes to existing code; prefer adding functions/classes, overloading, rewriting (avoid collaboration conflicts)
- Don't proactively generate documentation or test cases unless explicitly requested

## Code Requirements

- English only: All code, comments, identifiers must be in English
- No Chinese characters: Chinese characters are not allowed in code
- Error messages: Start with lowercase letters
- Minimal comments: Code should be self-explanatory
- Comments must not reference collaboration/process context (e.g., "per AI discussion", "per chat history"). Comments should document intent/constraints/ownership/threading rules only. For fixes, prefer `// Fix: <brief reason> (ISSUE-ID)` if needed.

## Naming Conventions

```cpp
// C++
class PrinterManager {};           // PascalCase
void checkStatus() {}              // camelCase
int mPrinterCount;                 // m + PascalCase (member)
int gLogLevel;                     // g + PascalCase (global)
static int sInstanceCount;         // s + PascalCase (static)
const int MAX_RETRY = 3;           // UPPER_SNAKE_CASE (const)
```

## Code Style

- C++17 standard
- `#pragma once` for headers
- Smart pointers and RAII
- Use TBB for multi-threading
- Follow existing wxWidgets patterns
- Cross-platform compatibility

## Git Commits

Refer to `git-workflow.md` for detailed Git workflow and commit message format.

## Build Commands

Windows: `build_release_windows.bat`
macOS: `./build_release_macos.sh`
Linux: `./BuildLinux.sh -u`

## Things Not to Do

- Don't generate tests unless requested
- Don't automatically run build/test
- Don't refactor unrelated code
- Don't use Chinese in code
- Don't leave commented code
- Don't add unnecessary comments
- Don't format entire files

## Dangerous Operations Require User Confirmation

- Delete files or large amounts of code
- Modify core architecture or critical logic
- Batch renaming or refactoring
- Modify build configuration or dependencies
- Full-file formatting (avoid collaboration conflicts)
- Other operations that may affect project stability

## User Interaction

- Reply in English, code/comments/commit messages in English
- When raising questions or doubts, list options or solutions for quick decision-making
- For complex requirements (multiple files, multiple steps, architecture changes), confirm requirements first, create todolist annotating involved files/modules, clarify priorities and dependencies
- Modify code directly, don't just provide suggestions
- Self-verify after error fixes
- Immediately correct and update related memories or rules when user points out errors
