# HaiClaude (Linux/Qt6)

Native Qt6 GUI launcher for Claude Code (the Anthropic CLI).

## Project Overview

This is a desktop launcher application that provides a GUI for launching the Claude Code CLI with various configuration options. It supports both Cloud mode (OAuth or existing API key) and API mode (custom endpoint/key).

## Build

```sh
cmake -B build && cmake --build build
```

Executable outputs to `build/haiclaude`.

## Architecture

### Core Files

- `main.cpp` - Application entry point. Sets up `QApplication`, creates `LauncherWindow`, and handles deferred execution via `gPendingExec` (allows the Qt event loop to exit cleanly before exec'ing the terminal command)
- `LauncherWindow.h` - Main window class declaration
- `LauncherWindow.cpp` - All UI construction, settings persistence, command building, and launch logic
- `CMakeLists.txt` - Build configuration (Qt6 Widgets, C++17, MOC enabled)

### Key Components

**LauncherWindow** manages:
- Mode selection (Cloud vs API)
- Model selection (Opus/Sonnet/Haiku) - only visible in Cloud mode
- Working directory picker
- API settings (URL, key, model overrides) - only visible in API mode
- Settings persistence via `QSettings` (stored under `DavidMasson/HaiClaude`)
- Terminal detection and command execution

### Launch Flow

1. User configures settings and clicks Launch
2. `saveSettings()` persists current state
3. `buildCommand()` constructs the shell command with proper escaping
4. If running in a TTY: command stored in `gPendingExec`, window closes, Qt exits, then `main()` exec's the command
5. If not in TTY: `launchInTerminal()` finds a terminal emulator and runs the command via `QProcess::startDetached()`

### Security

- All user input is shell-escaped via `shellEscape()` (wraps in single quotes, escapes internal quotes)
- API key storage is opt-in via "Remember key" checkbox
- In API mode, existing credentials are backed up and restored via a trap on EXIT

## Conventions

- **Qt pattern**: Uses Qt's signal/slot mechanism with lambdas for UI connections
- **Layout**: `QVBoxLayout` root with nested `QHBoxLayout`/`QFormLayout` for rows
- **Settings**: All persisted via `QSettings` with string keys (e.g., `"mode"`, `"apiUrl"`, `"workDir"`)
- **Model IDs**: 0=Opus, 1=Sonnet, 2=Haiku (used in `QButtonGroup` IDs)
- **Terminal priority**: `$TERMINAL` env var → xterm → alacritty → kitty → gnome-terminal → konsole → xfce4-terminal → lxterminal → x-terminal-emulator

## Dependencies

- Qt6 (Widgets module)
- CMake 3.16+
- C++17 compiler
- Claude Code CLI (`claude`) - expected in `~/.npm-global/bin/claude`, `~/.local/bin/claude`, `/usr/local/bin/claude`, or PATH
