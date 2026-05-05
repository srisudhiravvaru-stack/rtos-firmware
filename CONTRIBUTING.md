# Contributing Guide

Thank you for contributing to this industrial firmware project. Please read this document fully before opening a pull request.

---

## Branch Strategy (GitFlow)

```
main         ← production releases (tagged vX.Y.Z)
develop      ← integration branch (all PRs target here)
feature/*    ← new features branched from develop
bugfix/*     ← bug fixes branched from develop
hotfix/*     ← critical fixes branched from main
release/*    ← release stabilization branches
```

---

## Commit Convention (Conventional Commits)

Format: `<type>(<scope>): <short description>`

| Type | When to use |
|---|---|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `refactor` | Code restructuring, no behavior change |
| `test` | Adding or updating tests |
| `docs` | Documentation only |
| `ci` | CI/CD workflow changes |
| `chore` | Tooling, config, deps |
| `perf` | Performance improvement |

Example: `feat(sensor): add BME688 temperature compensation`

---

## Pull Request Checklist

Before marking a PR ready for review, verify all of the following:

### Code Quality
- [ ] `clang-format` applied — no format warnings (`make lint`)
- [ ] `cppcheck` passes with zero errors
- [ ] No new compiler warnings (`-Wall -Wextra -Werror`)
- [ ] All public functions have Doxygen `@brief`, `@param`, `@return`

### Testing
- [ ] Unit tests added/updated for all changed logic
- [ ] All existing tests pass (`ctest --output-on-failure`)
- [ ] Stack high-water marks checked (no task < 32 words remaining)

### RTOS Safety
- [ ] No blocking calls in ISR context
- [ ] `FromISR` variants used for all ISR→task notification
- [ ] All shared resources protected by mutex or critical section
- [ ] No unbounded loops without `vTaskDelay` or blocking queue call
- [ ] Stack sizes justify by worst-case analysis (printf, math, recursion)

### Memory
- [ ] No dynamic allocation in safety-critical tasks (use static)
- [ ] `configASSERT` guards on all non-NULL pointer dereferences
- [ ] `vApplicationMallocFailedHook` reachable path considered

### Documentation
- [ ] `README.md` updated if architecture changes
- [ ] Hardware pin assignments updated in Hardware Requirements table

---

## Code Style Summary

- **Indentation:** 4 spaces, no tabs
- **Braces:** Allman style (opening brace on its own line)
- **Naming:** `g_` prefix for globals, `s_` for file-static, `p` for pointers
- **Types:** Use `uint8_t`, `uint32_t` etc. (not `int`, `long`)
- **Constants:** ALL_CAPS with `#define` or typed `enum`
- **Column limit:** 100 characters
- See `.clang-format` for the full specification.

---

## Local Development Setup

```bash
# Install pre-commit hooks (runs clang-format on staged files)
pip install pre-commit
pre-commit install

# Run full CI locally
./scripts/run_tests.sh
```
