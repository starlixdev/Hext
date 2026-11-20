# Interaction design

The compact island and expanded panel share one physical surface. Size, corner radius, content opacity, and related controls move together. Target changes retain spring velocity, including when the pointer reverses an expansion or a toggle is clicked again before settling.

The sidebar uses a shared selection surface. Holding and dragging an item moves its neighbors into place; release saves the order, while Esc or capture loss restores it. Ctrl+Up/Down reorders a focused item. Recent accent colors keep the newest selection first, and the color wheel supports preview, Apply, and Cancel.

Toggles use a consistent 36 by 20 logical-pixel size. Labels sit beside them instead of at the opposite edge of a wide panel. Menus use equal outer padding. Action labels and custom glyphs are aligned using their visible DirectWrite bounds.

Permission cards align the visible left edges of the title and description and center both lines as a single group. The selection check is centered vertically with matching side padding, and all card content follows the press motion. The explanatory footer uses smaller type aligned with the card text.

Assistant requests are native cards in the chat scroll flow, alongside messages. They preserve the header, composer and earlier context rather than replacing the conversation. Titles, explanations, action rows and completed-state labels share the existing spacing/material/control styles. Arrival uses a short damped spring with scale/press behavior from existing buttons; reduced motion snaps it. Details expand inside the same scroll flow, and clipped or historical controls cannot receive input.

The system uses installed Segoe fonts and Fluent/MDL2 glyphs. High contrast, disabled transparency, and reduced motion have explicit alternatives. Material is a wallpaper approximation, not a live macOS vibrancy implementation.

References: [Apple motion guidance](https://developer.apple.com/design/human-interface-guidelines/motion), [Animate with springs](https://developer.apple.com/videos/play/wwdc2023/10158/), [SwiftUI animation](https://developer.apple.com/videos/play/wwdc2023/10156/), [Dynamic Live Activities](https://developer.apple.com/videos/play/wwdc2023/10194/), and [Materials](https://developer.apple.com/design/human-interface-guidelines/materials). These inform behavior; the implementation uses Windows APIs.

Assisted setup uses the existing composer for explanations and public values. Compact native components offer useful actions, while provider guidance remains ordinary conversation text. Restoring a pending component updates its earlier position so that it does not replace the latest answer with a new form. No additional configuration overlay or independent animation system is introduced.
