"""Read-only Unreal content checks used by Scripts/ValidateProject.ps1."""

import json
import os
import traceback

import unreal


PROJECT_ROOT = "/Game/OperationMouse"
RESULT_PATH_ENV = "OM_VALIDATION_RESULT_PATH"

INPUT_ACTIONS = {
    "Move": "/Game/OperationMouse/Input/IA_Move.IA_Move",
    "Look": "/Game/OperationMouse/Input/IA_Look.IA_Look",
    "Jump": "/Game/OperationMouse/Input/IA_Jump.IA_Jump",
    "Sprint": "/Game/OperationMouse/Input/IA_Sprint.IA_Sprint",
    "Crouch": "/Game/OperationMouse/Input/IA_Crouch.IA_Crouch",
}
MAPPING_CONTEXT_PATH = "/Game/OperationMouse/Input/IMC_Gameplay.IMC_Gameplay"
NATIVE_CLASSES = {
    "AOMGameMode": "/Script/OperationMouse.OMGameMode",
    "AOMGameState": "/Script/OperationMouse.OMGameState",
    "AOMPlayerState": "/Script/OperationMouse.OMPlayerState",
    "AOMPlayerController": "/Script/OperationMouse.OMPlayerController",
    "AOMMouseCharacter": "/Script/OperationMouse.OMMouseCharacter",
    "UOMTraversalComponent": "/Script/OperationMouse.OMTraversalComponent",
}


results = []


def add_result(name, passed, details):
    results.append({
        "name": name,
        "passed": bool(passed),
        "details": str(details),
    })
    status = "PASS" if passed else "FAIL"
    unreal.log(f"OM_PROJECT_VALIDATION|{status}|{name}|{details}")


def normalize_key_name(key):
    try:
        return str(key.get_editor_property("key_name"))
    except Exception:
        return str(key)


def validate_native_classes():
    missing = []
    for class_name, class_path in NATIVE_CLASSES.items():
        if unreal.load_class(None, class_path) is None:
            missing.append(class_name)

    add_result(
        "Core gameplay classes",
        not missing,
        "Loaded approved native classes from the OperationMouse module."
        if not missing else f"Could not load: {', '.join(missing)}",
    )


def validate_input():
    loaded_actions = {}
    missing_actions = []
    for action_name, asset_path in INPUT_ACTIONS.items():
        action = unreal.EditorAssetLibrary.load_asset(asset_path)
        if action is None:
            missing_actions.append(action_name)
        else:
            loaded_actions[action_name] = action

    add_result(
        "Enhanced Input assets",
        not missing_actions,
        "Move, Look, Jump, Sprint, and Crouch actions load."
        if not missing_actions else f"Missing actions: {', '.join(missing_actions)}",
    )

    mapping_context = unreal.EditorAssetLibrary.load_asset(MAPPING_CONTEXT_PATH)
    if mapping_context is None:
        add_result("IMC_Gameplay mappings", False, "IMC_Gameplay could not be loaded.")
        return

    action_keys = {name: [] for name in INPUT_ACTIONS}
    duplicates = []
    seen = set()
    raw_mappings = []
    try:
        mapping_data = mapping_context.get_editor_property("default_key_mappings")
        mappings = mapping_data.get_editor_property("mappings")
        for mapping in mappings:
            action = mapping.get_editor_property("action")
            key = mapping.get_editor_property("key")
            action_name = action.get_name() if action else "<None>"
            key_name = normalize_key_name(key)
            raw_mappings.append(f"{action_name}:{key_name}")
            pair = (action_name, key_name.lower())
            if pair in seen:
                duplicates.append(f"{action_name}:{key_name}")
            seen.add(pair)

            for logical_name, loaded_action in loaded_actions.items():
                if action_name == loaded_action.get_name():
                    action_keys[logical_name].append(key_name)
                    break
    except Exception as error:
        add_result("IMC_Gameplay mappings", False, f"Could not inspect mappings: {error}")
        return

    lowered = {
        name: {key.lower().replace(" ", "") for key in keys}
        for name, keys in action_keys.items()
    }
    required = {
        "Move": [{"w"}, {"a"}, {"s"}, {"d"}],
        "Jump": [{"spacebar", "space"}],
        "Sprint": [{"leftshift"}],
        "Crouch": [{"leftcontrol", "leftctrl"}],
    }
    missing_mappings = []
    for action_name, key_groups in required.items():
        actual = lowered[action_name]
        for alternatives in key_groups:
            if not actual.intersection(alternatives):
                missing_mappings.append(
                    f"{action_name}:{'/'.join(sorted(alternatives))}")

    look_keys = lowered["Look"]
    has_look = (
        bool(look_keys.intersection({"mouse2d", "mousexy", "mousex/y"}))
        or ({"mousex", "mousey"}.issubset(look_keys))
    )
    if not has_look:
        missing_mappings.append("Look:Mouse2D or MouseX+MouseY")

    passed = not missing_mappings and not duplicates
    details = "; ".join(
        f"{name}=[{', '.join(keys) or 'none'}]"
        for name, keys in action_keys.items()
    )
    if missing_mappings:
        details += f"; missing={', '.join(missing_mappings)}"
    if duplicates:
        details += f"; duplicates={', '.join(duplicates)}"
    details += f"; raw={', '.join(raw_mappings) or 'none'}"
    add_result("IMC_Gameplay mappings", passed, details)


