# ESP32-C6 Demo Monorepo

This repository manages multiple ESP32-C6 demo projects in one Git repo.

## Repository Layout

- `esp32_c6_demo/ble_adv`: BLE advertising demo.
- `esp32_c6_demo/helloworld`: basic hello-world demo.
- `esp32_c6_demo/ws2812b`: LED strip demo.

## Git Strategy

- Use `main` as the stable baseline branch.
- Create short-lived feature branches from `main`, for example:
	- `feature/ble-scan-interval`
	- `feature/ws2812b-effects`
- Merge back to `main` after build verification.

Detailed branch and release rules are documented in `docs/git-workflow.md`.

## Build Notes

Each demo is an independent ESP-IDF project. Enter the demo directory and build with ESP-IDF extension commands.

Example:

1. Open `esp32_c6_demo/ble_adv` in VS Code.
2. Run ESP-IDF build command from the extension.

## Commit Convention

Prefix commit messages by demo/module to make history easier to scan:

- `ble_adv: optimize advertising interval`
- `ws2812b: add rainbow animation preset`
- `repo: update gitignore and workflow docs`
