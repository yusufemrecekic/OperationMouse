"""Create a compact, interaction-driven Mission foundation test map."""

import unreal


SOURCE_MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint3_HeavyCarryTest"
MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint3_MissionTest"
HARNESS_TAG = unreal.Name("Sprint3MissionHarness")


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
    component.set_editor_property("world_size", 28.0)
    return actor


def configure_map():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP_PATH, MAP_PATH):
            raise RuntimeError(f"Could not duplicate {SOURCE_MAP_PATH}")
    if not level_subsystem.load_level(MAP_PATH):
        raise RuntimeError(f"Could not load {MAP_PATH}")

    mission_manager_class = unreal.load_class(None, "/Script/OperationMouse.OMMissionManager")
    mission_interaction_class = unreal.load_class(
        None, "/Script/OperationMouse.OMMissionInteractionActor"
    )
    carry_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryableActor")
    heavy_class = unreal.load_class(None, "/Script/OperationMouse.OMHeavyCarryableActor")
    test_interaction_class = unreal.load_class(None, "/Script/OperationMouse.OMTestInteractableActor")
    if None in (
        mission_manager_class,
        mission_interaction_class,
        carry_class,
        heavy_class,
        test_interaction_class,
    ):
        raise RuntimeError("Required Mission or donor classes could not be loaded")

    for actor in list(actor_subsystem.get_all_level_actors()):
        label = actor.get_actor_label()
        if (
            actor.get_class() in (carry_class, heavy_class, test_interaction_class, mission_manager_class, mission_interaction_class)
            or label.startswith(("Sprint2_Label_", "Sprint3_Label_", "Mission_Label_"))
            or label in ("Sprint3_DropArea", "Sprint2_DropArea")
        ):
            actor_subsystem.destroy_actor(actor)

    actors = list(actor_subsystem.get_all_level_actors())
    starts = [actor for actor in actors if actor.get_class().get_name() == "PlayerStart"]
    if len(starts) < 2:
        raise RuntimeError("Donor map requires two PlayerStarts")
    for index, start in enumerate(starts[:2], start=1):
        start.set_actor_label(f"Mission_PlayerStart_{index}")
        start.set_actor_location(
            unreal.Vector(-720.0, -100.0 if index == 1 else 100.0, 100.0),
            False,
            False,
        )
        start.set_editor_property("tags", [HARNESS_TAG])

    floor = next(
        (
            actor
            for actor in actors
            if actor.get_actor_label() in ("Sprint2_Floor", "Sprint3_Floor", "Mission_Floor")
        ),
        None,
    )
    if floor is None:
        raise RuntimeError("Accepted donor graybox floor is missing")
    floor.set_actor_label("Mission_Floor")
    floor.set_actor_scale3d(unreal.Vector(18.0, 11.0, 0.5))

    manager = spawn(
        actor_subsystem,
        mission_manager_class,
        "Mission_Manager",
        unreal.Vector(0.0, 180.0, 40.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0),
    )
    manager.set_editor_property("mission_id", unreal.Name("Sprint3_Mission_Prototype"))
    manager.set_editor_property("objective_target", 1)

    fixtures = (
        ("Mission_Start", unreal.OMMissionInteractionAction.START, unreal.Vector(-430.0, -160.0, 55.0)),
        ("Mission_Objective", unreal.OMMissionInteractionAction.COMPLETE_OBJECTIVE, unreal.Vector(-120.0, -160.0, 55.0)),
        ("Mission_Fail", unreal.OMMissionInteractionAction.FAIL, unreal.Vector(190.0, -160.0, 55.0)),
        ("Mission_Reset", unreal.OMMissionInteractionAction.RESET, unreal.Vector(500.0, -160.0, 55.0)),
        ("Mission_Retry", unreal.OMMissionInteractionAction.RETRY, unreal.Vector(500.0, 160.0, 55.0)),
    )
    for label, action, location in fixtures:
        fixture = spawn(
            actor_subsystem,
            mission_interaction_class,
            label,
            location,
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0),
        )
        fixture.set_editor_property("mission_manager", manager)
        fixture.set_editor_property("mission_action", action)

    add_text(actor_subsystem, "Mission_Label_Start", "START", unreal.Vector(-430.0, -160.0, 195.0))
    add_text(actor_subsystem, "Mission_Label_Objective", "OBJECTIVE", unreal.Vector(-120.0, -160.0, 195.0))
    add_text(actor_subsystem, "Mission_Label_Fail", "FAIL", unreal.Vector(190.0, -160.0, 195.0))
    add_text(actor_subsystem, "Mission_Label_Reset", "RESET", unreal.Vector(500.0, -160.0, 195.0))
    add_text(actor_subsystem, "Mission_Label_Retry", "RETRY", unreal.Vector(500.0, 160.0, 195.0))

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("kill_z", -1000.0)
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    if not level_subsystem.save_current_level():
        raise RuntimeError("Could not save Sprint 3 Mission test map")
    unreal.log("OM_SPRINT3_MISSION_CONFIG|RESULT|PASS")


configure_map()
