# Sprint 2 Grab / Carry / Drop Regression Tests

Run manual cases in `L_Sprint2_CarryTest`. Server-authoritative Carry state and
Drop requests are implemented, but multiplayer acceptance is not claimed until
the pending Host + Client retest passes.

| ID | Test | Current evidence |
| --- | --- | --- |
| C01 | Launch Sprint 2 map | Automated map load **PASSED**; manual pending |
| C02 | Prototype Character spawns | Automated GameMode/pawn chain **PASSED**; manual pending |
| C03 | Carryable receives local focus/prompt | **MANUAL PENDING** |
| C04 | Interact grabs available Carryable A | **MANUAL PENDING** |
| C05 | Carry state becomes active and object remains visible | **MANUAL PENDING** |
| C06 | Player moves and rotates camera while carrying | **MANUAL PENDING** |
| C07 | Interact again drops cleanly | **MANUAL PENDING** |
| C08 | Dropped object can be grabbed again | **MANUAL PENDING** |
| C09 | A second object is rejected while already carrying | Code eligibility check present; **MANUAL PENDING** |
| C10 | Null, invalid and non-carryable targets reject safely | Code eligibility check present; **MANUAL PENDING** |
| C11 | Destroyed carried object clears Carry state | Destroy callback present; **MANUAL PENDING** |
| C12 | Sprint 1 Button still completes | **MANUAL REGRESSION PENDING** |
| C13 | Sprint 1 Door still opens | **MANUAL REGRESSION PENDING** |
| C14 | Existing interaction prompt/focus remains usable | Interaction component/interface structure **PASSED**; **MANUAL PENDING** |
| C15 | Accepted Sprint 1 map still loads | Automated Sprint 1 validation **PASSED** |

## Multiplayer Carry retest (pending)

1. Client grabs Carryable A: Client, Server and other Client windows show the
   same holder and CarryPoint attachment.
2. The owning Client presses Interact again: server-authoritative Drop is
   visible in all windows and the object becomes interactable again.
3. Host repeats Grab and Drop with the same result.
4. Host and Client request the same available object as closely together as
   practical: exactly one becomes holder; the loser stays Idle.
5. Reset while an object is carried: all windows converge to no holder, Idle
   Character Carry state and one available object at its home transform.
6. Destroy/end the holder during Carry: the authoritative relationship clears
   without a duplicate or stale carried object.

## Manual Standalone route

1. Start at `START` and approach `CARRYABLE A`.
2. Confirm `[E] Grab`, press `E`, and verify the cube stays stable in front of
   the Character.
3. Walk, turn the Character and rotate the camera while carrying.
4. Enter `DROP AREA`, press `E`, and verify collision/physics return.
5. Grab the dropped object again, drop it, then repeat with `CARRYABLE B`.
6. While carrying, verify the same Interact input drops instead of grabbing a
   second object.
7. Use `RESET / RECOVERY`; both objects must return to their original positions
   and remain usable.
8. Run the accepted Sprint 1 Button, Door and prompt checks in
   `L_Phase5_InteractionTest`.
9. Move the mouse left/right and confirm the Character yaw stays aligned with
   the camera while mouse pitch changes only the camera.
10. Confirm the route, Character, Carryables and labels remain clearly readable
    without a lighting-rebuild warning.

## Acceptance boundary

- `MANUAL GAMEPLAY TEST: PENDING`
- `MULTIPLAYER MANUAL TEST: PENDING`
- `GAMEPAD MANUAL TEST: PENDING`

Host/Client Carry synchronization and contention require the pending manual
retest. Final disconnect polish, late join and latency evidence remain network
follow-up work and are not pass claims.

Technical evidence for this implementation: Editor and Game Development builds
passed, Sprint 1 and Sprint 2 automated validation passed, and
`L_Sprint2_CarryTest` Map Check reported 0 errors / 0 warnings.
