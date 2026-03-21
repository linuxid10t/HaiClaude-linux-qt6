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
- Profile management (save/load API configurations) - only visible in API mode
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

### Profile Management

**Feature**: Save and load complete API mode configurations as named profiles.

**How it works**:
- Profiles are stored in QSettings under the `DavidMasson/HaiClaude` organization
- Each profile stores: URL, API key, "Remember key" preference, and all model override settings
- Profile names are validated (no empty names, no special characters that break INI keys)

**Profile keys**:
- `apiProfiles` - QStringList of profile names
- `apiProfile_<name>_url` - API endpoint URL
- `apiProfile_<name>_key` - API key
- `apiProfile_<name>_saveKey` - Whether to remember key
- `apiProfile_<name>_currentModelCheck/Model` - Current model override
- `apiProfile_<name>_opusModelCheck/Model` - Opus model override
- `apiProfile_<name>_sonnetModelCheck/Model` - Sonnet model override
- `apiProfile_<name>_haikuModelCheck/Model` - Haiku model override
- `activeProfile` - Currently active profile name

**UI Components**:
- `fProfileComboBox` - Dropdown to select saved profiles
- `fProfileListWidget` - List showing all saved profiles
- `fSaveProfileButton` - Save current settings as new profile
- `fDeleteProfileButton` - Delete selected profile
- `fProfileNameEdit` - Input field for naming new profiles

**User Flow**:
1. Enter API settings in the UI
2. Type a profile name in the profile name input field
3. Click "Save Profile" to save current settings
4. Profile appears in both the combo box and list widget
5. Select a profile from the combo box to load its settings
6. Select a profile from the list widget and click "Delete" to remove it
7. Active profile is saved and restored on app restart

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
