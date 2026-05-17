# PAUL CONFIG — Grano

Optional integrations and configuration for the PAUL framework on this project.

## Integrations

No external integrations active yet. Possible future additions:

- **SonarQube** — code quality metrics. Wait until v1.0 ships; small team doesn't need it before then.
- **CI test runner integration** — once `Tests/` has more than ~20 test files, integrate a runner that PAUL can call from `verify` steps.
- **Plugin validator integration** — wire `pluginval` and `auval` into PAUL's `verify` semantics so that release-critical phases (F7) automatically validate.

## Conventions

- **Branching**: PAUL works on `main` directly during single-developer phases. Once a second contributor joins, switch to feature branches per plan (`feature/F4a-motion`).
- **Commits**: use `/caveman-commit` for every commit during `/paul:apply`. Conventional Commits format, subject ≤ 50 chars, why-over-what body.
- **Tags**: phase completion tags follow `phase-F<N>-complete`. Release tags follow semver (`v1.0.0-beta.1`, `v1.0.0`, etc.).

## Loop-specific overrides

- `/paul:plan` should default to `autonomous: false` for any plan that touches the audio thread. Manual approval required at each task verify step. (Override the PAUL default for safety on real-time-critical code.)
- `/paul:unify` is mandatory at every phase boundary. Sub-phases (F4a, F4b, F4c) also require unify before moving to the next sub-phase.

## State migration

If STATE.md format changes between PAUL versions, follow the migration path in PAUL's release notes. Do not edit STATE.md manually.
