# Interaction Gameplay / Network Contract

## Purpose

This document is the hand-off contract between Gameplay and Network for the
Sprint 1 basic interaction loop. It describes behavior and authority without
requiring a verbal explanation.

## Ownership

- **Gameplay / Yusuf:** input meaning, focus rules, interaction availability,
  interaction type, gameplay result, cancellation rules, and interactable
  actor behavior.
- **Network / Hilmi:** RPC transport, authority checks, replication conditions,
  contention, disconnect cleanup, latency behavior, and network profiling.
- **Design / Ali:** prompts, interaction timing, usable distance, test proxy
  acceptance, and Kitchen readability.

The current interaction branch contains a provisional server-authoritative
network implementation. It is not network-approved until Hilmi reviews it
against this contract.

## Player input

| Action | Keyboard / mouse | Gamepad | Gameplay meaning |
| --- | --- | --- | --- |
| Interact Started | `E` | Face Button Left | Request interaction with the current local focus target. |
| Interact Completed / Canceled | Release `E` | Release Face Button Left | End the input; cancel an unfinished Hold interaction. |

Input is read only by the locally controlled Character. Input never directly
changes authoritative world state.

## Local-only state

The following state must not replicate:

- focused actor;
- focus trace result;
- prompt visibility and prompt text rendering;
- whether the local physical input is currently held.

Local focus exists to provide responsive feedback. It is not proof that the
interaction is valid on the server.

## Authoritative states

| State | Meaning | Exit |
| --- | --- | --- |
| Idle | No accepted interaction. | A valid request is accepted. |
| Holding | Server accepted a Hold interaction and owns its timer. | Complete, cancel, target destruction, invalid state, or disconnect. |
| Completed | The interactable applies its gameplay result on the server. | Interactable decides its next available state. |
| Rejected | Server refused the request. No gameplay result occurs. | Client returns to local focus/idle feedback. |
| Canceled | An accepted Hold ended before completion. | Return to Idle. |

Instant interactions move from validation to Completed during the same server
request and never enter Holding.

## Interactable interface contract

An interactable provides:

- prompt, interaction type, hold duration, usable distance, and exclusivity;
- an interaction point used for distance and line-of-sight validation;
- `CanInteract` for current gameplay availability;
- `BeginInteraction` to claim or reject the interaction;
- `CancelInteraction` to release an unfinished claim;
- `CompleteInteraction` to apply the gameplay result.

Persistent gameplay state belongs to the interactable actor, not the player
prompt and not the map Level Blueprint.

## Begin request

1. Local focus finds an actor implementing the interaction interface.
2. Local input sends a request containing only that target actor.
3. The server obtains the requesting Character from the owning connection; the
   client does not choose the Interactor identity.
4. The server validates all rules below.
5. The server calls `BeginInteraction`.
6. For Instant, the server calls `CompleteInteraction` immediately.
7. For Hold, the server records target, server start time, and duration.

## Required server validation

The network implementation must reject the request when any item is false:

- requester owns the Character that sent the request;
- Character, target, world, and interface are valid;
- Character is in a gameplay state allowed to interact;
- distance to the target interaction point is inside both the target limit and
  the global safety cap;
- line of sight is clear or the first blocking hit is the target;
- `CanInteract` is true at validation time;
- an exclusive target is not already claimed by another Character.

The server must repeat state/range validity while a Hold interaction is active.

## Completion and cancellation

- The server owns Hold timing. Client frame time is never trusted.
- Releasing the input cancels an unfinished Hold.
- Walking out of range, losing an allowed Character state, target destruction,
  or disconnect cancels an unfinished Hold.
- Cancellation releases an exclusive claim.
- Completion occurs once only and applies gameplay state on the server.
- Late joiners receive persistent interactable state through replicated actor
  properties, not by replaying the original interaction RPC.

## Replication contract

- Local focus and prompt are never replicated.
- Active Hold target/start time/duration may replicate owner-only for progress
  display.
- Persistent Button/Pickup/Door state replicates from the interactable actor to
  relevant clients.
- Interaction RPCs do not carry transforms and do not run every frame.
- Gameplay code must not manually replicate Character transforms.

## Contention

For an exclusive target, the first request accepted by the server wins. Later
requests are rejected until the claim completes or is canceled. A client-side
prompt does not reserve a target.

## Required Sprint 1 edge cases

1. Press with no target: no request result and no error.
2. Target becomes invalid between focus and request: reject safely.
3. Player walks away during Hold: cancel and reset progress.
4. Player releases Hold early: cancel and reset progress.
5. Two players request an exclusive target: one winner, one rejection.
6. Completed one-shot target is requested again: reject unless gameplay resets it.
7. Target is destroyed during Hold: cancel without stale references.
8. Character disconnects during Hold: release the claim.
9. Client requests a distant or occluded target: reject on the server.
10. Reset proxy restores all test actors to their initial server state.

## Evidence required before network approval

- Local two-minute Move / Jump / Interact loop passes.
- Listen Server Host and Client can both interact.
- Persistent results are visible to Host and Client.
- Prompts remain local to each player.
- Exclusive contention produces one authoritative winner.
- Hold cancellation passes with temporary latency enabled.
- Logs identify requester, target, accepted/rejected result, and rejection reason.

## Sprint 2 Carry authority contract

Sprint 2 Carry has one server-owned relationship represented on both sides of
the invariant:

- Character `UOMCarryComponent::CarriedActor` replicates to owner and simulated
  clients;
- `AOMCarryableActor::CurrentHolder` replicates to all relevant clients;
- on the server, either both references describe the same relationship or both
  are empty.

Grab uses the existing owning-Character interaction RPC. The server repeats
range, line-of-sight, Character-state and interactable availability validation,
then `CompleteInteraction` calls `TryGrab` on the server. The first server
request that sets `CurrentHolder` wins; later requests see the object as
unavailable.

The owning client receives `CarriedActor`, so the same Interact input routes to
`ServerRequestDrop` on its replicated Character component. The server verifies
that the requesting Character is the Carryable's actual `CurrentHolder` before
detaching and restoring collision/physics. A different client cannot invoke an
RPC on that Character component and cannot pass the holder check.

`OnRep_CurrentHolder` reconciles visual attachment to the holder's
skeleton-independent CarryPoint on owning and simulated clients. Normal actor
attachment/movement replication carries the transform; no multicast or
per-frame transform RPC is used. The Carryable remains server-owned as a world
actor; `CurrentHolder` is gameplay holder identity, not RPC ownership.

Destroyed-object, holder EndPlay, normal Drop and Reset clear the relationship
on the server and replicate Idle/available state. Final disconnect polish,
late-join evidence and latency acceptance remain pending manual/network tests.