def discover_project_assets():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    return list(registry.get_assets_by_path(PROJECT_ROOT, recursive=True))


def validate_content_and_maps():
    asset_data_items = discover_project_assets()
    load_failures = []
    redirectors = []
    skeleton_paths_by_name = {}
    map_packages = []

    for asset_data in asset_data_items:
        package_name = str(asset_data.package_name)
        asset_name = str(asset_data.asset_name)
        class_name = str(asset_data.asset_class_path.asset_name)

        if class_name == "World":
            map_packages.append(package_name)
        if class_name == "ObjectRedirector":
            redirectors.append(package_name)
        if class_name == "Skeleton":
            skeleton_paths_by_name.setdefault(asset_name.lower(), []).append(package_name)

        try:
            if asset_data.get_asset() is None:
                load_failures.append(package_name)
        except Exception as error:
            load_failures.append(f"{package_name} ({error})")

    add_result(
        "Unreal package loading",
        not load_failures,
        f"Loaded {len(asset_data_items)} project assets."
        if not load_failures else f"Failed packages: {', '.join(load_failures[:8])}",
    )
    add_result(
        "Redirectors",
        not redirectors,
        "No redirectors detected."
        if not redirectors else f"Detected: {', '.join(redirectors)}",
    )

    duplicate_skeletons = [
        paths for paths in skeleton_paths_by_name.values() if len(paths) > 1
    ]
    if not skeleton_paths_by_name:
        skeleton_details = "No approved Skeleton asset set exists on main; check not applicable."
    else:
        skeleton_details = f"Inspected {len(skeleton_paths_by_name)} Skeleton asset name(s)."
    if duplicate_skeletons:
        skeleton_details = "Duplicate Skeleton names: " + "; ".join(
            ", ".join(paths) for paths in duplicate_skeletons)
    add_result("Skeleton duplicates", not duplicate_skeletons, skeleton_details)

    if not map_packages:
        add_result("Map discovery", False, f"No maps found below {PROJECT_ROOT}.")
        return

    add_result(
        "Map discovery",
        True,
        f"Discovered {len(map_packages)} map(s): {', '.join(sorted(map_packages))}",
    )
    for map_package in sorted(map_packages):
        try:
            world = unreal.EditorLoadingAndSavingUtils.load_map(map_package)
            add_result(
                map_package.rsplit("/", 1)[-1],
                world is not None,
                "Headless map load succeeded." if world is not None else "Map returned no World.",
            )
        except Exception as error:
            add_result(map_package.rsplit("/", 1)[-1], False, str(error))


def write_results(unexpected_error=None):
    result_path = os.environ.get(RESULT_PATH_ENV)
    if not result_path:
        raise RuntimeError(f"Environment variable {RESULT_PATH_ENV} was not set.")

    if unexpected_error:
        add_result("Unreal validation script", False, unexpected_error)

    os.makedirs(os.path.dirname(result_path), exist_ok=True)
    payload = {
        "overall_passed": all(item["passed"] for item in results),
        "checks": results,
    }
    with open(result_path, "w", encoding="utf-8") as result_file:
        json.dump(payload, result_file, indent=2)


def main():
    unexpected_error = None
    try:
        validate_native_classes()
        validate_input()
        validate_content_and_maps()
    except Exception:
        unexpected_error = traceback.format_exc()
        unreal.log_error(unexpected_error)
    finally:
        write_results(unexpected_error)


main()
