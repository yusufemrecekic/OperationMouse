"""Create isolated Candidate A/B player profiles and the scale calibration map."""

import unreal


SCALE_ROOT = "/Game/OperationMouse/Tests/Scale"
MATERIAL_ROOT = f"{SCALE_ROOT}/Materials"
MAP_PATH = f"{SCALE_ROOT}/L_ScaleCalibration"
LEGACY_PLAYER_PATH = f"{SCALE_ROOT}/BP_ScaleCalibrationMouse"
PLAYER_A_PATH = f"{SCALE_ROOT}/BP_ScaleCalibrationMouse_A"
PLAYER_B_PATH = f"{SCALE_ROOT}/BP_ScaleCalibrationMouse_B"
GAME_MODE_PATH = f"{SCALE_ROOT}/BP_ScaleCalibrationGameMode"
PROTOTYPE_PLAYER_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMMouseCharacter_Prototype"
)
PROTOTYPE_GAME_MODE_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMGameMode_Prototype"
)
FLOOR_MATERIAL_PATH = f"{MATERIAL_ROOT}/MI_ScaleCalibration_Floor"
FIXTURE_MATERIAL_PATH = f"{MATERIAL_ROOT}/MI_ScaleCalibration_Fixture"
MARKER_MATERIAL_PATH = f"{MATERIAL_ROOT}/MI_ScaleCalibration_Marker"
HARNESS_TAG = unreal.Name("ScaleCalibrationHarness")

CANDIDATE_B_RADIUS = 7.5
CANDIDATE_B_HALF_HEIGHT = 15.0
CANDIDATE_B_CROUCHED_HALF_HEIGHT = 8.0
CANDIDATE_B_VISUAL_SCALE = 0.15
CANDIDATE_B_MESH_Z = -15.0
CANDIDATE_B_WALK_SPEED = 270.0
CANDIDATE_B_SPRINT_SPEED = 400.0
CANDIDATE_B_MAX_ACCELERATION = 1400.0
CANDIDATE_B_BRAKING_DECELERATION = 1600.0
CANDIDATE_B_MAX_STEP_HEIGHT = 14.0
CANDIDATE_B_JUMP_Z = 245.0
CANDIDATE_B_CAMERA_ARM = 170.0
CANDIDATE_B_CAMERA_PROBE = 5.0
CANDIDATE_B_CAMERA_COLLISION_PADDING = 2.0
CANDIDATE_B_CAMERA_TARGET_Z = 18.0
CANDIDATE_B_CAMERA_RETRACT_SPEED = 30.0
CANDIDATE_B_CAMERA_EXTEND_SPEED = 5.0
CANDIDATE_B_CAMERA_MIN_SAFE_RADIUS_MULTIPLIER = 2.0
CANDIDATE_B_CAMERA_CLOSE_THRESHOLD = 0.55
CANDIDATE_B_CAMERA_CLOSE_VERTICAL_HALF_HEIGHT_MULTIPLIER = 1.0
CANDIDATE_B_CAMERA_CLOSE_BLEND_SPEED = 8.0


