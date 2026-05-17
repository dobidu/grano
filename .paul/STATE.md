# STATE — Grano

This file is managed by PAUL. Do not edit manually.

## Current state

```yaml
phase: pending-init
loop_position: pre-init
current_plan: null
last_unified: null
session_start: null
```

## Instructions

Run `/paul:init` inside Claude Code to populate this file. PAUL will read `PROJECT.md`, `ROADMAP.md`, and `SPECIAL-FLOWS.md` and set the initial state to F0.

After init, this file is updated automatically at every PLAN, APPLY, and UNIFY transition. It is the source of truth for "where are we right now". When you resume work via `/paul:resume`, PAUL reads this file and tells you the single next action.

## Phase history

(Empty — will populate as phases complete.)

## Accumulated decisions

(Empty — will populate as UNIFY steps log decisions.)

## Deferred issues

(Empty — will populate via `/paul:consider-issues`.)
