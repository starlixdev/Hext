# Tasks, capabilities, and integrations

Hext keeps the user's original request while it plans, executes tools, waits for permission or configuration, and observes results. A task can span multiple provider turns. It does not require a separate prompt rule for each possible user request.

## Architecture

The previous implementation had separate hand-maintained action validation and provider schemas, a short execution queue, and a Spotify settings tab. The existing native executors, permission gate, DPAPI store, conversations, memory, and renderer remain in use. The new components generalize discovery and orchestration around them:

| Component | Responsibility |
| --- | --- |
| `capability-registry.h` | Capability metadata and one typed tool registry. Validation, provider response schemas, and model-visible tool descriptions use the same definitions. |
| `task-engine.h` | Original goal, owner conversation, planning/execution state, step limits, duplicate protection, and declarative skill validation/expansion. |
| `task-runtime.h` | Readiness inspection, preparation, exact pending-step resumption, scoped resources, native adapters, public configuration, and disconnect callbacks. |
| `native-resources.h` | Bounded installed desktop-app discovery and native authorized text-file operations. |
| `task-ui.h` | On-demand connection/setup inside a conversation and skill review, without a capabilities settings list. |
| `authorization-policy.h`, `authorization-runtime.h` | Native global-mode gate, separate AI review, one-use human/reviewer permits, sensitive-result sharing. |
| `personal-runtime.h` | Existing conversation/memory controller, native dispatch, provider turns, and host-authored receipts. |

The flow is **request → catalog/readiness → provider plan → typed validation → permission gate → preparation or approval → native execution → tool result → next plan**. Each turn can request up to four actions. Dependent actions should wait for preceding results; for example, read a file before constructing a new document from its contents. Native execution remains serial within a task.

Tasks have bounded planning: twenty-four provider turns and sixty-four dispatch attempts. Ordinary writes cannot silently repeat within one task. Read-only observations have a sixteen-call per-request budget; repeatable media controls have a separate bounded allowance. Permission denial, cancellation, unsupported tools, ambiguous failed writes, and unrecoverable errors stop further execution. Completed effects are not rolled back.

Codex structured responses are decoded from complete JSON stream events. A validated final agent message followed by `turn.completed` finishes the inference turn even if the CLI remains alive during cleanup. Hext then terminates only its owned process job. It does not wait indefinitely for CLI exit or write private answers into an output-message file. The normal timeout still applies when no complete response arrives.

## On-demand preparation

A missing registered capability suspends its exact pending request and preserves the remaining queue and original goal. The conversation displays the relevant setup card. Local checks and fields are prepared by Hext; selecting a file, authenticating, granting consent, or supplying a platform-required public identifier remains the user's responsibility.

After readiness is confirmed, Hext reinserts the pending step and evaluates current permissions again. Configuration does not grant execution permission. Old approval tickets are not reused. Canceling, changing conversations, revoking permissions, or disconnecting cancels pending work and prevents late worker results from being attached to another conversation.

Provider setup can also begin from a message: the request remains while the user selects/connects Codex or Claude. A configured provider resumes planning without another copy of the request. A registered desktop executable that changed after authorization requires fresh authorization in the native chooser, then resumes the original launch request.

Pending tasks now have encrypted checkpoints and explicit restart recovery. See [general agent persistence and boundaries](agent.md). External effects are never automatically replayed after restart.

## Permissions

Only three global modes are exposed in **Personalizar → Assistant → Permissões**: Sempre Perguntar, Aprovar por mim, and Permitir Sempre. There is no capabilities navigation or category/service/tool override. Old settings migrate to Sempre Perguntar, while memory, language, chats and connections remain. Internal capability discovery and typed tool registration remain authoritative for supported actions and readiness.

