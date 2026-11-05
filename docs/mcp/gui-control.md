# Interactive Memory Breakpoints controls

Use `retro_gui` to operate the C64 Memory Breakpoints panel and its add editor through the actual rendered widgets.  This supports inspecting a GUI workflow rather than replacing it with backend breakpoint commands.

## Availability and scope

Call `{"action":"capabilities"}` first.  A visible application built with ImGui test support and native C64 support reports `available:true`.  The driver initializes on demand on the UI thread; no CLI test mode is required.  Headless mode, missing GUI support, and disabled test support are unavailable, not successful no-ops.  A temporarily busy test engine refuses an operation.

Only these targets exist:

- `c64.memory_breakpoints`: `show`, `open`, and `capture`.
- `c64.memory_breakpoints.editor`: `set_address`, `submit`, `cancel`, and `capture`.

`show` makes the panel visible and focuses it.  `open` clicks its `+` button.  `set_address` replaces the address input, `submit` presses Enter in that input, and `cancel` uses the popup's existing outside-click dismissal.  It chooses a point from the visible panel bounds outside the editor, not fixed screen coordinates; if none is available it reports failure rather than moving other windows.  No new Cancel button or global keyboard-navigation change is required.  The address text accepts one through 64 printable ASCII characters, including symbol names.  Validation and breakpoint defaults remain the editor's own behavior; invalid addresses can leave the editor open and are reported as a failed submit.

The optional bundled ImGui Test Engine's existing licensing terms still apply; this does not add another driver dependency.

There are no arbitrary window names, item paths, global navigation keys, custom-view shortcuts, OS events, or generic keyboard/mouse commands.  Emulator key APIs are unchanged.  GUI inputs use ImGui's driver, not the emulator keyboard dispatch.  Other open popups are not dismissed to reach this panel.

The direct debugger endpoint is `gui/control`, with the same arguments and closed JSON Schema published through endpoint discovery.  It is global, not a per-emulator keyboard endpoint.  MCP uses the same endpoint both directly and through the bridge, including after reconnect.

## Safe call sequence

For each action below, retain the returned `requestId`, and call `{"action":"status","requestId":"<returned id>"}` until a terminal outcome before continuing:

```json
{"action":"show","target":"c64.memory_breakpoints"}
{"action":"open","target":"c64.memory_breakpoints"}
{"action":"set_address","target":"c64.memory_breakpoints.editor","text":"ce00"}
{"action":"capture","target":"c64.memory_breakpoints.editor"}
{"action":"submit","target":"c64.memory_breakpoints.editor"}
{"action":"capture","target":"c64.memory_breakpoints"}
```

Use `cancel` instead of `submit` to close the add editor without creating a breakpoint.  Use the existing read-only memory-breakpoint list tool to verify backend state independently when needed.

Actions return `outcome:"queued"`, never an assertion of completion.  Status returns `queued`, `running`, `completed`, `failed`, or `expired`.  Only `completed` confirms the UI action finished; it is not a claim about unrelated emulator execution.  Inspect `error` on failures.  A successful MCP envelope alone does not mean an action completed.

There is one outstanding operation across clients and one retained result, not a general job queue.  Save a terminal result before issuing another action, which replaces the retained record.  Unknown or replaced IDs return an error.  A busy rejection does not queue a second action.

Unstarted operations expire after five seconds and cannot later mutate the UI, even if their UI callback runs after expiration.  Running operations check the same deadline between driver steps.  A delayed running operation remains explicitly `running`, with `deadlineExceeded:true`, until the UI driver returns; do not retry it blindly.  If the deadline is exceeded after a step, the final failure reports that partial effects are possible.  Inputs already consumed cannot be rolled back.  Client-side waits should be bounded; after a client timeout, retain the ID, inspect status, and observe the UI before deciding what to do next.

Capture is of the rendered named panel or editor, not the emulated C64 screen.  The completed result contains `image` with `mimeType:"image/png"`, `encoding:"base64"`, positive `width` and `height`, and `data`.  It writes no host file.  Missing windows or unsupported framebuffer readback are explicit failures, not empty successful images.
