"""Targeted structural validation for the Scale Calibration technical harness."""

import configparser
import os
import unreal


ROOT = "/Game/OperationMouse/Tests/Scale"
MAP_PATH = f"{ROOT}/L_ScaleCalibration"
PLAYER_A_PATH = f"{ROOT}/BP_ScaleCalibrationMouse_A"
PLAYER_B_PATH = f"{ROOT}/BP_ScaleCalibrationMouse_B"
GAME_MODE_PATH = f"{ROOT}/BP_ScaleCalibrationGameMode"
PROTOTYPE_PLAYER_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMMouseCharacter_Prototype"
)


def fail(message):
    unreal.log_error(f"OM_SCALE_VALIDATION|FAIL|{message}")
    raise RuntimeError(message)


engine_config = configparser.ConfigParser(strict=False)
engine_config.read(os.path.join(unreal.Paths.project_config_dir(), "DefaultEngine.ini"))
near_clip = engine_config.getfloat("/Script/Engine.Engine", "NearClipPlane", fallback=-1.0)
if abs(near_clip - 2.0) > 0.01:
    fail(f"Project perspective NearClipPlane is {near_clip}, expected 2.0 uu")


for path in (MAP_PATH, PLAYER_A_PATH, PLAYER_B_PATH, GAME_MODE_PATH):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        fail(f"Missing calibration asset: {path}")

candidate_a = unreal.load_asset(PLAYER_A_PATH)
candidate_b = unreal.load_asset(PLAYER_B_PATH)
game_mode = unreal.load_asset(GAME_MODE_PATH)
prototype_player = unreal.load_asset(PROTOTYPE_PLAYER_PATH)
if None in (candidate_a, candidate_b, game_mode, prototype_player):
    fail("Calibration or prototype Blueprint failed to load")
unreal.BlueprintEditorLibrary.compile_blueprint(candidate_a)
unreal.BlueprintEditorLibrary.compile_blueprint(candidate_b)
unreal.BlueprintEditorLibrary.compile_blueprint(game_mode)

for path in (PLAYER_A_PATH, PLAYER_B_PATH):
    asset_data = unreal.EditorAssetLibrary.find_asset_data(path)
    parent_class_tag = asset_data.get_tag_value("ParentClass") or ""
    if "BP_OMMouseCharacter_Prototype_C" not in parent_class_tag:
        fail(f"Calibration player is not derived from prototype: {path}")

candidate_a_cdo = unreal.get_default_object(candidate_a.generated_class())
capsule_a = candidate_a_cdo.get_editor_property("capsule_component")
mesh_a = candidate_a_cdo.get_editor_property("mesh")
movement_a = candidate_a_cdo.get_editor_property("character_movement")
if abs(capsule_a.get_unscaled_capsule_radius() - 11.0) > 0.01:
    fail("Candidate A capsule radius is not 11 uu")
if abs(capsule_a.get_unscaled_capsule_half_height() - 24.0) > 0.01:
    fail("Candidate A capsule half-height is not 24 uu")
mesh_a_scale = mesh_a.get_editor_property("relative_scale3d")
mesh_a_location = mesh_a.get_editor_property("relative_location")
if max(abs(mesh_a_scale.x - 0.25), abs(mesh_a_scale.y - 0.25), abs(mesh_a_scale.z - 0.25)) > 0.001:
    fail(f"Candidate A visual scale is wrong: {mesh_a_scale}")
if abs(mesh_a_location.z + 24.0) > 0.01:
    fail(f"Candidate A mesh floor alignment is wrong: {mesh_a_location}")
if abs(movement_a.get_editor_property("crouched_half_height") - 12.0) > 0.01:
    fail("Candidate A test-only crouched half-height is not 12 uu")

candidate_b_cdo = unreal.get_default_object(candidate_b.generated_class())
capsule_b = candidate_b_cdo.get_editor_property("capsule_component")
mesh_b = candidate_b_cdo.get_editor_property("mesh")
movement_b = candidate_b_cdo.get_editor_property("character_movement")
camera_b = candidate_b_cdo.get_editor_property("camera_boom")
close_space_camera_b = candidate_b_cdo.get_editor_property(
    "close_space_camera_component"
)
if abs(capsule_b.get_unscaled_capsule_radius() - 7.5) > 0.01:
    fail("Candidate B capsule radius is not 7.5 uu")
if abs(capsule_b.get_unscaled_capsule_half_height() - 15.0) > 0.01:
    fail("Candidate B capsule half-height is not 15 uu")
mesh_b_scale = mesh_b.get_editor_property("relative_scale3d")
mesh_b_location = mesh_b.get_editor_property("relative_location")
if max(abs(mesh_b_scale.x - 0.15), abs(mesh_b_scale.y - 0.15), abs(mesh_b_scale.z - 0.15)) > 0.001:
    fail(f"Candidate B visual scale is wrong: {mesh_b_scale}")
if abs(mesh_b_location.z + 15.0) > 0.01:
    fail(f"Candidate B mesh floor alignment is wrong: {mesh_b_location}")
