# Personal assistant

Open **Personalizar → Assistant**. The five tabs manage the provider, actions, memory, local data, and a general **Capacidades** resource list. Services do not require separate main tabs. Responses support automatic language selection, Brazilian Portuguese, and English. This changes assistant responses; the application interface remains Portuguese. See [Tasks and capabilities](tasks.md) for planning, on-demand preparation, and extension points.

## Connect a provider

Install and sign in to Codex CLI or Claude Code, select it in **Provedor**, then click **Conectar**. Hext checks the executable, supported options, and authentication. Login opens the provider's own flow. Hext does not include a model, subscription, API credit, or a connection to an existing desktop chat.

Codex also has to pass a protected Windows sandbox probe. Hext requests a dedicated read-only work directory, denies other filesystem access except runtime essentials, and disables command networking, shell, browsing, apps, plugins, hooks, and MCP integrations. The CLI's model/authentication transport remains necessary. Some editing tools can remain exposed by Codex; filesystem restrictions, rather than the prompt alone, block their writes. Unsupported sandbox configurations fail closed. See [Codex Windows sandbox](https://learn.chatgpt.com/docs/windows/windows-sandbox.md). Windows 10 may require the official elevated sandbox setup; Hext does not install it or weaken restrictions automatically.

Claude uses empty tools, strict empty MCP configuration, plan permissions, restricted mode, safe mode, disabled local hooks and auto memory, and no CLI session persistence. It requires a version advertising these options. Administrator-managed provider policy remains outside Hext's control. The installed CLI and its administrator must be trusted. Claude was not installed on the validation computer, so account-backed Claude execution remains unverified. See [Claude CLI reference](https://code.claude.com/docs/en/cli-reference).

The older generic CLI selector remains for configuration compatibility, but arbitrary executables cannot send messages in the protected action protocol. No plain-text or unrestricted fallback is used. Disconnecting a provider in Hext does not sign out its global CLI session.

## Actions and autonomy

The model returns a typed JSON envelope generated from Hext's tool registry. Hext rejects unknown actions, extra fields, invalid parameters, and incompatible responses. It validates the complete action batch before executing any action. The host issues execution receipts. The model uses those results to plan dependent steps and stops when the task is answered, blocked, or completed. Missing registered resources can be configured directly in the conversation without repeating the original request.

| Mode | Behavior |
| --- | --- |
| Conversar | All assistant-requested actions are blocked. |
| Pedir autorização | Each allowed category requires a concrete one-time approval. |
| Ações permitidas | Categories marked Permitir can execute directly; Perguntar still requires approval. |

**Negar** always blocks a category. Switching output devices and setting volume above 80% always requires confirmation. Approvals expire after two minutes and are bound to the exact request, conversation, and permission revision. Changing permissions, switching conversations, disconnecting, and canceling invalidate pending work. Failure stops the remaining action batch. Cancellation cannot undo an external operation already accepted by Windows or Spotify.

Supported actions include native Windows volume/mute/default output, audio device discovery, Hext timers, default-browser searches, opening authorized applications/documents/folders, Windows media controls, Spotify controls, and optional saved-history search. History access defaults to denied. Manual controls explicitly clicked in the application remain available independently of the assistant's grants.

Authorize individual executables, documents, or folders under **Dados → Itens autorizados**, or through a task's native chooser. Hext can discover installed desktop applications without authorizing their launch; a new target still requires explicit approval. It exposes opaque IDs and display names to the model, not arbitrary command lines. Executables must retain their registered modification timestamp and canonical path; changed programs require authorization again. Scripts, shortcuts, remote paths, and unlisted document extensions cannot be registered. This is a launch permission, not a sandbox for the application or document viewer you choose to trust. Text reads, bounded folder listing, and new non-overwriting `.txt`/`.md` creation are available with separate confirmation. Hext does not expose file deletion, installation, administration, or arbitrary shell actions.

Browser searches open the default browser through Windows. A launch receipt confirms Windows accepted the request; it does not claim that the web page loaded or that Hext inspected its contents. Windows media controls use the current GSMTC player session; availability and supported commands depend on that player.

## Conversations and memory

