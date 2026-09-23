"""Runs material build scripts outside the editor against a stand-in unreal module, checking every function-call pin.

Catches Python errors, and pins or outputs a called function does not declare, before the editor runs anything.
Engine node pin names are not checked; only the editor knows them. Run with the engine's bundled Python
(Engine/Binaries/ThirdParty/Python3/Win64/python.exe), passing the scripts project-relative, in build order.
"""
import os
import sys
import traceback
import types

PROJECT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..")).replace("\\", "/") + "/"


class Obj:
    """Any engine object or struct: editor properties in a dict, a default for the ones never set."""
    DEFAULTS = {}

    def __init__(self, *args, **kwargs):
        self.props = dict(kwargs)
        self.inputs = []
        self.outputs = []
        self.asset = None

    def set_editor_property(self, name, value, notify=None):
        self.props[name] = value
        if isinstance(self, stand_in.MaterialExpressionFunctionInput) and name == "input_name":
            self.asset.inputs.append(value)
        elif isinstance(self, stand_in.MaterialExpressionFunctionOutput) and name == "output_name":
            self.asset.outputs.append(value)

    def get_editor_property(self, name):
        if name not in self.props:
            default = type(self).DEFAULTS.get(name, Obj)
            self.props[name] = default() if callable(default) else default
        return self.props[name]

    def copy(self):
        clone = type(self)()
        clone.props = {key: value.copy() if isinstance(value, Obj) else value for key, value in self.props.items()}
        return clone

    def get_name(self):
        return self.props.get("path", type(self).__name__)

    def get_path_name(self):
        return self.get_name()

    def get_outer(self):
        return None


classes = {}


def engine_class(name, base=Obj, defaults=None):
    if name not in classes:
        classes[name] = type(name, (base,), {"DEFAULTS": defaults or {}})
    return classes[name]


def module_attribute(name):
    if name.startswith("MaterialExpression"):
        return engine_class(name, stand_in.MaterialExpression)
    if name[0].isupper():
        return engine_class(name)
    raise AttributeError(name)


class Enum:
    def __getattr__(self, name):
        return name


class Paths:
    @staticmethod
    def project_dir():
        return PROJECT

    @staticmethod
    def project_saved_dir():
        return PROJECT + "Saved/"


assets = {}


def asset_for(path):
    """One asset per path, typed by its name prefix, so a function built early is the one loaded later."""
    if path not in assets:
        name = path.rsplit("/", 1)[-1]
        prefixes = (("MLB_", "MaterialFunctionMaterialLayerBlend"), ("ML_", "MaterialFunctionMaterialLayer"),
                    ("MF_", "MaterialFunction"), ("MPC_", "MaterialParameterCollection"),
                    ("MI_", "MaterialInstanceConstant"), ("M_", "Material"))
        kind = next(kind for prefix, kind in prefixes if name.startswith(prefix))
        assets[path] = getattr(stand_in, kind)(path=path)
    return assets[path]


class EditorAssetLibrary:
    @staticmethod
    def load_asset(path):
        return asset_for(path)

    @staticmethod
    def does_asset_exist(path):
        return True

    @staticmethod
    def save_loaded_asset(asset, only_if_is_dirty=True):
        return True


class AssetTools:
    def create_asset(self, name, folder, cls, factory):
        return asset_for(f"{folder}/{name}")


class AssetToolsHelpers:
    @staticmethod
    def get_asset_tools():
        return AssetTools()


class SystemLibrary:
    @staticmethod
    def is_valid(obj):
        return True


class GuidLibrary:
    @staticmethod
    def parse_string_to_guid(text):
        return Obj(text=text), len(text) == 32


class MaterialEditingLibrary:
    @staticmethod
    def create_material_expression_in_function(asset, cls, x, y):
        assert isinstance(asset, stand_in.MaterialFunction), f"{asset.get_name()} is not a function"
        node = cls()
        node.asset = asset
        return node

    @staticmethod
    def create_material_expression(asset, cls, x, y):
        assert isinstance(asset, stand_in.Material), f"{asset.get_name()} is not a material"
        return cls()

    @staticmethod
    def connect_material_expressions(node, output, target, pin):
        call = stand_in.MaterialExpressionMaterialFunctionCall
        if isinstance(target, call):
            function = target.props["material_function"]
            assert pin in function.inputs, f"{function.get_name()} has no input {pin!r}, only {function.inputs}"
        if isinstance(node, call) and output:
            function = node.props["material_function"]
            assert output in function.outputs, \
                f"{function.get_name()} has no output {output!r}, only {function.outputs}"
        return True

    @staticmethod
    def connect_material_property(node, output, prop):
        return True

    @staticmethod
    def get_num_material_expressions_in_function(asset):
        return 0

    @staticmethod
    def get_num_material_expressions(asset):
        return 0

    @staticmethod
    def delete_material_expression_in_function(asset, node):
        pass

    @staticmethod
    def delete_all_material_expressions(asset):
        pass

    @staticmethod
    def update_material_function(asset):
        pass

    @staticmethod
    def recompile_material(asset):
        pass

    @staticmethod
    def set_material_instance_parent(instance, parent):
        pass


stand_in = types.ModuleType("unreal")
stand_in.__getattr__ = module_attribute
for name in ("Obj", "Paths", "EditorAssetLibrary", "AssetToolsHelpers", "SystemLibrary", "GuidLibrary",
             "MaterialEditingLibrary"):
    setattr(stand_in, name, globals()[name])
for name in ("FunctionInputType", "PropertyAccessChangeNotifyMode", "MaterialProperty", "BlendMode",
             "MaterialShadingModel", "MaterialLayerLinkState"):
    setattr(stand_in, name, Enum())
stand_in.Text = str
stand_in.ObjectIterator = lambda cls: []
stand_in.log = print
stand_in.MaterialExpression = engine_class("MaterialExpression")
stand_in.MaterialFunction = engine_class("MaterialFunction")
stand_in.MaterialFunctionMaterialLayer = engine_class("MaterialFunctionMaterialLayer", stand_in.MaterialFunction)
stand_in.MaterialFunctionMaterialLayerBlend = engine_class("MaterialFunctionMaterialLayerBlend",
                                                           stand_in.MaterialFunction)
stand_in.MaterialExpressionMaterialAttributeLayers = engine_class(
    "MaterialExpressionMaterialAttributeLayers", stand_in.MaterialExpression,
    {"default_layers": lambda: engine_class("MaterialLayersFunctions", Obj, {
        "layers": lambda: [None], "blends": list,
        "editor_only": lambda: Obj(layer_guids=[Obj(text="background")])})()})
stand_in.MaterialParameterCollection = engine_class("MaterialParameterCollection")
stand_in.MaterialParameterCollection.get_vector_parameter_names = \
    lambda self: [slot.get_editor_property("parameter_name") for slot in self.props.get("vector_parameters", [])]
sys.modules["unreal"] = stand_in

failed = False
for script in sys.argv[1:]:
    path = PROJECT + script
    try:
        exec(compile(open(path, encoding="ascii").read(), path, "exec"), {"__name__": "__main__"})
        print("OK", script)
    except Exception:
        failed = True
        print("FAILED", script)
        traceback.print_exc()

sys.exit(1 if failed else 0)
