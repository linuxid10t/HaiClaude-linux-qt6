# HaiClaude (Linux/Qt6)

Native Qt6 GUI launcher for [Claude Code](https://docs.anthropic.com/en/docs/claude-code) (the Anthropic CLI).

## Features

- **Cloud mode** — launches Claude Code using OAuth or your existing `ANTHROPIC_API_KEY` environment variable.
  - **Model selection** — choose between Opus, Sonnet, or Haiku; selector appears under Cloud and hides when API mode is active
- **API mode** — launches Claude Code with a custom API configuration:
  - **API URL** — defaults to Anthropic's API, but can be changed for proxies or alternative endpoints
  - **API Key** — your Anthropic API key (input is masked)
  - **Model overrides** — optionally override the default Opus, Sonnet, and Haiku model versions
  - **Current model override** — force a specific model via `--model`
- **Working directory** picker — persistent text field with a **Browse…** button; Claude Code starts in the chosen directory
- **Persistent settings** — all settings (mode, model, directory, API configuration) are saved across sessions; defaults to Cloud mode on first run
- **Security** — shell injection protection, opt-in API key storage, input validation, and safe credentials handling

## Requirements

- Linux
- Qt6
- CMake 3.16+
- C++17 compiler
- Claude Code CLI (`claude`)

## Build

```sh
cmake -B build && cmake --build build
```

## Install (Optional)

```sh
sudo cmake --install build
```

Installs to `/usr/local/bin/haiclaude` by default.

## Build AppImage

To create a portable AppImage:

```sh
./build-appimage.sh
```

This downloads linuxdeploy and creates `HaiClaude-x86_64.AppImage`.

## Claude Code Setup

Install Claude Code via npm:

```sh
npm install -g @anthropic-ai/claude-code
```

Or use your distribution's package manager if available.

## Usage

Run the launcher:

```sh
./build/haiclaude
```

Or if installed:

```sh
haiclaude
```

Select your preferred mode (Cloud or API), choose a model, pick a working directory, and click **Launch**.

## Security Notes

- **API key storage is opt-in** — the key is only saved to disk if you check "Remember key"
- **Shell injection protection** — all user input is properly escaped before use in shell commands
- **Input validation** — working directory must exist; API fields cannot be empty
- **Credentials backup** — in API mode, existing credentials are backed up and restored automatically

## License

MIT — see [LICENSE](LICENSE).