def create_blueprint(asset_name, parent_class):
    asset_path = f"{SCALE_ROOT}/{asset_name}"
    blueprint = unreal.load_asset(asset_path)
    if blueprint is None:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, SCALE_ROOT, unreal.Blueprint, factory
        )
    if blueprint is None:
        raise RuntimeError(f"Could not create {asset_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    return blueprint


def configure_candidate_assets():
    prototype_player = unreal.load_asset(PROTOTYPE_PLAYER_PATH)
    prototype_game_mode = unreal.load_asset(PROTOTYPE_GAME_MODE_PATH)
    if prototype_player is None or prototype_game_mode is None:
        raise RuntimeError("Prototype Character/GameMode assets could not be loaded")

    unreal.BlueprintEditorLibrary.compile_blueprint(prototype_player)
    unreal.BlueprintEditorLibrary.compile_blueprint(prototype_game_mode)
    # Preserve the exact Candidate A asset produced by the first calibration pass.
    if (
        unreal.EditorAssetLibrary.does_asset_exist(LEGACY_PLAYER_PATH)
        and not unreal.EditorAssetLibrary.does_asset_exist(PLAYER_A_PATH)
    ):
        if not unreal.EditorAssetLibrary.rename_asset(
            LEGACY_PLAYER_PATH, PLAYER_A_PATH
        ):
            raise RuntimeError("Could not preserve legacy Candidate A asset")

    candidate_a = unreal.load_asset(PLAYER_A_PATH)
    if candidate_a is None:
        raise RuntimeError("Candidate A comparison asset is missing")
    unreal.BlueprintEditorLibrary.compile_blueprint(candidate_a)
    unreal.EditorAssetLibrary.save_loaded_asset(candidate_a)

    candidate_b = create_blueprint(
        "BP_ScaleCalibrationMouse_B", prototype_player.generated_class()
    )
    player_cdo = unreal.get_default_object(candidate_b.generated_class())
    capsule = player_cdo.get_editor_property("capsule_component")
    mesh = player_cdo.get_editor_property("mesh")
    movement = player_cdo.get_editor_property("character_movement")
    camera_boom = player_cdo.get_editor_property("camera_boom")
    close_space_camera = player_cdo.get_editor_property(
        "close_space_camera_component"
    )
    capsule.set_capsule_size(
        CANDIDATE_B_RADIUS, CANDIDATE_B_HALF_HEIGHT, True
    )
    mesh.set_editor_property(
        "relative_scale3d",
        unreal.Vector(
            CANDIDATE_B_VISUAL_SCALE,
            CANDIDATE_B_VISUAL_SCALE,
            CANDIDATE_B_VISUAL_SCALE,
        ),
    )
    mesh.set_editor_property(
        "relative_location", unreal.Vector(0.0, 0.0, CANDIDATE_B_MESH_Z)
    )
    mesh.set_editor_property(
        "relative_rotation", unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0)
    )
    movement.set_editor_property(
        "crouched_half_height", CANDIDATE_B_CROUCHED_HALF_HEIGHT
    )
    movement.set_editor_property(
        "max_acceleration", CANDIDATE_B_MAX_ACCELERATION
    )
    movement.set_editor_property(
        "braking_deceleration_walking", CANDIDATE_B_BRAKING_DECELERATION
    )
    movement.set_editor_property(
        "max_step_height", CANDIDATE_B_MAX_STEP_HEIGHT
    )
    movement.set_editor_property("jump_z_velocity", CANDIDATE_B_JUMP_Z)
    player_cdo.set_editor_property(
        "normal_walk_speed", CANDIDATE_B_WALK_SPEED
    )
    player_cdo.set_editor_property("sprint_speed", CANDIDATE_B_SPRINT_SPEED)
    camera_boom.set_editor_property(
        "target_arm_length", CANDIDATE_B_CAMERA_ARM
    )
    camera_boom.set_editor_property("probe_size", CANDIDATE_B_CAMERA_PROBE)
    # Candidate B opts into the local resolver; production defaults stay unchanged.
    camera_boom.set_editor_property(
        "target_offset", unreal.Vector(0.0, 0.0, CANDIDATE_B_CAMERA_TARGET_Z)
    )
    camera_boom.set_editor_property("do_collision_test", False)
    close_space_camera.set_editor_property("close_space_camera_enabled", True)
    close_space_camera.set_editor_property(
        "desired_arm_length", CANDIDATE_B_CAMERA_ARM
    )
    close_space_camera.set_editor_property(
        "camera_probe_radius", CANDIDATE_B_CAMERA_PROBE
    )
    close_space_camera.set_editor_property(
        "camera_collision_padding", CANDIDATE_B_CAMERA_COLLISION_PADDING
    )
    close_space_camera.set_editor_property(
        "camera_retract_speed", CANDIDATE_B_CAMERA_RETRACT_SPEED
    )
    close_space_camera.set_editor_property(
        "camera_extend_speed", CANDIDATE_B_CAMERA_EXTEND_SPEED
    )
    close_space_camera.set_editor_property(
        "min_safe_distance_radius_multiplier",
        CANDIDATE_B_CAMERA_MIN_SAFE_RADIUS_MULTIPLIER,
    )
    close_space_camera.set_editor_property(
        "close_space_threshold", CANDIDATE_B_CAMERA_CLOSE_THRESHOLD
    )
    close_space_camera.set_editor_property(
        "close_space_vertical_offset_half_height_multiplier",
        CANDIDATE_B_CAMERA_CLOSE_VERTICAL_HALF_HEIGHT_MULTIPLIER,
    )
    close_space_camera.set_editor_property(
        "close_space_blend_speed", CANDIDATE_B_CAMERA_CLOSE_BLEND_SPEED
    )
    close_space_camera.set_editor_property(
        "open_space_target_offset",
        unreal.Vector(0.0, 0.0, CANDIDATE_B_CAMERA_TARGET_Z),
    )
    unreal.BlueprintEditorLibrary.compile_blueprint(candidate_b)
    unreal.EditorAssetLibrary.save_loaded_asset(candidate_b)

    game_mode = create_blueprint(
        "BP_ScaleCalibrationGameMode", prototype_game_mode.generated_class()
    )
    game_mode_cdo = unreal.get_default_object(game_mode.generated_class())
    game_mode_cdo.set_editor_property(
        "default_pawn_class", candidate_b.generated_class()
    )
    unreal.BlueprintEditorLibrary.compile_blueprint(game_mode)
    unreal.EditorAssetLibrary.save_loaded_asset(game_mode)
    return candidate_a, candidate_b, game_mode


