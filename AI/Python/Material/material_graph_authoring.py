"""Material, material-function and material-layer building blocks: in-place rebuilds, asserted wiring, pins, calls,
parameters, layer stacks. A graph places its own nodes when it finishes (AI/Materials.md, "Keeping a graph readable")."""
import unreal

mel = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary
ALWAYS = unreal.PropertyAccessChangeNotifyMode.ALWAYS
PIN_CLASSES = (unreal.MaterialExpressionFunctionInput, unreal.MaterialExpressionFunctionOutput)
INPUT_TYPES = {
    "Scalar": unreal.FunctionInputType.FUNCTION_INPUT_SCALAR,
    "Vector2": unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2,
    "Vector3": unreal.FunctionInputType.FUNCTION_INPUT_VECTOR3,
    "Vector4": unreal.FunctionInputType.FUNCTION_INPUT_VECTOR4,
    "Texture2D": unreal.FunctionInputType.FUNCTION_INPUT_TEXTURE2D,
    "MaterialAttributes": unreal.FunctionInputType.FUNCTION_INPUT_MATERIAL_ATTRIBUTES,
}

# The editor does not expose node sizes: estimates, close enough that nodes sit near each other without touching.
CHARACTER_WIDTH = 8
MIN_WIDTH = 130
TITLE_PADDING = 60
PIN_PADDING = 70
COLUMN_GAP = 48
WIRE_GAP = 12
MAX_COLUMN_GAP = 360
ROW_GAP = 28
HEADER_HEIGHT = 34
PIN_HEIGHT = 26
PREVIEW_HEIGHT = 100
NODE_PADDING = 16
REROUTE_SIZE = 24


def struct(cls, **members):
    """An engine struct with members set one by one: a struct whose members Blueprint cannot see takes no keywords."""
    value = cls()
    for name, member in members.items():
        value.set_editor_property(name, member)

    return value


def save(asset):
    assert assets.save_loaded_asset(asset, only_if_is_dirty=False), f"{asset.get_path_name()}: would not save"


def load(path):
    asset = assets.load_asset(path)
    assert asset, f"{path}: missing"
    return asset


def load_or_create(folder, name, asset_class, factory):
    if assets.does_asset_exist(f"{folder}/{name}"):
        return load(f"{folder}/{name}")
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, asset_class, factory)


def open_material(folder, name):
    """Load or create a material and strip its graph; references to the asset stay intact."""
    return Graph(load_or_create(folder, name, unreal.Material, unreal.MaterialFactoryNew()))


def open_function(folder, name, description, category):
    """Load or create a material function, describe it for the palette, and strip its body."""
    function = load_or_create(folder, name, unreal.MaterialFunction, unreal.MaterialFunctionFactoryNew())
    function.set_editor_property("description", description)
    function.set_editor_property("expose_to_library", True)
    function.set_editor_property("library_categories_text", [unreal.Text(category)])
    return Graph(function)


def open_layer(folder, name, description, asset_class, factory):
    """Load or create a material layer or layer blend, describe it, and strip its body."""
    layer = load_or_create(folder, name, asset_class, factory)
    layer.set_editor_property("description", description)
    return Graph(layer)


def split(source):
    """A source is a node, or a (node, output name) pair; an empty name is the first output."""
    return source if isinstance(source, tuple) else (source, "")


def find_layer_stack(material):
    nodes = [node for node in unreal.ObjectIterator(unreal.MaterialExpressionMaterialAttributeLayers)
             if node.get_outer() == material and unreal.SystemLibrary.is_valid(node)]
    assert len(nodes) == 1, f"{material.get_name()}: expected one layer stack, found {len(nodes)}"
    return nodes[0]


def replace_layer(material, index, layer):
    """Puts another layer asset in one slot of a material's stack and recompiles; no list changes length."""
    node = find_layer_stack(material)
    # A struct read off an object writes each member set back into it: edit a copy, assign it once.
    stack = node.get_editor_property("default_layers").copy()
    layers = list(stack.get_editor_property("layers"))
    layers[index] = layer
    stack.set_editor_property("layers", layers)
    node.set_editor_property("default_layers", stack, ALWAYS)
    mel.recompile_material(material)


