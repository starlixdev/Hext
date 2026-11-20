# Personal assistant

Open **Personalizar → Assistant**. Four tabs manage the provider, permissions, memory, and local data. Capability/tool lists are internal; needed configuration appears inside the conversation. Responses support automatic language selection, Brazilian Portuguese, and English. This changes assistant responses; the application interface remains Portuguese. See [Tasks and capabilities](tasks.md) for planning, on-demand preparation, and extension points.

## Connect a provider

Install and sign in to Codex CLI or Claude Code, select it in **Provedor**, then click **Conectar**. Hext checks the executable, supported options, and authentication. Login opens the provider's own flow. Hext does not include a model, subscription, API credit, or a connection to an existing desktop chat.

Codex also has to pass a protected Windows sandbox probe. Hext requests a dedicated read-only work directory, denies other filesystem access except runtime essentials, and disables command networking, shell, browsing, apps, plugins, hooks, and MCP integrations. The CLI's model/authentication transport remains necessary. Some editing tools can remain exposed by Codex; filesystem restrictions, rather than the prompt alone, block their writes. Unsupported sandbox configurations fail closed. See [Codex Windows sandbox](https://learn.chatgpt.com/docs/windows/windows-sandbox.md). Windows 10 may require the official elevated sandbox setup; Hext does not install it or weaken restrictions automatically.

Claude uses empty tools, strict empty MCP configuration, plan permissions, restricted mode, safe mode, disabled local hooks and auto memory, and no CLI session persistence. It requires a version advertising these options. Administrator-managed provider policy remains outside Hext's control. The installed CLI and its administrator must be trusted. Claude was not installed on the validation computer, so account-backed Claude execution remains unverified. See [Claude CLI reference](https://code.claude.com/docs/en/cli-reference).

The older generic CLI selector remains for configuration compatibility, but arbitrary executables cannot send messages in the protected action protocol. No plain-text or unrestricted fallback is used. Disconnecting a provider in Hext does not sign out its global CLI session.

## Actions and autonomy

The model returns a typed JSON envelope generated from Hext's tool registry. Hext rejects unknown actions, extra fields, invalid parameters, and incompatible responses. It validates the complete action batch before executing any action. The host issues execution receipts. The model uses those results to plan dependent steps and stops when the task is answered, blocked, or completed. Missing registered resources can be configured directly in the conversation without repeating the original request.

| Mode | Behavior |
| --- | --- |
| Sempre Perguntar | Ask before computer actions. Each approved request is exact and one-use. |
| Aprovar por mim | A separate restricted AI review evaluates the original goal and exact native action. Safe, appropriate actions can execute automatically; meaningful uncertainty asks you. A refusal stops the task. |
| Permitir Sempre | Routine actions execute automatically within existing native scopes and boundaries. |

There are no category, service, or tool overrides. Existing settings migrate conservatively to **Sempre Perguntar**, preserving chats, memory, language, and connections. AI review uses the selected authenticated Codex/Claude adapter, consumes provider usage, and can add latency. Missing authentication, timeout, malformed review, or failed review falls back to your approval; it never silently allows an action. Cached internal metadata inspection has no computer effect and needs no dialog.

Native protections remain in every mode: installations, historical-chat disclosure, recognized sensitive content, known destructive scripts, critical/financial UI, unknown UI invocations, and volume above 80% require explicit consent. Unsupported deletion/administration tools remain unsupported. Ordinary volume, device switching, timers, bounded text operations, and scoped benign UI follow the global mode. Authentication, file selection, Windows UAC, service consent and scope checks cannot be bypassed. Privacy classification is conservative and incomplete; the modes are not a guarantee that every downstream consequence can be inferred.

Approvals expire after two minutes and bind the exact request, conversation, and permission revision. Human and AI tickets are distinct and consumed once at native dispatch; the model cannot mint either. Changing modes, switching conversations, disconnecting, and canceling invalidate pending work. Failure stops the remaining action batch. Cancellation cannot undo an external operation already accepted by Windows or Spotify.

Supported actions include native Windows volume/mute/default output, audio device discovery, Hext timers, default-browser searches, opening scoped applications/documents/folders, Windows media controls, Spotify controls, and explicitly approved saved-history search. Manual controls clicked in the application remain available independently of the assistant's selected mode.

