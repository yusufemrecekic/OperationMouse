import unreal


MESH_PATH = "/Game/OperationMouse/Characters/Prototype/Mixamo/Meshes/SK_Mixamo_YBot"
SKELETON_PATH = "/Game/OperationMouse/Characters/Prototype/Mixamo/Meshes/SK_Mixamo_YBot_Skeleton"
ANIMATION_FOLDER = "/Game/OperationMouse/Characters/Prototype/Mixamo/Animations"
ANIM_BP_PATH = "/Game/OperationMouse/Characters/Prototype/Animation/ABP_OMPrototypeLocomotion"
CHARACTER_BP_PATH = "/Game/OperationMouse/Characters/Prototype/Blueprints/BP_OMMouseCharacter_Prototype"
GAME_MODE_BP_PATH = "/Game/OperationMouse/Characters/Prototype/Blueprints/BP_OMGameMode_Prototype"
MAP_PATH = "/Game/OperationMouse/Characters/Prototype/Maps/L_PrototypeCharacterTest"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)
    unreal.log(f"OM_VALIDATE_PASS: {message}")


mesh = unreal.load_asset(MESH_PATH)
skeleton = unreal.load_asset(SKELETON_PATH)
require(mesh is not None, "Y Bot Skeletal Mesh loads")
require(skeleton is not None, "Y Bot Skeleton loads")
require(mesh.get_editor_property("skeleton") == skeleton, "Skeletal Mesh uses the intended Skeleton")

animation_names = [
    "A_Mixamo_Idle", "A_Mixamo_Walk", "A_Mixamo_Run", "A_Mixamo_Jump",
    "A_Mixamo_Fall", "A_Mixamo_Land", "A_Mixamo_CrouchIdle", "A_Mixamo_CrouchWalk",
]
animations = {}
for name in animation_names:
    animation = unreal.load_asset(f"{ANIMATION_FOLDER}/{name}")
    require(animation is not None, f"{name} loads")
    require(animation.get_editor_property("skeleton") == skeleton, f"{name} uses the single Y Bot Skeleton")
    require(not animation.get_editor_property("enable_root_motion"), f"{name} root motion is disabled")
    animations[name] = animation

all_assets = unreal.EditorAssetLibrary.list_assets("/Game/OperationMouse/Characters/Prototype", recursive=True, include_folder=False)
skeleton_assets = [path for path in all_assets if unreal.load_asset(path).__class__.__name__ == "Skeleton"]
require(len(skeleton_assets) == 1, f"exactly one prototype Skeleton exists ({len(skeleton_assets)})")

animation_blueprint = unreal.load_asset(ANIM_BP_PATH)
native_anim_class = unreal.load_class(None, "/Script/OperationMouse.OMAnimInstance")
require(native_anim_class is not None, "native UOMAnimInstance class is present in the loaded Editor module")
require(animation_blueprint is not None, "prototype Animation Blueprint loads")
require(unreal.BlueprintEditorLibrary.compile_blueprint(animation_blueprint), "prototype Animation Blueprint compiles")
require(animation_blueprint.generated_class() is not None, "prototype Animation Blueprint generated class exists")
anim_default = unreal.get_default_object(animation_blueprint.generated_class())
for property_name, animation_name in {
    "idle_animation": "A_Mixamo_Idle",
    "walk_animation": "A_Mixamo_Walk",
    "run_animation": "A_Mixamo_Run",
    "jump_animation": "A_Mixamo_Jump",
    "fall_animation": "A_Mixamo_Fall",
    "land_animation": "A_Mixamo_Land",
    "crouch_idle_animation": "A_Mixamo_CrouchIdle",
    "crouch_walk_animation": "A_Mixamo_CrouchWalk",
}.items():
    require(anim_default.get_editor_property(property_name) == animations[animation_name], f"AnimBP maps {property_name}")

character_blueprint = unreal.load_asset(CHARACTER_BP_PATH)
require(character_blueprint is not None, "prototype Character Blueprint loads")
require(unreal.BlueprintEditorLibrary.compile_blueprint(character_blueprint), "prototype Character Blueprint compiles")
character_default = unreal.get_default_object(character_blueprint.generated_class())
mesh_component = character_default.get_editor_property("mesh")
placeholder = character_default.get_editor_property("placeholder_visual")
require(mesh_component.get_editor_property("skeletal_mesh_asset") == mesh, "prototype Character uses Y Bot visual mesh")
require(mesh_component.get_editor_property("animation_mode") == unreal.AnimationMode.ANIMATION_BLUEPRINT, "prototype Character mesh uses Animation Blueprint mode")
require(mesh_component.get_editor_property("anim_class") == animation_blueprint.generated_class(), "prototype Character uses prototype AnimBP")
require(placeholder.get_editor_property("hidden_in_game"), "placeholder sphere is hidden")

game_mode_blueprint = unreal.load_asset(GAME_MODE_BP_PATH)
require(game_mode_blueprint is not None, "prototype GameMode Blueprint loads")
require(unreal.BlueprintEditorLibrary.compile_blueprint(game_mode_blueprint), "prototype GameMode Blueprint compiles")
game_mode_default = unreal.get_default_object(game_mode_blueprint.generated_class())
require(game_mode_default.get_editor_property("default_pawn_class") == character_blueprint.generated_class(), "prototype GameMode uses prototype Character")

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
require(world is not None, "prototype test map loads")
require(world.get_world_settings().get_editor_property("default_game_mode") == game_mode_blueprint.generated_class(), "test map overrides GameMode")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
player_starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
require(len(player_starts) >= 2, f"test map has at least two PlayerStarts ({len(player_starts)})")
require(len(actors) >= 10, f"test map retained Phase 4 test geometry ({len(actors)} actors)")

unreal.log("OM_PROTOTYPE_VALIDATION_SUCCESS")