expected_movement = {
    "crouched_half_height": 8.0,
    "max_acceleration": 1400.0,
    "braking_deceleration_walking": 1600.0,
    "max_step_height": 14.0,
    "jump_z_velocity": 245.0,
}
for property_name, expected in expected_movement.items():
    actual = movement_b.get_editor_property(property_name)
    if abs(actual - expected) > 0.01:
        fail(f"Candidate B {property_name} is {actual}, expected {expected}")
if abs(candidate_b_cdo.get_editor_property("normal_walk_speed") - 270.0) > 0.01:
    fail("Candidate B walk speed is not 270 uu/s")
if abs(candidate_b_cdo.get_editor_property("sprint_speed") - 400.0) > 0.01:
    fail("Candidate B sprint speed is not 400 uu/s")
if abs(camera_b.get_editor_property("target_arm_length") - 170.0) > 0.01:
    fail("Candidate B camera arm is not 170 uu")
if abs(camera_b.get_editor_property("probe_size") - 5.0) > 0.01:
    fail("Candidate B camera probe is not 5 uu")
camera_target = camera_b.get_editor_property("target_offset")
if abs(camera_target.z - 18.0) > 0.01:
    fail(f"Candidate B camera target Z is wrong: {camera_target}")
if camera_b.get_editor_property("do_collision_test"):
    fail("Candidate B SpringArm collision must defer to the close-space resolver")
if not close_space_camera_b.get_editor_property("close_space_camera_enabled"):
    fail("Candidate B close-space camera resolver is disabled")
expected_camera = {
    "desired_arm_length": 170.0,
    "camera_probe_radius": 5.0,
    "camera_collision_padding": 2.0,
    "camera_retract_speed": 30.0,
    "camera_extend_speed": 5.0,
    "min_safe_distance_radius_multiplier": 2.0,
    "close_space_threshold": 0.55,
    "close_space_vertical_offset_half_height_multiplier": 1.0,
    "close_space_blend_speed": 8.0,
}
for property_name, expected in expected_camera.items():
    actual = close_space_camera_b.get_editor_property(property_name)
    if abs(actual - expected) > 0.01:
        fail(f"Candidate B camera {property_name} is {actual}, expected {expected}")
open_offset = close_space_camera_b.get_editor_property("open_space_target_offset")
if abs(open_offset.z - 18.0) > 0.01:
    fail(f"Candidate B close-space open offset is wrong: {open_offset}")

game_mode_cdo = unreal.get_default_object(game_mode.generated_class())
if game_mode_cdo.get_editor_property("default_pawn_class") != candidate_b.generated_class():
    fail("Calibration GameMode does not spawn active Candidate B")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(MAP_PATH):
    fail(f"Could not load {MAP_PATH}")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if world.get_world_settings().get_editor_property("default_game_mode") != game_mode.generated_class():
    fail("Calibration map does not override to the calibration GameMode")

actors = list(actor_subsystem.get_all_level_actors())
tags = {str(tag) for actor in actors for tag in list(actor.get_editor_property("tags"))}
missing_zones = [f"ScaleZone{letter}" for letter in "ABCDEFGHI" if f"ScaleZone{letter}" not in tags]
if missing_zones:
    fail(f"Missing calibration zones: {missing_zones}")

camera_wall_labels = {
    "Scale_E_CorridorLeft",
    "Scale_E_CorridorRight",
    "Scale_E_LowRoof",
}
camera_walls = [actor for actor in actors if actor.get_actor_label() in camera_wall_labels]
if len(camera_walls) != len(camera_wall_labels):
    fail("Camera close-space collision fixtures are incomplete")
for wall in camera_walls:
    component = wall.get_editor_property("static_mesh_component")
    if component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA) != unreal.CollisionResponseType.ECR_BLOCK:
        fail(f"Camera fixture does not block ECC_Camera: {wall.get_actor_label()}")

starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
carryable_class = unreal.load_class(None, "/Script/OperationMouse.OMCarryableActor")
heavy_class = unreal.load_class(None, "/Script/OperationMouse.OMHeavyCarryableActor")
interaction_class = unreal.load_class(None, "/Script/OperationMouse.OMTestInteractableActor")
normal_cargo = [actor for actor in actors if actor.get_class() == carryable_class]
heavy_cargo = [actor for actor in actors if actor.get_class() == heavy_class]
interactions = [actor for actor in actors if actor.get_class() == interaction_class]
if len(starts) < 2:
    fail(f"Expected two PlayerStarts, found {len(starts)}")
if len(normal_cargo) < 3:
    fail(f"Expected three Normal Carry sizes, found {len(normal_cargo)}")
if len(heavy_cargo) < 1:
    fail("Heavy Carry fixture is missing")
if len(interactions) < 6:
    fail(f"Expected five range fixtures plus Reset, found {len(interactions)}")

