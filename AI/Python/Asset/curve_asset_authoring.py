"""Write a curve asset from a table of keys.

A curve asset holds its channels in a fixed-size array the reflection system does not expose, so the values go in
through the CSV importer rather than the property setter. The importer reads one row per key — a time followed by
one value per channel, no header — and lands every key linear.

Run via mcp-unreal execute_script. Adjust the example call at the bottom for the asset you are writing.
"""
import unreal

CURVE_TYPES = {1: unreal.CSVImportType.ECSV_CURVE_FLOAT,
               3: unreal.CSVImportType.ECSV_CURVE_VECTOR,
               4: unreal.CSVImportType.ECSV_CURVE_LINEAR_COLOR}


def import_curve(package_path, asset_name, rows, csv_path=None):
    """Write `rows` ({time: values per channel}) as a curve asset and return it.

    How many values a row carries picks the curve class, so three of them give a vector curve and one a float curve.
    Re-runnable: the import replaces the asset in place rather than failing on a name already in use.
    """
    channels = len(next(iter(rows.values())))
    if channels not in CURVE_TYPES:
        raise ValueError("no curve class carries {} channels".format(channels))

    csv_path = csv_path or "{}{}.csv".format(unreal.Paths.project_saved_dir(), asset_name)
    with open(csv_path, "w") as handle:
        for time in sorted(rows):
            handle.write(",".join("%.4f" % value for value in (time,) + tuple(rows[time])) + "\n")

    settings = unreal.CSVImportSettings()
    settings.set_editor_property("ImportType", CURVE_TYPES[channels])
    settings.set_editor_property("ImportCurveInterpMode", unreal.RichCurveInterpMode.RCIM_LINEAR)
    factory = unreal.CSVImportFactory()
    factory.set_editor_property("AutomatedImportSettings", settings)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", csv_path)
    task.set_editor_property("destination_path", package_path)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("factory", factory)
    task.set_editor_property("automated", True)  # an unattended run stalls on the options dialog without this
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.load_asset("{}/{}".format(package_path, asset_name))


# --- Example call — uncomment and adjust -------------------------------------------------------

# import_curve("/Game/Camera", "Curve_Example", {0.0: (0.0, 12.0, 0.0), 1.5: (46.0, 36.0, 1.5)})
