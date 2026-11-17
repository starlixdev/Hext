# Hext

A compact desktop island for Windows 10 and 11. Hover to expand it into a panel with an assistant, audio controls, timers, hardware readings, and file shortcuts.

Hext is written in C++17 with Win32, Direct2D, and DirectWrite. It does not require Windows activation. The interface is currently in Brazilian Portuguese; this repository's documentation is in English.

![Hext settings](docs/images/settings.png)

## Download and run

Download **Hext-Windows-x64.zip** from [Releases](https://github.com/starlixdev/Hext/releases), extract it into a writable folder, and run `Hext.exe`. The executable is not code-signed. Keep Windows' normal download and application checks enabled.

Hover over the compact island at the top of the desktop to expand it. Move the pointer away to collapse it. A window that leaves the island's footprint exposed does not hide it. Windows that cover that area suppress normal desktop presence.

| Shortcut | Action |
| --- | --- |
| Ctrl + Alt + H | Show or completely hide the island |
| Ctrl + Alt + Space | Expand or collapse, including above other applications |
| Esc | Close the current menu or collapse the panel |

Change both global shortcuts in **Personalizar → Geral**. Hext checks whether Windows can register a new combination before releasing the old one. Global reservations are detectable; every application's local shortcuts are not. Hext must be running for these shortcuts to work. Use the tray icon to resume desktop activation or quit.

## What is implemented

- Assistant adapters for Codex CLI, Claude Code, and a configurable text CLI. Connection checks verify installation, required options, and authentication. Hext does not include a model or a paid assistant subscription.
- Output and input device selection, volume, mute, stereo balance, explicit default-device switching, and Bluetooth management through Windows.
- Optional audio amplification and equalization through the downloadable Hext Audio add-on.
- Timers, local commands, CPU and GPU readings, RAM usage, and temporary file shortcuts.
- A reorderable sidebar, recent accent colors, a color wheel, material and scale settings, and reduced motion.

Voice recognition and notification access are not implemented. The voice animation is a labeled preview. Hardware temperatures and power depend on available sensors; missing values remain unavailable.

## Audio add-on

Open **Som → Efeitos** and choose **Baixar add-on**. Hext downloads the pinned installer from this repository's GitHub Release, verifies its size and SHA-256, and then presents **Instalar add-on**. Installation is a separate action. Select an output through **Configurar saída** and follow the audio component's setup instructions.

Hext Audio reuses the unmodified [EqualizerAPO-XT](https://github.com/115dkk/EqualizerAPO-XT) SSE2 installer. The add-on release includes the corresponding upstream source archive and licenses. Hext supplies device-scoped controls and the download flow; it does not replace the upstream audio engine. See [Audio](docs/audio.md) for limitations and removal instructions.

## Build and test

Install MinGW-w64 with `g++` and `windres`, then run in PowerShell:

```powershell
./app/build.ps1 -Preview
./scripts/test.ps1
./app/dist/Hext-preview.exe --verify-render
```

The hidden renderer exports images and checks interaction states without recording the desktop. Build prerequisites, test boundaries, and release steps are in [Development](docs/development.md).

## Repository layout

```text
app/                 Windows application, resources, and tests
addons/audio/        Pinned upstream metadata and redistribution licenses
docs/                Usage, architecture, design, and development notes
scripts/             Tests, packaging, and add-on release preparation
.github/             CI, release workflow, and contribution templates
```

Local settings, assistant sessions, downloads, previous executables, and generated test images are excluded from version control. Release ZIPs contain a clean executable and documentation, not a developer configuration.

Read [Assistant setup](docs/assistant.md), [Hardware readings](docs/hardware.md), [Architecture](docs/architecture.md), and [Contributing](CONTRIBUTING.md) for details.

## License and credits

Hext is distributed under [GPL-3.0-or-later](LICENSE). EqualizerAPO-XT retains its upstream copyright and license terms; see [Third-party notices](THIRD_PARTY_NOTICES.md). Apple motion guidance and Kobi informed the interaction design. Hext uses Windows fonts and icons, and is not affiliated with Apple or Kobi.
