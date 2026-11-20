# Hext 0.12.1 verification

Validation completed on Windows 11 x64 on November 20, 2026. This patch completes the interactive-chat release described in [0.12.0 validation](verification-0.12.0.md).

Native authorization messages can display the user's selected absolute file path locally. Their body and expanded details are now excluded from automatic provider context; only bounded kind/state/title summaries enter it. Actual tool receipts and authorized results still use the existing execution and sensitive-disclosure rules. A native regression creates a real generated file, stages its read approval and verifies that the local request contains its unique path while provider context does not. The request is then canceled; the file is not read or shared.

The chat verifier also rejects reporting a legacy generic CLI as a successful protected assistant connection. Real provider selection remains Codex/Claude through their existing compatibility, authentication and protection checks.

The native chat, separate-process restart, global authorization, personal controller and renderer suites pass. The original feature work also passed actual protected Codex planning/native participation and independent AI review, as recorded in 0.12.0; protected-provider planning was rerun after the context change. The installed executable matches the verified preview and native shortcut-message checks pass.

One provider test stopped when its owned fixture window became unavailable; the native window guard refused further interaction. Repeating that test in isolation succeeded through all six actual calculation/UI/timer steps. No window validation or permission guard was bypassed to obtain that result.

External account/login limitations remain: successful fresh Spotify OAuth/playback and authenticated Claude execution were not established by these checks. Official login, OS choosers and mandatory platform authorization remain outside Hext when required. Historical components never restore a live ticket or automatically replay an external effect.
