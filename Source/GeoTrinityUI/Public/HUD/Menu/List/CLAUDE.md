# HUD/Menu/List

One list look for the whole game: server browser and leaderboard differ only by columns.

| File | Role |
|---|---|
| `GeoListFrameWidget.h` | The list inside a page frame: header strip, scrolling rows, footer strip (`WBP_ListPanel`); named slots `HeaderSlot` / `FooterSlot` |
| `GeoListPanelWidget.h` | Base of every full-screen list page; fills the frame with `RowWidgetClass` rows (`MakeRow…`) |
| `GeoListRowWidget.h` | One line (`WBP_ListRow`): columns, optional `OnClicked`; text roles from theme |
| `GeoTableWidget.h` | Column definitions / table helper for list pages |
| `GeoLeaderboardWidget.h` | Fights recorded on this machine: boss tabs, difficulty tabs (Nightmare, Original, Reduced, Safe; opens on Original), best attempts first (least health left, ties by faster); a row is one attempt that is some player's best, so a group that stayed together is one row and each player appears once per boss (`System/Stats/CLAUDE.md`, "Leaderboard"); each row shows the date and each player's class level |

- Editing `WBP_ListPanel` / `WBP_ListRow` re-skins every list.
