# Sprint 3 Heavy Carry Gameplay Foundation

## Scope and ownership

This branch contains Yusuf's Heavy Carry gameplay foundation. It reuses the
accepted Sprint 2 Carry component. A minimal replicated state/holder and
collision-consistency layer keeps owning-client CharacterMovement prediction
aligned with the server; it does not replace Hilmi's authoritative contention,
disconnect, late-join, latency or physical-network evidence pass.

## Gameplay state flow

- `Idle` (`0` holders): physics/collision behave like a dropped object.
- `WaitingForSecondHolder` (`1` holder): the object is frozen at its current
  transform and waits for a second distinct Character.
- `Carrying` (`2` holders): the object follows the midpoint of the two gameplay
  CarryPoints, the first/second Characters align to `LeftCarrySlot` and
  `RightCarrySlot`, and both receive the Heavy Carry movement penalty. The
  separate slots keep the Characters outside the Heavy Carryable volume.
- If one of two holders releases, the object freezes and returns to `Waiting`;
  the remaining Character keeps its hold without a speed penalty.
- If the final holder releases, the object returns to the normal dropped/idle
  presentation.
- Reset releases every gameplay holder, clears movement penalties and restores
  the Heavy Carryable to its home transform.

## Test harness and automated evidence

`L_Sprint3_HeavyCarryTest` contains two PlayerStarts, one accepted normal
Carryable regression fixture, one Heavy Carryable, one Reset fixture, prototype
Characters and a compact version of the accepted daylight graybox setup.

- OperationMouseEditor Win64 Development: PASSED
- Sprint 2 automated Carry regression: PASSED
- Heavy Carry structural validation: PASSED
- L_Sprint3_HeavyCarryTest Map Check: 0 errors / 0 warnings
- OperationMouse Win64 Development: PASSED

Automated validation proves class/map structure and guards the ownership
boundary. It does not replace manual gameplay or multiplayer evidence.

## Final manual evidence

The final two-player Listen Server retest PASSED:

- Small Carry blocks other Characters and world geometry without pushing them.
- Small Carry obstruction clamps cargo and holder movement in the blocked direction.
- Heavy Carry client correction/jitter is resolved.
- Close holder spacing remains stable, including while jumping.
- Heavy Carry blocks Pawn/world obstruction without penetration or physics pushing.
- Release and Reset clear holder state, penalties and collision exceptions correctly.

This is Yusuf gameplay acceptance. It is not Hilmi network acceptance and does
not claim contention, disconnect, late-join, latency/loss or physical two-PC evidence.

## Hilmi network handoff

Hilmi must design and approve the authoritative network contract before this
system can be considered multiplayer-ready:

1. Replicate the Heavy Carry state and the two authoritative holder identities.
2. Add server-validated join/leave/drop/reset requests using the existing
   Interaction/Carry request path where practical.
3. Define deterministic first/second-slot contention and simultaneous-request
   rejection; never allow duplicate holders or one Character in both slots.
4. Reconcile holder CarryComponent state, movement penalty and Heavy Carryable
   presentation for owner, simulated peers and late packets.
5. On disconnect/destruction, release the missing holder, clear stale Character
   state and recover/drop the critical object without duplication or loss.
6. Validate movement under latency and packet loss without prediction fighting
   or permanent state locks.
7. Record Host/Client, two-PC, contention, disconnect, recovery and adverse
   network evidence before network acceptance.

## Acceptance status

- `MANUAL HEAVY CARRY RETEST: PASSED` for Waiting/Carrying, slots, obstruction,
  holder stability, jump stability, Release and Reset.
- `NETWORK HEAVY CARRY TEST: PENDING`
- `GAMEPAD MANUAL TEST: PENDING`
