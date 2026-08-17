# Sprint 2 Phase 5 Donor Map

**READ-ONLY AUDIT — no Grab / Carry / Drop gameplay was implemented.**

## Compared sources

- Current baseline: `main` at `bc59790`
- Old donor branch: `feature/interaction-foundation` at `8e7b407`
- Preserved stash: `stash@{0}` at `1766451` (metadata inspected only)

The old branch has no unique interaction source file that is missing from
current `main`. `OMInteractableInterface`, `OMInteractionPromptWidget` and the
Character interaction wiring are byte-equivalent at the compared revisions.
Current `main` supersedes the old `OMInteractionComponent` with stable rejection
reasons/logging and supersedes the old generic test actor with the five-role
Sprint 1 harness.

The stash was not applied, popped, dropped or modified. It contains only an old
binary map change:

| Source | LFS SHA-256 | Size |
| --- | --- | ---: |
| Old branch map | `702daa2c8df18fb25ee0e5f265acf405805d81666321d15fd4d65c1ba0c6871d` | 88,234 bytes |
| Preserved stash map | `f5f62ed284f9a962af6931f2a59c40e258c7564d9628bd0647b363ac21922af3` | 88,630 bytes |
| Current `main` map | `b3379c2eb340272267dc96d77e835d10d9218cc9ff3f62791976765c9259355c` | 88,149 bytes |

These are three different `L_Phase5_InteractionTest.umap` binaries. The stash
contains no source-code donor and must not overwrite the current map.

## Exact donor decisions

### Interaction contract

| File / class | Important functions/data | Current behavior | Decision | Reason |
| --- | --- | --- | --- | --- |
| `Source/OperationMouse/Interaction/OMInteractableInterface.h/.cpp` — `IOMInteractableInterface`, `FOMInteractionInfo` | `GetInteractionInfo`, `GetInteractionPoint`, `CanInteract`, `BeginInteraction`, `CancelInteraction`, `CompleteInteraction`; `Instant`, `Hold`, distance and exclusivity | Blueprint-friendly generic interaction lifecycle with harmless defaults | **REUSE AS-IS** | A carryable can implement the same lifecycle; no Carry-specific method is proven necessary yet. Do not extend the interface speculatively. |

### Local focus, interaction flow and network request

| File / class | Important functions/data | Current behavior | Decision | Reason |
| --- | --- | --- | --- | --- |
| `Source/OperationMouse/Interaction/OMInteractionComponent.cpp/.h` — `UOMInteractionComponent` | `FindLocalFocus`, `GetFocusedActor`, `IsCharacterStateValid` | Local camera-direction sphere sweep, interface check and grounded-state focus gating | **REUSE AS-IS** | Responsive local focus is independent of Carry authority. Carry restrictions may later be supplied through `CanInteract`, not a rewritten trace. |
| Same component | `BeginInteractionInput`, `EndInteractionInput`, `TickComponent`, `CancelActiveInteraction` | Starts/cancels Instant or server-timed Hold interactions | **ADAPT** | Keep interaction Hold separate from persistent Carry. `ActiveInteractionTarget` must never be reinterpreted as the carried object. |
| Same component | `ServerBeginInteraction`, `ServerEndInteraction`, `IsServerInteractionValid`, `ClientInteractionRejected` | Reliable owner RPC, target/interface/state/range/LOS/availability validation and reason logs | **ADAPT** | Strong donor, but it is provisional network code. Hilmi must approve RPC/authority rules and decide how validated completion hands off to Carry. Do not copy the older branch version because it lacks reason output. |
| Same component | `ActiveInteractionTarget`, `ActiveHoldServerStartTime`, `ActiveHoldDuration`, `OnRep_ActiveInteraction`, `GetLifetimeReplicatedProps` | Owner-only replicated Hold progress using synchronized server time | **REUSE AS-IS for Hold; DO NOT USE for Carry state** | Hold progress ends at completion. Carry persists after completion and needs separately reviewed replicated holder/object state. |

### Prompt and input bridge

| File / class | Important functions/data | Current behavior | Decision | Reason |
| --- | --- | --- | --- | --- |
| `Source/OperationMouse/Interaction/OMInteractionPromptWidget.cpp/.h` — `UOMInteractionPromptWidget` | `NativeOnInitialized`, `SetPromptState`, `HidePrompt` | Local-only text plus Hold progress bar | **REUSE AS-IS**, then **ADAPT only if Ali requires Carry feedback** | No network state belongs in the widget. A simple Drop prompt can be added later without replacing the widget. |
| `Source/OperationMouse/Characters/OMMouseCharacter.cpp/.h` — `AOMMouseCharacter` | `StartInteraction`, `StopInteraction`, `SetupPlayerInputComponent`; `InteractionComponent`, `InteractAction` | Creates the interaction component and routes `IA_Interact` Started/Completed/Canceled | **ADAPT minimally** | Add the Carry component and route an approved Drop action. Prefer the existing Interact action unless Yusuf/Ali choose a separate control; do not disturb movement code. |
| `Content/OperationMouse/Input/IA_Interact.uasset`, `IMC_Gameplay.uasset` | Existing `E` / Gamepad Face Button Left mapping | One local Interact action for both devices | **REUSE AS-IS provisionally** | A Grab-on-interact / Drop-while-carrying rule avoids binary input edits. Physical gamepad evidence remains pending. |