def get_or_create_material(asset_path, color):
    material = unreal.load_asset(asset_path)
    if material is None:
        parent = unreal.load_asset(
            "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"
        )
        factory = unreal.MaterialInstanceConstantFactoryNew()
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_path.rsplit("/", 1)[-1],
            MATERIAL_ROOT,
            unreal.MaterialInstanceConstant,
            factory,
        )
        if material is None or parent is None:
            raise RuntimeError(f"Could not create material {asset_path}")
        unreal.MaterialEditingLibrary.set_material_instance_parent(material, parent)
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            material, "Color", color
        )
        unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def spawn(actor_subsystem, actor_class, label, location, rotation=None, scale=None, zone=None):
    actor = actor_subsystem.spawn_actor_from_class(
        actor_class, location, rotation or unreal.Rotator(), transient=False
    )
    if actor is None:
        raise RuntimeError(f"Could not spawn {label}")
    actor.set_actor_label(label)
    tags = [HARNESS_TAG]
    if zone:
        tags.append(unreal.Name(zone))
    actor.set_editor_property("tags", tags)
    if scale is not None:
        actor.set_actor_scale3d(scale)
    return actor


def add_cube(actor_subsystem, cube, material, label, location, dimensions, zone):
    actor = spawn(
        actor_subsystem,
        unreal.StaticMeshActor,
        label,
        location,
        scale=unreal.Vector(
            dimensions[0] / 100.0,
            dimensions[1] / 100.0,
            dimensions[2] / 100.0,
        ),
        zone=zone,
    )
    component = actor.get_editor_property("static_mesh_component")
    component.set_editor_property("static_mesh", cube)
    component.set_material(0, material)
    return actor


def add_text(actor_subsystem, label, text, location, zone, size=20.0):
    actor = spawn(
        actor_subsystem,
        unreal.TextRenderActor,
        label,
        location,
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0),
        zone=zone,
    )
    component = actor.get_editor_property("text_render")
    component.set_editor_property("text", text)
    component.set_editor_property("world_size", size)
    component.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    return actor


def add_zone_floor(actor_subsystem, cube, floor_material, zone, center, size):
    add_cube(
        actor_subsystem,
        cube,
        floor_material,
        f"Scale_{zone}_Floor",
        unreal.Vector(center[0], center[1], -5.0),
        (size[0], size[1], 10.0),
        zone,
    )


