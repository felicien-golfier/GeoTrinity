"""Material, material-function and material-layer building blocks: in-place rebuilds, asserted wiring, pins, calls,
parameters, layer stacks."""
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
    "MaterialAttributes": unreal.FunctionInputType.FUNCTION_INPUT_MATERIAL_ATTRIBUTES,
}


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


class Graph:
    """The nodes of one material or material function, rebuilt in place.

    A function keeps its input and output nodes across rebuilds: callers are wired to them by id.
    """

    def __init__(self, asset):
        self.asset = asset
        self.is_function = isinstance(asset, unreal.MaterialFunction)
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

    def node(self, cls, x, y, **properties):
        if self.is_function:
            node = mel.create_material_expression_in_function(self.asset, cls, x, y)
        else:
            node = mel.create_material_expression(self.asset, cls, x, y)
        assert node, f"{self.asset.get_name()}: {cls.__name__} would not create"
        for name, value in properties.items():
            node.set_editor_property(name, value)

        return node

    def connect(self, source, target, pin=""):
        """An unknown pin name is a silent no-op in the library, hence the assert; an empty pin is the first."""
        node, output = split(source)
        assert mel.connect_material_expressions(node, output, target, pin), \
            f"{self.asset.get_name()}: {node.get_name()}.{output} -> {target.get_name()}.{pin}"

    def op(self, cls, x, y, source=None, **pins):
        """A node fed on its first input by source, and on each named pin by its value."""
        node = self.node(cls, x, y)
        if source is not None:
            self.connect(source, node)

        for pin, pin_source in pins.items():
            self.connect(pin_source, node, pin)

        return node

    def mask(self, source, channels, x, y):
        node = self.node(unreal.MaterialExpressionComponentMask, x, y,
                         **{channel: channel in channels for channel in "rgba"})
        self.connect(source, node)
        return node

    def call(self, function, x, y, **pins):
        node = self.node(unreal.MaterialExpressionMaterialFunctionCall, x, y)
        # The call builds its pins in the post-change notification only.
        node.set_editor_property("material_function", function, ALWAYS)
        for pin, source in pins.items():
            self.connect(source, node, pin)

        return node

    def parameter(self, cls, name, default, group, sort_priority, description, x, y):
        return self.node(cls, x, y, parameter_name=name, default_value=default, group=group,
                         sort_priority=sort_priority, desc=description)

    def collection_parameter(self, collection, name, x, y):
        node = self.node(unreal.MaterialExpressionCollectionParameter, x, y)
        # The parameter id resolves in the post-change notification only; unresolved it compiles to zero.
        node.set_editor_property("collection", collection, ALWAYS)
        node.set_editor_property("parameter_name", name, ALWAYS)
        return node

    def input(self, name, input_type, sort_priority, description, x, y):
        node = self._pin(("input", name), unreal.MaterialExpressionFunctionInput, x, y)
        node.set_editor_property("input_name", name)
        node.set_editor_property("input_type", INPUT_TYPES[input_type])
        node.set_editor_property("sort_priority", sort_priority)
        node.set_editor_property("description", description)
        node.set_editor_property("use_preview_value_as_default", False)
        return node

    def output(self, name, sort_priority, description, source, x, y):
        node = self._pin(("output", name), unreal.MaterialExpressionFunctionOutput, x, y)
        node.set_editor_property("output_name", name)
        node.set_editor_property("sort_priority", sort_priority)
        node.set_editor_property("description", description)
        self.connect(source, node)
        return node

    def layer_stack(self, layers, blends, names, guids, x, y):
        """A material's layer stack, layer 0 first, set in one assignment.

        guids are fixed strings for every layer after layer 0, so an instance's own stack stays linked across rebuilds.
        """
        assert len(blends) == len(layers) - 1 and len(guids) == len(layers) - 1 and len(names) == len(layers), \
            f"{self.asset.get_name()}: one blend and one guid per layer after the first, one name per layer"
        node = self.node(unreal.MaterialExpressionMaterialAttributeLayers, x, y)
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

    def finish(self):
        """Compile and save; a function also drops the pins this build no longer declares."""
        if self.is_function:
            for node in self.previous_pins.values():
                mel.delete_material_expression_in_function(self.asset, node)

            mel.update_material_function(self.asset)
        else:
            mel.recompile_material(self.asset)

        save(self.asset)

    def _pin(self, key, cls, x, y):
        """The node this pin had in the last build, so callers keep their wires, or a new one."""
        if key in self.previous_pins:
            node = self.previous_pins.pop(key)
            node.set_editor_property("material_expression_editor_x", x)
            node.set_editor_property("material_expression_editor_y", y)
            return node

        return self.node(cls, x, y)
