# Operation: Mouse Development Status

## Current Milestone

Sprint 1 Gameplay Foundation / VS-Y01

## Milestone Progress

Completed: Phase 1 multiplayer framework, Phase 2 Enhanced Input + basic replicated movement, Phase 3 core locomotion, Phase 4 Basic Mantle, and Modular Prototype Character / Animation Layer (PASSED and merged)

Current: Sprint 2 core two-player Grab / Carry / Drop, Drop/Reset synchronization and readability manually passed; controlled v5.1 closeout gate remains PARTIAL pending network edge-case evidence

Next: Hilmi contention, invalid-request logs, disconnect/critical-loot recovery, 3/4-player and physical two-PC evidence; emulated latency/loss before full network approval; physical gamepad evidence remains pending

## Completed

- Production GDD v1.0 reviewed and accepted as the source of truth.
- Initial multiplayer-first technical direction approved.
- Minimum Unreal Engine 5.8 C++ project and `OperationMouse` runtime module created.
- Game and Editor targets generated and verified by UnrealBuildTool.
- `OperationMouseEditor` Win64 Development build completed successfully.
- Headless Unreal Editor startup reached engine initialization and completed map check with 0 errors and 0 warnings.
- Git repository standardized on `main`; Unreal ignore rules and local Git LFS hooks configured.
- Portable VS Code build/project-generation tasks and team collaboration documentation created.
- Active development repository relocated outside OneDrive and verified at `C:\Dev\OperationMouse`.
- Private GitHub repository, initial commit, `origin`, and `main` upstream configured.
- Multiplayer Gameplay Framework foundation completed with project-specific GameMode, GameState, PlayerState, PlayerController, and Character classes.
- Server-side default pawn spawn and possession verified in a headless Unreal runtime smoke test.
- Manual two-player Listen Server PIE test passed with two separate `OMMouseCharacter` instances in the same session.
- Phase 1 network test map created at `Content/OperationMouse/Maps/L_Phase1_NetworkTest.umap`.
- Phase 2 Enhanced Input assets, camera-relative WASD movement, mouse look, and third-person camera implemented.
- `OperationMouseEditor` Win64 Development build and headless input mapping smoke test passed for Phase 2.
- Manual Phase 2 Listen Server test passed for Host/Client movement, camera control, ownership isolation, and bidirectional movement replication.
- Conventional vertical mouse-look correction completed; `OperationMouse` Win64 Development build passed.
- Final Phase 2 Host/Client test passed with conventional mouse directions, working WASD, bidirectional movement visibility, and local-only camera ownership.
- Phase 1 network test map configured as the Editor startup map for continued multiplayer iteration.
- Phase 3 Enhanced Input actions and mappings implemented: Space jump, Left Shift sprint, and Left Ctrl crouch.
- Built-in Character jump/crouch support extended with editable 0.15-second Coyote Time and Jump Input Buffer windows.
- Editable normal/sprint speeds added while preserving built-in CharacterMovement replication and prediction.
- A minimal raised locomotion test platform added to `L_Phase1_NetworkTest` for Coyote Time and Input Buffer checks.
- `OperationMouseEditor` Win64 Development build and automated Phase 3 input asset/mapping validation passed.
- Manual Phase 3 two-player Listen Server PIE verification passed on Host and Client: WASD, mouse camera, jump, sprint, crouch, Coyote Time, Jump Input Buffer, bidirectional movement visibility, and local-only character ownership were verified with no blocking gameplay issue.
- Phase 4 basic static-surface mantle prototype implemented in `UOMTraversalComponent` with low/high profiles, obstacle and top-surface traces, slope/range checks, destination capsule clearance, and transition-path validation.
- Mantle requests are validated from the authoritative Character state on the server; a replicated start/target/time snapshot drives the short transition without per-frame RPCs or a custom CharacterMovementComponent.
- Dedicated `L_Phase4_MantleTest` map created with labeled low, high, too-high, blocked-clearance, and normal-jump test areas.
- `OperationMouseEditor` and `OperationMouse` Win64 Development builds passed; headless Unreal validation confirmed the new map, traversal component, and preserved Phase 2/3 input mappings.
- The first Phase 4 manual Host/Client playtest confirmed low/high mantle, too-high rejection, network visibility, and existing movement, but found camera-direction sensitivity, a bypassable blocked-clearance fixture, and a subtle block-transition hitch.
- Focused Phase 4 fixes use a Character-facing 30-degree detection half-cone, validate the final destination with the real Character capsule, preserve horizontal exit velocity, minimize floor-settling offset, and provide enclosed blocked-clearance plus isolated normal-jump fixtures.
- The focused mantle retest confirmed low/high mantle, too-high and blocked-clearance rejection, Host/Client mantle, and hitch removal. A locomotion follow-up found UE's default 0.05 falling AirControl too weak for arcade steering, so editable built-in AirControl was set to 1.0 for the final retest.
- Final Phase 4 two-player Listen Server PIE verification PASSED on Host and Client: Basic Mantle, Low/High mantle, Too High rejection, Blocked Clearance rejection, controlled Character-forward detection, airborne reverse/strafe/camera-relative steering, ownership isolation, and bidirectional mantle/movement visibility were verified with no blocking gameplay issue.
- Modular prototype visual layer implemented on top of the unchanged `AOMMouseCharacter` gameplay core using `BP_OMMouseCharacter_Prototype` and `BP_OMGameMode_Prototype`.
- Mixamo Y Bot Skeletal Mesh and eight in-place/root-motion-disabled prototype locomotion sequences imported under a prototype-only content tree and verified to share one Skeleton.
- `UOMAnimInstance` exposes generic Ground Speed, Movement Direction, Falling, Grounded, Ascending, Crouched, Sprinting, and Mantling state derived from Character/CharacterMovement data.
- `ABP_OMPrototypeLocomotion` maps visual-only Idle, Walk/Run blend, Jump, Fall, Land, Crouch Idle, and Crouch Walk assets without changing gameplay speeds or movement authority.
- Dedicated `L_PrototypeCharacterTest` map created from the approved Phase 4 mantle test layout with two PlayerStarts and a prototype-only GameMode override.
- Automated prototype asset validation passed: all assets load, all animations use the single Y Bot Skeleton, root motion is disabled, all Blueprints compile, and the test map retains multiplayer and mantle fixtures.
- `OperationMouseEditor` and `OperationMouse` Win64 Development builds passed; uncooked Editor game startup loaded `L_PrototypeCharacterTest`, selected `BP_OMGameMode_Prototype`, and spawned/possessed `BP_OMMouseCharacter_Prototype` successfully.
- Clean no-AnimBP Y Bot reference-pose inspection confirmed a structurally normal upright humanoid; the apparent deformation came from applying `-90` degrees to Pitch through a positional `Rotator` constructor instead of explicitly applying it to Yaw.
- The prototype mesh now uses an explicit `Yaw = -90` visual rotation with the existing `Z = -96` capsule alignment. All eight animation sequences were previewed individually without skeletal collapse, and a runtime AnimBP smoke test showed the character upright, intact, forward-facing, and grounded.
- A manual T-Pose retest was traced to a stale local `main` Editor DLL after switching back to `feature/prototype-character`: `/Script/OperationMouse.OMAnimInstance` was absent, so the AnimBP generated class and runtime AnimInstance could not load. Rebuilding the current branch restored runtime Idle evaluation without changing movement or reimporting animations.
- Prototype validation now fails explicitly when the native animation class, AnimBP generated class, Animation Blueprint mode, or mesh Anim Class is unavailable. `LaunchPrototypeCharacterTest.ps1` builds the current Editor module before opening the test map to prevent recurrence after branch switches.
- Final two-player Listen Server PIE visual verification PASSED on Host and Client: upright Y Bot presentation, Idle, Walk, Run, Jump/Fall/Land, Crouch Idle, Crouch Walk, existing movement/air control/mantle, ownership isolation, capsule/mesh alignment, and bidirectional multiplayer animation visibility were verified with no blocking deformation or separation issue.
- The modular Y Bot visual/animation layer is verified independently from gameplay movement and remains replaceable/retargetable for the future biped mouse.
- Prototype Character Pull Request #5 was merged into `main` at `c04aeeb`.
- Sprint 1 gameplay/network hand-off contract documented with input, authoritative states, validation, replication boundaries, contention, disconnect, and edge-case rules.
- Gamepad defaults added to `IMC_Gameplay`: Left Stick movement, Right Stick look with radial dead zone and conventional vertical direction, Face Button Bottom jump, Left Thumbstick sprint, Face Button Right crouch, and Face Button Left interact.
- The reusable Interaction Interface/ActorComponent foundation was carried forward without Level Blueprint gameplay logic; its existing RPC/replication implementation remains provisional until Hilmi's review.
- `L_Phase5_InteractionTest` now contains the official Sprint 1 Button, Pickup, Door, Expected Fail, and Reset proxy set plus two PlayerStarts and existing locomotion fixtures.
- Sprint 1 automated validation passed for all keyboard/mouse and gamepad mappings, gamepad look modifiers, five interaction proxy roles, two PlayerStarts, Character InteractionComponent, map load, and Map Check with 0 errors / 0 warnings.
- Sprint 1 integration retest fix reoriented all test-map TextRender labels without mirrored scale, separated Button/Pickup/Door/Fail/Reset into a clear 600-UU-spaced route, removed obsolete generic Phase 5 labels, and restored the map override to `BP_OMGameMode_Prototype` so normal Play spawns `BP_OMMouseCharacter_Prototype` while retaining the unchanged `AOMMouseCharacter` gameplay base.
- Final Sprint 1 manual PC verification PASSED for readable labels, separated fixtures, prototype Character spawn, WASD, mouse camera, jump, crouch, Interact, Button, Pickup proxy, Door, Expected Fail, and Reset.
- Sprint 1 Pull Request #6 was merged into `main` at `db4f56b`; `main` contains all three Sprint 1 commits (`80f300d`, `72ac72b`, `020b26b`).
- Sprint 1 closeout evidence now maps Production Control v5.1 onboarding/Sprint items without claiming physical gamepad, Hilmi network acceptance, Kitchen route, or session flow.
- ONB-018 DONE / PASSED: `codex/onb018-revert-exercise` added one temporary Markdown file (`b515864`), merged it through PR #8, removed it with real `git revert` commit `6107caa`, and merged the revert through PR #9; final tree equality and clean status were verified with no gameplay/content/stash impact.
- Sprint 2 donor audit compared `main` with `feature/interaction-foundation` and inspected the preserved stash through Git metadata only. Current `main` supersedes all old interaction source; the stash contains only a distinct obsolete test-map binary and is not a Sprint 2 code donor.
- Sixteen repeatable Sprint 1 Given/When/Then regression cases, a two-minute VS-Y01 loop, evidence status, and a searchable debug/log reproduction standard were documented.
- Sprint 2 Step 1 adds reusable `UOMCarryComponent` Idle/Carrying gameplay state, a dedicated skeleton-independent CarryPoint, and `AOMCarryableActor` integration through the existing interaction interface.
- Carry uses deterministic attachment with collision/physics suspension, clean Drop restoration, re-grab support, destroyed/invalid target cleanup, EndPlay cleanup, and targeted Reset-to-home recovery.
- Dedicated `L_Sprint2_CarryTest` contains two separated carryables, a Drop area, two PlayerStarts, a Reset/recovery fixture, readable route labels, and the prototype GameMode without modifying the accepted Sprint 1 map.
- `OperationMouseEditor` and `OperationMouse` Win64 Development builds, Sprint 1 regression validation, Sprint 2 structural validation, and Sprint 2 Map Check (0 errors / 0 warnings) passed. Manual gameplay acceptance remains pending.
- Initial Sprint 2 manual testing confirmed Standalone Grab / Carry / Drop works and identified two integration issues: the technical map was too dark and Character facing followed movement instead of mouse yaw. The focused follow-up uses controller-yaw Character rotation (`Use Controller Rotation Yaw=true`, `Orient Rotation to Movement=false`) and movable shadowless graybox lights with precomputed lighting disabled. Editor/Game builds, Sprint 1/2 validation, and Map Check (0 errors / 0 warnings) pass; final manual gameplay retest remains pending.
- Two-player Sprint 2 testing exposed that Grab completed only on the server while `UOMCarryComponent::CarriedActor` and the Carryable holder were transient, leaving the owning Client locally Idle and unable to request Drop. The focused network fix replicates the server-owned Character/Carryable relationship, reconciles CarryPoint attachment through `OnRep`, routes owner Drop input through a Character-component Server RPC, and validates the actual holder. Multiplayer manual retest remains pending; final disconnect/late-join/latency evidence is not yet claimed.
- The follow-up Client Drop/Reset desync fix replaces client-local physics restoration with a replicated authoritative Drop/Reset transform plus an incrementing world-state revision, while Unreal `ReplicatedMovement` continues server-authoritative dropped physics. Reset clears velocities and holder/carrier state before teleporting to the stored home transform. The Sprint 2 map now uses a neutral SkyAtmosphere/Directional/SkyLight setup, fixed manual exposure, a technical grid floor, no high-intensity fill-light stack, and route-aligned positive-scale labels. Editor/Game builds, Sprint 1/Sprint 2 validation and Map Check (0 errors / 0 warnings) passed; multiplayer manual acceptance remains pending.
- Final Sprint 2 core Host + Client manual retest PASSED: Client Grab, Client Drop synchronization, Server Grab/Drop, Reset synchronization, Carry movement synchronization, lighting/readability and text readability were verified.
- PR #12 merged into `main` at `6c6e1a7`. The controlled Production Control/Tracker v5.1 closeout audit classifies Sprint 2 as PARTIAL: normal two-player Carry is proven, while simultaneous contention, invalid-request logs, disconnect/critical-loot recovery, 3/4-player regression, physical two-PC topology and latency/loss network approval evidence remain open.