### Test actors, Pickup proxy and reset

| File / class | Important functions/data | Current behavior | Decision | Reason |
| --- | --- | --- | --- | --- |
| `Source/OperationMouse/Interaction/OMTestInteractableActor.cpp/.h` — `AOMTestInteractableActor` | `CanInteract`, `BeginInteraction`, `CancelInteraction`, `CompleteInteraction`; `bActivated`, `ActiveInteractor`, `CompletionCount` | Server-gated exclusive test claim and replicated one-shot result | **REUSE AS-IS only for Sprint 1 regression** | It is a test harness actor, not a production Carry base. Gameplay outcomes and network state are intentionally combined for test simplicity. |
| Same actor | `BuildInteractionInfo`, `GetRoleLabel`, `UpdateVisualState` | Button, Pickup, Door, Fail and Reset visual proxy behavior | **DO NOT USE for real Carry** | Pickup only hides the cube/disables collision; it has no holder, attachment, drop, recovery or physics state. Door/Button visuals are also test-only. |
| Same actor | `ResetTestState`; Reset path in `CompleteInteraction` using `TActorIterator` | Globally resets all `AOMTestInteractableActor` instances | **ADAPT concept; DO NOT COPY implementation** | Useful recovery evidence pattern, but production critical loot needs targeted server-owned home/reset data, not a global test-actor iteration. |
| `Content/OperationMouse/Tests/Maps/L_Phase5_InteractionTest.umap` on current `main` | Five proxies, two PlayerStarts, locomotion fixtures, prototype GameMode | Accepted Sprint 1 regression map | **REUSE AS-IS as regression baseline; DO NOT EDIT for Sprint 2** | Preserve known-good evidence and avoid binary conflicts. Duplicate it to a new Sprint 2 map when implementation begins. |
| Same map on old branch and in `stash@{0}` | Older/saved Phase 5 actor layouts | Binary-only unpublished map variants | **DO NOT USE** | They predate current layout, labels, harness roles and prototype GameMode integration; merging binary actor changes is unsafe. |

### Documentation and validation donors

| File | Current behavior | Decision | Reason |
| --- | --- | --- | --- |
| `Documentation/Development/INTERACTION_GAMEPLAY_NETWORK_CONTRACT.md` | Defines local/server state, RPC validation, contention, cancellation, disconnect and evidence | **ADAPT** | Add explicit Carry state/authority/recovery rules after Yusuf/Hilmi agreement; retain current Interaction contract. |
| `Documentation/Development/DEBUG_LOGGING_STANDARD.md` | Stable result/reason format and repro template | **REUSE AS-IS** | Carry logs should follow the same searchable format. |
| `Scripts/Editor/configure_sprint1_foundation.py`, `validate_sprint1_foundation.py`, `Scripts/ValidateSprint1.ps1` | Deterministic Sprint 1 harness and validation | **REUSE AS-IS for regression; ADAPT pattern into new Sprint 2 scripts** | Do not rewrite Sprint 1 evidence scripts or make them create Carry assets. |

## Ownership conflicts to resolve before reuse

### `UOMInteractionComponent`

The component currently contains both Yusuf and Hilmi concerns:

- **Yusuf:** `BeginInteractionInput`, `EndInteractionInput`, `FindLocalFocus`,
  `IsCharacterStateValid`, prompt meaning and interface lifecycle.
- **Hilmi:** `ServerBeginInteraction`, `ServerEndInteraction`,
  `IsServerInteractionValid`, replicated Hold fields, `OnRep_ActiveInteraction`,
  synchronized timing, contention/cleanup and replication conditions.
- **Ali acceptance:** prompt text/progress placement, detection readability and
  usable range feel.

Do not blindly copy the class into Carry. A new Carry API must be reviewed at
the public gameplay/network boundary. This does not require an immediate large
class split: function-level ownership plus a small `UOMCarryComponent` is enough
for Sprint 2.

### `AOMTestInteractableActor`

`CompleteInteraction`/`ResetTestState` (Yusuf behavior) and
`HasAuthority`/replicated properties/`ForceNetUpdate` (Hilmi behavior) share one
test class. That is acceptable for a disposable harness, but it must not become
the real carryable base. A production carryable should expose gameplay rules to
Yusuf while Hilmi reviews authoritative mutations and replication hooks.

### Prompt/progress

Prompt wording and visibility logic are Yusuf gameplay/UI behavior with Ali
readability acceptance. Owner-only Hold timing is Hilmi's network concern. Carry
presentation must consume replicated state; the UI must never become authority.

