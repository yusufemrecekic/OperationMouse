# Sprint 2 Grab / Carry / Drop Regression Tests

Run manual cases in `L_Sprint2_CarryTest`. The final core Host + Client Carry
retest passed. Sprint 2 remains partial until the controlled production
contention, disconnect/recovery and remaining network evidence is recorded.

| ID | Test | Current evidence |
| --- | --- | --- |
| C01 | Launch Sprint 2 map | Automated map load and final manual route **PASSED** |
| C02 | Prototype Character spawns | Automated GameMode/pawn chain and final manual route **PASSED** |
| C03 | Carryable receives local focus/prompt | Carry interaction works; exact prompt evidence remains **PARTIAL** |
| C04 | Interact grabs available Carryable A | Client and Server Grab **PASSED** |
| C05 | Carry state becomes active and object remains visible | Host/Client Carry state visibility **PASSED** |
| C06 | Player moves and rotates camera while carrying | Carry movement synchronization **PASSED** |
| C07 | Interact again drops cleanly | Client and Server Drop synchronization **PASSED** |
| C08 | Dropped object can be grabbed again | **MANUAL PENDING** |
| C09 | A second object is rejected while already carrying | Code eligibility check present; **MANUAL PENDING** |
| C10 | Null, invalid and non-carryable targets reject safely | Code eligibility check present; **MANUAL PENDING** |
| C11 | Destroyed carried object clears Carry state | Destroy callback present; **MANUAL PENDING** |
| C12 | Sprint 1 Button still completes | **MANUAL REGRESSION PENDING** |
| C13 | Sprint 1 Door still opens | **MANUAL REGRESSION PENDING** |
| C14 | Existing interaction prompt/focus remains usable | Interaction component/interface structure **PASSED**; focused Carry prompt evidence remains **PARTIAL** |
| C15 | Accepted Sprint 1 map still loads | Automated Sprint 1 validation **PASSED** |

## Multiplayer Carry retest

1. Client grabs Carryable A: Client, Server and other Client windows show the
   same holder and CarryPoint attachment.
2. The owning Client presses Interact again: server-authoritative Drop is
   visible in all windows, server physics settles through ReplicatedMovement,
   no Client copy remains suspended, and the object becomes interactable again.
3. Host repeats Grab and Drop with the same result.
4. Host and Client request the same available object as closely together as
   practical: exactly one becomes holder; the loser stays Idle.
5. Reset while an object is carried: all windows converge to no holder, Idle
   Character Carry state and one available object at its exact home transform.
6. Reset an idle and a recently dropped object: every window receives the new
   world-state revision and converges to the same home location and rotation.
7. Destroy/end the holder during Carry: the authoritative relationship clears
   without a duplicate or stale carried object.

Recorded final evidence:

- Steps 1-3: **PASSED** (Client and Server Grab/Drop synchronization).
- Carry movement synchronization: **PASSED**.
- Reset synchronization: **PASSED** for the exercised reset route.
- Lighting and TextRender readability: **PASSED**.
- Step 4 contention: **PENDING**.
- Step 7 holder destruction/disconnect: **PENDING**.
- Three/four-player and physical two-PC topology evidence: **PENDING**.

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

- `CORE MANUAL GAMEPLAY TEST: PASSED`
- `CORE HOST + CLIENT CARRY SYNC TEST: PASSED`
- `SPRINT 2 PRODUCTION GATE: PARTIAL`
- `GAMEPAD MANUAL TEST: PENDING`

Host/Client normal-path Carry synchronization passed. Simultaneous contention,
disconnect/critical-loot recovery, invalid-request reason logs, 3/4-player
regression and physical two-PC topology evidence are not pass claims. Active
mission late join is not a Sprint 2 Gate 2 blocker under controlled v5.1 because
mission-time new join is currently rejected. Emulated latency/loss remains
required before full network-owner approval.

Technical evidence for the Drop/Reset reconciliation and neutral daylight-map
follow-up: Editor and Game Development builds passed, Sprint 1 and Sprint 2
automated validation passed, and `L_Sprint2_CarryTest` Map Check reported
0 errors / 0 warnings. See `SPRINT2_CLOSEOUT_AUDIT.md` for the controlled-source
DONE / PARTIAL / PENDING classification.
