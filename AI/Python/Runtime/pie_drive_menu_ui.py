"""
Drive live UMG widgets in a running PIE session without input simulation:
find the widget instance, invoke a click handler by name, and enter text into an input field.
Usage: run via MCP execute_script (world: pie). Adjust the example call at the bottom.
"""
import unreal


def find_live_widgets(generated_class_name):
    """Live PIE widget instances: generated class name + transient path + valid world."""
    found = []
    for obj in unreal.ObjectIterator():
        try:
            if (obj.get_class().get_name() == generated_class_name
                    and 'Transient' in obj.get_path_name()
                    and obj.get_world()):
                found.append(obj)
        except Exception:
            pass
    return found


def click(widget, handler_name):
    """Invoke a reflected function (including a private UFUNCTION click handler) by exact name."""
    widget.call_method(handler_name)


def enter_text(widget, input_property_name, text):
    """Write into a bound text box; the value must be wrapped in unreal.Text — plain str is rejected."""
    widget.get_editor_property(input_property_name).set_text(unreal.Text(text))


def click_all(generated_class_name, handler_name):
    """Invoke handler_name on every live instance of the widget class."""
    for widget in find_live_widgets(generated_class_name):
        click(widget, handler_name)


def enter_text_all(generated_class_name, input_property_name, text):
    """Enter text into input_property_name on every live instance of the widget class."""
    for widget in find_live_widgets(generated_class_name):
        enter_text(widget, input_property_name, text)


# --- Example: open the Play Local panel and fill the IP field ---
click_all('WBP_MainMenuWidget_C', 'HandlePlayLocal')
enter_text_all('WBP_LocalConnect_C', 'IPInput', '127.0.0.1')
