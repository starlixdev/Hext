# Contributing

Build and run the tests in [Development](docs/development.md) before submitting changes. Describe the behavior that changed and how it was verified. Include native renderer exports when a layout or animation changes.

Keep changes focused. Preserve Windows 10 and 11 compatibility, reduced motion, keyboard access, and existing settings. Do not add a continuously running service or install an audio component on application startup.

Use English for code comments, documentation, issues, and pull requests. Keep existing Brazilian Portuguese interface labels consistent. Store credentials and machine-specific settings outside the repository.

Third-party code must retain its copyright, license, and source provenance. Update `THIRD_PARTY_NOTICES.md` when adding a dependency. An add-on version update must update the pinned hashes, source archive, release metadata, and tests together.

By submitting a contribution, you agree to license it under GPL-3.0-or-later. Report security issues using [SECURITY.md](SECURITY.md).
