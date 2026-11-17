# Assistant setup

Select Codex, Claude, or another CLI under **Personalizar → Assistant**. Click **Conectar**. Hext checks the executable, version, required command options, and authentication before reporting a connected state. If authentication is missing, **Entrar** opens the selected provider's login flow. Custom executables can be selected with the file picker.

Codex and Claude use their installed CLI sessions. This does not connect to an existing desktop chat or copy its history. Hext does not include a model, API key, subscription, or billing credit. Provider terms and authentication still apply.

Messages are sent to the selected executable through standard input. Arguments are quoted separately, response output is collected asynchronously, and cancellation terminates the process job. The send button changes to a stop button during a request. Conversation history is held in memory; file shortcuts are not uploaded automatically.

The generic adapter requires a CLI that accepts text on standard input and returns plain text on standard output. Compatibility with arbitrary assistants is not guaranteed.

The message field supports a visible caret, mouse positioning, selection, paste, arrow navigation, Home/End, Delete, and horizontal scrolling. UTF-16 surrogate boundaries are preserved.

`--verify-connection` checks the configured provider without requesting an AI response. It exports boolean verification results, not raw login output. Do not publish session files or personal configuration with bug reports.
