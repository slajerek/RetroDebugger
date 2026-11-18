# GUI protocol checks

These focused checks catch parser/schema drift and reject unsupported targets without building Retro Debugger.  They compile the production header against the existing minimal SYS_Defs test stub, then independently validate the generated schema with Python jsonschema (Draft 7).  They do not prove widget behavior or rendering.

Prerequisites: a C++17 compiler, Python 3 with jsonschema, and the directory containing the engine's json.hpp.  From the repository root, pass a new evidence directory:

```sh
JSON_INCLUDE=../MTEngineSDL/src/Engine/Libs/nlohmann bash tests/gui_control/run.sh /tmp/retro-gui-protocol-evidence
```

The typed-request check consumes that same unchanged case corpus to verify action/target normalization, field preservation, and rejection without mutating the output request:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -Itests/disk_directory/support -Isrc/Remote -I../MTEngineSDL/src/Engine/Libs/nlohmann tests/gui_control/request_test.cpp -o /tmp/retro-gui-request
/tmp/retro-gui-request /tmp/retro-gui-protocol-evidence/schema-cases.json
```

The frame-startup check separately exercises the production readiness predicate.  A newly initialized test engine must observe the current GUI frame before an action can be queued:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -Isrc/Remote tests/gui_control/frame_startup_test.cpp -o /tmp/retro-gui-frame-startup && /tmp/retro-gui-frame-startup
```

This check does not execute the GUI frame hooks or establish native startup behavior.  The application retains the original action deadline while waiting for frame synchronization; it does not disable the engine's missing-signal assertion.

For real endpoint-driven widgets and capture, use a test-enabled visible native build with the existing runner:

```sh
bash tests/run_test.sh --skip-build --visible MCPGuiControl
```

The GUI test uses an unused address between CE00 and CEFF, verifies the popup's existing outside-click dismissal against the unchanged backend list, verifies Enter creates exactly one breakpoint, and checks panel/editor PNG output.  Backend removal is fixture cleanup only, not the GUI implementation.  The driver is invoked through the endpoint from a worker so it does not wait on its own serialized ImGui test queue.  Headless or test-disabled runs verify refusal, then explicitly skip real-widget coverage.
