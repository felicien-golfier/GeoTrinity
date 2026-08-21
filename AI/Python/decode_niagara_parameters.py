"""
Decodes a Niagara rapid-iteration parameter store dumped from the Remote Control property API.

The store is unreachable from the Python bindings, so fetch it from outside the editor first — a synchronous
self-request would block the editor's own game thread:

    curl -s -X PUT http://localhost:30010/remote/object/property \
      -H "Content-Type: application/json" \
      -d '{"objectPath":"/Game/Path/NS_Foo.NS_Foo:EmitterName.SpawnScript",
           "propertyName":"RapidIterationParameters","access":"READ_ACCESS"}' -o store.json

Script paths are the emitter's SpawnScript, UpdateScript, EmitterSpawnScript or EmitterUpdateScript.
Then run this through MCP execute_script, writing to a file the caller reads back.
"""

import json
import struct

def decode_store(json_path, name_filter=""):
    """Returns [(name, values)] for each parameter whose name contains name_filter, in offset order.

    Component sizes come from the gap to the next offset — the dump carries no explicit size — so the
    entries must be walked sorted by offset.
    """
    store = json.load(open(json_path, encoding="utf-8"))["RapidIterationParameters"]
    data = bytes(bytearray(store["ParameterData"]))
    entries = sorted(store["SortedParameterOffsets"], key=lambda entry: entry["Offset"])

    decoded = []
    for index, entry in enumerate(entries):
        if name_filter not in entry["Name"]:
            continue
        start = entry["Offset"]
        end = entries[index + 1]["Offset"] if index + 1 < len(entries) else len(data)
        count = (end - start) // 4
        decoded.append((entry["Name"], struct.unpack_from("<%df" % count, data, start)))
    return decoded


def write_report(json_path, report_path, name_filter=""):
    lines = ["%-64s %s" % (name, ", ".join("%g" % value for value in values))
             for name, values in decode_store(json_path, name_filter)]
    with open(report_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))


def example_report_one_module():
    """Reports a single module's constants from a fetched store. Deliberately not invoked."""
    write_report(r"C:\Temp\store.json", r"C:\Temp\store_decoded.txt", "ShapeLocation")
