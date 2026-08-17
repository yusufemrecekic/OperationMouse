"""Read-only Sprint 1 asset and interaction harness validation."""

import unreal


MAPPING_CONTEXT_PATH = "/Game/OperationMouse/Input/IMC_Gameplay.IMC_Gameplay"
MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Phase5_InteractionTest"

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

    character_class = unreal.load_class(None, "/Script/OperationMouse.OMMouseCharacter")
    character_cdo = unreal.get_default_object(character_class)
    if character_cdo.get_editor_property("interaction_component") is None:
        fail("AOMMouseCharacter has no InteractionComponent")

    unreal.log(
        f"OM_SPRINT1_VALIDATION|PASS|HARNESS|roles={sorted(roles)}|"
        f"player_starts={player_start_count}"
    )


def main():
    validate_input()
    validate_harness()
    unreal.log("OM_SPRINT1_VALIDATION|FINAL|PASS")


main()
