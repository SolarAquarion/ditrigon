# Ditrigon Code & Feature Modernization Roadmap

Ordered by dependency chain and practical impact. Items higher on the list unblock or simplify items below them.

---

## Priority 1: Foundation & Scaffolding
*These changes are prerequisite to almost everything else. They enable modern C patterns, establish the correct application lifecycle, and remove the most dangerous deprecated dependency.*

- [ ] **7.1: C Standard Bump** — Change `meson.build` from `gnu99` to `gnu11` or `gnu17`. Enables `g_autoptr`, anonymous structs, `_Thread_local` everywhere else.
- [ ] **6.2c: Application Lifecycle** — Rewrite `int main()` in `hexchat.c` around `AdwApplication`. This is the single biggest architectural change: it unlocks single-instance, `GAction` routing, `GNotification`, Wayland init, and XDG Portal support. Almost every GUI and IPC modernization depends on this.
- [ ] **6.2a: D-Bus Migration** — Port all `dbus-glib` IPC to native `GDBus`. Removes the most critical deprecated dependency from the build.

## Priority 2: Memory Safety & Core Cleanup
*Incremental, low-risk changes that make every subsequent refactor safer and cleaner.*

- [ ] **2.2: Memory Management with `g_autoptr`/`g_autofree`** — Adopt GLib auto-cleanup macros across event handlers (`ignore.c`, `userlist.c`). Can be done file-by-file.
- [x] **2.1: Transition to `GString` for Dynamic Buffers** — ~~Target message building loops.~~ **Already partially adopted** in `url.c`, `text.c`, `outbound.c`, `proto-irc.c`, `util.c`, `modes.c`. Audit remaining files for stragglers.
- [ ] **7.2: Replace Custom Tree** — Delete `src/common/tree.c` and replace with GLib's `GTree`, `GSequence`, or `GPtrArray`. Standalone change, no external dependencies.

## Priority 3: Core Networking & Server Management
*The largest subsystem rewrite. Depends on Priority 1 (AdwApplication) for clean lifecycle integration. Unblocks all IRCv3 features.*

- [ ] **1.1: `GSocketClient` for Primary Connections** — Transition `server_connect()` from raw `socket()`/`connect()` to `GSocketClient`.
- [ ] **1.2: Modern SSL/TLS Handshake** — Replace direct OpenSSL/GnuTLS calls with `GTlsClientConnection`.
- [ ] **1.3: `GInputStream`/`GOutputStream` for Server I/O** — Replace `server_read` and `tcp_send_queue` with `GDataInputStream` and `GOutputStream`.

## Priority 4: Core API Modernization
*Independent of networking but benefits from the cleaner codebase established in Priority 2. Each item is self-contained.*

- [ ] **6.1a: Configuration Parsing** — Migrate `cfgfiles.c` custom text parser to `GKeyFile`. *(NOT `GSettings` — that would add a `dconf` runtime dependency and store config in a binary database, removing the ability to hand-edit config files. IRC power users expect editable text configs.)*
- [ ] **6.1b: URI Parsing** — Replace `url.c` regex structures with GLib 2.66's `GUri`.
- [ ] **6.1c: Date/Time Handling** — Replace `time_t`, `localtime_r`, custom `strftime` wrappers with `GDateTime`.
- [ ] **6.2b: Notifications** — Upgrade manual FreeDesktop DBus calls to `GNotification` / `g_application_send_notification`. *(Double dependency: requires both 6.2c AdwApplication AND 6.2a D-Bus migration, since the current code uses `dbus-glib` directly.)*
- [ ] **6.3: Password Management (OPTIONAL)** — Consider `libsecret` for credential storage. **However**, this adds a hard dependency on a desktop keyring daemon, breaking `fe-text` and headless usage. IRC passwords are low-sensitivity. May not be worth the complexity.

## Priority 5: GUI Polish & Wayland Parity
*Depends on AdwApplication (Priority 1) being in place. Each item is visually independent and can be tackled in any order.*

- [ ] **5.1: Dialog Migrations to `AdwWindow`** — Migrate `banlist-window.ui`, `chanlist-window.ui` from `GtkWindow` to `AdwWindow` + `AdwToolbarView` + `AdwHeaderBar`. Replace alerts with `AdwMessageDialog`/`AdwToastOverlay`. Standardize `AdwAboutWindow`.
- [ ] **5.2: Userlist Layout Modernization** — Migrate `GtkPaned` to `AdwFlap` or `AdwOverlaySplitView`.
- [ ] **5.3: Visual Polish (Avatars & Status Pages)** — `AdwAvatar` in `userlist-row.ui`; `AdwStatusPage` for connecting/empty states.
- [ ] **5.4: Unified Event Dispatcher** — Refactor `EMIT_SIGNAL` macros to use `GSignal` on a central `DitrigonApplication` object. **⚠️ 173+ call sites across the codebase** — this is one of the largest individual refactors on the list.
- [ ] **5.5: AppStream Metadata** — `io.github.Ditrigon.appdata.xml.in` already exists but uses stale screenshots (imgur links), outdated URLs, and old HexChat-era release notes. Needs a refresh with current screenshots and Ditrigon-specific content.
- [ ] **5.6: Link Unfurling & Inline Media** — `url_preview.c` with `libsoup3` already exists and parses OpenGraph metadata. However, **the build crashes with libsoup3 enabled**. Task is to debug the build failure, stabilize the libsoup3 integration, and wire the preview data into the GTK4 frontend with `GtkPicture`/`GtkMediaStream`.
- [ ] **5.7: Accessibility (a11y) Overhaul** — GTK4 overhauled accessibility (AT-SPI replaces ATK). Audit `xtext.c` and custom widgets to ensure screen readers (Orca) can parse chat history and announce notifications.

