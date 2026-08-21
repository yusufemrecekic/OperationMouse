"""Structural validation for Yusuf's Heavy Carry gameplay foundation."""

from pathlib import Path

import unreal


MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint3_HeavyCarryTest"
SPRINT2_MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint2_CarryTest"
PROTOTYPE_GAME_MODE_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMGameMode_Prototype.BP_OMGameMode_Prototype_C"
)
PROTOTYPE_CHARACTER_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMMouseCharacter_Prototype.BP_OMMouseCharacter_Prototype_C"
)


def fail(message):
    unreal.log_error(f"OM_SPRINT3_HEAVY_VALIDATION|FAIL|{message}")
    raise RuntimeError(message)


def validate_source_contract():
    root = Path(unreal.Paths.project_dir()) / "Source" / "OperationMouse"
    heavy_h = (root / "Carry" / "OMHeavyCarryableActor.h").read_text(encoding="utf-8")
    heavy_cpp = (root / "Carry" / "OMHeavyCarryableActor.cpp").read_text(encoding="utf-8")
    character_cpp = (root / "Characters" / "OMMouseCharacter.cpp").read_text(encoding="utf-8")
    required = (
        "WaitingForSecondHolder",
        "LeftCarrySlot",
        "RightCarrySlot",
        "AlignCarriersToSlots",
        "RefreshCarrierCollisionIgnores",
        "IgnoreActorWhenMoving",
        "GetMoveIgnoreActors",
        "MoveIgnoreActorAdd",
        "MoveIgnoreActorRemove",
		"SetHeavyCarryObstructed",
		"ConstrainHolderMovement",
		"HeavyObstructionNormal",
		"MinimumStableSeparation",
		"SetActorLocationAndRotation",
		"ECollisionEnabled::QueryOnly",
        "EOMHeavyCarryState::Carrying",
        "ActiveCarriers.Num() >= 2",
        "SetMovementPenaltyForAllHolders(true)",
        "SetMovementPenaltyForAllHolders(false)",
        "ReleaseForRecovery(this)",
        "RestoreWorldPresentation(HeavyHomeTransform)",
    )
    combined = heavy_h + heavy_cpp
    for token in required:
        if token not in combined:
            fail(f"Heavy Carry gameplay token missing: {token}")
    if heavy_cpp.count("AlignCarriersToSlots();") != 1:
        fail("Heavy Carry holder alignment must run once on join, not force-move holders every Tick")
    if "SetHeavyCarryMovementPenaltyActive" not in character_cpp:
        fail("Character movement-penalty gameplay hook is missing")
    carryable_h = (root / "Carry" / "OMCarryableActor.h").read_text(encoding="utf-8")
    carryable_cpp = (root / "Carry" / "OMCarryableActor.cpp").read_text(encoding="utf-8")
    for token in (
        "MinimumCarryDistance",
        "CarryClearance",
        "Mesh->Bounds.SphereRadius",
        "SetActorLocationAndRotation",
        "ECollisionEnabled::QueryOnly",
        "ApplyHolderCollisionIgnores",
        "ClearHolderCollisionIgnores",
    ):
        if token not in carryable_h + carryable_cpp:
            fail(f"Bounds-aware normal Carry placement token missing: {token}")
    if "AttachToComponent(NewCarryPoint" in carryable_cpp:
        fail("Normal Carry still attaches cargo directly and bypasses obstruction sweeps")
    for token in (
        "ReplicatedUsing = OnRep_HeavyCarryNetworkState",
        "DOREPLIFETIME(AOMHeavyCarryableActor, HeavyCarryState)",
        "DOREPLIFETIME(AOMHeavyCarryableActor, ReplicatedFirstHolder)",
        "DOREPLIFETIME(AOMHeavyCarryableActor, ReplicatedSecondHolder)",
        "ApplyReplicatedCarryPresentation",
    ):
        if token not in combined:
            fail(f"Heavy Carry collision-consistency token missing: {token}")
    if "UFUNCTION(Server" in heavy_h or "NetMulticast" in combined:
        fail("Heavy Carry added a custom movement RPC instead of replicated collision consistency")
    unreal.log("OM_SPRINT3_HEAVY_VALIDATION|PASS|GAMEPLAY_AND_COLLISION_CONSISTENCY_SOURCE_CONTRACT")


