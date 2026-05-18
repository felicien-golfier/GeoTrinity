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

# Remove a state
cdo.remove_state(st, "MyState")
