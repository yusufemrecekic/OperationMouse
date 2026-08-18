"""Read-only structural validation for the Sprint 2 Carry gameplay foundation."""

from pathlib import Path

import unreal


MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint2_CarryTest"
SPRINT1_MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Phase5_InteractionTest"
PROTOTYPE_CHARACTER_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMMouseCharacter_Prototype.BP_OMMouseCharacter_Prototype_C"
)
PROTOTYPE_GAME_MODE_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMGameMode_Prototype.BP_OMGameMode_Prototype_C"
)
FLOOR_MATERIAL_PATH = "/Game/OperationMouse/Tests/Materials/MI_Sprint2_FloorGray"
CARRYABLE_MATERIAL_PATH = "/Game/OperationMouse/Tests/Materials/MI_Sprint2_CarryableGray"


def fail(message):
    unreal.log_error(f"OM_SPRINT2_VALIDATION|FAIL|{message}")
    raise RuntimeError(message)


def validate_network_contract_source():
    source_root = Path(unreal.Paths.project_dir()) / "Source" / "OperationMouse"
    files = {
        "carry_h": (source_root / "Carry" / "OMCarryComponent.h").read_text(encoding="utf-8"),
        "carry_cpp": (source_root / "Carry" / "OMCarryComponent.cpp").read_text(encoding="utf-8"),
        "actor_h": (source_root / "Carry" / "OMCarryableActor.h").read_text(encoding="utf-8"),
        "actor_cpp": (source_root / "Carry" / "OMCarryableActor.cpp").read_text(encoding="utf-8"),
        "interaction_cpp": (source_root / "Interaction" / "OMInteractionComponent.cpp").read_text(encoding="utf-8"),
        "character_cpp": (source_root / "Characters" / "OMMouseCharacter.cpp").read_text(encoding="utf-8"),
    }
    required_tokens = {
        "carry_h": (
            "ReplicatedUsing = OnRep_CarriedActor",
            "UFUNCTION(Server, Reliable)",
            "ServerRequestDrop",
        ),
        "carry_cpp": (
            "SetIsReplicatedByDefault(true)",
            "DOREPLIFETIME(UOMCarryComponent, CarriedActor)",
            "Reason=HolderMismatch",
            "AlreadyCarrying",
            "CarryableUnavailable",
        ),
        "actor_h": (
            "ReplicatedUsing = OnRep_CurrentHolder",
            "ReplicatedUsing = OnRep_WorldStateRevision",
            "AuthoritativeWorldTransform",
        ),
        "actor_cpp": (
            "DOREPLIFETIME(AOMCarryableActor, CurrentHolder)",
            "DOREPLIFETIME(AOMCarryableActor, AuthoritativeWorldTransform)",
            "DOREPLIFETIME(AOMCarryableActor, WorldStateRevision)",
            "ApplyCarryPresentation",
            "PublishAuthoritativeWorldState",
            "SetPhysicsLinearVelocity(FVector::ZeroVector)",
            "SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector)",
        ),
        "interaction_cpp": (
            "ServerBeginInteraction_Implementation",
            "Execute_CompleteInteraction",
        ),
        "character_cpp": ("CarryComponent->RequestDrop()",),
    }
    for file_key, tokens in required_tokens.items():
        for token in tokens:
            if token not in files[file_key]:
                fail(f"Network Carry contract token missing in {file_key}: {token}")

    unreal.log("OM_SPRINT2_VALIDATION|PASS|NETWORK_SOURCE_CONTRACT")


