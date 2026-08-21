# Sprint 3 Mission Gameplay Foundation

## Scope and ownership

This branch contains Yusuf's reusable Mission Base gameplay foundation. It is
deliberately local and non-replicated: it provides deterministic gameplay
state and test-harness behavior, but does not claim Hilmi's network authority
or synchronization acceptance.

The foundation is intentionally independent of Level Blueprint gameplay,
Kitchen content, Cat rescue and any one mission's art or mission script.

## Reusable foundation

- `UOMMissionDefinition` is an optional data asset containing the stable
  `MissionId` and objective target. The manager also exposes safe editable
  fallbacks for the technical test harness.
- `AOMMissionManager` owns the Mission ID, objective target/progress, public
  state delegates and transition validation.
- `AOMMissionInteractionActor` reuses `IOMInteractableInterface`; its five
  instance-configured actions are Start, Complete Objective, Fail, Reset and
  Retry. It is a thin adapter, not a second interaction system.

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
only Failed or Completed. Rejected requests produce a `[Mission][Rejected]`
log, while accepted transitions produce `[Mission][Transition]`. This keeps
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
- Targeted Mission structural validation: PASSED
- Sprint 2 Carry regression: PASSED
- Sprint 3 Heavy Carry regression: PASSED
- `L_Sprint3_MissionTest` Map Check: 0 errors / 0 warnings

Automated validation checks the reusable source contract, rejects Mission RPC
or replication additions, validates map ownership/configuration and invokes
Map Check. It does not replace manual or network evidence.

## Hilmi network handoff

The future authoritative public Mission state needs, at minimum:

1. A mission identity/definition reference, `MissionState`, objective target
   and objective progress replicated from the server. Any player-facing
   per-objective completion state must be part of the same authoritative
   snapshot.
2. Server-validated requests for Start, objective completion, Fail, Reset and
   Retry. Validation must reject an inactive/completed/failed action, duplicate
   objective credit and ineligible interacting players.
3. Deterministic 2/3/4-player contention rules, including simultaneous Start,
   Complete and Reset/Retry requests. A single completion must not produce
   duplicate progress or rewards.
4. Defined disconnect, destruction and late-state behavior. On a player loss,
   the active Mission and critical objective objects must recover without stale
   state, duplication or critical-loot loss.
5. Host/Client, 2-PC and emulated latency/packet-loss evidence proving each
   player receives the same active/failed/completed state and objective
   progress without prediction-only UI or stale interaction prompts.

## Acceptance status

- `MANUAL MISSION TEST: PENDING`
- `NETWORK MISSION TEST: PENDING`
- `GAMEPAD MANUAL TEST: PENDING`
