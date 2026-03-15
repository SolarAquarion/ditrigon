# Ditrigon GTK4/Libadwaita Code Quality Standards

**Status:** Active — Applies to all `src/fe-gtk4/` development

This document defines the code quality standards for the Ditrigon modernization project. All contributors (human and AI) must follow these guidelines to ensure consistency, maintainability, and adherence to modern GTK4/Libadwaita best practices.

---

## Architecture Overview

**Current State (as of 2026-03-15):**

| Component | Status | Notes |
|-----------|--------|-------|
| **Frontend** | `src/fe-gtk4/` (39 files) | GTK4 + Libadwaita active |
| **Legacy Frontend** | `src/fe-gtk/` (61 files) | GTK3 — deprecated, no new development |
| **Application Model** | ✅ `AdwApplication` | Proper single-instance, GAction support |
| **Window Base** | ✅ `AdwWindow` / `AdwApplicationWindow` | Via `fe_gtk4_adw_window_new()` |
| **Layout** | ✅ `AdwToolbarView` + `AdwHeaderBar` | Standard modern layout |
| **Notifications** | ✅ `AdwToastOverlay` | Via `fe_toast_send()` |
| **Memory Safety** | ❌ Manual `g_free()` | **Priority gap** — needs `g_autoptr` migration |
| **C Standard** | ⚠️ Check `meson.build` | Should be `gnu17` |

---

## Table of Contents

