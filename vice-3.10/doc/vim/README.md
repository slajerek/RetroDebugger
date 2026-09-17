# Vim files for VICE

A collection of vim scripts for use with VICE.

## Hotkeys syntax highlighting

Syntax highlighting for (Gtk3) hotkeys files.

- `syntax/vhk.vim`
- `ftdetect/vhk.vim`

### Auto-generating the action name matches

To generate a list of `syn match` lines for the current UI actions a script
can be used: `uiactions.py`.
Run the script with `./uiactions.py vim` to parse `src/arch/gtk3/uiactions.h`
for all available UI actions and generate syntax match rules for the vim
syntax script.