## Smallest practical Sprint 2 file plan

### New C++ files

| Proposed file/class | Purpose |
| --- | --- |
| `Source/OperationMouse/Carry/OMCarryComponent.h/.cpp` — `UOMCarryComponent` | Character-owned Grab/Carry/Drop gameplay state, public Blueprint-friendly queries/events, server-only state mutation entry points and recovery coordination. RPC/replication sections require Hilmi ownership. |
| `Source/OperationMouse/Carry/OMCarryableActor.h/.cpp` — `AOMCarryableActor` | One lightweight carryable implementing `IOMInteractableInterface`, safe/home transform, holder claim, collision/attachment state and targeted authoritative reset. Keep presentation tunable. |

Do not add a generic inventory, physics framework, ability system or extra type
file unless compilation proves it necessary.

### Minimal existing C++ edits

- `OMMouseCharacter.h/.cpp`: create `UOMCarryComponent`; minimally route the
  approved Grab/Drop input. Do not change movement, traversal or camera logic.
- `OMInteractionComponent.h/.cpp`: change only the smallest hand-off/API needed
  after Hilmi review. Keep focus, Instant/Hold and validation behavior intact.
- `INTERACTION_GAMEPLAY_NETWORK_CONTRACT.md`: add the agreed Carry state and
  recovery contract before network acceptance.

### Test/validation files when implementation is authorized

- new `Content/OperationMouse/Tests/Maps/L_Sprint2_CarryTest.umap`, duplicated
  from the accepted map rather than modifying it;
- new `Scripts/Editor/configure_sprint2_carry_test.py` and
  `validate_sprint2_carry.py`;
- new `Scripts/ValidateSprint2.ps1` and Sprint 2 regression evidence;
- minimum new fixtures: one critical lightweight carryable and one explicit
  recovery/reset station. The same carryable supports two-player contention.

## Files and assets not to touch

- `AOMGameMode`, `AOMGameState`, `AOMPlayerState`, session or mission classes;
- movement, jump, crouch, mantle and `UOMTraversalComponent` behavior;
- Mixamo mesh, Skeleton, Animation Blueprint, animation sequences and prototype
  Character Blueprint;
- `IA_Interact` / `IMC_Gameplay` unless input design explicitly changes;
- accepted `L_Phase5_InteractionTest.umap` and the preserved stash;
- Heavy Carry, Cat AI, mission, rescue, inventory and polish systems.

## Binary and merge-conflict risks

- `OMMouseCharacter.h/.cpp` is shared by movement, animation, interaction and
  future Carry work: Yusuf coordinates edits before Hilmi changes RPC wiring.
- `OMInteractionComponent.h/.cpp` is the highest Yusuf/Hilmi text-conflict risk;
  agree the API before parallel work.
- `IMC_Gameplay.uasset`, Character Blueprints and maps are binary: only one owner
  edits each at a time.
- Ali owns the new Sprint 2 test-map/readability pass; Yusuf/Hilmi should not
  concurrently save that map.
- Never resolve the stash/current map difference by choosing the old binary.

## Minimum recovery cases

1. Normal Drop clears both holder and object state once.
2. Holder disconnect or destruction safely drops/resets the object.
3. Carryable destruction/invalid reference clears the holder without a stale lock.
4. Out-of-world/kill-volume object returns to its server-owned safe transform.
5. Unsafe drop is rejected or corrected to a safe server transform.
6. Reset produces exactly one usable critical item—never loss or duplication.
7. Late join observes the current holder/object or recovered state.

Critical loot anti-loss is mandatory: no player action, disconnect or physics
failure may leave the vertical slice impossible to complete.

## Minimum multiplayer test matrix

| Mode | Required check |
| --- | --- |
| Standalone PC | Grab, carry, move and drop repeatedly; recovery/reset; Sprint 1 regression |
| Listen Server Host | Host grabs/drops; Client sees holder/object/collision state |
| Listen Server Client | Client grabs/drops; Host sees and validates the same state |
| Host + Client contention | Simultaneous Grab gives exactly one authoritative winner |
| Invalid requests | Distance, LOS, invalid state and already-carried target reject with stable reasons |
| Disconnect / destroy | Unexpected holder/object loss clears claims and recovers critical loot |
| Late join | New Client receives current Carry or recovered state |
| Emulated latency/loss | No duplicate, stuck holder, teleport loop or unsafe client authority |
| Gamepad | **PENDING until physical hardware is available**; never infer from asset validation |

Add regression cases for normal Grab/Drop, contention, invalid request,
disconnect, carryable invalidation, unsafe drop, out-of-world recovery, critical
loot uniqueness, late join and latency. Heavy Carry compatibility requires only
that the lightweight actor/component API is not hard-coded to two-player logic;
no Heavy Carry state should be designed or implemented in Sprint 2.
