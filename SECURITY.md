# Security

For security reports, use GitHub's private vulnerability reporting when it is available on this repository. Otherwise, open an issue that asks for a private contact route without including credentials or exploit details.

The audio add-on downloader accepts one pinned HTTPS release asset and verifies its exact size and SHA-256. Installation requires a separate user action and preserves Windows' downloaded-file checks. Do not bypass SmartScreen or disable antivirus protection to install Hext or an add-on.

Assistant adapters use installed CLIs and their authentication. Never attach session files, API keys, login output, or a personal `hext.ini` to an issue. Redact personal paths from logs and screenshots.

Only the current release is maintained. The application and upstream audio installer are currently unsigned. Hext has not undergone an independent security audit.
