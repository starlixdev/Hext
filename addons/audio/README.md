# Hext Audio

This add-on reuses the unmodified EqualizerAPO-XT 2.64.0 x64 SSE2 installer. Its processing engine, installation, and device registration are upstream components. Hext adds a download and configuration interface.

`upstream.json` records the original GitHub asset URLs, commit, version, exact installer size, and SHA-256 hashes. `scripts/prepare-audio-addon.ps1` verifies those files before preparing a release. The installer is copied byte-for-byte to `Hext-Audio-Setup.exe`; its internal product name and copyright are retained.

The Hext Audio release includes the matching upstream source ZIP and the original GPL version 2 and version 3 texts. These files must remain available together. See [third-party notices](../../THIRD_PARTY_NOTICES.md) and [audio usage](../../docs/audio.md).
