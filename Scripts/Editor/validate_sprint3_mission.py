"""Structural validation for Yusuf's reusable Mission gameplay foundation."""

from pathlib import Path

import unreal


MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint3_MissionTest"
SPRINT2_MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint2_CarryTest"
SPRINT3_MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint3_HeavyCarryTest"
PROTOTYPE_GAME_MODE_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMGameMode_Prototype.BP_OMGameMode_Prototype_C"
)
PROTOTYPE_CHARACTER_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMMouseCharacter_Prototype.BP_OMMouseCharacter_Prototype_C"
)


def fail(message):
    unreal.log_error(f"OM_SPRINT3_MISSION_VALIDATION|FAIL|{message}")
    raise RuntimeError(message)


def validate_source_contract():
    root = Path(unreal.Paths.project_dir()) / "Source" / "OperationMouse" / "Mission"
    definition_h = (root / "OMMissionDefinition.h").read_text(encoding="utf-8")
    manager_h = (root / "OMMissionManager.h").read_text(encoding="utf-8")
    manager_cpp = (root / "OMMissionManager.cpp").read_text(encoding="utf-8")
    trigger_h = (root / "OMMissionInteractionActor.h").read_text(encoding="utf-8")
    trigger_cpp = (root / "OMMissionInteractionActor.cpp").read_text(encoding="utf-8")
    interaction_root = Path(unreal.Paths.project_dir()) / "Source" / "OperationMouse" / "Interaction"
    interaction_h = (interaction_root / "OMInteractionComponent.h").read_text(encoding="utf-8")
    interaction_cpp = (interaction_root / "OMInteractionComponent.cpp").read_text(encoding="utf-8")
    combined = definition_h + manager_h + manager_cpp + trigger_h + trigger_cpp
    for token in (
        "UOMMissionDefinition",
        "MissionId",
        "ObjectiveTarget",
        "EOMMissionState",
        "Inactive",
        "Active",
        "Completed",
        "Failed",
        "StartMission",
        "CompleteObjective",
        "FailMission",
        "ResetMission",
        "RetryMission",
        "[Mission][State]",
        "[Mission][Reject]",
        "[Mission][Request]",
        "[Mission][Replication]",
        "AOMMissionInteractionActor",
        "IOMInteractableInterface",
        "bObjectiveConsumed",
        "ReplicatedUsing = OnRep_MissionState",
        "ReplicatedUsing = OnRep_ObjectiveProgress",
        "ReplicatedUsing = OnRep_ObjectiveConsumed",
        "DOREPLIFETIME(AOMMissionManager, MissionState)",
        "DOREPLIFETIME(AOMMissionManager, ObjectiveProgress)",
        "DOREPLIFETIME(AOMMissionInteractionActor, bObjectiveConsumed)",
        "HasAuthority()",
        "bReplicates = true",
    ):
        if token not in combined:
            fail(f"Mission foundation token missing: {token}")
    for token in ("ServerBeginInteraction", "IsServerInteractionValid", "Execute_BeginInteraction", "Execute_CompleteInteraction"):
        if token not in interaction_h + interaction_cpp:
            fail(f"Existing authoritative interaction flow token missing: {token}")
    if "UFUNCTION(Server" in combined:
        fail("Mission actors must reuse InteractionComponent's existing Server RPC instead of adding another RPC path")
    unreal.log("OM_SPRINT3_MISSION_VALIDATION|PASS|MISSION_REPLICATION_AND_INTERACTION_CONTRACT")


def validate():
    validate_source_contract()
    if not unreal.EditorAssetLibrary.does_asset_exist(SPRINT2_MAP_PATH):
        fail("Sprint 2 Carry regression map is missing")
    if not unreal.EditorAssetLibrary.does_asset_exist(SPRINT3_MAP_PATH):
        fail("Sprint 3 Heavy Carry regression map is missing")
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH):
        fail(f"Could not load {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(actor_subsystem.get_all_level_actors())
    manager_class = unreal.load_class(None, "/Script/OperationMouse.OMMissionManager")
    trigger_class = unreal.load_class(None, "/Script/OperationMouse.OMMissionInteractionActor")
    if None in (manager_class, trigger_class):
        fail("Mission manager or interaction actor class is missing")
    managers = [actor for actor in actors if actor.get_class() == manager_class]
    triggers = [actor for actor in actors if actor.get_class() == trigger_class]
    if len(managers) != 1:
        fail(f"Expected one Mission Manager, found {len(managers)}")
    if len(triggers) != 5:
        fail(f"Expected Start/Objective/Fail/Reset/Retry fixtures, found {len(triggers)}")
    if any(trigger.get_editor_property("mission_manager") != managers[0] for trigger in triggers):
        fail("Every Mission fixture must reference the single Mission Manager")

    expected_labels = {
        "Mission_Start", "Mission_Objective", "Mission_Fail", "Mission_Reset", "Mission_Retry"
    }
    if {trigger.get_actor_label() for trigger in triggers} != expected_labels:
        fail("Mission fixture labels/actions are incomplete")
    starts = [actor for actor in actors if actor.get_class().get_name() == "PlayerStart"]
    if len(starts) < 2:
        fail(f"Expected two PlayerStarts, found {len(starts)}")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    active_game_mode = world.get_world_settings().get_editor_property("default_game_mode")
    prototype_game_mode = unreal.load_class(None, PROTOTYPE_GAME_MODE_PATH)
    prototype_character = unreal.load_class(None, PROTOTYPE_CHARACTER_PATH)
    if active_game_mode != prototype_game_mode:
        fail(f"Wrong Mission test GameMode: {active_game_mode}")
    if unreal.get_default_object(prototype_game_mode).get_editor_property("default_pawn_class") != prototype_character:
        fail("Mission test map does not spawn BP_OMMouseCharacter_Prototype")
    if not world.get_world_settings().get_editor_property("force_no_precomputed_lighting"):
        fail("Mission graybox still depends on baked lighting")

    labels = [actor for actor in actors if actor.get_actor_label().startswith("Mission_Label_")]
    if len(labels) != 5:
        fail(f"Expected five readable Mission labels, found {len(labels)}")
    for actor in labels:
        rotation = actor.get_actor_rotation()
        scale = actor.get_actor_scale3d()
        if abs(abs(rotation.yaw) - 180.0) > 0.1 or min(scale.x, scale.y, scale.z) <= 0.0:
            fail(f"Unreadable Mission label: {actor.get_actor_label()}")

    unreal.SystemLibrary.execute_console_command(world, "MAP CHECK")
    unreal.log(
        "OM_SPRINT3_MISSION_VALIDATION|PASS|"
        "manager=1|fixtures=5|starts=2|prototype_game_mode=present|"
        "daylight_graybox=present|map_check=executed|"
        "replicated_state=present|existing_server_interaction_flow=present|"
        "manual_network_evidence=pending"
    )
    unreal.log("OM_SPRINT3_MISSION_VALIDATION|FINAL|PASS")


validate()
