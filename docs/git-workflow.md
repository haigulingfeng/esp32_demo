# Git Workflow for ESP32-C6 Demos

## 1) Branch Model

- `main`: stable branch, always expected to compile for active demos.
- `feature/*`: short-lived branches for development tasks.
- Optional `release/*`: used only when you need a frozen release window.

Recommended naming:

- `feature/ble-adv-fix-conn-state`
- `feature/ws2812b-add-patterns`
- `release/2026-05`

## 2) Daily Flow

1. Sync baseline:
   - `git checkout main`
   - `git pull`
2. Create branch:
   - `git checkout -b feature/<topic>`
3. Develop and build only impacted demos.
4. Commit with clear module prefix.
5. Merge into `main` after review and build verification.

## 3) Commit Message Rules

Use format:

`<scope>: <summary>`

Scope examples:

- `ble_adv`
- `helloworld`
- `ws2812b`
- `repo`
- `docs`

## 4) Tags and Releases

- Use repo-wide tags for synchronized milestones:
  - `c6-demos-v1.0.0`
- Use demo-specific tags if needed:
  - `ble-adv-v1.2.0`

## 5) ESP-IDF File Tracking Policy

Tracked:

- Source code (`main/`, component code, CMake files)
- Documentation
- `sdkconfig.defaults` (if used)

Ignored:

- `build/`
- `sdkconfig` and temporary generated files
- local editor settings and logs

## 6) Practical Guardrails

- Keep feature branches short (usually less than 3 days).
- Avoid mixing unrelated demos in one commit.
- Before merge, verify each changed demo can build.
