# Lists dirty content packages and saves them, writing the result to Saved/mcp_result.txt.
# Run after any asset-mutating MCP operation; repeat until no modified packages remain dirty.
import unreal

dirty = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
results = {name: unreal.EditorAssetLibrary.save_asset(name, only_if_is_dirty=True) for name in dirty}
with open(r"C:\GeoTrinity\Saved\mcp_result.txt", "w") as f:
    f.write(repr(results))