1. [C Standard & Compiler Settings](#c-standard--compiler-settings)
2. [Memory Management](#memory-management) (**HIGH PRIORITY**)
3. [Libadwaita Usage Patterns](#libadwaita-usage-patterns)
4. [Widget Guidelines](#widget-guidelines)
5. [Signal & Event Handling](#signal--event-handling)
6. [UI File Standards (GtkBuilder)](#ui-file-standards-gtkbuilder)
7. [Error Handling & Logging](#error-handling--logging)
8. [Code Style & Formatting](#code-style--formatting)
9. [Testing Requirements](#testing-requirements)

---

## C Standard & Compiler Settings

### Current State
**Check:** `src/common/meson.build` and `src/fe-gtk4/meson.build`

### Required Standard
- **Minimum:** `gnu11` (C11 with GNU extensions)
- **Preferred:** `gnu17` (C17) for improved `_Thread_local` and anonymous struct support

### Verification
```bash
grep 'c_std' src/fe-gtk4/meson.build src/common/meson.build
```

If not set, add to `meson.build`:
```meson
project('ditrigon', 'c',
  default_options: ['c_std=gnu17']
)
```

### Required Compiler Flags
```meson
add_project_arguments(
  '-Wall',
  '-Wextra',
  '-Wno-unused-parameter',  # Common in callback signatures
  '-Wno-missing-field-initializers',
  language: 'c'
)
```

---

## Memory Management

### ⚠️ HIGH PRIORITY MIGRATION TARGET

**Current State:** `src/fe-gtk4/` uses manual `g_free()`, `g_new0()`, `g_strdup()` throughout.

**Goal:** Migrate to `g_autoptr`/`g_autofree` for automatic cleanup.

---

### Priority Order (Best to Worst)

1. **`g_autoptr` / `g_autofree`** (GLib auto-cleanup) — **USE THIS**
2. **`g_steal_pointer()`** for ownership transfers
3. **Explicit `g_free()` / `g_object_unref()`** (only when auto-cleanup impossible)
4. **Never:** Raw `malloc()` / `free()`

---

### Migration Examples

#### Strings
```c
// ❌ CURRENT (fe-gtk4/banlist.c:306)
char *title = g_strdup_printf ("Ban List - %s", sess->channel);
// ... use title ...
g_free (title);  // Manual - easy to forget on early return

// ✅ MIGRATED
g_autofree char *title = g_strdup_printf ("Ban List - %s", sess->channel);
// ... use title ...
// Automatically freed on scope exit - no g_free() needed
```

#### Structs with Cleanup
```c
// ❌ CURRENT (fe-gtk4/banlist.c:44-49)
typedef struct
{
    char *mask;
    char mode;
} HcBanRowData;

static void
ban_row_data_free (gpointer data)
{
    HcBanRowData *row = data;
    g_free (row->mask);  // Manual nested free
    g_free (row);
}

// ✅ MIGRATED with G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC
typedef struct
{
    g_autofree char *mask;  // Auto-freed when struct is freed
    char mode;
} HcBanRowData;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (HcBanRowData, g_free)

static void
ban_row_data_free (gpointer data)
{
    g_autoptr(HcBanRowData) row = data;
    // row->mask auto-freed, then row auto-freed
    // No explicit g_free() calls needed
}
```

#### GObjects
```c
// ❌ CURRENT
GtkBuilder *builder = gtk_builder_new_from_resource ("/path/ui");
GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (builder, "widget"));
g_object_unref (builder);  // Manual

// ✅ MIGRATED
g_autoptr(GtkBuilder) builder = gtk_builder_new_from_resource ("/path/ui");
GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (builder, "widget"));
// builder auto-unref on scope exit
```

#### GError Pattern
```c
// ❌ CURRENT (common pattern in fe-gtk4/)
GError *error = NULL;
if (!do_something (&error))
{
    g_warning ("Failed: %s", error->message);
    g_error_free (error);  // Manual
}

// ✅ MIGRATED
g_autoptr(GError) error = NULL;
if (!do_something (&error))
{
    g_warning ("Failed: %s", error->message);
    // error auto-freed on scope exit
}
```

---

### Quick Migration Checklist

When modifying any `fe-gtk4/` file:

- [ ] Replace `g_strdup()` → `g_autofree char *var = g_strdup()`
- [ ] Replace `g_new0()` → `g_autoptr(Type) var = g_new0(Type, 1)`
- [ ] Add `G_DEFINE_AUTOPTR_CLEANUP_FUNC(Type, g_free)` for custom structs
- [ ] Remove corresponding `g_free()` calls at end of scope
- [ ] Use `g_steal_pointer()` when transferring ownership out of scope

---

### Ownership Rules

- Functions returning newly allocated memory: Document with `/* (transfer full) */`
- Functions returning borrowed references: Document with `/* (transfer none) */`
- Use `g_steal_pointer()` when transferring ownership out of a scope

---

## Libadwaita Usage Patterns

### ✅ Already Implemented (fe-gtk4/)

Your `src/fe-gtk4/adw.c` provides helper functions — **use these** instead of calling libadwaita directly:

| Function | Purpose | Usage |
|----------|---------|-------|
| `fe_gtk4_adw_init()` | Initialize libadwaita | Called once in `fe_args()` |
| `fe_gtk4_adw_application_new()` | Create `AdwApplication` | In `fe_args()` |
| `fe_gtk4_adw_window_new()` | Create app window | In `maingui.c:1253` |
| `fe_gtk4_adw_window_set_content()` | Set window content | After creating window |
| `fe_gtk4_adw_set_window_title()` | Set title | On network/channel change |
| `fe_gtk4_adw_set_window_subtitle()` | Set subtitle | For status text |
| `fe_toast_send()` | Show toast notification | Instead of manual `AdwToast` |

### Example: Creating a New Dialog Window

```c
// ✅ Use existing pattern (from banlist.c)
#include "fe-gtk4.h"
#include <adwaita.h>

#define BANLIST_UI_PATH "/org/ditrigon/ui/gtk4/dialogs/banlist-window.ui"

void
banlist_opengui (session *sess)
{
    g_autoptr(GtkBuilder) builder = fe_gtk4_builder_new_from_resource (BANLIST_UI_PATH);
    GtkWidget *window = fe_gtk4_builder_get_widget (builder, "banlist_window", GTK_TYPE_WINDOW);
    
    // Window is already AdwWindow from UI file
    gtk_window_present (GTK_WINDOW (window));
}
```

### Toast Notifications

```c
// ✅ Use helper (already in adw.c)
fe_toast_send (_("Connected to server"));

// Instead of manual:
// AdwToast *toast = adw_toast_new ("Connected");
// adw_toast_overlay_add (overlay, toast);
```

---

## Widget Guidelines

### Current Architecture (fe-gtk4/)

**Main Window Structure** (`maingui.c`):
```
AdwWindow
└─ AdwToolbarView
   ├─ Top: AdwHeaderBar (with menu, sidebar, userlist buttons)
   └─ Content: AdwToastOverlay
      └─ Main layout (session list, chat view, userlist)
```

### Adding New Dialogs/Windows

1. **Create UI file** in `data/ui/gtk4/dialogs/`
2. **Use AdwWindow template** (not raw GtkWindow)
3. **Load via helper**: `fe_gtk4_builder_new_from_resource()`
4. **Present**: `gtk_window_present()`

### Example: New Dialog Template

```xml
<!-- data/ui/gtk4/dialogs/example-window.ui -->
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <template class="DitrigonExampleWindow" parent="AdwWindow">
    <property name="modal">True</property>
    <property name="default-width">500</property>
    <property name="default-height">400</property>
    <child>
      <object class="AdwToolbarView">
        <child type="top">
          <object class="AdwHeaderBar"/>
        </child>
        <child>
          <!-- Your content here -->
        </child>
      </object>
    </child>
  </template>
</interface>
```

---

## Signal & Event Handling

### GAction Pattern

**Current State:** `maingui.c` has `window_actions` (`GSimpleActionGroup`)

**Best Practice:** Use GActions for app-level commands, not raw signal handlers.

```c
// Define action
static void
action_reconnect (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    
    server *serv = fe_gtk4_window_target_session ()->server;
    if (serv)
        server_reconnect (serv);
}

static const GActionEntry window_actions[] = {
    { "reconnect", action_reconnect, NULL, NULL, NULL }
};

// Register (typically in menu_init or maingui_init)
g_action_map_add_action_entries (G_ACTION_MAP (window_actions),
    window_actions, G_N_ELEMENTS (window_actions), NULL);

// Bind to keyboard shortcut (in startup)
gtk_application_set_accels_for_action (GTK_APPLICATION (app),
    "window.reconnect", (const char *[]) { "<Control>R", NULL });
```

### Signal Connection Safety

```c
// ✅ Use destroy notifications for long-lived connections
g_signal_connect_object (button, "clicked",
    G_CALLBACK (on_button_clicked), data,
    G_CONNECT_SWAPPED | G_CONNECT_AFTER);

// Or with cleanup:
g_signal_connect_data (button, "clicked",
    G_CALLBACK (on_button_clicked),
    g_object_ref_weak (data),
    (GClosureNotify) g_object_unref_weak,
    0);
```

---

## UI File Standards (GtkBuilder)

### Current Organization

```
data/
├── ditrigon.gresource.xml      # Resource manifest
└── ui/gtk4/
    ├── maingui.ui               # Main window base
    ├── menus/
    │   └── static-menus.ui      # Menu models
    └── dialogs/
        ├── ascii-window.ui
        ├── banlist-window.ui
        ├── chanlist-window.ui
        └── ...
```

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| **UI files** | `kebab-case.ui` | `banlist-window.ui` |
| **Object IDs** | `snake_case` | `banlist_refresh_button` |
| **Template classes** | `Ditrigon{Name}` | `DitrigonBanlistWindow` |
| **Resource paths** | `/org/ditrigon/ui/gtk4/...` | `/org/ditrigon/ui/gtk4/dialogs/banlist-window.ui` |

### Loading UI Files

**Use the helper** (from `fe-gtk4.h`):
```c
#define BANLIST_UI_PATH "/org/ditrigon/ui/gtk4/dialogs/banlist-window.ui"

g_autoptr(GtkBuilder) builder = fe_gtk4_builder_new_from_resource (BANLIST_UI_PATH);
GtkWidget *window = fe_gtk4_builder_get_widget (builder, "banlist_window", GTK_TYPE_WINDOW);
```

**Don't** use raw `gtk_builder_new_from_resource()` — the helper adds type checking.

---

## Error Handling & Logging

### GLib Error Pattern

**Use `g_autoptr(GError)`** for automatic cleanup:

```c
gboolean
load_config (const char *path, GError **error)
{
    g_autofree char *content = NULL;
    
    if (!g_file_get_contents (path, &content, NULL, error)) {
        // Error already set by g_file_get_contents
        return FALSE;
    }
    
    if (!parse_config (content, error)) {
        return FALSE;
    }
    
    return TRUE;
}

// Usage
g_autoptr(GError) err = NULL;
if (!load_config ("/path/to/config", &err)) {
    g_warning ("Failed to load config: %s", err->message);
    // err auto-freed on scope exit
}
```

### Logging Levels

```c
g_debug ("Detailed debug info: %s", value);      // Development only
g_message ("User-facing info: %s", msg);         // Default level
g_warning ("Recoverable issue: %s", reason);     // Warning
g_critical ("Serious error: %s", details);       // Critical
g_error ("Fatal - terminates program");          // Fatal (abort)
```

### Toast vs. Log

| Use Case | Method |
|----------|--------|
| User notification | `fe_toast_send()` |
| Debug/diagnostic | `g_debug()` / `g_message()` |
| Error reporting | `g_warning()` + toast |
| Critical failure | `g_critical()` + dialog |

---

## Code Style & Formatting

### General Rules

| Element | Convention | Example |
|---------|------------|---------|
| **Line length** | 120 chars max | |
| **Indentation** | 4 spaces (no tabs) | |
| **Functions** | `snake_case` with prefix | `fe_gtk4_maingui_init()` |
| **Types** | `PascalCase` | `HcBanView`, `SessionData` |
| **Macros** | `UPPER_SNAKE_CASE` | `BANLIST_UI_PATH` |
| **Structs** | `typedef` + `G_DEFINE_AUTOPTR_CLEANUP_FUNC` | See Memory section |

### Function Structure

```c
// Documentation comment (GObject Introspection compatible)
/**
 * fe_gtk4_adw_set_window_title:
 * @title: (nullable): New window title, or %NULL for default
 *
 * Updates the main window title. Subtitle cleared.
 */
void
fe_gtk4_adw_set_window_title (const char *title)
{
    g_return_if_fail (ADW_IS_WINDOW_TITLE (adw_title_widget));
    
    adw_window_title_set_title (ADW_WINDOW_TITLE (adw_title_widget),
        (title && title[0]) ? title : PACKAGE_NAME);
}
```

### File Organization

- **Headers:** `#include "fe-gtk4.h"` first, then system libs, then common/
- **License:** `/* SPDX-License_Identifier: GPL-2.0-or-later */` at top
- **Guards:** `#ifndef HEXCHAT_FE_GTK4_H` / `#define HEXCHAT_FE_GTK4_H`

---

## Testing Requirements

### Per-Feature Checklist

Each modernization task from `modernization_roadmap.md` must include:

- [ ] **Manual test plan** documented in commit message or roadmap checkbox
- [ ] **Memory check**: Run with `G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --leak-check=full`
- [ ] **Wayland test**: Verify works on both X11 and Wayland
- [ ] **Theme test**: Check with dark/light themes

### Memory Leak Detection

```bash
# Build with debug symbols
meson setup build --buildtype=debug

# Run with valgrind
G_SLICE=always-malloc G_DEBUG=gc-friendly \
  valgrind --leak-check=full --show-leak-kinds=all \
  ./build/src/fe-gtk4/ditrigon

# Look for "definitely lost" or "indirectly lost"
```

### Sanitizer Build (Optional)

```bash
meson setup build \
  -Db_sanitize=address,undefined \
  -Doptimization=0

meson compile -C build
ASAN_OPTIONS=detect_leaks=1 ./build/src/fe-gtk4/ditrigon
```

---

## Quick Reference for AI Contributors

**Vibe Coding Checklist:**

1. **Check Roadmap** → `modernization_roadmap.md` for task priority
2. **Memory first** → Use `g_autoptr`/`g_autofree` for ALL allocations
3. **Use helpers** → `fe_gtk4_*()` functions from `fe-gtk4.h`
4. **UI files** → Add to `data/ui/gtk4/`, reference via resource path
5. **Toast, don't log** → User notifications via `fe_toast_send()`
6. **Test build** → `meson compile -C build` after changes
7. **Update roadmap** — Check off completed items

**Common Patterns:**

```c
// String
g_autofree char *text = g_strdup ("hello");

// Struct
g_autoptr(MyStruct) s = g_new0 (MyStruct, 1);

// GError
g_autoptr(GError) err = NULL;

// GObject
g_autoptr(GtkBuilder) builder = fe_gtk4_builder_new_from_resource (path);

// Toast
fe_toast_send (_("Done!"));
```

**DON'T:**

- ❌ Manual `g_free()` at end of function
- ❌ Raw `gtk_builder_new_from_resource()` — use helper
- ❌ `printf()` for user messages — use toast or log
- ❌ GTK3 widgets — this is `fe-gtk4/`, not `fe-gtk/`

---

_This is a living document. Last updated: 2026-03-15_

_Archive previous version before major edits: `cp GTK_CODE_QUALITY.md GTK_CODE_QUALITY.md.2026-03-15`_