def validate():
    validate_network_contract_source()
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(SPRINT1_MAP_PATH):
        fail("Accepted Sprint 1 interaction map is missing")
    if not level_subsystem.load_level(MAP_PATH):
        fail(f"Could not load {MAP_PATH}")

    carry_component_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryComponent")
    carryable_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryableActor")
    character_class = unreal.load_class(None, "/Script/OperationMouse.OMMouseCharacter")
    interaction_class = unreal.load_class(None, "/Script/OperationMouse.OMInteractionComponent")
    interface_class = unreal.load_class(None, "/Script/OperationMouse.OMInteractableInterface")
    if None in (carry_component_class, carryable_class, character_class, interaction_class, interface_class):
        fail("A required Carry or Sprint 1 interaction class is missing")

    character_cdo = unreal.get_default_object(character_class)
    if character_cdo.get_editor_property("carry_component") is None:
        fail("AOMMouseCharacter does not create CarryComponent")
    if character_cdo.get_editor_property("carry_point") is None:
        fail("AOMMouseCharacter does not create CarryPoint")
    if character_cdo.get_editor_property("interaction_component") is None:
        fail("Sprint 1 InteractionComponent regression")
    if not character_cdo.get_editor_property("use_controller_rotation_yaw"):
        fail("AOMMouseCharacter is not aligned to controller yaw")
    movement_component = character_cdo.get_editor_property("character_movement")
    if movement_component.get_editor_property("orient_rotation_to_movement"):
        fail("CharacterMovement still overrides controller-yaw facing")

    actors = list(actor_subsystem.get_all_level_actors())
    carryables = [actor for actor in actors if actor.get_class() == carryable_class]
    if len(carryables) != 2:
        fail(f"Expected 2 carryables, found {len(carryables)}")
    if len({actor.get_actor_label() for actor in carryables}) != 2:
        fail("Carryable labels are not unique")
    if not unreal.EditorAssetLibrary.does_asset_exist(FLOOR_MATERIAL_PATH):
        fail("Sprint 2 medium-gray floor material is missing")
    if not unreal.EditorAssetLibrary.does_asset_exist(CARRYABLE_MATERIAL_PATH):
        fail("Sprint 2 dark-gray Carryable material is missing")
    for actor in carryables:
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        material = component.get_material(0) if component else None
        if material is None or not material.get_path_name().startswith(CARRYABLE_MATERIAL_PATH):
            fail(f"Dark-gray Carryable material missing on {actor.get_actor_label()}")
    carryable_cdo = unreal.get_default_object(carryable_class)
    if not carryable_cdo.get_editor_property("replicates"):
        fail("AOMCarryableActor replication is disabled")
    if not carryable_cdo.get_editor_property("replicate_movement"):
        fail("AOMCarryableActor movement replication is disabled")

    reset_actors = [
        actor
        for actor in actors
        if actor.get_class().get_name() == "OMTestInteractableActor"
        and str(actor.get_editor_property("test_role")).split(".")[-1].split(":")[0] == "RESET"
    ]
    if len(reset_actors) != 1:
        fail(f"Expected one Reset fixture, found {len(reset_actors)}")

    starts = [actor for actor in actors if actor.get_class().get_name() == "PlayerStart"]
    if len(starts) < 2:
        fail(f"Expected at least two PlayerStarts, found {len(starts)}")

    prototype_game_mode = unreal.load_class(None, PROTOTYPE_GAME_MODE_PATH)
    prototype_character = unreal.load_class(None, PROTOTYPE_CHARACTER_PATH)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    active_game_mode = world.get_world_settings().get_editor_property("default_game_mode")
    if active_game_mode != prototype_game_mode:
        fail(f"Wrong GameMode: {active_game_mode}")
    if unreal.get_default_object(prototype_game_mode).get_editor_property("default_pawn_class") != prototype_character:
        fail("Prototype GameMode does not spawn BP_OMMouseCharacter_Prototype")
    if not world.get_world_settings().get_editor_property("force_no_precomputed_lighting"):
        fail("Sprint 2 map still depends on precomputed lighting")

    if any(actor.get_actor_label().startswith("Sprint2_FillLight_") for actor in actors):
        fail("Obsolete high-intensity fill lights remain in the Sprint 2 map")
    required_daylight = {
        "Sprint2_DirectionalLight",
        "Sprint2_SkyLight",
        "Sprint2_SkyAtmosphere",
        "Sprint2_PostProcess",
    }
    actor_labels = {actor.get_actor_label() for actor in actors}
    missing_daylight = sorted(required_daylight - actor_labels)
    if missing_daylight:
        fail(f"Missing neutral daylight actors: {missing_daylight}")
    directional = next(
        actor for actor in actors if actor.get_actor_label() == "Sprint2_DirectionalLight"
    )
    directional_component = directional.get_component_by_class(unreal.DirectionalLightComponent)
    if directional_component is None or directional_component.get_editor_property("intensity") < 50.0:
        fail("Directional Light is not configured for a real daylight exposure")
    post_process = next(
        actor for actor in actors if actor.get_actor_label() == "Sprint2_PostProcess"
    )
    if not post_process.get_editor_property("unbound"):
        fail("Sprint 2 technical exposure is not fixed globally")
    post_settings = post_process.get_editor_property("settings")
    if post_settings.get_editor_property("auto_exposure_method") != unreal.AutoExposureMethod.AEM_HISTOGRAM:
        fail("Sprint 2 exposure method is not the fixed daylight histogram setup")
    if abs(post_settings.get_editor_property("auto_exposure_min_brightness") - 15.0) > 0.01:
        fail("Sprint 2 minimum exposure is not fixed to EV100 15")
    if abs(post_settings.get_editor_property("auto_exposure_max_brightness") - 15.0) > 0.01:
        fail("Sprint 2 maximum exposure is not fixed to EV100 15")
    if post_settings.get_editor_property("bloom_intensity") > 0.01:
        fail("Sprint 2 technical map bloom is not disabled")

    floor = next((actor for actor in actors if actor.get_actor_label() == "Sprint2_Floor"), None)
    floor_component = floor.get_component_by_class(unreal.StaticMeshComponent) if floor else None
    floor_material = floor_component.get_material(0) if floor_component else None
    if floor_material is None or not floor_material.get_path_name().startswith(FLOOR_MATERIAL_PATH):
        fail("Medium-gray floor material is not assigned")

    route_x = sorted(actor.get_actor_location().x for actor in carryables + reset_actors)
    if route_x[1] - route_x[0] < 500.0 or route_x[2] - route_x[1] < 400.0:
        fail(f"Carry route fixtures are too close: {route_x}")

    for actor in actors:
        if not actor.get_actor_label().startswith("Sprint2_Label_"):
            continue
        rotation = actor.get_actor_rotation()
        scale = actor.get_actor_scale3d()
        if abs(abs(rotation.yaw) - 180.0) > 0.1 or abs(rotation.pitch) > 0.1 or abs(rotation.roll) > 0.1:
            fail(f"Unreadable label rotation: {actor.get_actor_label()} {rotation}")
        if scale.x <= 0.0 or scale.y <= 0.0 or scale.z <= 0.0:
            fail(f"Mirrored label scale: {actor.get_actor_label()} {scale}")

    unreal.log(
        "OM_SPRINT2_VALIDATION|PASS|"
        f"carryables={len(carryables)}|starts={len(starts)}|reset={len(reset_actors)}|"
        "carry_component=present|interaction_regression=present|prototype_game_mode=present|"
        "controller_yaw=present|neutral_daylight_debug_lighting=present|"
        "authoritative_world_reconciliation=present"
    )
    unreal.log("OM_SPRINT2_VALIDATION|FINAL|PASS")


validate()
