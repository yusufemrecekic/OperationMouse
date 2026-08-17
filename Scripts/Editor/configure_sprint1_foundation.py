"""Configure Sprint 1 gamepad mappings and the reusable interaction harness."""

import unreal


MAPPING_CONTEXT_PATH = "/Game/OperationMouse/Input/IMC_Gameplay.IMC_Gameplay"
MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Phase5_InteractionTest"
HARNESS_TAG = "Sprint1InteractionHarness"
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

GAMEPAD_MAPPINGS = {
    "Move": "Gamepad_Left2D",
    "Look": "Gamepad_Right2D",
    "Jump": "Gamepad_FaceButton_Bottom",
    "Sprint": "Gamepad_LeftThumbstick",
    "Crouch": "Gamepad_FaceButton_Right",
    "Interact": "Gamepad_FaceButton_Left",
}

ROLES = ("BUTTON", "PICKUP", "DOOR", "FAIL", "RESET")


def key_name(mapping):
    key = mapping.get_editor_property("key")
    try:
        return str(key.get_editor_property("key_name"))
    except Exception:
        return str(key)


def action_name(mapping):
    action = mapping.get_editor_property("action")
    return action.get_name() if action else "<None>"


def get_default_mappings(context):
    mapping_data = context.get_editor_property("default_key_mappings")
    return list(mapping_data.get_editor_property("mappings"))


def make_key(name):
    key = unreal.Key()
    key.set_editor_property("key_name", unreal.Name(name))
    return key


def apply_axis_modifiers(context, action, gamepad_key_name, negate_y=False):
    """Write modifiers through the owning mapping-data struct (UE 5.8 returns copies)."""
    mapping_data = context.get_editor_property("default_key_mappings")
    mappings = list(mapping_data.get_editor_property("mappings"))
    found = False

    for mapping in mappings:
        mapped_action = mapping.get_editor_property("action")
        if mapped_action != action or key_name(mapping) != gamepad_key_name:
            continue

        dead_zone = unreal.new_object(unreal.InputModifierDeadZone, outer=context)
        dead_zone.set_editor_property("lower_threshold", 0.2)
        dead_zone.set_editor_property("upper_threshold", 1.0)
        dead_zone.set_editor_property("type", unreal.DeadZoneType.RADIAL)
        modifiers = [dead_zone]

        if negate_y:
            negate = unreal.new_object(unreal.InputModifierNegate, outer=context)
            negate.set_editor_property("x", False)
            negate.set_editor_property("y", True)
            negate.set_editor_property("z", False)
            modifiers.append(negate)

        mapping.set_editor_property("modifiers", modifiers)
        found = True

    if not found:
        raise RuntimeError(f"Could not find {action.get_name()}:{gamepad_key_name}")

    mapping_data.set_editor_property("mappings", mappings)
    context.set_editor_property("default_key_mappings", mapping_data)


def add_gamepad_mappings():
    context = unreal.EditorAssetLibrary.load_asset(MAPPING_CONTEXT_PATH)
    if context is None:
        raise RuntimeError(f"Could not load {MAPPING_CONTEXT_PATH}")

    loaded_actions = {}
    for logical_name, path in ACTIONS.items():
        action = unreal.EditorAssetLibrary.load_asset(path)
        if action is None:
            raise RuntimeError(f"Could not load {logical_name} action at {path}")
        loaded_actions[logical_name] = action

    existing = {
        (action_name(mapping), key_name(mapping).lower())
        for mapping in get_default_mappings(context)
    }

    for logical_name, gamepad_key_name in GAMEPAD_MAPPINGS.items():
        action = loaded_actions[logical_name]
        pair = (action.get_name(), gamepad_key_name.lower())
        if pair in existing:
            unreal.log(f"OM_SPRINT1|INPUT|EXISTS|{logical_name}|{gamepad_key_name}")
            continue

        context.map_key(action, make_key(gamepad_key_name))

        unreal.log(f"OM_SPRINT1|INPUT|ADDED|{logical_name}|{gamepad_key_name}")

    apply_axis_modifiers(
        context, loaded_actions["Move"], GAMEPAD_MAPPINGS["Move"], negate_y=False
    )
    # OMMouseCharacter already negates pitch for Mouse2D. Gamepad Right2D uses
    # the opposite vertical convention, so mapping-level Y negate keeps
    # stick-up = camera-up without changing mouse behavior.
    apply_axis_modifiers(
        context, loaded_actions["Look"], GAMEPAD_MAPPINGS["Look"], negate_y=True
    )

    unreal.EditorAssetLibrary.save_loaded_asset(context)

    final_pairs = {
        (action_name(mapping), key_name(mapping).lower())
        for mapping in get_default_mappings(context)
    }
    missing = []
    for logical_name, gamepad_key_name in GAMEPAD_MAPPINGS.items():
        expected = (loaded_actions[logical_name].get_name(), gamepad_key_name.lower())
        if expected not in final_pairs:
            missing.append(f"{logical_name}:{gamepad_key_name}")
    if missing:
        raise RuntimeError("Missing saved gamepad mappings: " + ", ".join(missing))


