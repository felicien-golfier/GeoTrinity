"""
Inject Enhanced Input actions into a running PIE session and measure the gameplay result.
Runs a phase-driven probe: for each phase it holds an Input Action at a given value for N frames
(re-injected every frame via a Slate post-tick callback), waits a settle period, then logs a JSON
result line (pawn location delta, actor count of a watched class) under a grep-able marker.
Injection enters below the viewport input gate, so it tests the binding/ability pipeline even when
real keys are blocked by the current input mode.
Usage: run via MCP execute_script while PIE is active, then read the marker lines with the
output-log tool (category LogPython). Adjust the example call at the bottom.
"""
import json

import unreal


def find_local_player_world():
    """Return (world, player_controller) for the PIE world owning the local player."""
    for w in unreal.ObjectIterator(unreal.World):
        if 'UEDPIE' in w.get_path_name():
            pc = unreal.GameplayStatics.get_player_controller(w, 0)
            if pc and pc.is_local_player_controller():
                return w, pc
    return None, None


def find_live_input_subsystems():
    """All live Enhanced Input local-player subsystems.

    Iteration also returns instances left over from earlier PIE sessions; injecting into every
    live instance is the safe way to reach the current one.
    """
    return [o for o in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
            if 'Default__' not in o.get_name()]


def run_input_probe(phases, watched_actor_class_path, marker='INPUTPROBE'):
    """Drive the phase list and log one JSON result line per phase under the marker.

    phases: list of dicts {'name', 'action' (loaded InputAction asset), 'vec' (x,y,z tuple),
            'hold' (frames injected), 'settle' (frames waited before sampling)}.
    watched_actor_class_path: script/asset path of an actor class whose world count is sampled
            (e.g. a projectile base class) — pass None to skip.
    """
    world, pc = find_local_player_world()
    pawn = pc.get_controlled_pawn()  # plain pawn getter is not exposed to Python
    subsystems = find_live_input_subsystems()
    watched_cls = unreal.load_class(None, watched_actor_class_path) if watched_actor_class_path else None

    state = {'phase': 0, 'tick': 0, 'start': None, 'handle': None, 'peak': 0}

    def snap():
        loc = pawn.get_actor_location()
        entry = {'loc': [round(loc.x, 1), round(loc.y, 1), round(loc.z, 1)]}
        if watched_cls:
            actors = unreal.GameplayStatics.get_all_actors_of_class(world, watched_cls)
            entry['actor_total'] = len(actors)
            entry['actor_visible'] = sum(0 if a.get_editor_property('hidden') else 1 for a in actors)
        return entry

    def tick(_dt):
        try:
            if state['phase'] >= len(phases):
                unreal.unregister_slate_post_tick_callback(state['handle'])
                unreal.log_warning(f'{marker}_DONE')
                return
            ph = phases[state['phase']]
            if state['tick'] == 0:
                state['start'] = snap()
                state['peak'] = state['start'].get('actor_visible', 0)
            if state['tick'] < ph['hold']:
                for s in subsystems:
                    s.inject_input_vector_for_action(ph['action'], unreal.Vector(*ph['vec']), [], [])
            state['peak'] = max(state['peak'], snap().get('actor_visible', 0))
            state['tick'] += 1
            if state['tick'] >= ph['hold'] + ph['settle']:
                end = snap()
                dist = unreal.Vector(*(e - s for e, s in zip(end['loc'], state['start']['loc']))).length()
                unreal.log_warning(marker + ' ' + json.dumps(
                    {'phase': ph['name'], 'start': state['start'], 'end': end,
                     'moved_dist': round(dist, 1), 'peak_visible': state['peak']}))
                state['phase'] += 1
                state['tick'] = 0
        except Exception as exc:
            try:
                unreal.unregister_slate_post_tick_callback(state['handle'])
            except Exception:
                pass
            unreal.log_warning(f'{marker}_ERR {exc}')

    state['handle'] = unreal.register_slate_post_tick_callback(tick)
    unreal.log_warning(f'{marker}_START phases={len(phases)} subsystems={len(subsystems)}')


def load_input_action(name, folder='/Game/Input/InputActions'):
    return unreal.load_asset(f'{folder}/{name}.{name}')


# --- Example: GeoTrinity ability inputs against the projectile base class ---
run_input_probe(
    phases=[
        {'name': 'move',    'action': load_input_action('MoveAction'),            'vec': (0.0, 1.0, 0.0), 'hold': 30, 'settle': 5},
        {'name': 'basic',   'action': load_input_action('IA_LaunchSpell_Basic'),  'vec': (1.0, 0.0, 0.0), 'hold': 45, 'settle': 20},
        {'name': 'dash',    'action': load_input_action('IA_Dash'),               'vec': (1.0, 0.0, 0.0), 'hold': 3,  'settle': 30},
        {'name': 'special', 'action': load_input_action('IA_LaunchSpell_Special'),'vec': (1.0, 0.0, 0.0), 'hold': 45, 'settle': 20},
    ],
    watched_actor_class_path='/Script/GeoTrinity.GeoProjectile',
    marker='INPUTPROBE',
)