Tasks use native choosers for personal files/folders and can discover installed desktop applications as opaque IDs. Routine launches follow the global mode; obvious installers and critical system tools require consent. Canonical paths and modification timestamps are rechecked. Scripts, remote paths, and unlisted document extensions cannot be registered as files/apps. A launched application has its own capabilities; Hext does not sandbox that application. Scoped text reads, bounded folder listing, and new non-overwriting `.txt`/`.md` creation follow the global mode. General PowerShell remains AppContainer-isolated, and WinGet installations require a native reviewed proposal and user consent. See [agent boundaries](agent.md).

Recognized sensitive tool output is held locally before entering provider context or a durable task result. A separate sharing approval releases that exact result without repeating the original operation. Declining discards it. This marker-based guard cannot recognize all private information.

Browser searches open the default browser through Windows. A launch receipt confirms Windows accepted the request; it does not claim that the web page loaded or that Hext inspected its contents. Windows media controls use the current GSMTC player session; availability and supported commands depend on that player.

## Requests inside the conversation

Authorization, window access, connection/setup, user participation, recipe review and proposed memory appear as native interactive messages in the chat. The conversation and composer remain available. Permissions use **Permitir**, **Negar**, and **Cancelar**; needed configuration offers the relevant official login, chooser, public configuration field or readiness check. Public fields use the existing chat composer, with Save/Back controls. Passwords stay in the external service's own login flow.

These components call the existing gate, executors and task controller. Model-generated wording cannot create an interactive request or authorize an action. Each native button binds its exact component, conversation, task, permission revision and current approval nonce. Resolved or stale controls cannot approve another step. Decisions update the component and remain in the conversation; denial stops queued execution and offers manual guidance or an adapted request. A new message while a task is waiting is retained without replacing its original goal or starting concurrent execution.

Detailed authorization presentation remains local. Provider context includes only a bounded component kind/state/title summary, rather than its native body or expanded details. Actual tool receipts and explicitly authorized results still follow the existing disclosure rules; an unapproved selection path is not transmitted merely because a request appeared in the chat.

After successful configuration Hext checks actual readiness and resumes the exact pending step, including the current permission mode. Authentication failures remain visible; opening a login window alone is not successful authentication. Spotify choices stay local and are not included in model context or persistent track history. OS-required file choosers, official browser login and Windows authorization remain external when required.

Scrolling, opening/closing history management, navigating to another page and collapsing the island preserve a live request. Switching conversations pauses/checkpoints active work and invalidates its old authority. Pending tasks also retain bounded encrypted chat context when the conversation has not been explicitly saved. On restart, previous components are historical/inactive and the conversation offers **Retomar tarefa**. Resuming preserves the original goal but rechecks current state and obtains new approvals; no stored button or ticket automatically executes after restart. Completed external effects are never rolled back or blindly replayed. Expired approvals similarly offer a fresh checked resumption.

## Conversations and memory

The history button opens saved conversations. **+** offers to save or discard the current conversation. Saved conversations autosave further messages, can be reopened, renamed, or deleted, and restore after restarting Hext. Ordinary unsaved drafts are not retained across process exit; pending tasks retain bounded checkpoint context for explicit resumption. Limits are 200 saved conversations and 500 messages per conversation. The provider receives at most 30 recent whole messages within a 24 KB text budget, rather than the entire database.

Memory is a separate collection of at most 200 short preferences. The assistant proposes useful entries, and you approve each proposal. You can also add, edit, delete, or disable memory. Repeated normalized keys update one entry. Inferences cannot silently replace explicit preferences; a reviewed replacement shows the previous value. Manually entered preferences are pinned. Unpinned inferred entries expire after 90 days. Context selection is bounded lexical selection, at most 12 entries / 4 KB; it is not semantic search or a complete account of the user.

Deleting or discarding a conversation removes its unpinned derived preferences. Manually pinned entries remain until explicitly deleted. Disabling memory omits it from subsequent provider context without deleting it. Common credential and sensitive-ID markers are rejected, but this filter is not a general sensitive-data detector. Review what you save.

Searching previous saved chats always requires a concrete sharing approval. It returns at most five matching excerpts within 4 KB and excludes the current conversation. Other histories are not automatically sent with a new chat.

