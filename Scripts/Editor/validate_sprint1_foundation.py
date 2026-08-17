"""Read-only Sprint 1 asset and interaction harness validation."""

import unreal


MAPPING_CONTEXT_PATH = "/Game/OperationMouse/Input/IMC_Gameplay.IMC_Gameplay"
MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Phase5_InteractionTest"
PROTOTYPE_CHARACTER_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMMouseCharacter_Prototype.BP_OMMouseCharacter_Prototype_C"
)
PROTOTYPE_GAME_MODE_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMGameMode_Prototype.BP_OMGameMode_Prototype_C"
)

ACTIONS = {
    "Move": "/Game/OperationMouse/Input/IA_Move.IA_Move",
    "Look": "/Game/OperationMouse/Input/IA_Look.IA_Look",
    "Jump": "/Game/OperationMouse/Input/IA_Jump.IA_Jump",
    "Sprint": "/Game/OperationMouse/Input/IA_Sprint.IA_Sprint",
    "Crouch": "/Game/OperationMouse/Input/IA_Crouch.IA_Crouch",
    "Interact": "/Game/OperationMouse/Input/IA_Interact.IA_Interact",
}

REQUIRED_KEYS = {
    "Move": {"W", "A", "S", "D", "Gamepad_Left2D"},
    "Look": {"Mouse2D", "Gamepad_Right2D"},
    "Jump": {"SpaceBar", "Gamepad_FaceButton_Bottom"},
    "Sprint": {"LeftShift", "Gamepad_LeftThumbstick"},
    "Crouch": {"LeftControl", "Gamepad_FaceButton_Right"},
    "Interact": {"E", "Gamepad_FaceButton_Left"},
}

REQUIRED_ROLES = {"BUTTON", "PICKUP", "DOOR", "FAIL", "RESET"}


def key_name(mapping):
    key = mapping.get_editor_property("key")
    try:
        return str(key.get_editor_property("key_name"))
    except Exception:
        return str(key)


def fail(message):
    unreal.log_error(f"OM_SPRINT1_VALIDATION|FAIL|{message}")
    raise RuntimeError(message)


def validate_input():
    context = unreal.EditorAssetLibrary.load_asset(MAPPING_CONTEXT_PATH)
    if context is None:
        fail(f"Missing {MAPPING_CONTEXT_PATH}")

    loaded_actions = {}
    for logical_name, path in ACTIONS.items():
        action = unreal.EditorAssetLibrary.load_asset(path)
        if action is None:
            fail(f"Missing action {logical_name}: {path}")
        loaded_actions[action.get_name()] = logical_name

    action_keys = {name: set() for name in ACTIONS}
    gamepad_look_modifiers = set()
    mapping_data = context.get_editor_property("default_key_mappings")
    mappings = mapping_data.get_editor_property("mappings")
    for mapping in mappings:
        action = mapping.get_editor_property("action")
        if action is None or action.get_name() not in loaded_actions:
            continue
        logical_name = loaded_actions[action.get_name()]
        current_key = key_name(mapping)
        action_keys[logical_name].add(current_key)
        if logical_name == "Look" and current_key == "Gamepad_Right2D":
            gamepad_look_modifiers = {
                modifier.get_class().get_name()
                for modifier in mapping.get_editor_property("modifiers")
                if modifier is not None
            }

    for logical_name, required_keys in REQUIRED_KEYS.items():
        missing = required_keys - action_keys[logical_name]
        if missing:
            fail(f"{logical_name} missing keys: {sorted(missing)}")

    required_modifiers = {"InputModifierDeadZone", "InputModifierNegate"}
    if not required_modifiers.issubset(gamepad_look_modifiers):
        fail(f"Gamepad Look modifiers invalid: {sorted(gamepad_look_modifiers)}")

    unreal.log(f"OM_SPRINT1_VALIDATION|PASS|INPUT|{action_keys}")


def validate_harness():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem.load_level(MAP_PATH):
        fail(f"Could not load {MAP_PATH}")

    proxy_class = unreal.load_class(None, "/Script/OperationMouse.OMTestInteractableActor")
    if proxy_class is None:
        fail("Missing AOMTestInteractableActor")

    actors = list(actor_subsystem.get_all_level_actors())
    proxies = [actor for actor in actors if actor.get_class() == proxy_class]
    roles = {
        str(actor.get_editor_property("test_role")).split(".")[-1].split(":")[0]
        for actor in proxies
    }
    if roles != REQUIRED_ROLES:
        fail(f"Harness roles were {sorted(roles)}, expected {sorted(REQUIRED_ROLES)}")

    player_start_count = sum(
        1 for actor in actors if actor.get_class().get_name() == "PlayerStart"
    )
    if player_start_count < 2:
        fail(f"Expected at least two PlayerStarts, found {player_start_count}")

    prototype_character = unreal.load_class(None, PROTOTYPE_CHARACTER_PATH)
    prototype_game_mode = unreal.load_class(None, PROTOTYPE_GAME_MODE_PATH)
    if prototype_character is None or prototype_game_mode is None:
        fail("Prototype Character or GameMode Blueprint class did not load")
    world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem
    ).get_editor_world()
    active_game_mode = world.get_world_settings().get_editor_property(
        "default_game_mode"
    )
    if active_game_mode != prototype_game_mode:
        fail(f"Map GameMode is {active_game_mode}, expected {prototype_game_mode}")
    game_mode_cdo = unreal.get_default_object(prototype_game_mode)
    if game_mode_cdo.get_editor_property("default_pawn_class") != prototype_character:
        fail("Prototype GameMode does not use BP_OMMouseCharacter_Prototype")

    proxy_locations = [actor.get_actor_location() for actor in proxies]
    for index, first in enumerate(proxy_locations):
        for second in proxy_locations[index + 1 :]:
            if (first - second).length() < 500.0:
                fail("Sprint 1 fixtures are too close or intersecting")

    for actor in proxies:
        rotation = actor.get_actor_rotation()
        if abs(abs(rotation.yaw) - 180.0) > 0.1:
            fail(f"{actor.get_actor_label()} text is not facing the approach route")

    obsolete_labels = [
        actor
        for actor in actors
        if actor.get_actor_label().startswith("Phase5_Label_")
    ]
    if obsolete_labels:
        fail("Obsolete Phase 5 labels still clutter the Sprint 1 route")

    for actor in actors:
        if actor.get_class().get_name() != "TextRenderActor":
            continue
        rotation = actor.get_actor_rotation()
        if (
            abs(abs(rotation.yaw) - 180.0) > 0.1
            or abs(rotation.pitch) > 0.1
            or abs(rotation.roll) > 0.1
        ):
            fail(f"{actor.get_actor_label()} has mirrored/unreadable text rotation")

    character_class = unreal.load_class(None, "/Script/OperationMouse.OMMouseCharacter")
    character_cdo = unreal.get_default_object(character_class)
    if character_cdo.get_editor_property("interaction_component") is None:
        fail("AOMMouseCharacter has no InteractionComponent")

    unreal.log(
        f"OM_SPRINT1_VALIDATION|PASS|HARNESS|roles={sorted(roles)}|"
        f"player_starts={player_start_count}|game_mode={prototype_game_mode.get_name()}|"
        f"pawn={prototype_character.get_name()}"
    )


def main():
    validate_input()
    validate_harness()
    unreal.log("OM_SPRINT1_VALIDATION|FINAL|PASS")


main()
