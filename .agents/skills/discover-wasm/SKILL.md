---
name: discover-wasm
description: Route WebAssembly tasks to the correct implementation skill. Use when requests mention WebAssembly, WASM, Emscripten, WASI, C/C++ to WASM, browser runtime issues, wasm-pack, Rust to WASM, or contract/runtime debugging. Prefer cpp-wasm-engineer for C/C++ workloads and multiversx-wasm-debug for Rust smart-contract binary analysis.
license: MIT
metadata:
  author: rand
  version: "3.2"
compatibility: Designed for Claude Code. Compatible with any agent supporting the Agent Skills format.
---

# Wasm Skills Discovery

Use this skill as a gateway only. Route quickly, then load the specialist skill.

## When This Skill Activates

Activate for:
- WebAssembly
- WASM
- Emscripten
- WASI
- browser WASM runtime issues
- C/C++ to WASM builds
- Rust to WASM or wasm-pack
- portable bytecode

## Routing Rules

1. If the task is C/C++, CMake/Make migration, Emscripten, browser rendering loop, JS interop, pthread/COOP/COEP, filesystem persistence, or WASI runtime behavior:
Load `.agents/skills/cpp-wasm-engineer/SKILL.md`
2. If the task is Rust smart-contract binary size/panic/debug flow (MultiversX context):
Load `.agents/skills/multiversx-wasm-debug/SKILL.md`
3. If ambiguous:
Ask one routing question: "Is this C/C++ runtime/build debugging, or Rust contract binary analysis?"

## Why This Routing

1. Avoid Rust-oriented guidance for C++ Emscripten/WASI tasks.
2. Reduce time-to-fix by loading the right edge-case playbook first.
3. Keep gateway context light and progressive.

## Load Order

1. Start here (`discover-wasm`) to classify.
2. Load one implementation skill:
- `.agents/skills/cpp-wasm-engineer/SKILL.md`, or
- `.agents/skills/multiversx-wasm-debug/SKILL.md`
3. For C++ tasks, then load only needed references from `cpp-wasm-engineer/references/`.

## Quick Commands

1. C++ path:
`Read .agents/skills/cpp-wasm-engineer/SKILL.md`
2. Rust contract path:
`Read .agents/skills/multiversx-wasm-debug/SKILL.md`
