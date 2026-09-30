# Builds the four labelled volume rows of WBP_SoundSettings, between the title and BackButton.
# Slider names must stay as below — UGeoSoundSettingsWidget BindWidget's them.
# Re-run safe: the tree primitives remove any existing widget of the same name first.
import unreal

WBP_PATH = "/Game/HUD/InGameMenu/WBP_SoundSettings"
ROWS = [("GeneralVolumeSlider", "General"),
        ("EffectsVolumeSlider", "Sound Effects"),
        ("MusicVolumeSlider", "Music"),
        ("InterfaceVolumeSlider", "Interface")]
# The placeholder single slider the rows replace.
LEGACY_NAMES = ["MasterVolumeSlider"]
LABEL_WIDTH = 220.0
SLIDER_WIDTH = 320.0
# Keyboard and gamepad move by this; the mouse drags freely.
STEP = 0.05

util = unreal.GeoWidgetBuilderUtil
wbp = unreal.load_asset(WBP_PATH)

for legacy in LEGACY_NAMES:
    util.remove_widget(wbp, legacy)

for index, (slider_name, label_text) in enumerate(ROWS):
    base = slider_name.replace("Slider", "")
    row_name, label_box_name, label_name, slider_box_name = (base + "Row", base + "LabelBox", base + "Label",
                                                             base + "SliderBox")
    row = util.construct_widget_in_tree(wbp, unreal.HorizontalBox, row_name, False)
    label_box = util.construct_widget_in_tree(wbp, unreal.SizeBox, label_box_name, False)
    label = util.construct_widget_in_tree(wbp, unreal.TextBlock, label_name, False)
    slider_box = util.construct_widget_in_tree(wbp, unreal.SizeBox, slider_box_name, False)
    slider = util.construct_widget_in_tree(wbp, unreal.Slider, slider_name, True)

    # After SoundLabel, in order.
    util.attach_widget(wbp, "Root", row_name, 1 + index)
    label_box_slot = util.attach_widget(wbp, row_name, label_box_name, -1)
    util.attach_widget(wbp, label_box_name, label_name, -1)
    slider_box_slot = util.attach_widget(wbp, row_name, slider_box_name, -1)
    util.attach_widget(wbp, slider_box_name, slider_name, -1)

    label_box.set_width_override(LABEL_WIDTH)
    slider_box.set_width_override(SLIDER_WIDTH)

    font = label.get_editor_property("font")
    font.size = 18
    font.typeface_font_name = "Regular"
    label.set_editor_property("font", font)
    label.set_text(unreal.Text(label_text))

    slider.set_step_size(STEP)
    # Left/right adjust the focused slider straight away, up/down move between rows.
    slider.set_editor_property("requires_controller_lock", False)

    label_box_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    slider_box_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    row.slot.set_editor_property("padding", unreal.Margin(0.0, 8.0, 0.0, 8.0))
    row.slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
    row.slot.set_editor_property("size", unreal.SlateChildSize(1.0, unreal.SlateSizeRule.AUTOMATIC))

util.commit_tree(wbp)
unreal.EditorAssetLibrary.save_asset(WBP_PATH, only_if_is_dirty=False)

report = ["%d %s (%s)" % (i, child.get_name(), child.get_class().get_name())
          for i, child in enumerate(util.find_widget(wbp, "Root").get_all_children())]
open((unreal.Paths.project_dir() + "AI/Output/") + "volume_sliders.txt", "w").write("\n".join(report))
