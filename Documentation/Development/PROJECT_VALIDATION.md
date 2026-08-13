# Project Validation

`Scripts/ValidateProject.ps1` is a read-only health and regression check for the approved content currently present on `main`. It reports short PASS/FAIL lines and keeps detailed Unreal/build logs under `Saved/Validation/`.

## Run QUICK

From the repository root:

```powershell
.\Scripts\ValidateProject.ps1 -Mode Quick
```

QUICK checks repository hygiene, working-tree state, Git LFS coverage, core project files/classes, approved Enhanced Input assets and mappings, project asset loading, redirectors, duplicate Skeleton names, automatic map discovery/loading, and headless project/module startup. It does not rebuild the C++ targets.

## Run FULL

```powershell
.\Scripts\ValidateProject.ps1 -Mode Full
```

FULL runs every QUICK check, then builds both `OperationMouseEditor Win64 Development` and `OperationMouse Win64 Development`.

## Reading the result

- `PASS` means that specific automated check succeeded.
- `FAIL` includes a short cause. Open the reported folder under `Saved/Validation/` for the complete Unreal or build log.
- The script returns exit code `0` only when every check passes; otherwise it returns `1`.

The validator intentionally follows assets and maps available on the current approved branch. It does not require unmerged prototype-character or Phase 5 content.

Automated validation does **not** prove gameplay feel, multiplayer ownership, replication under latency, or visual correctness. Host + Client PIE verification remains mandatory for gameplay milestones.

