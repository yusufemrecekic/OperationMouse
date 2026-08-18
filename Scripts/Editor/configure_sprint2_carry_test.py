"""Create the dedicated Sprint 2 Grab / Carry / Drop technical test map."""

import unreal


MAP_PATH = "/Game/OperationMouse/Tests/Maps/L_Sprint2_CarryTest"
HARNESS_TAG = unreal.Name("Sprint2CarryHarness")
PROTOTYPE_GAME_MODE_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMGameMode_Prototype.BP_OMGameMode_Prototype_C"
)
MATERIAL_FOLDER = "/Game/OperationMouse/Tests/Materials"
FLOOR_MATERIAL_PATH = f"{MATERIAL_FOLDER}/MI_Sprint2_FloorGray"
CARRYABLE_MATERIAL_PATH = f"{MATERIAL_FOLDER}/MI_Sprint2_CarryableGray"


def get_or_create_color_material(asset_path, color):
    material = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )
    if material is None:
        parent = unreal.EditorAssetLibrary.load_asset(
            "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"
        )
        if parent is None:
            raise RuntimeError("BasicShapeMaterial could not be loaded")
        asset_name = asset_path.rsplit("/", 1)[-1]
        factory = unreal.MaterialInstanceConstantFactoryNew()
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            MATERIAL_FOLDER,
            unreal.MaterialInstanceConstant,
            factory,
        )
        if material is None:
            raise RuntimeError(f"Could not create {asset_path}")
        unreal.MaterialEditingLibrary.set_material_instance_parent(material, parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        material,
        "Color",
        color,
    )
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


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


def make_light_movable(light_actor, intensity, attenuation_radius=None):
    component = light_actor.get_component_by_class(unreal.LightComponent)
    if component is None:
        raise RuntimeError(f"{light_actor.get_actor_label()} has no LightComponent")
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity", intensity)
    component.set_editor_property("cast_shadows", False)
    if attenuation_radius is not None:
        component.set_editor_property("attenuation_radius", attenuation_radius)
    return component


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
    # This is a debug/graybox map. All lights are movable, so it must not depend
    # on a baked-lighting pass or display "LIGHTING NEEDS TO BE REBUILT".
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    floor_material = get_or_create_color_material(
        FLOOR_MATERIAL_PATH,
        unreal.LinearColor(0.32, 0.34, 0.37, 1.0),
    )
    carryable_material = get_or_create_color_material(
        CARRYABLE_MATERIAL_PATH,
        unreal.LinearColor(0.10, 0.12, 0.15, 1.0),
    )

    floor = spawn(
        actor_subsystem,
        unreal.StaticMeshActor,
        "Sprint2_Floor",
        unreal.Vector(0.0, 0.0, -25.0),
        scale=unreal.Vector(200.0, 200.0, 0.5),
    )
    floor_component = floor.get_editor_property("static_mesh_component")
    floor_component.set_editor_property("static_mesh", cube)
    floor_component.set_material(0, floor_material)

    drop_pad = spawn(
        actor_subsystem,
        unreal.StaticMeshActor,
        "Sprint2_DropArea",
        unreal.Vector(250.0, 0.0, 2.5),
        scale=unreal.Vector(2.0, 3.0, 0.05),
    )
    drop_component = drop_pad.get_editor_property("static_mesh_component")
    drop_component.set_editor_property("static_mesh", cube)
    drop_component.set_material(0, carryable_material)

    for index, y in enumerate((-180.0, 180.0), start=1):
        spawn(
            actor_subsystem,
            unreal.PlayerStart,
            f"Sprint2_PlayerStart_{index}",
            unreal.Vector(-1050.0, y, 100.0),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0),
        )

    directional = spawn(
        actor_subsystem,
        unreal.DirectionalLight,
        "Sprint2_DirectionalLight",
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(roll=0.0, pitch=-45.0, yaw=-35.0),
    )
    directional_component = make_light_movable(directional, 100.0)
    directional_component.set_editor_property("cast_shadows", True)
    directional_component.set_editor_property("atmosphere_sun_light", True)

    spawn(
        actor_subsystem,
        unreal.SkyAtmosphere,
        "Sprint2_SkyAtmosphere",
        unreal.Vector(0.0, 0.0, 0.0),
    )

    sky = spawn(
        actor_subsystem,
        unreal.SkyLight,
        "Sprint2_SkyLight",
        unreal.Vector(0.0, 0.0, 300.0),
    )
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    if sky_component is None:
        raise RuntimeError("Sprint2_SkyLight has no SkyLightComponent")
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("intensity", 1.0)
    sky_component.set_editor_property("real_time_capture", True)

    post_process = spawn(
        actor_subsystem,
        unreal.PostProcessVolume,
        "Sprint2_PostProcess",
        unreal.Vector(0.0, 0.0, 0.0),
    )
    post_process.set_editor_property("unbound", True)
    post_settings = post_process.get_editor_property("settings")
    post_settings.set_editor_property("override_auto_exposure_method", True)
    post_settings.set_editor_property(
        "auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM
    )
    post_settings.set_editor_property("override_auto_exposure_min_brightness", True)
    post_settings.set_editor_property("auto_exposure_min_brightness", 15.0)
    post_settings.set_editor_property("override_auto_exposure_max_brightness", True)
    post_settings.set_editor_property("auto_exposure_max_brightness", 15.0)
    post_settings.set_editor_property("override_auto_exposure_bias", True)
    post_settings.set_editor_property("auto_exposure_bias", 0.0)
    post_settings.set_editor_property("override_bloom_intensity", True)
    post_settings.set_editor_property("bloom_intensity", 0.0)
    post_process.set_editor_property("settings", post_settings)

    readable_rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0)
    carryable_a = spawn(
        actor_subsystem,
        carryable_class,
        "Sprint2_Carryable_A",
        unreal.Vector(-400.0, -170.0, 80.0),
        readable_rotation,
    )
    carryable_a.get_component_by_class(unreal.StaticMeshComponent).set_material(
        0, carryable_material
    )
    carryable_b = spawn(
        actor_subsystem,
        carryable_class,
        "Sprint2_Carryable_B",
        unreal.Vector(650.0, 170.0, 80.0),
        readable_rotation,
    )
    carryable_b.get_component_by_class(unreal.StaticMeshComponent).set_material(
        0, carryable_material
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
    add_text(actor_subsystem, "Sprint2_Label_A", "CARRYABLE A", unreal.Vector(-400.0, -170.0, 190.0))
    add_text(actor_subsystem, "Sprint2_Label_Drop", "DROP AREA", unreal.Vector(250.0, 0.0, 190.0))
    add_text(actor_subsystem, "Sprint2_Label_B", "CARRYABLE B", unreal.Vector(650.0, 170.0, 190.0))
    add_text(actor_subsystem, "Sprint2_Label_Reset", "RESET / RECOVERY", unreal.Vector(1150.0, 0.0, 190.0))

    if not level_subsystem.save_current_level():
        raise RuntimeError("Could not save Sprint 2 carry test map")

    unreal.log("OM_SPRINT2_CONFIG|RESULT|PASS")


configure_map()