def validate():
    validate_source_contract()
    if not unreal.EditorAssetLibrary.does_asset_exist(SPRINT2_MAP_PATH):
        fail("Accepted Sprint 2 carry regression map is missing")
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH):
        fail(f"Could not load {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(actor_subsystem.get_all_level_actors())
    normal_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryableActor")
    heavy_class = unreal.load_class(None, "/Script/OperationMouse.OMHeavyCarryableActor")
    if None in (normal_class, heavy_class):
        fail("Carry actor classes are missing")
    normal = [actor for actor in actors if actor.get_class() == normal_class]
    heavy = [actor for actor in actors if actor.get_class() == heavy_class]
    if len(normal) != 1 or len(heavy) != 1:
        fail(f"Expected one normal and one Heavy Carryable, found normal={len(normal)} heavy={len(heavy)}")

    heavy_cdo = unreal.get_default_object(heavy_class)
    left_slot = heavy_cdo.get_editor_property("left_carry_slot")
    right_slot = heavy_cdo.get_editor_property("right_carry_slot")
    if left_slot is None or right_slot is None:
        fail("Heavy Carryable does not expose LeftCarrySlot and RightCarrySlot")
    if left_slot.get_editor_property("relative_location").y >= 0.0:
        fail("LeftCarrySlot is not on the left side")
    if right_slot.get_editor_property("relative_location").y <= 0.0:
        fail("RightCarrySlot is not on the right side")

    starts = [actor for actor in actors if actor.get_class().get_name() == "PlayerStart"]
    if len(starts) < 2:
        fail(f"Expected two PlayerStarts, found {len(starts)}")
    reset = [actor for actor in actors if actor.get_actor_label() == "Sprint3_Reset"]
    if len(reset) != 1:
        fail(f"Expected one Sprint 3 Reset fixture, found {len(reset)}")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    prototype_game_mode = unreal.load_class(None, PROTOTYPE_GAME_MODE_PATH)
    prototype_character = unreal.load_class(None, PROTOTYPE_CHARACTER_PATH)
    active_game_mode = world.get_world_settings().get_editor_property("default_game_mode")
    if active_game_mode != prototype_game_mode:
        fail(f"Wrong Sprint 3 GameMode: {active_game_mode}")
    if unreal.get_default_object(prototype_game_mode).get_editor_property("default_pawn_class") != prototype_character:
        fail("Sprint 3 map does not spawn BP_OMMouseCharacter_Prototype")

    required_daylight = {
        "Sprint2_DirectionalLight",
        "Sprint2_SkyLight",
        "Sprint2_SkyAtmosphere",
        "Sprint2_PostProcess",
    }
    labels = {actor.get_actor_label() for actor in actors}
    missing = sorted(required_daylight - labels)
    if missing:
        fail(f"Accepted daylight rig is incomplete: {missing}")
    if not world.get_world_settings().get_editor_property("force_no_precomputed_lighting"):
        fail("Sprint 3 graybox still depends on baked lighting")

    floor = next((actor for actor in actors if actor.get_actor_label() == "Sprint3_Floor"), None)
    if floor is None:
        fail("Compact Sprint 3 floor is missing")
    floor_scale = floor.get_actor_scale3d()
    if floor_scale.x > 24.0 or floor_scale.y > 16.0:
        fail(f"Sprint 3 technical floor is still oversized: {floor_scale}")

    text_actors = [actor for actor in actors if actor.get_actor_label().startswith("Sprint3_Label_")]
    if len(text_actors) < 4:
        fail(f"Expected four readable route labels, found {len(text_actors)}")
    for actor in text_actors:
        rotation = actor.get_actor_rotation()
        scale = actor.get_actor_scale3d()
        if abs(abs(rotation.yaw) - 180.0) > 0.1:
            fail(f"Unreadable label rotation: {actor.get_actor_label()} {rotation}")
        if min(scale.x, scale.y, scale.z) <= 0.0:
            fail(f"Mirrored label scale: {actor.get_actor_label()} {scale}")

    unreal.SystemLibrary.execute_console_command(world, "MAP CHECK")
    unreal.log(
        "OM_SPRINT3_HEAVY_VALIDATION|PASS|"
        "normal_carry_regression_fixture=1|heavy_carry_fixture=1|starts=2|"
        "prototype_game_mode=present|daylight_graybox=present|map_check=executed|"
        "network_movement_architecture=intentionally_pending|collision_consistency=present"
    )
    unreal.log("OM_SPRINT3_HEAVY_VALIDATION|FINAL|PASS")


validate()
