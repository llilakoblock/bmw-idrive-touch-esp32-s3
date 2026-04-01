# Contributing

Thanks for your interest in the BMW iDrive Touch ESP32-S3 project!

## Reporting Issues

If you have a different iDrive hardware revision or encounter problems, please [open an issue](https://github.com/llilakoblock/bmw-idrive-touch-esp32-s3/issues/new/choose) using the Bug Report template. The most helpful info you can provide:

1. **iDrive part number** (full number from the label, including any suffix like `-03`)
2. **Serial log with CAN debug enabled** — set `kDebugCan = true` in `include/config/config.h` and capture the output while pressing buttons / turning the rotary / touching the touchpad
3. **What works and what doesn't** (touchpad? buttons? rotary? init sequence?)

## Development Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- ESP32-S3 board (DevKitC-1 recommended)
- CAN transceiver (SN65HVD230 or similar)
- iDrive controller for testing

### Build & Flash

```bash
# Build
pio run

# Flash
pio run -t upload

# Monitor serial output
pio device monitor
```

## Making Changes

1. Fork the repository
2. Create a feature branch from `main`: `git checkout -b feature/your-change`
3. Make your changes
4. Test on real hardware (this project can't be meaningfully tested without the actual iDrive controller)
5. Ensure it compiles cleanly: `pio run`
6. Push and open a Pull Request

### Code Style

The project uses `clang-format`. Format your code before committing:

```bash
# Format all source files
find src include -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

### Key Files

| File | Purpose |
|------|---------|
| `include/config/config.h` | CAN IDs, protocol constants, timing, debug flags |
| `src/idrive/idrive_controller.cpp` | Main controller logic, CAN message handling |
| `src/input/touchpad_handler.cpp` | Touchpad input processing and gestures |
| `src/input/button_handler.cpp` | Button-to-HID key mapping |
| `docs/BMW_iDrive_CAN_Protocol_Research.md` | Protocol documentation |

### CAN Protocol Notes

Different iDrive hardware revisions may use different CAN message IDs. The current code targets **G-series ZBE4** (part number `65826829079`). If you have a different revision:

- Enable `kDebugCan = true` to see all CAN traffic
- Document any new CAN IDs you discover
- Known IDs for our ZBE4: buttons `0x267`, rotary `0x264`, touchpad `0x0BF`, status `0x5E7`

## Questions?

Use [GitHub Discussions](https://github.com/llilakoblock/bmw-idrive-touch-esp32-s3/discussions) for questions about setup, hardware compatibility, or general discussion.
