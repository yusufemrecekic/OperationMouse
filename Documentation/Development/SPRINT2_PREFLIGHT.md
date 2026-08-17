# Sprint 2 Engineering Pre-flight

**READ-ONLY — Grab / Carry / Drop implementation is NOT STARTED.**

## Reuse boundary

- **[GDD]** Reuse `IOMInteractableInterface`, `UOMInteractionComponent`, the
  shared `AOMMouseCharacter` base and the written gameplay/network contract.
- **[DERIVED]** Keep local focus/prompt separate from server-approved world
  state. Do not manually replicate transforms every frame.
- **[RECOMMENDATION]** Isolate carry gameplay state in a small reusable
  `UOMCarryComponent`; approve this surface with Yusuf and Hilmi before coding.
- Do not copy the old Phase 5 test-map stash, test-proxy behavior or provisional
  RPC details blindly. The stash contains only a superseded binary test-map
  change; current `main` is the baseline. Hilmi must review authority behavior.

## Proposed state flow

`Idle → Local Focus → Grab Requested → Server Validated → Carried → Drop Requested → Server Drop → Idle`

Any invalid holder/object, disconnect or unsafe/out-of-world position enters
`Recovery`, clears both references once, restores one valid object at a safe
server-owned transform, then returns to `Idle`.

## Ownership

- **Yusuf — gameplay:** eligibility, Grab/Carry/Drop meaning, movement-state
  restrictions, collision intent, recovery rules and gameplay-facing events.
- **Hilmi — network:** RPC, validation, authority, replicated holder/object
  state, contention, disconnect/late-join behavior and latency profiling.
- **Ali — design/readability/level:** object scale, hold point, prompts, route
  clearance, state readability and safe reset placement.

## Anti-loss and recovery

Critical loot must never become permanently inaccessible or duplicated. The
server records a safe/home transform, rejects or corrects unsafe drops, releases
on unexpected player loss, resets out-of-world objects, and atomically clears
holder/object links. Reset and reconnect must result in exactly one usable item.

## Minimum pass evidence

- repeated Standalone PC Grab/Carry/Drop loop;
- Host and Client can each carry and both see the same authoritative state;
- simultaneous claim has exactly one winner;
- distance, occlusion and invalid-state rejection logs;
- safe server-approved drop on both peers;
- disconnect, destruction and out-of-world recovery without loss/duplication;
- late-join state correctness;
- temporary latency/packet-loss pass;
- existing movement and Sprint 1 interaction regression remains green;
- add regression cases for grab, drop, contention, invalid request, disconnect,
  critical-loot reset, late join and latency.

Do not expand this pre-flight into Throw, Heavy Carry, Cat AI, missions, final
animations, inventory or broad physics tooling.
