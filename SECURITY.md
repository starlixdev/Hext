# Security

For security reports, use GitHub's private vulnerability reporting when it is available on this repository. Otherwise, open an issue that asks for a private contact route without including credentials or exploit details.

Hext 0.7.3 and later do not download or install audio add-ons. Keep Windows' normal download and application checks enabled.

Assistant adapters use installed CLIs and their authentication. Never attach session files, API keys, login output, or a personal `hext.ini` to an issue. Redact personal paths from logs and screenshots.

Only the current release is maintained. The application is currently unsigned. Hext has not undergone an independent security audit.

The assistant validates a fixed action catalog and enforces permissions in native code. Prompts are not a security boundary. Codex requests restricted filesystem/network execution and checks a compatible Windows sandbox; incompatible installations fail closed. Claude requires restricted/safe mode and empty tools/MCP configuration. Provider executables and administrator-managed policy must be trusted; these controls do not protect against a compromised CLI or malware running as the user.

Private payloads use current-user DPAPI encryption and a restricted directory ACL under LocalAppData. Preference memory is separate from history. Both can be deleted in settings. Provider retention and global sign-ins remain provider-controlled. See [privacy and removal](docs/assistant.md#privacy-and-removal).

Spotify uses PKCE, loopback state validation, encrypted tokens, fixed HTTPS endpoints, and no client secret. Disconnect deletes local tokens and caches; server-side revocation uses Spotify's account page. Spotify metadata is not sent to the model, and Spotify voice control is refused.
