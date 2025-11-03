---
description: Apply when user mentions git, commit, push, merge, rebase, conflict, or code submission. Contains critical git pull --rebase workflow and commit message format.
globs:
alwaysApply: false
---

# Git Workflow

## Standard Workflow (CRITICAL: Always Rebase!)

```bash
# 1. Stage changes
git add <files>

# 2. Commit locally
git commit -m "type(scope): brief description"

# 3. Pull with rebase (CRITICAL!)
git pull --rebase origin <branch>

# 4a. No conflicts: Push directly
git push origin <branch>

# 4b. If conflicts:
#   - Edit conflicted files (look for <<<<<<<, =======, >>>>>>>)
#   - Stage resolved files: git add <files>
#   - Continue rebase: git rebase --continue
#   - Push: git push origin <branch>

# 4c. Abort rebase if needed:
git rebase --abort
```

## Why Rebase?

Regular `git pull` creates merge commits (branching history).
`git pull --rebase` replays your commits on top (linear history).

```
Without rebase:      With rebase:
A → B → C → merge    A → B → C → D
     ↘ D ↗           (linear)
```

## Commit Message Format

```
<type>(<scope>): <subject>
```

### Type (Required)
- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation
- **style**: Code style (formatting)
- **refactor**: Code refactoring
- **perf**: Performance
- **test**: Tests
- **chore**: Build/tools

### Scope (Optional)
- gui, slicer, network, printer, config, build, deps
- Or specific module name

### Subject (Required)
- Use imperative mood: "add feature" not "added"
- Start with lowercase
- No period at end
- Max 50 characters

### Examples

Good:
```
feat(gui): add crash test menu for debugging
fix(network): resolve timeout in printer discovery
refactor(slicer): simplify layer calculation logic
```

Bad:
```
fixed bug           // Too vague
update code         // Not descriptive
Added feature.      // Wrong tense
```

## Before Committing

- Remove debugging code
- Remove commented-out code
- Test your changes
- Write meaningful commit message in English
- Keep changes minimal (NO auto-formatting)