## Repository

Local development root: `C:\Dev\OperationMouse`  
Repository: Private GitHub repository configured  
Primary branch: `main`  
Repository setup: Complete

## Known Issues

- The first build briefly retried compile actions because of low available memory; Unreal Build Accelerator recovered and the build succeeded.
- The headless Editor smoke test initialized successfully, but its scripted quit did not close the process; the test process was stopped after verification.
- No blocking Phase 4 gameplay issue remains after final Host/Client verification.
- Mixamo import still reports recoverable source warnings for missing smoothing-group metadata and bind poses, but clean Reference Pose, all eight individual animation previews, automated asset validation, and the runtime AnimBP smoke test are visually structurally correct.
- Developers must rebuild `OperationMouseEditor` after switching between `main` and `feature/prototype-character`; ignored local DLLs are branch-specific and Git cannot switch them.
- Accepted non-blocking future animation polish: crouch transitions currently snap too abruptly.
- Accepted non-blocking future animation polish: Fall/Landing animation choice, timing, and transition feel require tuning.
- Gamepad hardware feel and Right Stick direction still require the first manual device pass; asset structure and modifiers pass automated validation.
- Sprint 1 PC Button/Pickup/Door/Fail/Reset and two-minute local gameplay evidence passed; the separate physical gamepad pass remains pending.
- Interaction RPC, authority, replication conditions, contention, and disconnect behavior are not network-owner approved until Hilmi reviews the written contract and implementation.
- Sprint 2 Carry now has a server-authoritative replicated holder/object relationship and owning-client Drop request path. Core Host/Client synchronization passed; contention, disconnect/recovery, 3/4-player, physical two-PC and latency/loss evidence remain pending. Active-mission late join Carry state is deferred because controlled v5.1 currently rejects mission-time new joins; same-mission reconnect is V1 OUT.
- Sprint 3 Heavy Carry gameplay foundation is implemented on Yusuf's branch only. Its holder/state replication, authoritative join/leave, contention, disconnect and adverse-network behavior intentionally remain unimplemented for Hilmi's network pass.
- Sprint 3 initial manual gameplay check confirmed first-holder Waiting, second-holder activation and two-player object movement. The follow-up Left/Right slot alignment and compact-map visual retest remains pending and is not yet a milestone pass.
- The authoritative GDD v3.3 / Production Control v5.1 package was supplied as production direction but its controlled source files are not yet tracked under `Documentation/Design/`.