## Spotify

Spotify setup appears on demand in a task, with local disconnect controls. Basic controls can use an identified Spotify Windows media session without API credentials. Catalog search and Web API playback are optional: create your own application in the [Spotify developer dashboard](https://developer.spotify.com/dashboard), register **http://127.0.0.1/callback**, and enter its public **Client ID** in Hext. No client secret is needed. The loopback callback uses a dynamically assigned port, random state, and PKCE S256. Hext requests `user-read-playback-state` and `user-modify-playback-state` only. Login and consent occur in your browser. The original pending task resumes after readiness and fresh permission checks.

Open Spotify on a device, connect in Hext, then search, choose a track, select a device, and use playback controls. Search returns up to five tracks. Device enumeration, transfer, pause/resume, previous/next, and volume requests use the official Web API. State is checked after play/pause/volume when available. A command-accepted result is distinguished from a confirmed state. Network failures, expired sessions, restricted devices, missing players, authorization failures, and rate limits are reported without a simulated success. Ambiguous failed writes are not retried automatically.

Web API playback requires Spotify Premium and the application's approved access. Development mode has account and user allowlist limits; a personal Client ID does not provide unrestricted public access. Windows-session control depends on the player's own support and does not perform an API search. See [quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes), [PKCE](https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow), and [redirect rules](https://developer.spotify.com/documentation/web-api/concepts/redirect_uri).

This is a text/manual control integration. Spotify voice control is blocked, and Spotify track metadata stays in the local Spotify panel rather than entering the model context, memory, or saved chat history. It includes Spotify attribution and links. Hext does not stream, amplify, equalize, record, or analyze Spotify audio. See [Spotify developer policy](https://developer.spotify.com/policy).

**Desconectar** deletes Hext's stored tokens and cached account/device/track state. **Gerenciar acesso** opens Spotify's account apps page to revoke the server-side grant; deleting a local token alone cannot revoke that grant.

## Privacy and removal

Saved chats, preferences, registered paths, audit records, and Spotify credentials are stored in `%LOCALAPPDATA%\Hext\assistant-data\assistant.db`. Each JSON payload is encrypted with Windows DPAPI for the current user, and the directory ACL is restricted to that user and SYSTEM. SQLite record kinds and opaque IDs remain visible; DPAPI does not protect against malicious software running as your user or an administrator. Moving the database to another account/machine may make it unreadable.

Messages and selected context are transmitted to your selected provider. CLI diagnostics, service retention, authentication, and administrator policy are governed by that provider; Hext cannot erase remote logs. The local audit retains up to 200 action names, outcomes, and timestamps, without raw arguments or tokens.

**Dados → Apagar dados do Assistant** removes local histories, memory, authorized items, connection data, and assistant preferences, and cancels pending requests. It preserves island appearance, audio, and keyboard settings and leaves global CLI sign-ins intact. SQLite secure deletion clears deleted record payloads, but this is not a guarantee of forensic removal from backups or storage hardware. Review any backups separately.

Ask the assistant to import a skill or disconnect a supported integration when needed; these use registered host tools and native review/configuration. Provider disconnect remains in Provedor. **Dados → Limpar ambientes** removes Hext's isolated script environments. See [0.11.0 validation](verification-0.11.0.md) for tested flows and remaining account-dependent verification.

## Conversational configuration

Pending configuration does not capture the composer as a form. Every message goes to the selected protected provider with the original goal, bounded history and safe native readiness metadata. The provider can investigate supported alternatives and official documentation through registered tools. Public configuration uses a typed native tool; its value must occur in the current user clarification and pass the adapter validator. Automatic guidance cannot invent a value. Recognized token, password and private-key patterns are removed before storage or provider context; this is not a universal secret detector. Use official sign-in pages for credentials.

Spotify basic controls can use an available Windows media session without Web API registration. Catalog access uses official desktop PKCE. This build has no bundled registered Spotify Client ID: if API access is needed, an eligible account and a registered app's public Client ID are necessary. Never enter a Client Secret in chat. The assistant receives official reference URLs and can consult current requirements: [PKCE](https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow), [applications](https://developer.spotify.com/documentation/web-api/concepts/apps), [quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes). Opening a page does not prove successful authentication.
