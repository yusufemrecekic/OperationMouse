# Sprint 2 Grab / Carry / Drop Readiness

## Status

**READ-ONLY PLAN — IMPLEMENTATION NOT STARTED**

Sprint 2 vertical-slice scope is limited to one lightweight object completing a
Grab / Carry / Drop loop with server validation. Throw, Heavy Carry, Cat AI,
missions and polish are outside this plan.

The preserved Phase 5 stash was inspected only through Git metadata. It contains
one modified interaction test map and was not applied, dropped or changed. The
current `main` interaction implementation is the planning baseline.

## Existing reusable foundation

- `IOMInteractableInterface` exposes availability, interaction point, begin,
  cancel and complete events.
- `UOMInteractionComponent` provides local focus/prompt and a provisional
  server-authoritative request/validation flow.
- `AOMMouseCharacter` remains the shared gameplay base.
- `AOMTestInteractableActor` validates Sprint 1 flow only; its Pickup proxy is
  not a carry implementation.
- `INTERACTION_GAMEPLAY_NETWORK_CONTRACT.md` defines current ownership and
  validation boundaries.

## Responsibility split

### Yusuf — gameplay behavior

- define eligible lightweight carryable behavior;
- define Grab, Carry and Drop state transitions and interaction meaning;
- decide how carrying restricts interaction, jump, crouch and mantle;
- keep carry state in a reusable Character component rather than Level Blueprint;
- define collision behavior and safe drop intent;
- define recovery/reset behavior for lost critical test objects;
- provide designer-facing tuning properties and gameplay logs.

### Hilmi — network behavior

- review the provisional interaction RPC layer before extending it;
- own server authority, RPC validation and replicated carry ownership/state;
- validate target identity, range, line of sight, availability and contention;
- define server-approved attachment/drop transform and physics ownership rules;
- handle disconnect, target destruction, late join and stale-reference cleanup;
- profile Host/Client behavior under temporary latency and packet loss;
- approve relevancy and replication conditions without per-frame transform RPCs.

### Ali — design, art and readability

- select the test object's readable size, weight class and Kitchen-scale context;
- approve prompt wording, carry pose/hold point and visual state readability;
- approve graybox pickup/drop/reset locations and walking clearance;
- decide the expected gameplay response when movement states conflict with carry;
- accept the five-minute test route without requesting final art polish.

## Likely implementation surface

These are candidates for review, not pre-approved edits:

- add a focused `UOMCarryComponent` to `AOMMouseCharacter` for gameplay carry
  state and designer-facing events;
- extend `IOMInteractableInterface` only if the existing completion event cannot
  express a carry claim cleanly;
- add one reusable lightweight carryable actor/base class, with a Blueprint child
  for the test fixture if visual tuning is needed;
- minimally extend `UOMInteractionComponent` to hand an accepted target to the
  carry gameplay component;
- add only the input action/mapping required by the approved Grab/Drop control;
- extend the Sprint test map and validation scripts after the contract is agreed;
- update the network contract and regression evidence with carry-specific rules.

Do not hard-code Mixamo bones or animation names into gameplay. A named hold
scene component/socket may be exposed by the visual Blueprint while authoritative
carry state stays independent of the prototype mesh.

## Main risks

- two clients claiming the same object;
- client-authoritative transforms or unnecessary per-frame RPCs;
- physics/attachment divergence between Host and Client;
- collision pushing, capsule clipping or object tunneling on drop;
- unclear rules for jump, crouch, sprint and mantle while carrying;
- stale ownership after disconnect, death, destruction or level reset;
- lost objects making the vertical slice impossible to finish;
- late joiners seeing the wrong holder/object state;
- expanding into Throw, Heavy Carry or final animation before the basic loop passes.

## Recovery and reset requirements

- record a server-owned safe/home transform for the test object;
- reject unsafe drop locations or choose a safe server drop transform;
- drop or reset the object when its holder disconnects or becomes invalid;
- clear both holder and object references atomically;
- provide an authoritative reset path for out-of-bounds or inaccessible objects;
- ensure Reset cannot duplicate the object or leave two holders;
- log Grab, Drop, Reject and Recovery with stable reasons.

## Required evidence before Sprint 2 acceptance

1. Standalone PC Grab / Carry / Drop loop passes repeatedly.
2. Host and Client can each grab and drop the same test type.
3. Both peers see the correct holder and object state.
4. Simultaneous claims produce exactly one server-approved winner.
5. Distant, occluded and invalid requests are rejected with reason logs.
6. Drop position is server-approved and stable on both peers.
7. Holder disconnect releases or safely resets the object.
8. Out-of-bounds recovery restores one usable object.
9. Late join receives the current carry state.
10. Temporary latency/packet-loss test produces no duplication or stuck ownership.
11. Existing movement and interaction regressions remain green.
12. Physical gamepad evidence remains separate until a device is available.

## Explicitly not in Sprint 2

- Throw or aiming/trajectory systems;
- Heavy Carry or two-player synchronized carry;
- rescue, teammate pull, boost or ledge hang;
- Cat/Robot AI, traps, mission or extraction systems;
- final mouse mesh, carry animation polish, IK or cosmetic work;
- broad physics-object libraries or general inventory;
- Kitchen art production beyond the smallest approved test fixture.