Ask requires exact user consent; Delegate obtains a separate restricted AI verdict for each concrete action and original goal; Always runs routine operations directly. Invalid/unavailable review asks the user, and a denial stops the task. Every adapter and expanded recipe action uses the same policy. Native sensitive/critical guards require user consent in all modes. Parameters, scopes, identity, approval revision and a one-use execution permit are checked in native code. Model prose, recipes, and metadata cannot grant authority. See [mode details and limits](assistant.md#actions-and-autonomy).

Windows media control verifies the actual source application locally. An identified Spotify player uses the same global mode plus native Spotify restrictions; the host creates a scoped request that the model cannot forge. Website identity inside a browser media session is not available from this Windows API. Voice recognition is not implemented; known Spotify voice-control requests remain blocked.

Permissions control Hext's tools, rather than the behavior of a separate application the user has chosen to launch.

## Available resources

- **Windows audio and Hext timers:** existing native volume, mute, default-output, device discovery, start, and pause operations remain available.
- **Browser:** searches open the user's default browser. Hext reports launch acceptance and does not claim to have read the resulting web page.
- **Applications:** a worker scans App Paths registrations and local Start menu links, without resolving network targets or executing shortcut arguments. It excludes command interpreters and obvious installers/uninstallers. Candidate IDs are revalidated at dispatch; ordinary launches follow the global mode. Discovery is bounded and does not cover every packaged Store application or launcher with arguments.
- **Files:** a native chooser authorizes a file/folder. Folder listing returns at most forty supported items with scoped IDs and marks partial results. Revoking the parent invalidates derived IDs. Text reads require an authorized UTF-8 document of at most 32 KB and return a bounded 12 KB excerpt with a truncation flag. New `.txt`/`.md` files are limited to 16 KB, cannot escape the authorized folder, and never overwrite an existing file. Hext flushes, commits, and reads the new file back before confirming its contents. Scripts, arbitrary paths, deletion, and administration are not exposed.
- **Windows media / Spotify:** basic Spotify playback can use an identified native Spotify session without a Web API connection. Named-track search and API-specific operations use official Spotify OAuth/PKCE. If a native session is missing, the conversation can open Spotify and check for it, or guide official API setup while retaining the request. This does not install Spotify. Platform-required Client ID, account consent, Premium/API access, and active-device requirements cannot be invented or bypassed. Spotify catalog results stay local and never enter provider context or preference memory.
- **Saved history and memory:** existing bounded history search, reviewed shared preferences, language, and provider selection remain intact. Task planning retrieves preferences against the original goal rather than external tool text.

Unknown resources are reported honestly. General research, isolated scripts, authorized UI controls and reviewed WinGet proposals can prepare a solution without a dedicated service adapter. Hext does not fabricate a connection or execute an arbitrary download. Additional specialized APIs still require reviewed adapters; see [general tools and limits](agent.md).

## Skills without executable plugins

Skills are versioned JSON recipes made of existing tools and typed input references. They cannot contain shell commands, recursive skill calls, downloads, unknown tools, or code. Ask the assistant to import a recipe; `skills.import` opens the native chooser and step review. Explicitly add it, or cancel the task. Every expanded action still passes its own validation and global permission gate.

The built-in focus recipe combines a timer and Windows volume. [Example recipe](examples/focus.skill.json) illustrates the format. Importing a recipe does not run it or authorize its actions. The library is limited to twenty entries including the built-in recipe. Custom recipes are encrypted in the private store and removed by Assistant data deletion. A mode change or cancellation invalidates pending approvals and stops current execution.

## Native adapter extension point

`personalRegisterAdapter` accepts a new capability definition, typed tool definitions, and a `NativeCapabilityAdapter`. This is an API for reviewed, compiled native modules, not a dynamic DLL or executable loader. It is unavailable to model output and skill JSON. Register modules during initialization, before tasks or provider requests are active.

The adapter supplies:

- `inspect`: cached readiness/status information, without tokens, private metadata, network requests, or system changes.
- `execute`: the real implementation for registered tools, returning a host receipt and using observed results. The gate has already validated permissions and typed inputs.
- `prepare`: user-initiated official configuration/authentication, with asynchronous workers as needed.
- Optional `publicConfiguration` and `configure`: a public identifier edited through the same native inline editor; these fields are not for passwords or bearer tokens.
- Optional `disconnect`: cancel/revoke local access, remove Hext-owned credentials and caches, and expose the platform's official grant-revocation route where needed.

Use owned service objects with a lifetime covering their callbacks. Credentials belong in the DPAPI-protected store, not INI files, prompts, status messages, or recipes. Native schema validation supports bounded strings, integers, booleans, enums, and nested objects. New schema types need validation support before use. New adapters enter the internal registry and on-demand setup without a new settings tab or a rewrite of the planner. `connections.disconnect` invokes supported native cleanup; provider disconnect remains available directly in Provedor.

The [MCP tools specification](https://modelcontextprotocol.io/specification/2025-11-25/server/tools) informed the catalog/schema separation. Hext does not currently load arbitrary MCP servers. The provider's own MCP/apps/shell integrations remain disabled; future transports must pass through the host's permission and credential boundaries.

See [Assistant setup](assistant.md), [development checks](development.md), and [0.11.0 verification](verification-0.11.0.md).