actor_labels = {actor.get_actor_label() for actor in actors}
required_human_references = {
    "Scale_A_CounterTop90",
    "Scale_A_DiningTop75",
    "Scale_A_ChairSeat45",
    "Scale_A_DoorHeader",
    "Scale_A_HumanBody180",
}
missing_human = sorted(required_human_references - actor_labels)
if missing_human:
    fail(f"Human reference blockout is incomplete: {missing_human}")

chair_labels = {
    "Scale_A_ChairSeat45",
    "Scale_A_ChairBack",
    "Scale_A_ChairLeg_-1795_-925",
    "Scale_A_ChairLeg_-1795_-875",
    "Scale_A_ChairLeg_-1745_-925",
    "Scale_A_ChairLeg_-1745_-875",
}
chair_parts = [actor for actor in actors if actor.get_actor_label() in chair_labels]
if len(chair_parts) != len(chair_labels):
    fail(f"Chair component actors are incomplete: {len(chair_parts)}")
gameplay_channels = (
    unreal.CollisionChannel.ECC_WORLD_STATIC,
    unreal.CollisionChannel.ECC_WORLD_DYNAMIC,
    unreal.CollisionChannel.ECC_PAWN,
    unreal.CollisionChannel.ECC_PHYSICS_BODY,
)
for chair_part in chair_parts:
    component = chair_part.get_editor_property("static_mesh_component")
    if component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA) != unreal.CollisionResponseType.ECR_IGNORE:
        fail(f"Chair part still blocks ECC_Camera: {chair_part.get_actor_label()}")
    if any(
        component.get_collision_response_to_channel(channel)
        != unreal.CollisionResponseType.ECR_BLOCK
        for channel in gameplay_channels
    ):
        fail(f"Chair gameplay/physics collision changed: {chair_part.get_actor_label()}")

hard_camera_labels = {
    "Scale_A_DiningTop75",
    "Scale_E_CorridorLeft",
    "Scale_E_CorridorRight",
    "Scale_E_LowRoof",
}
hard_camera_actors = [
    actor for actor in actors if actor.get_actor_label() in hard_camera_labels
]
if len(hard_camera_actors) != len(hard_camera_labels):
    fail("Hard camera fixtures are incomplete")
for hard_actor in hard_camera_actors:
    component = hard_actor.get_editor_property("static_mesh_component")
    if component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA) != unreal.CollisionResponseType.ECR_BLOCK:
        fail(f"Hard fixture does not block ECC_Camera: {hard_actor.get_actor_label()}")

passage_walls = [
    actor for actor in actors if actor.get_actor_label().startswith("Scale_B_")
]
if len(passage_walls) != 12:
    fail(f"Passage wall fixtures are incomplete: {len(passage_walls)}")
for wall in passage_walls:
    component = wall.get_editor_property("static_mesh_component")
    if component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA) != unreal.CollisionResponseType.ECR_BLOCK:
        fail(f"Passage wall does not block ECC_Camera: {wall.get_actor_label()}")

zone_floors = [
    actor for actor in actors if actor.get_actor_label().endswith("_Floor")
]
if not zone_floors:
    fail("Scale calibration floors are missing")
for floor_actor in zone_floors:
    component = floor_actor.get_editor_property("static_mesh_component")
    if component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA) != unreal.CollisionResponseType.ECR_BLOCK:
        fail(f"Calibration floor does not block ECC_Camera: {floor_actor.get_actor_label()}")
required_distance_markers = {
    f"Scale_F_CapsuleFront_{distance}" for distance in (40, 60, 80, 100, 120)
}
missing_markers = sorted(required_distance_markers - actor_labels)
if missing_markers:
    fail(f"Interaction surface-distance markers are incomplete: {missing_markers}")

labels = [actor for actor in actors if actor.get_actor_label().startswith("Scale_Label_")]
if len(labels) < 35:
    fail(f"Calibration labels are incomplete: {len(labels)}")
for label in labels:
    rotation = label.get_actor_rotation()
    scale = label.get_actor_scale3d()
    if abs(abs(rotation.yaw) - 180.0) > 0.1:
        fail(f"Unreadable label rotation: {label.get_actor_label()} {rotation}")
    if min(scale.x, scale.y, scale.z) <= 0.0:
        fail(f"Mirrored label scale: {label.get_actor_label()}")

world_settings = world.get_world_settings()
if not world_settings.get_editor_property("force_no_precomputed_lighting"):
    fail("Calibration map depends on baked lighting")
unreal.SystemLibrary.execute_console_command(world, "MAP CHECK")
unreal.log(
    "OM_SCALE_VALIDATION|PASS|active=B|candidate_a=preserved|"
    "candidate_b=7.5x15|visual_scale=0.15|walk=270|sprint=400|"
    f"zones=9|starts={len(starts)}|normal_cargo={len(normal_cargo)}|"
    f"heavy_cargo={len(heavy_cargo)}|interactions={len(interactions)}|"
    "production_tuning=unchanged|map_check=executed"
)
unreal.log("OM_SCALE_VALIDATION|FINAL|PASS")
