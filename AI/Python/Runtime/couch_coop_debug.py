# Dumps couch-coop input ownership from a running PIE session.
# Input routes purely by platform user: UEngine::GetLocalPlayerFromInputDevice matches a device's platform user
# against each ULocalPlayer's, and APlayerController::InputKey drops anything that does not match while
# bFilterInputByPlatformUser is on. So a controller that "does nothing" is almost always a platform-user mismatch.
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    unreal.log_warning("GEOCOOP no PIE world - press Play first")
else:
    for Index in range(4):
        PlayerController = unreal.GameplayStatics.get_player_controller(world, Index)
        if not PlayerController:
            continue
        Pawn = PlayerController.get_controlled_pawn()
        unreal.log_warning(
            "GEOCOOP player %d: pc=%s platform_user=%s local=%s pawn=%s"
            % (
                Index,
                PlayerController.get_name(),
                PlayerController.get_platform_user_id(),
                PlayerController.is_local_player_controller(),
                Pawn.get_name() if Pawn else None,
            )
        )

unreal.log_warning(
    "GEOCOOP bFilterInputByPlatformUser=%s"
    % unreal.get_default_object(unreal.InputSettings).get_editor_property("filter_input_by_platform_user")
)
