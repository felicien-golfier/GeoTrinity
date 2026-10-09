"""Convert between Data/gems.csv and the Gem catalog table of the "GeoTrinity — Gem System" doc, and keep the sync stamp.

Plain Python, no editor. The doc table is a markdown pipe table with DOC_COLUMNS; read it from the doc, write its rows
to a file in that same form, and pass the file path. Commands:
  doc-table                      print the doc table built from the CSV
  diff <table.md>                list every gem whose doc row differs from its CSV row
  to-csv <table.md>              validate the doc table and write it over the CSV
  status [<table_hash>]          has the CSV, or the doc table (by its read hash), changed since the last sync?
  stamp <table_id> <table_hash> <direction>   record the current CSV and doc table as in sync
"""
import csv
import datetime
import hashlib
import io
import json
import sys

CSV_PATH = r"C:\GeoTrinity\Data\gems.csv"
STAMP_PATH = r"C:\GeoTrinity\Data\gems.sync.json"
DOC_URL = "https://claude.ai/artifact/SQYwGjtE7bspcCA5KKV67K"

CSV_COLUMNS = ["Tier", "Id", "Name", "Attribute", "Operation", "MagnitudePerGem", "Effect", "Color", "GrantedTag"]
DOC_COLUMNS = ["Tier", "Id", "Gem", "Effect", "Per gem", "Max per class", "Attribute", "Op", "Colour", "Tag"]
# Sockets of each tier a class has at level 20, from GeoGem::GetSockets(); a Core counts once, every Core being unique.
SOCKETS_PER_CLASS = {"Chip": 30, "Cut": 15, "Prism": 9, "Core": 1}
TIERS = list(SOCKETS_PER_CLASS)
EMPTY = "—"
MINUS = "−"


def read_csv_text():
    with open(CSV_PATH, encoding="utf-8") as f:
        return f.read().replace("\r\n", "\n")


def csv_rows():
    return list(csv.DictReader(io.StringIO(read_csv_text())))


def csv_hash():
    return hashlib.sha256(read_csv_text().encode("utf-8")).hexdigest()


def format_percent(fraction, decimals=4):
    """0.00667 as "+0.667%", 0 as the empty mark."""
    if fraction == 0:
        return EMPTY
    text = "{:.{}f}".format(abs(fraction) * 100, decimals).rstrip("0").rstrip(".")
    return ("+" if fraction > 0 else MINUS) + text + "%"


def parse_percent(text):
    text = text.strip()
    if text in ("", EMPTY, "0", "0%"):
        return 0.0
    return float(text.replace(MINUS, "-").replace("%", "").replace("+", "")) / 100


def format_magnitude(fraction):
    """The magnitude as the editor's CSV export writes it (C's %g), so a doc-made CSV never gets rewritten."""
    return "{:g}".format(float("{:.6g}".format(fraction)))


def max_per_class(row):
    magnitude = float(row["MagnitudePerGem"])
    if magnitude == 0:
        return "Rule change" if row["Tier"] == "Core" else EMPTY
    return format_percent(magnitude * SOCKETS_PER_CLASS[row["Tier"]], 1)


def doc_cell(value):
    return value.replace("|", "\\|") if value else EMPTY


def doc_table():
    lines = ["| " + " | ".join(DOC_COLUMNS) + " |", "|" + " --- |" * len(DOC_COLUMNS)]
    for row in csv_rows():
        cells = [row["Tier"], row["Id"], row["Name"], row["Effect"], format_percent(float(row["MagnitudePerGem"])),
                 max_per_class(row), row["Attribute"], row["Operation"], row["Color"], row["GrantedTag"]]
        lines.append("| " + " | ".join(doc_cell(cell) for cell in cells) + " |")
    return "\n".join(lines)