The history button opens saved conversations. **+** offers to save or discard the current conversation. Saved conversations autosave further messages, can be reopened, renamed, or deleted, and restore after restarting Hext. Unsaved drafts are not retained across process exit. Limits are 200 saved conversations and 500 messages per conversation. The provider receives at most 30 recent whole messages within a 24 KB text budget, rather than the entire database.

Memory is a separate collection of at most 200 short preferences. The assistant proposes useful entries, and you approve each proposal. You can also add, edit, delete, or disable memory. Repeated normalized keys update one entry. Inferences cannot silently replace explicit preferences; a reviewed replacement shows the previous value. Manually entered preferences are pinned. Unpinned inferred entries expire after 90 days. Context selection is bounded lexical selection, at most 12 entries / 4 KB; it is not semantic search or a complete account of the user.

Deleting or discarding a conversation removes its unpinned derived preferences. Manually pinned entries remain until explicitly deleted. Disabling memory omits it from subsequent provider context without deleting it. Common credential and sensitive-ID markers are rejected, but this filter is not a general sensitive-data detector. Review what you save.

Searching previous saved chats requires the separate **Conversas anteriores** permission. It returns at most five matching excerpts within 4 KB and excludes the current conversation. Other histories are not automatically sent with a new chat.

## Spotify

Spotify is a resource under **Capacidades**, with setup also available directly from a task. Basic controls can use an identified Spotify Windows media session without API credentials. Catalog search and Web API playback are optional: create your own application in the [Spotify developer dashboard](https://developer.spotify.com/dashboard), register **http://127.0.0.1/callback**, and enter its public **Client ID** in Hext. No client secret is needed. The loopback callback uses a dynamically assigned port, random state, and PKCE S256. Hext requests `user-read-playback-state` and `user-modify-playback-state` only. Login and consent occur in your browser. The original pending task resumes after readiness and fresh permission checks.

Open Spotify on a device, connect in Hext, then search, choose a track, select a device, and use playback controls. Search returns up to five tracks. Device enumeration, transfer, pause/resume, previous/next, and volume requests use the official Web API. State is checked after play/pause/volume when available. A command-accepted result is distinguished from a confirmed state. Network failures, expired sessions, restricted devices, missing players, authorization failures, and rate limits are reported without a simulated success. Ambiguous failed writes are not retried automatically.

Web API playback requires Spotify Premium and the application's approved access. Development mode has account and user allowlist limits; a personal Client ID does not provide unrestricted public access. Windows-session control depends on the player's own support and does not perform an API search. See [quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes), [PKCE](https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow), and [redirect rules](https://developer.spotify.com/documentation/web-api/concepts/redirect_uri).

This is a text/manual control integration. Spotify voice control is blocked, and Spotify track metadata stays in the local Spotify panel rather than entering the model context, memory, or saved chat history. It includes Spotify attribution and links. Hext does not stream, amplify, equalize, record, or analyze Spotify audio. See [Spotify developer policy](https://developer.spotify.com/policy).

**Desconectar** deletes Hext's stored tokens and cached account/device/track state. **Gerenciar acesso** opens Spotify's account apps page to revoke the server-side grant; deleting a local token alone cannot revoke that grant.

## Privacy and removal

Saved chats, preferences, registered paths, audit records, and Spotify credentials are stored in `%LOCALAPPDATA%\Hext\assistant-data\assistant.db`. Each JSON payload is encrypted with Windows DPAPI for the current user, and the directory ACL is restricted to that user and SYSTEM. SQLite record kinds and opaque IDs remain visible; DPAPI does not protect against malicious software running as your user or an administrator. Moving the database to another account/machine may make it unreadable.

Messages and selected context are transmitted to your selected provider. CLI diagnostics, service retention, authentication, and administrator policy are governed by that provider; Hext cannot erase remote logs. The local audit retains up to 200 action names, outcomes, and timestamps, without raw arguments or tokens.

**Dados → Apagar dados do Assistant** removes local histories, memory, authorized items, connection data, and assistant preferences, and cancels pending requests. It preserves island appearance, audio, and keyboard settings and leaves global CLI sign-ins intact. SQLite secure deletion clears deleted record payloads, but this is not a guarantee of forensic removal from backups or storage hardware. Review any backups separately.

See [0.9.0 validation](verification-0.9.0.md) for tested flows and remaining account-dependent verification.
