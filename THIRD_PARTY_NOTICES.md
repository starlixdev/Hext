# Third-party notices

Hext 0.7.3 and later do not redistribute an audio engine or installer.

## JSON for Modern C++ 3.12.0

[nlohmann/json](https://github.com/nlohmann/json/releases/tag/v3.12.0), copyright 2013–2025 Niels Lohmann, is used under the MIT license. Source: `https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp`, SHA-256 `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## SQLite 3.54.1

[SQLite](https://www.sqlite.org/copyright.html) is in the public domain. Source: `https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip`, SHA-256 `0a9c8e34352dad46006aa82836d5383bff9dc6bc9ce5a60553c05aa30e6ad1f9`. The build script fetches and checks this corresponding source. SQLite is linked statically with extension loading disabled. Hext uses Windows DPAPI to encrypt individual payloads, without a proprietary SQLite extension.

## Historical EqualizerAPO-XT redistribution

Hext releases 0.7.0 through 0.7.2 offered the unmodified x64 SSE2 installer from [115dkk/EqualizerAPO-XT](https://github.com/115dkk/EqualizerAPO-XT), version 2.64.0, commit `e00710497d7a15220f0f94cd944866c2e208eeb4`. Equalizer APO was created by Jonas Thedering. EqualizerAPO-XT includes contributions by 115dkk and its upstream contributors.

The upstream code is GPL-2.0-or-later; its installer includes an ASIO component distributed under GPL version 3. Corresponding source, original licenses, and redistribution metadata remain in the [historical audio release](https://github.com/starlixdev/Hext/releases/tag/audio-addon-2.64.0). This notice preserves attribution for earlier distributions; the current application contains no audio add-on integration.