def parse_doc_table(path):
    """The doc table's rows as CSV rows; raises with every bad line."""
    with open(path, encoding="utf-8") as f:
        lines = [line.strip() for line in f if line.strip().startswith("|")]
    split = [[cell.strip().replace("\\|", "|") for cell in line.strip("|").split(" | ")] for line in lines]
    if not split or split[0] != DOC_COLUMNS:
        raise ValueError("first row must be the header | {} |".format(" | ".join(DOC_COLUMNS)))

    rows, errors = [], []
    for number, cells in enumerate(split[2:], start=3):
        cells = [cell.strip() for cell in cells]
        if len(cells) != len(DOC_COLUMNS):
            errors.append("row {}: {} cells instead of {}".format(number, len(cells), len(DOC_COLUMNS)))
            continue
        doc = dict(zip(DOC_COLUMNS, ["" if cell == EMPTY else cell for cell in cells]))
        tier = doc["Tier"].split()[-1] if doc["Tier"] else ""
        if tier not in TIERS:
            errors.append("row {} ({}): no tier named '{}'".format(number, doc["Id"], doc["Tier"]))
            continue
        try:
            magnitude = parse_percent(doc["Per gem"])
        except ValueError:
            errors.append("row {} ({}): '{}' is not a percentage".format(number, doc["Id"], doc["Per gem"]))
            continue
        rows.append({"Tier": tier, "Id": doc["Id"], "Name": doc["Gem"], "Attribute": doc["Attribute"],
                     "Operation": doc["Op"], "MagnitudePerGem": format_magnitude(magnitude), "Effect": doc["Effect"],
                     "Color": doc["Colour"], "GrantedTag": doc["Tag"]})
    if errors:
        raise ValueError("\n".join(errors))
    return rows


def diff(path):
    doc = {row["Id"]: row for row in parse_doc_table(path)}
    game = {row["Id"]: row for row in csv_rows()}
    lines = []
    for gem_id in list(game) + [gem_id for gem_id in doc if gem_id not in game]:
        if gem_id not in doc:
            lines.append("{}: only in the CSV".format(gem_id))
        elif gem_id not in game:
            lines.append("{}: only in the doc".format(gem_id))
        else:
            for column in CSV_COLUMNS:
                if doc[gem_id][column] != game[gem_id][column]:
                    lines.append("{} {}: CSV '{}' / doc '{}'".format(gem_id, column, game[gem_id][column],
                                                                     doc[gem_id][column]))
    if list(doc) != [gem_id for gem_id in game if gem_id in doc] + [gem_id for gem_id in doc if gem_id not in game]:
        lines.append("row order differs")
    return "\n".join(lines) or "no difference"


def to_csv(path):
    rows = parse_doc_table(path)
    out = io.StringIO()
    writer = csv.DictWriter(out, fieldnames=CSV_COLUMNS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    with open(CSV_PATH, "w", encoding="utf-8", newline="") as f:
        f.write(out.getvalue())
    return "wrote {} gems to {}".format(len(rows), CSV_PATH)


def read_stamp():
    try:
        with open(STAMP_PATH, encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        return None


def status(table_hash=None):
    stamp = read_stamp()
    if stamp is None:
        return "no stamp: never synced, compare both sides with diff and ask which wins"
    lines = ["last sync {} ({})".format(stamp["synced_at"], stamp["direction"]),
             "csv changed since: {}".format(csv_hash() != stamp["csv_sha256"])]
    if table_hash:
        lines.append("doc table changed since: {}".format(table_hash != stamp["doc_table_hash"]))
    lines.append("doc table id at last sync: {}".format(stamp["doc_table_id"]))
    return "\n".join(lines)


def stamp(table_id, table_hash, direction):
    data = {"csv_sha256": csv_hash(), "doc_url": DOC_URL, "doc_table_id": table_id, "doc_table_hash": table_hash,
            "direction": direction, "synced_at": datetime.datetime.now().isoformat(timespec="seconds")}
    with open(STAMP_PATH, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2)
        f.write("\n")
    return "stamped {}".format(STAMP_PATH)


COMMANDS = {"doc-table": doc_table, "diff": diff, "to-csv": to_csv, "status": status, "stamp": stamp}

if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
        sys.exit(__doc__)
    print(COMMANDS[sys.argv[1]](*sys.argv[2:]))