## Priority 6: Data Storage & IRCv3 Features
*SQLite logging is a prerequisite for CHATHISTORY. Networking (Priority 3) must be stable first.*

- [ ] **3.1: SQLite Database Logging** — Migrate from plain-text logging to SQLite3 per channel/server. This is a **hard prerequisite** for CHATHISTORY playback and dramatically accelerates `/search`.
- [ ] **7.3: Asynchronous File I/O** — Replace synchronous `hexchat_fopen_file` wrappers with GIO `GFile`/`GFileOutputStream`/`g_file_replace_contents_async`. *(Natural companion to the logging overhaul.)*
- [ ] **4.1: SASL Authentication Modernization** — AppStream notes SCRAM SASL is already a feature. Task is to move the existing SASL lifecycle to a `GTask`/GIO state machine for cleaner async flow, not to implement SASL from scratch.
- [ ] **4.2: Native Message Tags (`draft/message-tags`)** — Expand `proto-irc.c` to serialize/deserialize tags into `GHashTable`. *(Note: this is purely a parser change and could technically be done as early as Priority 4, but is grouped here with related IRCv3 features.)*
- [ ] **4.3: CHATHISTORY Support** — Implement IRCv3 `CHATHISTORY` for fetching missed context. *(Requires 3.1 SQLite logging.)*
- [ ] **4.4: `MONITOR` command** — Replace the legacy ISON-based notify polling (which fires on a timer every N seconds) with the IRCv3 `MONITOR` command. MONITOR is event-driven — the server pushes online/offline events instantly instead of the client constantly asking "is this user online?" Massive reduction in unnecessary traffic.
- [ ] **4.5: `batch` message support** — Required for proper CHATHISTORY playback and `NETSPLIT` grouping. Without batch support, history replay floods the chat buffer as individual unlinked messages.
- [ ] **4.6: STS (Strict Transport Security)** — IRCv3 mechanism where a server advertises "always use TLS" via CAP. The client must remember this and auto-upgrade future connections to that network. Prevents accidental plaintext connections.
- [ ] **4.7: `draft/multiline`** — Allows sending multi-line messages as a single logical unit instead of splitting into separate PRIVMSGs. Important for pasting code snippets.
- [ ] **4.8: `draft/read-marker`** — Syncs the "last read" position across multiple devices connected to the same bouncer. Essential for modern bouncer workflows (soju, ergo).
- [ ] **4.9: `draft/react`** — Emoji reactions to messages. Lightweight interaction without typing a full reply.

## Priority 7: Plugin Architecture
*Benefits from the GSignal refactor (Priority 5).*

- [ ] **3.2: GObject Introspection (GIR) for Plugins** — Expose Ditrigon core through GIR for auto-generated plugin bindings.

## Priority 8: Comprehensive Protocol Testing
*Tests should be written alongside/immediately after the features they validate. Listed here as the dedicated "catch-up" pass.*

### CTCP Edge Cases
- [ ] `ctcp_flood_protection_test` — Hundreds of simultaneous CTCP requests.
- [ ] `ctcp_malformed_payload_test` — Unclosed `\x01` bytes and massive payloads.

### SASL & Connection Hardening
- [ ] `sasl_state_machine_security_poc` — Out-of-order `AUTHENTICATE` commands.
- [ ] `tls_handshake_failure_poc` — Expired/self-signed certificates.

### IRCv3 Parser Validation
- [ ] `irc_tags_buffer_overflow_test` — Deeply nested/long `draft/message-tags`.
- [ ] `dcc_path_traversal_poc` — Malicious paths in DCC SEND filenames.

### Server/Channel State Syncing
- [ ] `names_list_truncation_test` — Gigantic `/NAMES` replies (10,000+ users).
- [ ] `mode_flood_desync_test` — Rapid `+o`/`-o` mode spam.
- [ ] `ignore_list_evasion_poc` — Case-sensitivity, prefix, Unicode look-alike evasion.

### Text Formatting & Rendering
- [ ] `mirc_color_code_overflow_test` — Massive unclosed mIRC color codes.
- [ ] `url_regex_dos_test` — ReDoS in `url.c` regex engine.

### Plugin & Subprocess Management
- [ ] `plugin_event_loop_hijack_test` — Rogue plugin timer termination.
- [ ] `subprocess_injection_poc` — `/exec` bash metacharacter injection.

### IPC & D-Bus
- [ ] `dbus_method_fuzz_test` — Invalid GLib Variants to notification D-Bus proxy.

### Configuration & Local Storage
- [ ] `config_corruption_recovery_test` — Corrupted `servlist.conf` recovery.

## Priority 9: Windows Build Environment (MSVC & `gvsbuild`)
*Can begin at any point once the codebase compiles cleanly on Linux, but benefits from a stable API surface.*

- [ ] **9.1a: Drop MSYS2 MinGW** — Move Windows builds to `gvsbuild build gtk4 libadwaita openssl`.
- [ ] **9.1b: Audit Meson MSVC paths** — Ensure `cc.get_id() == 'msvc'` branches link against `gvsbuild` pkgconfig.
- [ ] **9.2a: GitHub Actions for MSVC** — Implement `.github/workflows/windows.yml`.
- [ ] **9.2b: Executable Packaging** — Bundle GTK4/GLib DLLs alongside `ditrigon.exe`.

## Priority 10: Modern Distribution
*Final polish. Depends on virtually everything above being stable.*

- [ ] **10.1: Flatpak Manifest** — Strict `org.ditrigon.Ditrigon.json` with GNOME 47 runtime.
- [ ] **10.2: Flathub Submission** — One-click universal Linux installation.
