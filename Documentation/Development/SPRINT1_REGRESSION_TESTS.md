# Sprint 1 Repeatable Regression Tests

Run these tests in `L_Phase5_InteractionTest`. Record branch, commit, input
device, result, and evidence for every pass.

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

