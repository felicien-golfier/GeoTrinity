# Adds the couch-coop "Use first gamepad for second player" row to WBP_KeyBindings.
# The checkbox name must stay SecondPlayerGamepadCheckBox — UGeoKeyBindingsWidget BindWidgetOptional's it.
# Re-run safe: the tree primitives remove any existing widget of the same name first.
import unreal

WBP_PATH = "/Game/HUD/InGameMenu/WBP_KeyBindings"
ROW_NAME = "SecondPlayerGamepadRow"
LABEL_NAME = "SecondPlayerGamepadLabel"
CHECKBOX_NAME = "SecondPlayerGamepadCheckBox"
LABEL_TEXT = "Use first gamepad for second player"
# Widgets from the superseded "Use keyboard for Player 1" version of this row.
LEGACY_NAMES = ["KeyboardPlayerRow", "KeyboardPlayerLabel", "KeyboardPlayerCheckBox"]
# Between KeyBindingsList and BackButton, so Back stays the last row.
ROW_INDEX = 2

util = unreal.GeoWidgetBuilderUtil
wbp = unreal.load_asset(WBP_PATH)

for legacy in LEGACY_NAMES:
    util.remove_widget(wbp, legacy)

row = util.construct_widget_in_tree(wbp, unreal.HorizontalBox, ROW_NAME, False)
label = util.construct_widget_in_tree(wbp, unreal.TextBlock, LABEL_NAME, False)
checkbox = util.construct_widget_in_tree(wbp, unreal.CheckBox, CHECKBOX_NAME, True)

util.attach_widget(wbp, "Root", ROW_NAME, ROW_INDEX)
label_slot = util.attach_widget(wbp, ROW_NAME, LABEL_NAME, -1)
checkbox_slot = util.attach_widget(wbp, ROW_NAME, CHECKBOX_NAME, -1)

font = label.get_editor_property("font")
font.size = 16
font.typeface_font_name = "Regular"
label.set_editor_property("font", font)
label.set_text(unreal.Text(LABEL_TEXT))

label_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 12.0, 0.0))
label_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
checkbox_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)

row.slot.set_editor_property("padding", unreal.Margin(0.0, 12.0, 0.0, 12.0))
row.slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
row.slot.set_editor_property("size", unreal.SlateChildSize(1.0, unreal.SlateSizeRule.AUTOMATIC))

util.commit_tree(wbp)

for i, child in enumerate(util.find_widget(wbp, "Root").get_all_children()):
    unreal.log_warning("GEOKB root child %d = %s (%s)" % (i, child.get_name(), child.get_class().get_name()))