def configure_daylight(actor_subsystem):
    directional = spawn(
        actor_subsystem,
        unreal.DirectionalLight,
        "Scale_DirectionalLight",
        unreal.Vector(0.0, 0.0, 800.0),
        unreal.Rotator(roll=0.0, pitch=-45.0, yaw=-35.0),
        zone="ScaleLighting",
    )
    directional_component = directional.get_component_by_class(
        unreal.DirectionalLightComponent
    )
    directional_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    directional_component.set_editor_property("intensity", 100.0)
    directional_component.set_editor_property("cast_shadows", True)
    directional_component.set_editor_property("atmosphere_sun_light", True)

    spawn(
        actor_subsystem,
        unreal.SkyAtmosphere,
        "Scale_SkyAtmosphere",
        unreal.Vector(),
        zone="ScaleLighting",
    )
    sky = spawn(
        actor_subsystem,
        unreal.SkyLight,
        "Scale_SkyLight",
        unreal.Vector(0.0, 0.0, 500.0),
        zone="ScaleLighting",
    )
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("intensity", 1.0)
    sky_component.set_editor_property("real_time_capture", True)

    post = spawn(
        actor_subsystem,
        unreal.PostProcessVolume,
        "Scale_PostProcess",
        unreal.Vector(),
        zone="ScaleLighting",
    )
    post.set_editor_property("unbound", True)
    settings = post.get_editor_property("settings")
    settings.set_editor_property("override_auto_exposure_method", True)
    settings.set_editor_property(
        "auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM
    )
    settings.set_editor_property("override_auto_exposure_min_brightness", True)
    settings.set_editor_property("auto_exposure_min_brightness", 15.0)
    settings.set_editor_property("override_auto_exposure_max_brightness", True)
    settings.set_editor_property("auto_exposure_max_brightness", 15.0)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 0.0)
    post.set_editor_property("settings", settings)


