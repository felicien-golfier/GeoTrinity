"""
StateTree editing via GeoStateTreeBuilderUtil.
Reference: AI/MCP/MCP_StateTree.md
"""
import unreal

cdo = unreal.GeoStateTreeBuilderUtil.get_default_object()
st  = unreal.load_asset("/Game/AI/ST_EnemyBehaviour")

# Inspect
cdo.list_states(st)

# Add a state at root
cdo.add_fire_ability_state_by_tag_name(st, "MyState", "Ability.Spell.MyAbility", None, -1)

# Add a child state under a parent
cdo.add_fire_ability_state_by_tag_name(st, "MyChild", "Ability.Spell.MyAbility", "ParentState", -1)

# Replace a task tag in an existing state
cdo.replace_fire_ability_tag_in_state(st, "MyState", "Ability.Spell.OtherAbility")

# Transitions
cdo.clear_transitions(st, "MyState")
cdo.add_transition(st, "MyState",  "NextState", unreal.StateTreeTransitionTrigger.ON_STATE_SUCCEEDED)
cdo.add_transition(st, "MyState",  "Root",      unreal.StateTreeTransitionTrigger.ON_STATE_FAILED)
cdo.add_transition(st, "MyState",  "SomeState", unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED)

# Float enter condition (e.g. HP < 50%)
cdo.add_float_enter_condition(st, "MyState", 0.5, unreal.GenericAICheck.LESS, False)
# Bind condition property to a Property Function (call immediately after add_float_enter_condition)
# ConditionIndex=0, ConditionProperty="Left", FuncStruct="FSTGetHealthRatioPropertyFunction",
# FuncOutput="Output", FuncInput="Input", ContextClass="GeoEnemyAIController"
cdo.bind_condition_property_to_property_function(
    st, "MyState", 0, "Left",
    "FSTGetHealthRatioPropertyFunction", "Output", "Input",
    "GeoEnemyAIController")

# Add STTask_SendEventAfterNCycles to an existing state
# cdo.add_send_event_after_n_cycles_task(st, "StateName", CyclesRequired, "Event.Tag.Name")

# Clear all enter conditions from a state
# cdo.clear_enter_conditions(st, "StateName")

# Set the Required Event To Enter on a state
# cdo.set_required_event_to_enter(st, "StateName", "Event.Tag.Name")

# Log all enter conditions on a state
# cdo.list_enter_conditions(st, "StateName")

# Remove a state
cdo.remove_state(st, "MyState")