def rebuild_interaction_harness():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_subsystem.load_level(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")

    proxy_class = unreal.load_class(None, "/Script/OperationMouse.OMTestInteractableActor")
    if proxy_class is None:
        raise RuntimeError("Could not load AOMTestInteractableActor")

    all_actors = list(actor_subsystem.get_all_level_actors())
    player_starts = [actor for actor in all_actors if actor.get_class().get_name() == "PlayerStart"]
    if not player_starts:
        raise RuntimeError("Interaction test map has no PlayerStart")

    old_proxies = [actor for actor in all_actors if actor.get_class() == proxy_class]
    if old_proxies:
        actor_subsystem.destroy_actors(old_proxies)

    # The carried-forward Phase 5 labels describe the old generic fixtures and
    # visually collide with the focused Sprint 1 route. They are not gameplay.
    obsolete_labels = [
        actor
        for actor in all_actors
        if actor.get_actor_label().startswith("Phase5_Label_")
    ]
    if obsolete_labels:
        actor_subsystem.destroy_actors(obsolete_labels)

    prototype_game_mode = unreal.load_class(None, PROTOTYPE_GAME_MODE_PATH)
    if prototype_game_mode is None:
        raise RuntimeError(f"Could not load {PROTOTYPE_GAME_MODE_PATH}")
    world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem
    ).get_editor_world()
    world.get_world_settings().set_editor_property(
        "default_game_mode", prototype_game_mode
    )

    start = sorted(player_starts, key=lambda actor: actor.get_actor_label())[0]
    origin = start.get_actor_location()
    forward = start.get_actor_forward_vector()
    right = start.get_actor_right_vector()
    # Keep the interaction row between the PlayerStarts and the mantle course,
    # but off the X=0 mantle-fixture line. Six metres between stations leaves a
    # clear technical-test walking lane with no intersecting cube geometry.
    center = origin + forward * 250.0 + right * 300.0
    offsets = (-1200.0, -600.0, 0.0, 600.0, 1200.0)
    readable_rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0)

    enum_type = unreal.OMTestInteractionRole
    spawned = []
    for role_name, right_offset in zip(ROLES, offsets):
        role = getattr(enum_type, role_name)
        location = center + right * right_offset
        actor = actor_subsystem.spawn_actor_from_class(
            proxy_class, location, readable_rotation, transient=False
        )
        if actor is None:
            raise RuntimeError(f"Could not spawn {role_name} proxy")
        actor.set_actor_label(f"Sprint1_{role_name.title()}Proxy")
        actor.set_editor_property("test_role", role)
        actor.set_editor_property("tags", [unreal.Name(HARNESS_TAG)])
        spawned.append(actor)
        unreal.log(f"OM_SPRINT1|HARNESS|SPAWNED|{role_name}|{location}")

    # TextRender fronts face local +X. The normal route approaches from -X, so
    # yaw 180 (without negative scale or roll mirroring) makes every retained
    # map label readable from the gameplay camera.
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_class().get_name() == "TextRenderActor":
            actor.set_actor_rotation(readable_rotation, False)

    if not level_subsystem.save_current_level():
        raise RuntimeError("Could not save the Sprint 1 interaction test map")

    if len(spawned) != len(ROLES):
        raise RuntimeError(f"Expected {len(ROLES)} proxies, spawned {len(spawned)}")


def main():
    add_gamepad_mappings()
    rebuild_interaction_harness()
    unreal.log("OM_SPRINT1|RESULT|PASS")


main()
