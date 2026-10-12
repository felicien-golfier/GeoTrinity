# HUD/Menu/Server

Pages reached from the main menu.

| File | Role |
|---|---|
| `GeoCreateServerWidget.h` | Form → session creation in C++; sets SERVER_NAME / LANGUAGE / MAP session keys; `MapDisplayNames`, `MapURLs` from BP |
| `GeoBrowseServersWidget.h` | Lists online sessions into the shared list frame; client-side name filter, server-side language query |
| `GeoLocalConnectWidget.h` | "Play Local": direct-IP host / join without Steam via `UGeoSessionSubsystem`; `HostMap`; `LocalIPText` |

- Browse rows come from `List/`; call `InitFromSearchResult` after creating a row.
