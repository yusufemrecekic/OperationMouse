import os
import unreal


PROJECT_ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_ROOT = os.path.join(PROJECT_ROOT, "SourceAssets", "Characters", "Prototype", "Mixamo")

MESH_FOLDER = "/Game/OperationMouse/Characters/Prototype/Mixamo/Meshes"
ANIMATION_FOLDER = "/Game/OperationMouse/Characters/Prototype/Mixamo/Animations"
ANIM_BP_FOLDER = "/Game/OperationMouse/Characters/Prototype/Animation"
BLENDSPACE_FOLDER = "/Game/OperationMouse/Characters/Prototype/Animation/BlendSpaces"
BLUEPRINT_FOLDER = "/Game/OperationMouse/Characters/Prototype/Blueprints"
RETARGET_FOLDER = "/Game/OperationMouse/Characters/Retarget"
MAP_FOLDER = "/Game/OperationMouse/Characters/Prototype/Maps"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_mesh():
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SOURCE_ROOT, "SK_Mixamo_YBot_TPose.fbx")
    task.destination_path = MESH_FOLDER
    task.destination_name = "SK_Mixamo_YBot"
    task.automated = True
    task.replace_existing = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_as_skeletal = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.import_mesh = True
    options.import_animations = False
    options.import_materials = True
    options.import_textures = True
    options.create_physics_asset = False
    options.skeletal_mesh_import_data.import_mesh_lods = False
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(f"{MESH_FOLDER}/SK_Mixamo_YBot")
    if not mesh:
        raise RuntimeError("Mixamo Y Bot Skeletal Mesh import failed")
    return mesh


def import_animations(skeleton):
    names = [
        "A_Mixamo_Idle",
        "A_Mixamo_Walk",
        "A_Mixamo_Run",
        "A_Mixamo_Jump",
        "A_Mixamo_Fall",
        "A_Mixamo_Land",
        "A_Mixamo_CrouchIdle",
        "A_Mixamo_CrouchWalk",
    ]
    tasks = []
    for name in names:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(SOURCE_ROOT, f"{name}.fbx")
        task.destination_path = ANIMATION_FOLDER
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True

        options = unreal.FbxImportUI()
        options.import_as_skeletal = True
        options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
        options.import_mesh = False
        options.import_animations = True
        options.import_materials = False
        options.import_textures = False
        options.skeleton = skeleton
        task.options = options
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    animations = {}
    for name in names:
        animation = unreal.load_asset(f"{ANIMATION_FOLDER}/{name}")
        if not animation:
            raise RuntimeError(f"Animation import failed: {name}")
        animations[name] = animation
        unreal.EditorAssetLibrary.save_loaded_asset(animation)
    return animations


def create_blueprint(asset_name, folder, parent_class):
    path = f"{folder}/{asset_name}"
    existing = unreal.load_asset(path)
    if existing:
        return existing
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, folder, unreal.Blueprint, factory)


def create_animation_blueprint(skeleton, animations):
    path = f"{ANIM_BP_FOLDER}/ABP_OMPrototypeLocomotion"
    blueprint = unreal.load_asset(path)
    if not blueprint:
        factory = unreal.AnimBlueprintFactory()
        factory.set_editor_property("target_skeleton", skeleton)
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/OperationMouse.OMAnimInstance"))
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "ABP_OMPrototypeLocomotion", ANIM_BP_FOLDER, unreal.AnimBlueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    default_object = unreal.get_default_object(blueprint.generated_class())
    assignments = {
        "idle_animation": "A_Mixamo_Idle",
        "walk_animation": "A_Mixamo_Walk",
        "run_animation": "A_Mixamo_Run",
        "jump_animation": "A_Mixamo_Jump",
        "fall_animation": "A_Mixamo_Fall",
        "land_animation": "A_Mixamo_Land",
        "crouch_idle_animation": "A_Mixamo_CrouchIdle",
        "crouch_walk_animation": "A_Mixamo_CrouchWalk",
    }
    for property_name, animation_name in assignments.items():
        default_object.set_editor_property(property_name, animations[animation_name])
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    return blueprint


def configure_character_blueprint(mesh, animation_blueprint):
    parent_class = unreal.load_class(None, "/Script/OperationMouse.OMMouseCharacter")
    blueprint = create_blueprint("BP_OMMouseCharacter_Prototype", BLUEPRINT_FOLDER, parent_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    default_object = unreal.get_default_object(blueprint.generated_class())

    mesh_component = default_object.get_editor_property("mesh")
    mesh_component.set_skeletal_mesh_asset(mesh)
    mesh_component.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -96.0))
    mesh_component.set_editor_property("relative_rotation", unreal.Rotator(0.0, -90.0, 0.0))
    mesh_component.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))
    mesh_component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    mesh_component.set_editor_property("anim_class", animation_blueprint.generated_class())

    placeholder = default_object.get_editor_property("placeholder_visual")
    placeholder.set_editor_property("visible", False)
    placeholder.set_editor_property("hidden_in_game", True)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    return blueprint


def configure_game_mode(character_blueprint):
    parent_class = unreal.load_class(None, "/Script/OperationMouse.OMGameMode")
    blueprint = create_blueprint("BP_OMGameMode_Prototype", BLUEPRINT_FOLDER, parent_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    default_object = unreal.get_default_object(blueprint.generated_class())
    default_object.set_editor_property("default_pawn_class", character_blueprint.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    return blueprint


def create_test_map(game_mode_blueprint):
    source = "/Game/OperationMouse/Tests/Maps/L_Phase4_MantleTest"
    destination = f"{MAP_FOLDER}/L_PrototypeCharacterTest"
    if not unreal.EditorAssetLibrary.does_asset_exist(destination):
        if not unreal.EditorAssetLibrary.duplicate_asset(source, destination):
            raise RuntimeError("Could not create prototype character test map")
    world = unreal.EditorLoadingAndSavingUtils.load_map(destination)
    world.get_world_settings().set_editor_property("default_game_mode", game_mode_blueprint.generated_class())
    unreal.EditorLoadingAndSavingUtils.save_map(world, destination)


def main():
    for folder in [MESH_FOLDER, ANIMATION_FOLDER, ANIM_BP_FOLDER, BLENDSPACE_FOLDER, BLUEPRINT_FOLDER, RETARGET_FOLDER, MAP_FOLDER]:
        ensure_directory(folder)
    mesh = import_mesh()
    skeleton = mesh.get_editor_property("skeleton")
    if not skeleton:
        raise RuntimeError("Imported Y Bot has no Skeleton")
    animations = import_animations(skeleton)
    animation_blueprint = create_animation_blueprint(skeleton, animations)
    character_blueprint = configure_character_blueprint(mesh, animation_blueprint)
    game_mode_blueprint = configure_game_mode(character_blueprint)
    create_test_map(game_mode_blueprint)
    unreal.EditorAssetLibrary.save_directory("/Game/OperationMouse/Characters", only_if_is_dirty=False, recursive=True)
    unreal.log("OM_PROTOTYPE_BUILD_SUCCESS")


main()
