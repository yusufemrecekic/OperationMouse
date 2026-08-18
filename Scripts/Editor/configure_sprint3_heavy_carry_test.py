"""Create the dedicated Yusuf-side Heavy Carry gameplay test map."""

import unreal


SOURCE_MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint2_CarryTest"
MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint3_HeavyCarryTest"
HARNESS_TAG = unreal.Name("Sprint3HeavyCarryHarness")
MATERIAL_PATH = "/Game/OperationMouse/Tests/Materials/MI_Sprint2_CarryableGray"


def spawn(actor_subsystem, actor_class, label, location, rotation=None, scale=None):
    actor = actor_subsystem.spawn_actor_from_class(
        actor_class, location, rotation or unreal.Rotator(), transient=False
    )
    if actor is None:
        raise RuntimeError(f"Could not spawn {label}")
    actor.set_actor_label(label)
    actor.set_editor_property("tags", [HARNESS_TAG])
    if scale is not None:
        actor.set_actor_scale3d(scale)
    return actor


def add_text(actor_subsystem, label, text, location):
    actor = spawn(
        actor_subsystem,
        unreal.TextRenderActor,
        label,
        location,
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0),
    )
    component = actor.get_editor_property("text_render")
    component.set_editor_property("text", text)
    component.set_editor_property("world_size", 32.0)
    return actor


def configure_map():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP_PATH, MAP_PATH):
            raise RuntimeError(f"Could not duplicate {SOURCE_MAP_PATH}")
    if not level_subsystem.load_level(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")

    actors = list(actor_subsystem.get_all_level_actors())
    carryable_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryableActor")
    heavy_class = unreal.load_class(None, "/Script/OperationMouse.OMHeavyCarryableActor")
    reset_class = unreal.load_class(None, "/Script/OperationMouse.OMTestInteractableActor")
    if None in (carryable_class, heavy_class, reset_class):
        raise RuntimeError("Required Carry classes could not be loaded")

    # Keep the accepted Sprint 2 daylight rig/floor as a stable graybox base.
    for actor in actors:
        label = actor.get_actor_label()
        if label.startswith("Sprint2_Label_") or label.startswith("Sprint3_Label_"):
            actor_subsystem.destroy_actor(actor)

    normal_carryables = [actor for actor in actors if actor.get_class() == carryable_class]
    if not normal_carryables:
        raise RuntimeError("Sprint 2 donor map has no normal Carryable")
    normal = normal_carryables[0]
    normal.set_actor_label("Sprint3_NormalCarryable")
    normal.set_actor_location(unreal.Vector(-350.0, -250.0, 80.0), False, False)
    normal.set_editor_property("tags", [HARNESS_TAG])
    if len(normal_carryables) > 1:
        actor_subsystem.destroy_actors(normal_carryables[1:])

    existing_heavy = [actor for actor in actors if actor.get_class() == heavy_class]
    if existing_heavy:
        heavy = existing_heavy[0]
        heavy.set_actor_location(unreal.Vector(350.0, 220.0, 95.0), False, False)
    else:
        heavy = spawn(
            actor_subsystem,
            heavy_class,
            "Sprint3_HeavyCarryable",
            unreal.Vector(350.0, 220.0, 95.0),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0),
            unreal.Vector(1.7, 1.15, 1.0),
        )
    heavy.set_actor_label("Sprint3_HeavyCarryable")
    heavy.set_editor_property("tags", [HARNESS_TAG])

    material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
    for actor in (normal, heavy):
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        if component and material:
            component.set_material(0, material)

    starts = [actor for actor in actors if actor.get_class().get_name() == "PlayerStart"]
    for index, start in enumerate(starts[:2], start=1):
        start.set_actor_label(f"Sprint3_PlayerStart_{index}")
        start.set_actor_location(
            unreal.Vector(-1050.0, -180.0 if index == 1 else 180.0, 100.0),
            False,
            False,
        )
        start.set_editor_property("tags", [HARNESS_TAG])

    reset_actors = [
        actor
        for actor in actors
        if actor.get_class().get_name() == "OMTestInteractableActor"
        and str(actor.get_editor_property("test_role")).split(".")[-1].split(":")[0]
        == "RESET"
    ]
    if not reset_actors:
        reset = spawn(
            actor_subsystem,
            reset_class,
            "Sprint3_Reset",
            unreal.Vector(1050.0, 0.0, 55.0),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0),
        )
        reset.set_editor_property("test_role", unreal.OMTestInteractionRole.RESET)
    else:
        reset = reset_actors[0]
        reset.set_actor_label("Sprint3_Reset")
        reset.set_actor_location(unreal.Vector(1050.0, 0.0, 55.0), False, False)
        reset.set_editor_property("tags", [HARNESS_TAG])

    add_text(actor_subsystem, "Sprint3_Label_Start", "START - 2 PLAYERS", unreal.Vector(-950.0, 0.0, 190.0))
    add_text(actor_subsystem, "Sprint3_Label_Normal", "NORMAL CARRY REGRESSION", unreal.Vector(-350.0, -250.0, 210.0))
    add_text(actor_subsystem, "Sprint3_Label_Heavy", "HEAVY CARRY - REQUIRES 2", unreal.Vector(350.0, 220.0, 240.0))
    add_text(actor_subsystem, "Sprint3_Label_Reset", "RESET / RECOVERY", unreal.Vector(1050.0, 0.0, 190.0))

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("kill_z", -1000.0)
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    if not level_subsystem.save_current_level():
        raise RuntimeError("Could not save Sprint 3 Heavy Carry test map")
    unreal.log("OM_SPRINT3_HEAVY_CONFIG|RESULT|PASS")


configure_map()
