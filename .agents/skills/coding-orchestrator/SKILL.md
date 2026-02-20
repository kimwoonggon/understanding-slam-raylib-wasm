---
name: coding-orchestrator
description: Orchestrate end-to-end software work with a plan-first workflow, task tracking, verification, and continuous self-correction. Use when handling non-trivial coding tasks (3+ steps), architecture decisions, bug-fix execution, CI/test failures, or any request that benefits from explicit plans in tasks/todo.md, validation before completion, and correction patterns captured in tasks/lessons.md.
---

# Coding Orchestrator

Run all substantial work through a disciplined loop: plan, execute, verify, and learn.

## Operating Rules

1. Enter plan mode by default for non-trivial tasks.
2. Treat tasks with 3+ steps or architecture decisions as non-trivial.
3. Stop and re-plan immediately when assumptions break or output diverges.
4. Use focused subagents for research, exploration, and parallel analysis.
5. Assign one task per subagent to keep ownership clear.
6. Fix reported bugs directly; do not ask for unnecessary user hand-holding.
7. Prefer minimal-impact, root-cause fixes over broad refactors.
8. Reject hacky implementations when a clearly better design is feasible.

## Required Files

1. Ensure `tasks/todo.md` exists before implementation for non-trivial work.
2. Ensure `tasks/lessons.md` exists for correction patterns and guardrails.
3. Keep both files concise and checkable.

Use this starter format for `tasks/todo.md`:

```md
# Todo
- [ ] Scope and assumptions
- [ ] Implementation
- [ ] Verification
- [ ] Review notes

# Review
- Outcome:
- Evidence:
- Risks/Follow-ups:
```

Use this starter format for `tasks/lessons.md`:

```md
# Lessons
## YYYY-MM-DD
- Correction:
- Rule to prevent recurrence:
- Applied in:
```

## Workflow

1. Plan first.
Write explicit, checkable steps in `tasks/todo.md`.
2. Verify plan before coding.
Confirm sequence, dependencies, and rollback points.
3. Execute with progress tracking.
Mark checklist items as done during execution.
4. Verify before completion.
Run tests, inspect logs, and compare behavior when relevant.
5. Record review.
Add outcome, evidence, and residual risks in `tasks/todo.md`.
6. Capture corrections.
After any user correction, append a concrete lesson in `tasks/lessons.md`.

## Verification Standard

1. Do not mark complete without proof.
2. Prefer automated tests first; add targeted manual checks when needed.
3. Include failing-to-passing evidence for bug fixes when available.
4. Ask: "Would a staff engineer approve this change?" before finalizing.

## Elegance Gate

1. For non-trivial changes, pause for one design pass before final answer.
2. If the current solution is brittle, implement the cleaner approach.
3. Skip over-engineering on simple one-step fixes.
