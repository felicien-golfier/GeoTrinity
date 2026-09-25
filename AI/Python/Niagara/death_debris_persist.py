"""The three class death-debris systems keep their chunks on the floor until the character revives.

A particle no longer dies of age, and the two modules that ended its life — the alpha fade and the shrink over the
last of its lifetime — are switched off, so the chunks settle and stay. The emitter's own Inactive Response already
lets living particles finish, so the burst ending kills nothing. The character destroys the system on revive.

Needs the bool switch support in GeoNiagaraBuilderUtil.SetStaticSwitch. Run through mcp-unreal execute_script or
AI/Python/Runtime/run_via_bridge.py. Re-runnable. Report written to Saved/death_debris_persist.txt.
"""
import unreal

SYSTEMS = [
    "/Game/Art/VFX/Assets/NS_DeathDebris",
    "/Game/Art/VFX/Assets/NS_DeathDebrisCircle",
    "/Game/Art/VFX/Assets/NS_DeathDebrisTriangle",
]
EMITTER = "UpwardMeshBurst"
UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT
KILL_BY_AGE = "Kill Particles When Lifetime Has Elapsed"
ENDING_MODULES = ["ScaleColor", "ScaleMeshSize"]
REPORT = unreal.Paths.project_saved_dir() + "death_debris_persist.txt"

LOG = []
builder = unreal.GeoNiagaraBuilderUtil.get_default_object()
for system in SYSTEMS:
    immortal = builder.set_static_switch(system, EMITTER, UPDATE, "ParticleState", KILL_BY_AGE, "false")
    disabled = [builder.set_module_enabled(system, EMITTER, UPDATE, module, False) for module in ENDING_MODULES]
    compiled = builder.compile_and_save(system)
    LOG.append("{}: immortal {}, fade/shrink disabled {}, compiled {}".format(system, immortal, disabled, compiled))

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
