# Sprint 3 Mission Gameplay Foundation

## Scope and ownership

This branch contains Yusuf's reusable Mission Base gameplay foundation and its
minimum server-authoritative multiplayer integration. `AOMMissionManager`
replicates the public Mission snapshot while clients reuse the established
Interaction Component Server RPC. This enables ordinary Listen Server Mission
use without claiming Hilmi's hardening or network acceptance.

The foundation is intentionally independent of Level Blueprint gameplay,
Kitchen content, Cat rescue and any one mission's art or mission script.

## Reusable foundation

- `UOMMissionDefinition` is an optional data asset containing the stable
  `MissionId` and objective target. The manager also exposes safe editable
  fallbacks for the technical test harness.
- `AOMMissionManager` owns the Mission ID, objective target/progress, public
  state delegates and transition validation. Its Mission ID, target, progress
  and state are replicated with RepNotify so client presentation follows the
  server's snapshot.
- `AOMMissionInteractionActor` reuses `IOMInteractableInterface`; its five
  instance-configured actions are Start, Complete Objective, Fail, Reset and
  Retry. It is a thin adapter, not a second interaction system. A consumed
  objective fixture is replicated so duplicate requests cannot award it twice.

## Gameplay state flow

```text
Inactive --StartMission--> Active --CompleteObjective--> Completed
                              |
                              +--FailMission--> Failed

Failed/Completed --ResetMission--> Inactive
Failed/Completed --RetryMission--> Active
```

`CompleteObjective` accepts only a positive progress amount while the Mission
is Active. Start accepts only Inactive; Fail accepts only Active; Retry accepts
only Failed or Completed. State-changing calls reject non-authority callers;
clients use the existing `UOMInteractionComponent::ServerBeginInteraction`
path, where distance, line of sight, target validity and `CanInteract` are
checked on the server. Rejected requests produce a `[Mission][Reject]` log,
while accepted transitions produce `[Mission][State]`. This keeps
invalid state changes visible rather than silently mutating mission state.

## Technical test harness

`L_Sprint3_MissionTest` is a compact daylight graybox with two PlayerStarts,
the prototype GameMode, one Mission Manager and five readable existing-
interaction fixtures:

1. Start Mission
2. Complete Objective
3. Fail Mission
4. Reset Mission
5. Retry Mission

It supports both manual routes:

- Start -> Objective -> Completed
- Start -> Fail -> Failed -> Reset -> Start -> Objective -> Completed

## Automated evidence

- OperationMouseEditor Win64 Development: PASSED
- Targeted Mission replication/interaction structural validation: PASSED
- Sprint 2 Carry regression: PASSED
- Sprint 3 Heavy Carry regression: PASSED
- `L_Sprint3_MissionTest` Map Check: 0 errors / 0 warnings

Automated validation checks the replicated public snapshot, the absence of a
second Mission-specific RPC path, reuse of the existing authoritative
Interaction Component flow, duplicate-objective protection, map
ownership/configuration and Map Check. It does not replace a manual Host/Client
network test.

## Hilmi network handoff

The basic server-authoritative snapshot and normal client interaction request
flow are now present. Hilmi still owns the following hardening and acceptance
work:

1. Expand the snapshot when a mission has a dynamic objective list, rewards or
   per-player visibility beyond the current Mission ID/state/target/progress.
2. Approve and extend server validation for all production objective rules and
   player eligibility beyond this generic test fixture.
3. Define deterministic 2/3/4-player contention rules, including simultaneous Start,
   Complete and Reset/Retry requests. A single completion must not produce
   duplicate progress or rewards.
4. Defined disconnect, destruction and late-state behavior. On a player loss,
   the active Mission and critical objective objects must recover without stale
   state, duplication or critical-loot loss.
5. Host/Client, 2-PC and emulated latency/packet-loss evidence proving each
   player receives the same active/failed/completed state and objective
   progress without prediction-only UI or stale interaction prompts.

## Acceptance status

- `MANUAL SINGLE PLAYER TEST: PASSED`
- `MANUAL 2-PLAYER NETWORK RETEST: PENDING`
- `HILMI NETWORK ACCEPTANCE: PENDING`
- `GAMEPAD MANUAL TEST: PENDING`
