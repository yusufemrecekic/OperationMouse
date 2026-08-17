# Sprint 1 Repeatable Regression Tests

Run these tests in `L_Phase5_InteractionTest`. Record branch, commit, input
device, result, and evidence for every pass.

## Closeout evidence snapshot

This table records the evidence available at Sprint 1 merge commit `db4f56b`.
`PASSED` is used only where the recorded manual or automated result supports it.

| Test | Closeout status | Evidence |
| --- | --- | --- |
| R01 Keyboard movement and mouse look | **PASSED** | Manual PC test |
| R02 Gamepad movement and look | **PENDING EVIDENCE** | Bindings/asset validation passed; no physical gamepad |
| R03 Jump and crouch | **PASSED on PC / gamepad pending** | Manual PC test; no physical gamepad |
| R04 Button | **PASSED** | Manual interaction test |
| R05 Pickup proxy | **PASSED** | Manual interaction test |
| R06 Door | **PASSED** | Manual interaction test |
| R07 Expected Fail | **PASSED** | Manual interaction test and reason-based implementation |
| R08 Reset | **PASSED** | Manual interaction test |
| R09 No-target safety | **PENDING EVIDENCE** | Repeatable test exists; not named in final manual result |
| R10 Existing locomotion regression | **PARTIAL** | Current PC movement/jump/crouch passed; full mantle/sprint sweep was not named in final Sprint 1 result |
| R11 Project/map launch | **PASSED** | Runtime smoke test and automated map load |
| R12 Prototype spawn and possession | **PASSED** | Runtime smoke test plus manual prototype visual check |
| R13 Enhanced Input context | **PASSED** | Runtime log and automated mapping validation |
| R14 Interaction prompt | **PASSED** | Manual Interact test |
| R15 Map Check | **PASSED** | 0 errors / 0 warnings |
| R16 Host/Client interaction | **PENDING EVIDENCE** | Hilmi network review and multiplayer interaction pass not completed |

## R01 - Keyboard movement and mouse look

- **Given** Standalone PIE owns the local Character.
- **When** the player uses WASD and moves the mouse in all four directions.
- **Then** movement is camera-relative and camera direction is conventional.

## R02 - Gamepad movement and look

- **Given** a supported XInput gamepad is connected before PIE starts.
- **When** the player uses Left Stick and Right Stick.
- **Then** Left Stick moves camera-relative; Right Stick right/up looks right/up.

## R03 - Jump and crouch on both input devices

- **Given** the Character is on the floor.
- **When** Space / Face Button Bottom is pressed, then Left Ctrl / Face Button
  Right is held and released.
- **Then** jump and crouch work without disabling later movement.

## R04 - Button proxy

- **Given** `BUTTON: READY` is focused.
- **When** Interact is pressed once.
- **Then** it changes once to `BUTTON: COMPLETE`.

## R05 - Pickup proxy

- **Given** `PICKUP: READY` is focused.
- **When** Interact is pressed once.
- **Then** it changes to complete and its cube disappears. This is only an
  interaction proxy; real Carry begins in Sprint 2.

## R06 - Door proxy

- **Given** `DOOR: READY` is focused.
- **When** Interact is pressed once.
- **Then** it changes to complete and visibly rotates open.

## R07 - Expected failure proxy

- **Given** `FAIL: EXPECTED FAIL` is focused.
- **When** Interact is pressed.
- **Then** it does not complete and the log contains
  `Reason=BeginInteractionRejected`.

## R08 - Reset proxy

- **Given** Button, Pickup, and Door are complete.
- **When** the Reset proxy is used.
- **Then** all five proxies return to their initial visual/gameplay state.

## R09 - No-target safety

- **Given** no interaction prompt is visible.
- **When** Interact is pressed and released.
- **Then** no world state changes and no error occurs.

## R10 - Existing locomotion regression

- **Given** the interaction tests are complete.
- **When** the player uses normal movement, jump, sprint, crouch, air control,
  and an existing mantle fixture.
- **Then** all existing locomotion remains usable and no proxy interaction
  changes Character movement architecture.

## R11 - Project and Sprint 1 map launch

- **Given** the current Development Editor build is available.
- **When** the project opens `L_Phase5_InteractionTest`.
- **Then** the map loads without a fatal error and selects the prototype GameMode.

## R12 - Prototype Character spawn and possession

- **Given** normal Play starts in the Sprint 1 map.
- **When** the local player joins the world.
- **Then** `BP_OMMouseCharacter_Prototype` spawns, is possessed, stands upright,
  and retains the `AOMMouseCharacter` gameplay base.

## R13 - Enhanced Input context

- **Given** the local Character is possessed.
- **When** local control initializes.
- **Then** `IMC_Gameplay` is applied once and the required actions are available.

## R14 - Interaction prompt

- **Given** the player approaches an available proxy from its readable side.
- **When** the focus sweep reaches the proxy.
- **Then** the local prompt appears with the correct action and disappears when
  focus is lost.

## R15 - Map validation

- **Given** the Sprint 1 validation script runs against the saved test map.
- **When** the map loads and Map Check executes.
- **Then** all five roles, two PlayerStarts, readable rotations, safe spacing,
  prototype pawn chain and 0 errors / 0 warnings are reported.

## R16 - Host/Client interaction hand-off

- **Given** a Listen Server Host and Client possess separate Characters.
- **When** each player uses an interaction proxy and both contend for an
  exclusive target.
- **Then** persistent results agree on both peers, prompts remain local and only
  one server-approved winner exists. This test remains pending Hilmi review.

## Two-minute VS-Y01 local loop

1. Start Standalone PIE.
2. Move and look with the selected input device.
3. Jump once.
4. Complete Button, Pickup, and Door.
5. Confirm Fail is rejected.
6. Use Reset and confirm all proxies restore.
7. Stop PIE within two minutes with no error.

Keyboard/mouse and gamepad must each receive a separate pass. Automated asset
validation does not replace this manual gameplay check.