def pin_key(node):
    if isinstance(node, unreal.MaterialExpressionFunctionInput):
        return "input", str(node.get_editor_property("input_name"))
    return "output", str(node.get_editor_property("output_name"))


class Box:
    """One node and the wires it takes part in, as the layout sees it."""

    def __init__(self, node, has_preview, is_reroute=False, label=""):
        self.node = node
        self.has_preview = has_preview
        self.is_reroute = is_reroute
        self.label = label
        self.keep = False
        self.sources = []
        self.input_names = []
        self.targets = []
        self.outputs = set()
        self.column = 0
        self.x = 0
        self.top = 0.0

    @property
    def width(self):
        if self.is_reroute:
            return REROUTE_SIZE

        width = len(self.label) * CHARACTER_WIDTH + TITLE_PADDING
        if not self.has_preview:
            pin_text = max(map(len, self.input_names), default=0) + max(map(len, self.outputs), default=0)
            width = max(width, pin_text * CHARACTER_WIDTH + PIN_PADDING)

        return max(width, MIN_WIDTH)

    @property
    def height(self):
        if self.is_reroute:
            return REROUTE_SIZE

        pins_height = PIN_HEIGHT * max(len(self.sources), len(self.outputs), 1)
        return HEADER_HEIGHT + (max(pins_height, PREVIEW_HEIGHT) if self.has_preview else pins_height) + NODE_PADDING

    @property
    def centre(self):
        return self.top + self.height / 2


def pack(boxes, wanted_centres):
    """Stacks a column top to bottom in the order of the centres each box wants, none over another."""
    bottom = None
    for box in sorted(boxes, key=wanted_centres.get):
        box.top = wanted_centres[box] - box.height / 2
        if bottom is not None:
            box.top = max(box.top, bottom + ROW_GAP)

        bottom = box.top + box.height


def segments_cross(first, second):
    (ax, ay), (bx, by) = first
    (cx, cy), (dx, dy) = second

    def side(px, py, qx, qy, rx, ry):
        return (qx - px) * (ry - py) - (qy - py) * (rx - px)

    return (side(ax, ay, bx, by, cx, cy) * side(ax, ay, bx, by, dx, dy) < 0
            and side(cx, cy, dx, dy, ax, ay) * side(cx, cy, dx, dy, bx, by) < 0)


