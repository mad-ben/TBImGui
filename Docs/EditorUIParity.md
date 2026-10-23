# Editor UI coverage

All ports use the existing **Enable ImGui UI** setting. The Outliner hierarchy and
Content Browser asset area replace presentation while retaining Unreal's models,
commands, permission checks, menus, and editing operations.

| Workflow | Implementation |
| --- | --- |
| Outliner selection and Delete/clipboard/duplicate/undo | One native user-selection notification per operation; editor and mode command routing |
| Outliner navigation and expansion | Arrow/Home/End/Page keys, Ctrl/Shift ranges, Ctrl+A, Ctrl+Space, recursive Shift expansion |
| Outliner visibility and pins | Selected-item visibility changes, visibility dragging in one transaction, native pin/unpin checks |
| Outliner drag/drop | Native operations, payload validation, folder/actor targets, hover expansion, edge scrolling |
| Outliner columns | Native visible column definitions and sorting; specialized row/header widgets open in native popovers |
| Rename in both views | Inline ImGui text input; native read-only checks, validation, commit/cancel callbacks; Enter confirms and Escape cancels |
| Asset creation and duplication | Native deferred creation; item-list refresh and scroll completion deliver the new-item rename request |
| Asset opening | Native activation delegate for each asset type, including Materials and Levels |
| Asset selection and navigation | Native selection modes, Ctrl/Shift ranges, Ctrl+A, Ctrl+Space, Page/Home/End, native type-to-select |
| Asset menus and commands | Native item/background menus and command routing, including F2 and Delete |
| Asset drag/drop | Native asset drag operations and item handlers; background drops bubble to native folder/collection/import handlers |
| Asset columns and sorting | Built-in/custom/metadata columns, native sort manager, secondary sorting with Shift, native CSV export |
| Thumbnails and tooltips | Unreal asset/folder widgets rendered into cached textures; wrapped names; native tooltip extensions |
| Thumbnail editing | Double-click in Unreal's thumbnail edit mode opens the native thumbnail editing widget |
| Scrolling and settings | Right-button panning, drag edge scrolling; per-browser grid/list and visible columns; shared saved thumbnail size in Project Settings |
| Level Editor Details | ImGui property tree and standard value editors; native property handles and specialized-row fallback; see [DetailsPanel.md](DetailsPanel.md) |

Specialized column controls and thumbnail editing still use native popovers. Asset
status/extra-state overlays, asset-specific thumbnail action buttons, and stacked
Outliner hierarchy headers are not drawn directly in ImGui. Those visual features
remain available in the native views; native context actions remain accessible.

## Verification

Source/API review and formatting checks cover this change. Compilation and editor
runtime testing were deliberately not run, following the source-only instruction.
The following runtime checks remain unverified:

- Create a Material, accept its inline name, open it, rename through F2 and the context menu, then cancel a second creation with Escape.
- Repeat with a Level, texture, Blueprint, folder, and duplicate; enter an invalid or duplicate name and confirm native validation.
- Select an actor, verify viewport/Details selection, then exercise Delete, undo, duplicate, clipboard commands, and actor/folder rename.
- Check Ctrl/Shift ranges, keyboard navigation, right-button panning, visibility dragging with undo, and World Partition pin/unpin.
- Drag between Outliners, folders, browser, and viewport; check invalid targets, hover expansion/navigation, and edge scrolling.
- Enable custom/source-control columns, use their native popovers, inspect extended asset tooltips, edit a thumbnail, and reopen browser tabs to verify saved choices.
