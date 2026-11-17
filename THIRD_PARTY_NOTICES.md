# Third-party notices

## EqualizerAPO-XT

Hext Audio redistributes the unmodified x64 SSE2 installer from [115dkk/EqualizerAPO-XT](https://github.com/115dkk/EqualizerAPO-XT), version 2.64.0, commit `e00710497d7a15220f0f94cd944866c2e208eeb4`.

Equalizer APO was created by Jonas Thedering. EqualizerAPO-XT includes contributions by 115dkk and the contributors credited upstream. This distribution does not claim authorship of their engine or installer. Both remain separate from Hext.exe.

Upstream's own code is GPL-2.0-or-later. Its installer includes an ASIO component distributed under GPL version 3. The upstream installer and matching source archive are mirrored together in the Hext Audio release, alongside the original GPL texts. Other dependencies retain the notices shipped by upstream. Exact asset URLs and hashes are in [upstream.json](addons/audio/upstream.json).

No third-party APO source has been relabeled as original Hext code. Reusing the published engine avoids maintaining a second implementation of audio filtering, registration, and device configuration.

## Windows interfaces and design references

Hext uses Windows system APIs and fonts installed with Windows. Segoe fonts and Fluent/MDL2 glyphs are not bundled. MinGW-w64 provides the build-time Windows API declarations and runtime support under its own licenses.

The isolated IPolicyConfig interface layout was checked against [EarTrumpet](https://github.com/File-New-Project/EarTrumpet/blob/master/EarTrumpet/Interop/MMDeviceAPI/IPolicyConfig.cs). Hext does not bundle EarTrumpet.

The interface draws on Apple's published motion guidance and the Kobi desktop-island concept. No Apple framework, SF Symbols asset, Apple font, Kobi executable, or reference video is included.
