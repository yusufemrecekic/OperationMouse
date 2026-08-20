# Sprint 3 Heavy Carry Gameplay Foundation

## Scope and ownership

This branch contains Yusuf's gameplay-only Heavy Carry foundation. It reuses
the accepted Sprint 2 Carry component and deliberately does not add Heavy Carry
RPCs, replicated holder properties, contention policy, disconnect handling or
latency behavior. Those items belong to Hilmi's follow-up network pass.

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

- Initial manual evidence: first holder entered Waiting, second holder activated
  Heavy Carry, and both players moved the object together.
- `MANUAL HEAVY CARRY RETEST: PENDING` for separate slot alignment and the
  compact technical map.
- `NETWORK HEAVY CARRY TEST: PENDING`
- `GAMEPAD MANUAL TEST: PENDING`