class Graph:
    """The nodes of one material or material function, rebuilt in place.

    A function keeps its input and output nodes across rebuilds: callers are wired to them by id.
    """

    def __init__(self, asset):
        self.asset = asset
        self.is_function = isinstance(asset, unreal.MaterialFunction)
        self.boxes = {}
        self.wires = []
        self.masks = {}
        self.previous_pins = {}
        if self.is_function:
            # Deleted nodes stay in the iterator, invalid, until garbage collection.
            owned = [node for node in unreal.ObjectIterator(unreal.MaterialExpression)
                     if node.get_outer() == asset and unreal.SystemLibrary.is_valid(node)]
            for node in owned:
                if isinstance(node, PIN_CLASSES):
                    self.previous_pins[pin_key(node)] = node
                else:
                    mel.delete_material_expression_in_function(asset, node)

            assert mel.get_num_material_expressions_in_function(asset) == len(self.previous_pins), \
                f"{asset.get_name()}: body would not clear"
        else:
            # One delete-all call removes only every other node.
            while mel.get_num_material_expressions(asset):
                before = mel.get_num_material_expressions(asset)
                mel.delete_all_material_expressions(asset)
                assert mel.get_num_material_expressions(asset) < before, f"{asset.get_name()}: graph would not clear"

    def node(self, cls, **properties):
        """A new node, placed when the graph finishes."""
        if self.is_function:
            node = mel.create_material_expression_in_function(self.asset, cls, 0, 0)
        else:
            node = mel.create_material_expression(self.asset, cls, 0, 0)
        assert node, f"{self.asset.get_name()}: {cls.__name__} would not create"
        for name, value in properties.items():
            node.set_editor_property(name, value)

        self.boxes[id(node)] = Box(node, cls is not unreal.MaterialExpressionMaterialFunctionCall,
                                   label=str(properties.get("parameter_name", cls.__name__.removeprefix("MaterialExpression"))))
        return node

    def connect(self, source, target, pin=""):
        """Wires source to a pin of target, an empty pin being the first; the wire is made when the graph finishes."""
        node, output = split(source)
        self.wires.append((self.boxes[id(node)], output, self.boxes[id(target)], pin))

    def op(self, cls, source=None, **pins):
        """A node fed on its first input by source, and on each named pin by its value.

        Every keyword is a pin; a node's own constants go through node() instead."""
        node = self.node(cls)
        if source is not None:
            self.connect(source, node)

        for pin, pin_source in pins.items():
            self.connect(pin_source, node, pin)

        return node

    def fold(self, cls, sources):
        """One node of cls over every source, chained: each node takes the result so far and the next source."""
        result, *rest = sources
        for source in rest:
            result = self.op(cls, A=result, B=source)

        return result

    def mask(self, source, channels):
        """The channels of a source; a mask of a mask is one mask of the first source."""
        channels = "".join(channel for channel in "rgba" if channel in channels)
        node, output = split(source)
        if id(node) in self.masks and not output:
            source, parent_channels = self.masks[id(node)]
            assert len(channels) and all("rgba".index(channel) < len(parent_channels) for channel in channels), \
                f"{self.asset.get_name()}: mask {channels} reads a channel the mask before it dropped"
            channels = "".join(parent_channels["rgba".index(channel)] for channel in channels)

        node = self.node(unreal.MaterialExpressionComponentMask, **{channel: channel in channels for channel in "rgba"})
        self.connect(source, node)
        self.masks[id(node)] = (source, channels)
        return node

    def call(self, function, **pins):
        node = self.node(unreal.MaterialExpressionMaterialFunctionCall)
        self.boxes[id(node)].label = function.get_name()
        # The call builds its pins in the post-change notification only.
        node.set_editor_property("material_function", function, ALWAYS)
        for pin, source in pins.items():
            self.connect(source, node, pin)

        return node

    def custom(self, description, code, output_type, inputs, outputs=None):
        """A Custom (HLSL) node: inputs maps each input name to its source, outputs each additional output name to its
        CustomMaterialOutputType. With additional outputs, the main one is addressed as (node, "return")."""
        node = self.node(unreal.MaterialExpressionCustom, code=code, output_type=output_type, description=description)
        self.boxes[id(node)].label = description
        # The node builds its pins in the post-change notification only.
        node.set_editor_property("inputs", [struct(unreal.CustomInput, input_name=name) for name in inputs], ALWAYS)
        node.set_editor_property("additional_outputs",
                                 [struct(unreal.CustomOutput, output_name=name, output_type=kind)
                                  for name, kind in (outputs or {}).items()], ALWAYS)
        for name, source in inputs.items():
            self.connect(source, node, name)

        return node

    def parameter(self, cls, name, default, group, sort_priority, description):
        return self.node(cls, parameter_name=name, default_value=default, group=group, sort_priority=sort_priority,
                         desc=description)

    def collection_parameter(self, collection, name):
        node = self.node(unreal.MaterialExpressionCollectionParameter)
        self.boxes[id(node)].label = name
        # The parameter id resolves in the post-change notification only; unresolved it compiles to zero.
        node.set_editor_property("collection", collection, ALWAYS)
        node.set_editor_property("parameter_name", name, ALWAYS)
        return node

    def input(self, name, input_type, sort_priority, description):
        node = self._pin(("input", name), unreal.MaterialExpressionFunctionInput)
        self.boxes[id(node)].label = name
        self.boxes[id(node)].keep = True
        node.set_editor_property("input_name", name)
        node.set_editor_property("input_type", INPUT_TYPES[input_type])
        node.set_editor_property("sort_priority", sort_priority)
        node.set_editor_property("description", description)
        node.set_editor_property("use_preview_value_as_default", False)
        return node

    def output(self, name, sort_priority, description, source):
        node = self._pin(("output", name), unreal.MaterialExpressionFunctionOutput)
        self.boxes[id(node)].label = name
        self.boxes[id(node)].keep = True
        node.set_editor_property("output_name", name)
        node.set_editor_property("sort_priority", sort_priority)
        node.set_editor_property("description", description)
        self.connect(source, node)
        return node

    def layer_stack(self, layers, blends, names, guids):
        """A material's layer stack, layer 0 first, set in one assignment.

        guids are fixed strings for every layer after layer 0, so an instance's own stack stays linked across rebuilds.
        """
        assert len(blends) == len(layers) - 1 and len(guids) == len(layers) - 1 and len(names) == len(layers), \
            f"{self.asset.get_name()}: one blend and one guid per layer after the first, one name per layer"
        node = self.node(unreal.MaterialExpressionMaterialAttributeLayers)
        # A struct read off an object writes each member set back into it, and the node checks every change.
        stack = node.get_editor_property("default_layers").copy()
        per_layer = stack.get_editor_property("editor_only")
        # A new node holds one empty background layer, under the guid every instance keys layer 0 on.
        background_guids = list(per_layer.get_editor_property("layer_guids"))
        assert len(background_guids) == 1, f"{self.asset.get_name()}: a new stack no longer starts with one layer"
        layer_guids = background_guids
        for text in guids:
            guid, parsed = unreal.GuidLibrary.parse_string_to_guid(text)
            assert parsed, f"layer guid {text} does not parse"
            layer_guids.append(guid)

        # Every per-layer list matches the layer count, or the engine halts the editor.
        count = len(layers)
        stack.set_editor_property("layers", layers)
        stack.set_editor_property("blends", blends)
        per_layer.set_editor_property("layer_states", [True] * count)
        per_layer.set_editor_property("layer_names", [unreal.Text(name) for name in names])
        per_layer.set_editor_property("restrict_to_layer_relatives", [False] * count)
        per_layer.set_editor_property("restrict_to_blend_relatives", [False] * (count - 1))
        per_layer.set_editor_property("layer_guids", layer_guids)
        per_layer.set_editor_property("layer_link_states", [unreal.MaterialLayerLinkState.NOT_FROM_PARENT] * count)
        stack.set_editor_property("editor_only", per_layer)
        node.set_editor_property("default_layers", stack, ALWAYS)
        return node

    def to_property(self, source, material_property):
        node, output = split(source)
        assert mel.connect_material_property(node, output, material_property), \
            f"{self.asset.get_name()}: {node.get_name()}.{output} -> {material_property}"
        self.boxes[id(node)].keep = True

    def finish(self):
        """Place the nodes, compile and save; a function also drops the pins this build no longer declares."""
        self.place_nodes()
        if self.is_function:
            for node in self.previous_pins.values():
                mel.delete_material_expression_in_function(self.asset, node)

            mel.update_material_function(self.asset)
        else:
            mel.recompile_material(self.asset)

        save(self.asset)

    def place_nodes(self):
        """Places every node and makes every wire: left to right, one column per step of the longest chain.

        A wire only ever joins two neighbouring columns, so none goes back and none passes over a node: a value
        needed further on travels a rail of reroutes, one per column. A node sits as far right as its consumers
        allow and starts level with them; a column then stacks its nodes downward from there, in the order they were
        made, so a chain of nodes stays on one line."""
        self.prune_unused_nodes()
        boxes = list(self.boxes.values())
        earliest = {}
        consumers = {box: [] for box in boxes}
        feeders = {box: [] for box in boxes}
        for source, _, target, _ in self.wires:
            feeders[target].append(source)
            consumers[source].append(target)

        def earliest_column(box):
            if box not in earliest:
                earliest[box] = 1 + max(map(earliest_column, feeders[box])) if feeders[box] else 0

            return earliest[box]

        last_column = max(map(earliest_column, boxes))
        latest = {}
        for box in sorted(boxes, key=earliest.get, reverse=True):
            latest[box] = min(latest[target] for target in consumers[box]) - 1 if consumers[box] else last_column

        used_columns = sorted(set(latest.values()))
        for box in boxes:
            box.column = used_columns.index(latest[box])

        links = self.route_through_rails()
        boxes = list(self.boxes.values())
        columns = [[box for box in boxes if box.column == column] for column in range(len(used_columns))]
        for box in boxes:
            box.sources = [source for source, _, target, _ in links if target is box]
            box.input_names = [pin for _, _, target, pin in links if target is box]
            box.targets = [target for source, _, target, _ in links if source is box]
            box.outputs = {output for source, output, _, _ in links if source is box}

        left = 0
        for index, column in enumerate(columns):
            for box in column:
                box.x = left

            wires_out = sum(source.column == index for source, _, _, _ in links)
            left += max(box.width for box in column) + min(COLUMN_GAP + WIRE_GAP * wires_out, MAX_COLUMN_GAP)

        row = {}
        for box in sorted(boxes, key=lambda box: box.column, reverse=True):
            row[box] = sum(row[target] for target in box.targets) / len(box.targets) if box.targets else 0.0

        for column in columns:
            pack(column, row)

        lowest = min(box.top for box in boxes)
        for box in boxes:
            box.top -= lowest
            box.node.set_editor_property("material_expression_editor_x", box.x)
            box.node.set_editor_property("material_expression_editor_y", int(box.top))

        for source, output, target, pin in links:
            assert mel.connect_material_expressions(source.node, output, target.node, pin), \
                f"{self.asset.get_name()}: {source.node.get_name()}.{output} -> {target.node.get_name()}.{pin}"

        self.check_layout(boxes, links)

    def prune_unused_nodes(self):
        """Deletes every node nothing reads, and the nodes only it read: a node that does nothing is a useless box.
        The material's outputs and a function's pins stay."""
        pruned = 0
        while True:
            read = {id(source.node) for source, _, _, _ in self.wires}
            dead = [box for box in self.boxes.values() if not box.keep and id(box.node) not in read]
            if not dead:
                break

            for box in dead:
                if self.is_function:
                    mel.delete_material_expression_in_function(self.asset, box.node)
                else:
                    mel.delete_material_expression(self.asset, box.node)

                del self.boxes[id(box.node)]

            alive = set(self.boxes.values())
            self.wires = [wire for wire in self.wires if wire[2] in alive]
            pruned += len(dead)

        if pruned:
            unreal.log(f"LAYOUT::{self.asset.get_name()}: {pruned} unused nodes deleted")

    def route_through_rails(self):
        """The wires between neighbouring columns that replace self.wires: a value read more than a column away is
        passed on by a reroute in every column between, shared by all the pins that read it."""
        links = []
        rails = {}
        for source, output, target, pin in sorted(self.wires, key=lambda wire: wire[2].column):
            rail = rails.setdefault((source, output), {source.column: (source, output)})
            while target.column - 1 not in rail:
                column = max(rail)
                hop = self.reroute(column + 1)
                links.append((*rail[column], hop, ""))
                rail[column + 1] = (hop, "")

            links.append((*rail[target.column - 1], target, pin))

        return links

    def reroute(self, column):
        if self.is_function:
            node = mel.create_material_expression_in_function(self.asset, unreal.MaterialExpressionReroute, 0, 0)
        else:
            node = mel.create_material_expression(self.asset, unreal.MaterialExpressionReroute, 0, 0)

        box = Box(node, False, True)
        box.column = column
        self.boxes[id(node)] = box
        return box

    def check_layout(self, boxes, links):
        """Asserts the two hard rules and logs the soft one: wires crossing each other."""
        for index, box in enumerate(boxes):
            for other in boxes[index + 1:]:
                assert box.column != other.column or box.top + box.height <= other.top or \
                    other.top + other.height <= box.top, f"{self.asset.get_name()}: two nodes overlap"

        for source, _, target, _ in links:
            assert target.column == source.column + 1, f"{self.asset.get_name()}: a wire skips or goes back"

        wires = [((source.x + source.width, source.centre), (target.x, target.centre)) for source, _, target, _ in links]
        crossings = sum(segments_cross(first, second) for index, first in enumerate(wires)
                        for second in wires[index + 1:])
        reroutes = sum(box.is_reroute for box in boxes)
        unreal.log(f"LAYOUT::{self.asset.get_name()}: {len(boxes)} nodes, {reroutes} reroutes, {crossings} crossings")

    def _pin(self, key, cls):
        """The node this pin had in the last build, so callers keep their wires, or a new one."""
        if key in self.previous_pins:
            node = self.previous_pins.pop(key)
            self.boxes[id(node)] = Box(node, True)
            return node

        return self.node(cls)
