"""Create the dedicated Sprint 2 Grab / Carry / Drop technical test map."""

import unreal


MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint2_CarryTest"
HARNESS_TAG = unreal.Name("Sprint2CarryHarness")
PROTOTYPE_GAME_MODE_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMGameMode_Prototype.BP_OMGameMode_Prototype_C"
)


def spawn(actor_subsystem, actor_class, label, location, rotation=None, scale=None):
    actor = actor_subsystem.spawn_actor_from_class(
        actor_class,
        location,
        rotation or unreal.Rotator(),
        transient=False,
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
    component.set_editor_property("world_size", 34.0)
    return actor


def configure_map():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not level_subsystem.load_level(MAP_PATH):
            raise RuntimeError(f"Could not load {MAP_PATH}")
        existing = [
            actor
            for actor in actor_subsystem.get_all_level_actors()
            if HARNESS_TAG in list(actor.get_editor_property("tags"))
        ]
        if existing:
            actor_subsystem.destroy_actors(existing)
    elif not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError(f"Could not create {MAP_PATH}")

    prototype_game_mode = unreal.load_class(None, PROTOTYPE_GAME_MODE_PATH)
    carryable_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryableActor")
    reset_class = unreal.load_class(None, "/Script/OperationMouse.OMTestInteractableActor")
    if prototype_game_mode is None or carryable_class is None or reset_class is None:
        raise RuntimeError("Required Sprint 2 class could not be loaded")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", prototype_game_mode)
    world.get_world_settings().set_editor_property("kill_z", -1000.0)

    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")

    floor = spawn(
        actor_subsystem,
        unreal.StaticMeshActor,
        "Sprint2_Floor",
        unreal.Vector(0.0, 0.0, -25.0),
        scale=unreal.Vector(28.0, 12.0, 0.5),
    )
    floor.get_editor_property("static_mesh_component").set_editor_property("static_mesh", cube)

    drop_pad = spawn(
        actor_subsystem,
        unreal.StaticMeshActor,
        "Sprint2_DropArea",
        unreal.Vector(250.0, 0.0, 2.5),
        scale=unreal.Vector(2.0, 3.0, 0.05),
    )
    drop_pad.get_editor_property("static_mesh_component").set_editor_property("static_mesh", cube)

    for index, y in enumerate((-180.0, 180.0), start=1):
        spawn(
            actor_subsystem,
            unreal.PlayerStart,
            f"Sprint2_PlayerStart_{index}",
            unreal.Vector(-1050.0, y, 100.0),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0),
        )

    spawn(
        actor_subsystem,
        unreal.DirectionalLight,
        "Sprint2_DirectionalLight",
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(roll=0.0, pitch=-45.0, yaw=-35.0),
    )
    spawn(
        actor_subsystem,
        unreal.SkyLight,
        "Sprint2_SkyLight",
        unreal.Vector(0.0, 0.0, 300.0),
    )

    readable_rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0)
    spawn(
        actor_subsystem,
        carryable_class,
        "Sprint2_Carryable_A",
        unreal.Vector(-400.0, -170.0, 80.0),
        readable_rotation,
    )
    spawn(
        actor_subsystem,
        carryable_class,
        "Sprint2_Carryable_B",
        unreal.Vector(650.0, 170.0, 80.0),
        readable_rotation,
    )

    reset = spawn(
        actor_subsystem,
        reset_class,
        "Sprint2_Reset",
        unreal.Vector(1150.0, 0.0, 55.0),
        readable_rotation,
    )
    reset.set_editor_property("test_role", unreal.OMTestInteractionRole.RESET)

    add_text(actor_subsystem, "Sprint2_Label_Start", "START", unreal.Vector(-950.0, 0.0, 180.0))
    add_text(actor_subsystem, "Sprint2_Label_A", "CARRYABLE A", unreal.Vector(-400.0, 0.0, 190.0))
    add_text(actor_subsystem, "Sprint2_Label_Drop", "DROP AREA", unreal.Vector(250.0, 0.0, 190.0))
    add_text(actor_subsystem, "Sprint2_Label_B", "CARRYABLE B", unreal.Vector(650.0, 0.0, 190.0))
    add_text(actor_subsystem, "Sprint2_Label_Reset", "RESET / RECOVERY", unreal.Vector(1150.0, 0.0, 190.0))

    if not level_subsystem.save_current_level():
        raise RuntimeError("Could not save Sprint 2 carry test map")

    unreal.log("OM_SPRINT2_CONFIG|RESULT|PASS")


configure_map()
