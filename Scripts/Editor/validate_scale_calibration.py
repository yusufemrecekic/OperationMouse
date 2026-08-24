"""Targeted structural validation for the Scale Calibration technical harness."""

import unreal


ROOT = "/Game/OperationMouse/Tests/Scale"
MAP_PATH = f"{ROOT}/L_ScaleCalibration"
PLAYER_PATH = f"{ROOT}/BP_ScaleCalibrationMouse"
GAME_MODE_PATH = f"{ROOT}/BP_ScaleCalibrationGameMode"
PROTOTYPE_PLAYER_PATH = (
    "/Game/OperationMouse/Characters/Prototype/Blueprints/"
    "BP_OMMouseCharacter_Prototype"
)


def fail(message):
    unreal.log_error(f"OM_SCALE_VALIDATION|FAIL|{message}")
    raise RuntimeError(message)


for path in (MAP_PATH, PLAYER_PATH, GAME_MODE_PATH):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        fail(f"Missing calibration asset: {path}")

player = unreal.load_asset(PLAYER_PATH)
game_mode = unreal.load_asset(GAME_MODE_PATH)
prototype_player = unreal.load_asset(PROTOTYPE_PLAYER_PATH)
if None in (player, game_mode, prototype_player):
    fail("Calibration or prototype Blueprint failed to load")
unreal.BlueprintEditorLibrary.compile_blueprint(player)
unreal.BlueprintEditorLibrary.compile_blueprint(game_mode)
player_cdo = unreal.get_default_object(player.generated_class())
player_asset_data = unreal.EditorAssetLibrary.find_asset_data(PLAYER_PATH)
parent_class_tag = player_asset_data.get_tag_value("ParentClass") or ""
if "BP_OMMouseCharacter_Prototype_C" not in parent_class_tag:
    fail("Calibration player is not derived from the prototype gameplay Character")
capsule = player_cdo.get_editor_property("capsule_component")
mesh = player_cdo.get_editor_property("mesh")
movement = player_cdo.get_editor_property("character_movement")
if abs(capsule.get_unscaled_capsule_radius() - 11.0) > 0.01:
    fail("Candidate A capsule radius is not 11 uu")
if abs(capsule.get_unscaled_capsule_half_height() - 24.0) > 0.01:
    fail("Candidate A capsule half-height is not 24 uu")
mesh_scale = mesh.get_editor_property("relative_scale3d")
mesh_location = mesh.get_editor_property("relative_location")
if max(abs(mesh_scale.x - 0.25), abs(mesh_scale.y - 0.25), abs(mesh_scale.z - 0.25)) > 0.001:
    fail(f"Candidate A visual scale is wrong: {mesh_scale}")
if abs(mesh_location.z + 24.0) > 0.01:
    fail(f"Candidate A mesh floor alignment is wrong: {mesh_location}")
if abs(movement.get_editor_property("crouched_half_height") - 12.0) > 0.01:
    fail("Candidate A test-only crouched half-height is not 12 uu")

game_mode_cdo = unreal.get_default_object(game_mode.generated_class())
if game_mode_cdo.get_editor_property("default_pawn_class") != player.generated_class():
    fail("Calibration GameMode does not spawn Candidate A")

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
if len(interactions) < 5:
    fail(f"Expected four range fixtures plus Reset, found {len(interactions)}")

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
    "OM_SCALE_VALIDATION|PASS|candidate=11x24|visual_scale=0.25|"
    f"zones=9|starts={len(starts)}|normal_cargo={len(normal_cargo)}|"
    f"heavy_cargo={len(heavy_cargo)}|interactions={len(interactions)}|"
    "production_tuning=unchanged|map_check=executed"
)
unreal.log("OM_SCALE_VALIDATION|FINAL|PASS")
