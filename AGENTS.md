# Project Agent Settings

## Mission

1. Migrate `ref2` (pygame-based SLAM simulation) to a Raylib C++ implementation.
2. Use `ref1` (successful pygame -> Raylib migration) as the primary reference pattern.
3. After migration confidence is high, make the Raylib C++ version run as WASM in Chrome.

Source of truth: `INSTRUCTIONS.md`.

## Repository Context

1. Primary workspace: `understanding-slam-raylib-wasm`
2. Reference remote 1: `ref1` -> `https://github.com/kimwoonggon/FlappyBird-CPP-Raylib-RL-GRPO.git`
3. Reference remote 2: `ref2` -> `https://github.com/kimwoonggon/slam-understanding-by-simulation-with-python.git`
4. Worktree for ref1: `/Users/wgkim/Downloads/understanding-slam-raylib-wasm-ref1`
5. Worktree for ref2: `/Users/wgkim/Downloads/understanding-slam-raylib-wasm-ref2`

## Default Delivery Phases

1. Phase A: Behavioral parity in native Raylib C++
2. Phase B: Robustness and test hardening
3. Phase C: WASM packaging and Chrome runtime validation

Do not start Phase C until core behavior in Phase A is verified.

## Skill Routing

1. Orchestration and plan/verify loop:
Use `.agents/skills/coding-orchestrator/SKILL.md`
2. C++ design and implementation:
Use `.agents/skills/cpp-pro/SKILL.md`
3. C++ test strategy and execution:
Use `.agents/skills/cpp-testing/SKILL.md`
4. TDD workflow:
Use `.agents/skills/test-driven-development/SKILL.md`
5. WASM gateway and routing:
Use `.agents/skills/discover-wasm/SKILL.md`
6. C++ WASM build/debug optimization:
Use `.agents/skills/cpp-wasm-engineer/SKILL.md`
7. Rust/MultiversX contract-specific binary debugging only:
Use `.agents/skills/multiversx-wasm-debug/SKILL.md`

## Operating Rules

1. For non-trivial work, write and maintain `tasks/todo.md` before coding.
2. After any user correction, append a prevention rule to `tasks/lessons.md`.
3. Keep changes minimal and root-cause driven.
4. Verify with runnable evidence before marking complete.
5. Record unresolved risks and follow-ups in `tasks/todo.md` review section.

## Verification Gate

1. Native parity checks against `ref2` behavior are required before WASM work.
2. WASM completion requires Chrome runtime smoke test and documented constraints.
3. If a step fails, stop and re-plan instead of continuing on broken assumptions.