def build_zone_a(actor_subsystem, cube, floor, fixture):
    zone = "ScaleZoneA"
    center = (-2000.0, -1000.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (900.0, 800.0))
    add_text(actor_subsystem, "Scale_Label_ZoneA", "A - HUMAN REFERENCES", unreal.Vector(-2000, -1320, 130), zone, 26)
    # Recognizable human blockout: counter, dining table/chair and a 180-uu silhouette.
    add_cube(actor_subsystem, cube, fixture, "Scale_A_CounterBase", unreal.Vector(-2200, -1210, 40), (300, 80, 80), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_A_CounterTop90", unreal.Vector(-2200, -1210, 85), (320, 100, 10), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_A_ToeKick10", unreal.Vector(-2200, -1150, 5), (260, 30, 10), zone)
    add_text(actor_subsystem, "Scale_Label_A_Counter", "KITCHEN COUNTER - 90 UU", unreal.Vector(-2200, -1210, 120), zone, 14)

    add_cube(actor_subsystem, cube, fixture, "Scale_A_DiningTop75", unreal.Vector(-2150, -900, 70), (260, 150, 10), zone)
    for x in (-2250.0, -2050.0):
        for y in (-950.0, -850.0):
            add_cube(actor_subsystem, cube, fixture, f"Scale_A_TableLeg_{int(x)}_{int(y)}", unreal.Vector(x, y, 35), (12, 12, 70), zone)
    add_text(actor_subsystem, "Scale_Label_A_Table", "HUMAN DINING TABLE - 75 UU", unreal.Vector(-2150, -900, 110), zone, 14)

    chair_parts = [
        add_cube(actor_subsystem, cube, fixture, "Scale_A_ChairSeat45", unreal.Vector(-1770, -900, 42), (70, 70, 6), zone),
        add_cube(actor_subsystem, cube, fixture, "Scale_A_ChairBack", unreal.Vector(-1802, -900, 78), (6, 70, 72), zone),
    ]
    for x in (-1795.0, -1745.0):
        for y in (-925.0, -875.0):
            chair_parts.append(
                add_cube(actor_subsystem, cube, fixture, f"Scale_A_ChairLeg_{int(x)}_{int(y)}", unreal.Vector(x, y, 21), (8, 8, 42), zone)
            )
    # Explicit camera-composition policy for this porous furniture fixture only.
    # Pawn/world/physics responses stay on the cube's existing blocking profile.
    for chair_part in chair_parts:
        chair_part.get_editor_property("static_mesh_component").set_collision_response_to_channel(
            unreal.CollisionChannel.ECC_CAMERA,
            unreal.CollisionResponseType.ECR_IGNORE,
        )
    add_text(actor_subsystem, "Scale_Label_A_Chair", "HUMAN CHAIR - SEAT 45 UU", unreal.Vector(-1770, -900, 125), zone, 14)

    add_cube(actor_subsystem, cube, fixture, "Scale_A_HumanBody180", unreal.Vector(-1630, -1190, 90), (35, 22, 130), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_A_HumanHead180", unreal.Vector(-1630, -1190, 165), (30, 30, 30), zone)
    add_text(actor_subsystem, "Scale_Label_A_Human", "HUMAN HEIGHT REF - 180 UU", unreal.Vector(-1630, -1190, 205), zone, 14)
    # Human doorway reference: 90 uu clear width and 210 uu clear height.
    add_cube(actor_subsystem, cube, fixture, "Scale_A_DoorLeft", unreal.Vector(-2000, -650, 105), (30, 30, 210), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_A_DoorRight", unreal.Vector(-1880, -650, 105), (30, 30, 210), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_A_DoorHeader", unreal.Vector(-1940, -650, 225), (150, 30, 30), zone)
    add_text(actor_subsystem, "Scale_Label_A_Door", "DOOR REF 90 W / 210 H", unreal.Vector(-1940, -650, 260), zone, 14)


def build_zone_b(actor_subsystem, cube, floor, fixture):
    zone = "ScaleZoneB"
    center = (-650.0, -1000.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (1000.0, 850.0))
    add_text(actor_subsystem, "Scale_Label_ZoneB", "B - PASSAGE WIDTH", unreal.Vector(-650, -1360, 110), zone, 26)
    widths = (12, 16, 20, 25, 30, 40)
    for index, width in enumerate(widths):
        lane_y = -1260.0 + index * 105.0
        x = -650.0
        wall_offset = width * 0.5 + 3.0
        add_cube(actor_subsystem, cube, fixture, f"Scale_B_{width}_Left", unreal.Vector(x, lane_y - wall_offset, 30), (260, 6, 60), zone)
        add_cube(actor_subsystem, cube, fixture, f"Scale_B_{width}_Right", unreal.Vector(x, lane_y + wall_offset, 30), (260, 6, 60), zone)
        add_text(actor_subsystem, f"Scale_Label_B_{width}", f"{width} UU", unreal.Vector(x - 165, lane_y, 60), zone, 13)


def build_zone_c(actor_subsystem, cube, floor, fixture):
    zone = "ScaleZoneC"
    center = (750.0, -1000.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (1100.0, 850.0))
    add_text(actor_subsystem, "Scale_Label_ZoneC", "C - STEP / LEDGE / MANTLE", unreal.Vector(750, -1360, 120), zone, 24)
    heights = (5, 10, 15, 20, 25, 30, 40)
    for index, height in enumerate(heights):
        x = 380.0 + (index % 4) * 220.0
        y = -1150.0 + (index // 4) * 300.0
        add_cube(actor_subsystem, cube, fixture, f"Scale_C_Ledge_{height}", unreal.Vector(x, y, height * 0.5), (120, 100, height), zone)
        add_text(actor_subsystem, f"Scale_Label_C_{height}", f"{height} UU", unreal.Vector(x, y, height + 28), zone, 14)


def build_zone_d(actor_subsystem, cube, floor, fixture, marker):
    zone = "ScaleZoneD"
    add_text(actor_subsystem, "Scale_Label_ZoneD", "D - SAFE GAP / JUMP", unreal.Vector(2050, -1360, 100), zone, 24)
    gaps = (10, 20, 30, 40, 50, 60)
    for index, gap in enumerate(gaps):
        lane_y = -1280.0 + index * 120.0
        left_edge = 1950.0
        add_cube(actor_subsystem, cube, fixture, f"Scale_D_{gap}_Start", unreal.Vector(left_edge - 100, lane_y, 0), (200, 90, 10), zone)
        add_cube(actor_subsystem, cube, fixture, f"Scale_D_{gap}_Land", unreal.Vector(left_edge + gap + 100, lane_y, 0), (200, 90, 10), zone)
        add_cube(actor_subsystem, cube, marker, f"Scale_D_{gap}_SafeFloor", unreal.Vector(left_edge + gap * 0.5, lane_y, -75), (gap, 90, 10), zone)
        add_text(actor_subsystem, f"Scale_Label_D_{gap}", f"GAP {gap} UU", unreal.Vector(left_edge + gap * 0.5, lane_y, 30), zone, 13)


def build_zone_e(actor_subsystem, cube, floor, fixture):
    zone = "ScaleZoneE"
    center = (-2000.0, 350.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (900.0, 850.0))
    add_text(actor_subsystem, "Scale_Label_ZoneE", "E - CAMERA / LOW CLEARANCE", unreal.Vector(-2000, -10, 120), zone, 23)
    # Table underside at 80 uu and a separate 55-uu low route.
    add_cube(actor_subsystem, cube, fixture, "Scale_E_TableTop", unreal.Vector(-2150, 350, 85), (350, 260, 10), zone)
    for x in (-2300, -2000):
        for y in (240, 460):
            add_cube(actor_subsystem, cube, fixture, f"Scale_E_Leg_{x}_{y}", unreal.Vector(x, y, 40), (20, 20, 80), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_E_LowRoof", unreal.Vector(-1700, 350, 60), (300, 180, 10), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_E_CorridorLeft", unreal.Vector(-1700, 245, 35), (300, 10, 70), zone)
    add_cube(actor_subsystem, cube, fixture, "Scale_E_CorridorRight", unreal.Vector(-1700, 455, 35), (300, 10, 70), zone)
    add_text(actor_subsystem, "Scale_Label_E_Table", "UNDER TABLE - 80 UU", unreal.Vector(-2150, 350, 115), zone, 14)
    add_text(actor_subsystem, "Scale_Label_E_Low", "LOW ROOF - 55 UU CLEAR", unreal.Vector(-1700, 350, 95), zone, 14)


def build_zone_f(actor_subsystem, cube, floor, marker, interaction_class):
    zone = "ScaleZoneF"
    center = (-700.0, 350.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (1000.0, 850.0))
    add_text(actor_subsystem, "Scale_Label_ZoneF", "F - INTERACTION RANGE", unreal.Vector(-700, -10, 120), zone, 24)
    distances = (40, 60, 80, 100, 120)
    roles = (
        unreal.OMTestInteractionRole.BUTTON,
        unreal.OMTestInteractionRole.PICKUP,
        unreal.OMTestInteractionRole.DOOR,
        unreal.OMTestInteractionRole.FAIL,
        unreal.OMTestInteractionRole.GENERIC,
    )
    for index, (distance, role) in enumerate(zip(distances, roles)):
        lane_y = 80.0 + index * 130.0
        capsule_front_x = -1080.0
        # The test proxy is a 75-uu cube. Its near surface is 37.5 uu before its center.
        target_center_x = capsule_front_x + distance + 37.5
        add_cube(actor_subsystem, cube, marker, f"Scale_F_CapsuleFront_{distance}", unreal.Vector(capsule_front_x, lane_y, 1), (3, 70, 2), zone)
        target = spawn(actor_subsystem, interaction_class, f"Scale_F_Interact_{distance}", unreal.Vector(target_center_x, lane_y, 37.5), unreal.Rotator(yaw=180.0), zone=zone)
        target.set_editor_property("test_role", role)
        add_text(actor_subsystem, f"Scale_Label_F_{distance}", f"CAPSULE FRONT -> SURFACE = {distance} UU", unreal.Vector(capsule_front_x + distance * 0.5, lane_y, 105), zone, 12)
    add_text(actor_subsystem, "Scale_Label_F_Note", "FEEL TARGET: 1.5-2 MOUSE BODY LENGTHS", unreal.Vector(-700, 700, 90), zone, 13)


def build_zone_g(actor_subsystem, cube, floor, fixture_material, carryable_class):
    zone = "ScaleZoneG"
    center = (500.0, 350.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (900.0, 850.0))
    add_text(actor_subsystem, "Scale_Label_ZoneG", "G - REAL NORMAL CARRY", unreal.Vector(500, -10, 120), zone, 24)
    cargo = (("Small", 0.25), ("Medium", 0.5), ("Large", 0.8))
    for index, (name, scale) in enumerate(cargo):
        # AOMCarryableActor's default 45-uu cube is actor-scaled; keep each size on the floor.
        cargo_z = 22.5 * scale
        actor = spawn(actor_subsystem, carryable_class, f"Scale_G_Carry_{name}", unreal.Vector(250 + index * 250, 350, cargo_z), unreal.Rotator(yaw=180.0), unreal.Vector(scale, scale, scale), zone)
        mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
        if mesh:
            mesh.set_material(0, fixture_material)
        add_text(actor_subsystem, f"Scale_Label_G_{name}", name.upper(), unreal.Vector(250 + index * 250, 350, 90), zone, 14)


def build_zone_h(actor_subsystem, cube, floor, fixture_material, marker, heavy_class, reset_class):
    zone = "ScaleZoneH"
    center = (1700.0, 350.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (1000.0, 850.0))
    add_text(actor_subsystem, "Scale_Label_ZoneH", "H - REAL HEAVY CARRY", unreal.Vector(1700, -10, 120), zone, 24)
    # Default heavy-cargo mesh is 45 uu high before actor scale.
    heavy = spawn(actor_subsystem, heavy_class, "Scale_H_HeavyCarry", unreal.Vector(1700, 350, 11.25), unreal.Rotator(yaw=180.0), unreal.Vector(1.0, 0.25, 0.5), zone)
    mesh = heavy.get_component_by_class(unreal.StaticMeshComponent)
    if mesh:
        mesh.set_material(0, fixture_material)
    for y in (290.0, 410.0):
        add_cube(actor_subsystem, cube, marker, f"Scale_H_Spacing_{int(y)}", unreal.Vector(1700, y, 1), (180, 3, 2), zone)
    add_text(actor_subsystem, "Scale_Label_H_Clearance", "TEST 2/2 VERY CLOSE, THEN STEP BACK", unreal.Vector(1700, 350, 115), zone, 14)
    reset = spawn(actor_subsystem, reset_class, "Scale_H_Reset", unreal.Vector(2050, 600, 25), unreal.Rotator(yaw=180.0), zone=zone)
    reset.set_editor_property("test_role", unreal.OMTestInteractionRole.RESET)


def build_zone_i(actor_subsystem, cube, floor, fixture):
    zone = "ScaleZoneI"
    center = (0.0, 1300.0)
    add_zone_floor(actor_subsystem, cube, floor, zone, center, (1700.0, 650.0))
    add_text(actor_subsystem, "Scale_Label_ZoneI", "I - TWO PLAYER SPACING", unreal.Vector(0, 1040, 110), zone, 25)
    widths = (25, 35, 50)
    for index, width in enumerate(widths):
        x = -450.0 + index * 450.0
        wall_offset = width * 0.5 + 4.0
        add_cube(actor_subsystem, cube, fixture, f"Scale_I_{width}_Left", unreal.Vector(x, 1370 - wall_offset, 35), (260, 8, 70), zone)
        add_cube(actor_subsystem, cube, fixture, f"Scale_I_{width}_Right", unreal.Vector(x, 1370 + wall_offset, 35), (260, 8, 70), zone)
        add_text(actor_subsystem, f"Scale_Label_I_{width}", f"TWO PLAYER GATE {width} UU", unreal.Vector(x - 170, 1370, 80), zone, 13)
    add_text(actor_subsystem, "Scale_Label_I_Shared", "SIDE-BY-SIDE / PASS / SHARED APPROACH", unreal.Vector(0, 1570, 70), zone, 14)


def build_walkable_route(actor_subsystem, cube, floor):
    """Connect isolated pads without covering any measured gap/clearance fixture."""
    route = "ScaleRoute"
    segments = (
        ("AB", (-1325.0, -1000.0), (450.0, 140.0, 10.0)),
        ("BC", (50.0, -1000.0), (300.0, 140.0, 10.0)),
        ("EF", (-1350.0, 350.0), (400.0, 140.0, 10.0)),
        ("FG", (-75.0, 350.0), (250.0, 140.0, 10.0)),
        ("GH", (1075.0, 350.0), (250.0, 140.0, 10.0)),
        ("AE", (-2000.0, -337.5), (140.0, 525.0, 10.0)),
        ("BF", (-700.0, -337.5), (140.0, 525.0, 10.0)),
        ("CG", (500.0, -337.5), (140.0, 525.0, 10.0)),
        ("GI", (500.0, 875.0), (180.0, 200.0, 10.0)),
    )
    for name, location, dimensions in segments:
        add_cube(
            actor_subsystem,
            cube,
            floor,
            f"Scale_Route_{name}",
            unreal.Vector(location[0], location[1], -5.0),
            dimensions,
            route,
        )


def configure_map(game_mode):
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
        actor_subsystem.destroy_actors(existing)
    elif not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError(f"Could not create {MAP_PATH}")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property(
        "default_game_mode", game_mode.generated_class()
    )
    world.get_world_settings().set_editor_property("kill_z", -500.0)
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    floor = get_or_create_material(
        FLOOR_MATERIAL_PATH, unreal.LinearColor(0.32, 0.34, 0.37, 1.0)
    )
    fixture = get_or_create_material(
        FIXTURE_MATERIAL_PATH, unreal.LinearColor(0.10, 0.12, 0.15, 1.0)
    )
    marker = get_or_create_material(
        MARKER_MATERIAL_PATH, unreal.LinearColor(0.65, 0.28, 0.05, 1.0)
    )
    if cube is None:
        raise RuntimeError("Engine cube mesh could not be loaded")

    # A low safety floor prevents calibration gaps from becoming punitive.
    add_cube(actor_subsystem, cube, floor, "Scale_SafetyFloor", unreal.Vector(0, 0, -105), (5600, 3600, 10), "ScaleSafety")
    configure_daylight(actor_subsystem)

    interaction_class = unreal.load_class(None, "/Script/OperationMouse.OMTestInteractableActor")
    carryable_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryableActor")
    heavy_class = unreal.load_class(None, "/Script/OperationMouse.OMHeavyCarryableActor")
    if None in (interaction_class, carryable_class, heavy_class):
        raise RuntimeError("Required reusable gameplay classes could not be loaded")

    build_zone_a(actor_subsystem, cube, floor, fixture)
    build_zone_b(actor_subsystem, cube, floor, fixture)
    build_zone_c(actor_subsystem, cube, floor, fixture)
    build_zone_d(actor_subsystem, cube, floor, fixture, marker)
    build_zone_e(actor_subsystem, cube, floor, fixture)
    build_zone_f(actor_subsystem, cube, floor, marker, interaction_class)
    build_zone_g(actor_subsystem, cube, floor, fixture, carryable_class)
    build_zone_h(actor_subsystem, cube, floor, fixture, marker, heavy_class, interaction_class)
    build_zone_i(actor_subsystem, cube, floor, fixture)
    build_walkable_route(actor_subsystem, cube, floor)

    for index, y in enumerate((1240.0, 1360.0), start=1):
        spawn(actor_subsystem, unreal.PlayerStart, f"Scale_PlayerStart_{index}", unreal.Vector(-750, y, 30), unreal.Rotator(yaw=0.0), zone="ScaleZoneI")

    if not level_subsystem.save_current_level():
        raise RuntimeError("Could not save Scale Calibration map")
    unreal.log(
        "OM_SCALE_CONFIG|PASS|active=B|candidate_a=preserved|radius=7.5|"
        "halfheight=15|visual_scale=0.15|mesh_z=-15|"
        "visual_height=27.071|walk=270|sprint=400|jump_z=245|zones=A-I"
    )


_, _, calibration_game_mode = configure_candidate_assets()
configure_map(calibration_game_mode)
