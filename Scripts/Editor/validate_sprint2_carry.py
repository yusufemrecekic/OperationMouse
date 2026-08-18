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
        "actor_h": ("ReplicatedUsing = OnRep_CurrentHolder",),
        "actor_cpp": (
            "DOREPLIFETIME(AOMCarryableActor, CurrentHolder)",
            "ApplyCarryPresentation",
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

    fill_lights = [
        actor for actor in actors if actor.get_actor_label().startswith("Sprint2_FillLight_")
    ]
    if len(fill_lights) != 3:
        fail(f"Expected 3 technical fill lights, found {len(fill_lights)}")
    for actor in fill_lights:
        component = actor.get_component_by_class(unreal.PointLightComponent)
        if component is None or component.get_editor_property("mobility") != unreal.ComponentMobility.MOVABLE:
            fail(f"{actor.get_actor_label()} is not a movable PointLight")

    route_x = sorted(actor.get_actor_location().x for actor in carryables + reset_actors)
    if route_x[1] - route_x[0] < 500.0 or route_x[2] - route_x[1] < 400.0:
        fail(f"Carry route fixtures are too close: {route_x}")

    for actor in actors:
        if not actor.get_actor_label().startswith("Sprint2_Label_"):
            continue
        rotation = actor.get_actor_rotation()
        if abs(abs(rotation.yaw) - 180.0) > 0.1 or abs(rotation.pitch) > 0.1 or abs(rotation.roll) > 0.1:
            fail(f"Unreadable label rotation: {actor.get_actor_label()} {rotation}")

    unreal.log(
        "OM_SPRINT2_VALIDATION|PASS|"
        f"carryables={len(carryables)}|starts={len(starts)}|reset={len(reset_actors)}|"
        "carry_component=present|interaction_regression=present|prototype_game_mode=present|"
        "controller_yaw=present|movable_debug_lighting=present"
    )
    unreal.log("OM_SPRINT2_VALIDATION|FINAL|PASS")


validate()
