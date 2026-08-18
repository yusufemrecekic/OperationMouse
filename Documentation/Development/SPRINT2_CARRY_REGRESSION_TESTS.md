# Sprint 2 Grab / Carry / Drop Regression Tests

Run manual cases in `L_Sprint2_CarryTest`. The implementation is a Yusuf-owned
gameplay foundation; Hilmi's authority/replication pass is intentionally not
claimed by this document.

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

## Acceptance boundary

- `MANUAL GAMEPLAY TEST: PENDING`
- `NETWORK AUTHORITY TEST: PENDING`
- `GAMEPAD MANUAL TEST: PENDING`

Host/Client contention, disconnect, late join, latency and final Carry
replication remain Hilmi-owned follow-up work. These are not Sprint 2 Step 1
pass claims.

Technical evidence for this implementation: Editor and Game Development builds
passed, Sprint 1 and Sprint 2 automated validation passed, and
`L_Sprint2_CarryTest` Map Check reported 0 errors / 0 warnings.