## Technical Decisions

- Multiplayer-first architecture.
- Server-authoritative critical gameplay.
- Built-in `CharacterMovementComponent` first; no custom subclass without a concrete need.
- No Gameplay Ability System for the initial prototype.
- `AOMPlayerState` is part of the approved framework architecture.
- Session logic will be separated into a `UGameInstanceSubsystem`.
- Save data will be separated from the save-management subsystem.
- Authoritative mission logic will be separated from replicated public mission state.
- Gameplay classes will be created only when the active milestone requires them.
- Phase 4 prototype mantle intentionally uses Character physical forward direction; mantle while the camera alone looks backward is accepted for now and may receive movement-intent refinement only in a later reviewed scope.
- Mixamo is a temporary visual layer only. CharacterMovement, traversal, replication, and gameplay components remain independent of Mixamo bones and asset names.
- Normal locomotion remains in-place and driven by built-in `CharacterMovementComponent`; the animation layer derives poses from replicated Character state without transform RPCs or animation authority.
- A future bipedal mouse will replace the prototype Skeletal Mesh/AnimBP through a visual Blueprint and IK Retargeter workflow; future tail/ear bones remain secondary visual concerns.
- Sprint 1 scale decision: preserve the working Character/capsule scale and create the giant-house feeling by scaling the Kitchen environment relative to the Character.
- Gameplay behavior and interactable outcomes belong to Yusuf; interaction RPC/authority/replication approval belongs to Hilmi; scale, graybox, prompt, and visual acceptance belong to Ali.
- Sprint 1 interaction proxies validate interaction flow only. Real Grab/Carry begins in Sprint 2 and is not implemented by the Pickup proxy.

## Active Work

Developer: Yusuf Emre (Heavy Carry gameplay foundation) / Hilmi Tunahan network implementation pending

System: Sprint 3 Heavy Carry Gameplay Foundation

Branch: `feature/yusuf-heavy-carry-foundation`

Main files/assets: `AOMHeavyCarryableActor`, reused `UOMCarryComponent`, character movement-penalty hook, and `L_Sprint3_HeavyCarryTest`

Status: HEAVY CARRY CORE FLOW MANUALLY OBSERVED - SWEPT CARGO OBSTRUCTION / HOLDER STABILITY MANUAL RETEST PENDING - NETWORK HEAVY CARRY TEST PENDING - GAMEPAD MANUAL TEST PENDING - OLD PHASE 5 STASH PRESERVED

## Deferred / Not V1

- New houses and procedural house generation.
- PvP, local split-screen, host migration, workshop/mod support, and live-service systems.
- Full human AI and a dedicated solo AI companion.
- Automatic video replay.
