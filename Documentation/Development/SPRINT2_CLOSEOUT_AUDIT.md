# Sprint 2 Grab / Carry / Drop Closeout Audit

Audit date: 2026-08-18  
Audited main: `6c6e1a7ea23e9de14b63707102e66d57df48eb1e`

## Result

**SPRINT 2 GATE: PARTIAL**

The accepted Host + Client retest proves the core two-player Grab / Carry / Drop
loop and the corrected Drop, Reset and movement synchronization. It does not yet
prove every duplicate, stuck-state and recovery edge case required by the
controlled production package.

No gameplay or network code was changed by this audit.

## Controlled source wording used

This audit uses the locally supplied controlled production files rather than
promoting the earlier repository readiness note into production policy:

- `Operation_Mouse_Production_Control_v5.1_VERTICAL_SLICE_MARKET_VALIDATION.xlsx`
  - `03 - HAFTA HAFTA`, Sprint 2: `VS Gate 2: 2P grab/carry/drop; duplicate/kalıcı state yok`;
  - `00F - DIKEY DEMO`, `VS-Y02`: two-player test map; object is not lost and state does not lock;
  - `02 - MASTER PLAN`, `OM4-018`, `OM4-020`, `OM4-022`, `OM4-023`, `OM4-024`;
  - `04 - PROGRAMCI PASLASMA`: simultaneous Grab and disconnect are the Interaction + Grab hand-off tests;
  - `09 - QA MATRIX`, `QA-008` and `QA-016`: critical-loot disconnect recovery and impairment testing.
- `Yusuf_Emre_Tracker_v5.1_VERTICAL_SLICE.xlsx`
  - `01A - DETAYLI GOREVLER`, `OM4-018` and `OM4-020`;
  - `06 - DIKEY DEMO`, `VS-Y02`.
- `Operation_Mouse_Beginner_Execution_Guide_v5.1_VERTICAL_SLICE.pdf`
  - section 6: Yusuf owns gameplay state/edge cases; Hilmi owns RPC, authority, duplicate/out-of-order, disconnect and late-packet behavior;
  - section 13: Sprint 2 output is server-validated two-player carrying.
- Repository Production GDD v1.0 sections 25.5, 27.4 and 29.3: critical
  interactions and physics are server-authoritative; system DoD includes
  disconnect testing; network QA includes late join, high ping and packet loss.

The controlled board references GDD v3.3, but the full GDD v3.3 source is not
tracked in this repository. This audit therefore does not invent wording beyond
the controlled v5.1 board/guide and the repository GDD v1.0.

## Recorded manual evidence

The project owner supplied the following final two-player evidence after PR #12:

- Client Grab: **PASSED**
- Client Drop synchronization: **PASSED**
- Server Grab / Drop: **PASSED**
- Reset synchronization: **PASSED**
- Carry movement synchronization: **PASSED**
- Lighting and route readability: **PASSED**
- Text readability: **PASSED**

The earlier Standalone PC Grab / Carry / Drop route, Editor and Game Development
builds, Sprint 1 and Sprint 2 automated validations, and Map Check with 0 errors
/ 0 warnings also remain valid evidence from PR #12.

## Acceptance matrix

| Production requirement | Status | Evidence | Exact remaining gap | Owner |
| --- | --- | --- | --- | --- |
| `VS-Y02`: two-player Grab / Carry / Drop in the test map | **DONE** | Client and Server Grab/Drop passed | None for the normal two-player loop | Yusuf + Hilmi |
| Server-validated authoritative holder/object state | **DONE** | Host/Client Drop, Reset and movement synchronization passed; replicated holder/object implementation is present | Formal network-owner sign-off is tracked separately below | Hilmi |
| Object remains visible and follows Carry movement on both peers | **DONE** | Carry movement synchronization passed | None for the tested route | Yusuf + Hilmi |
| Reset converges both peers to one usable home state | **DONE** | Reset synchronization passed | Out-of-bounds and disconnect-triggered recovery are separate cases | Yusuf + Hilmi |
| Ali visual/design readability acceptance | **DONE** | Lighting, object readability and text readability passed | Final Kitchen art is outside Sprint 2 | Ali |
| Written state/input/output/edge-case and authority contract | **DONE** | `INTERACTION_GAMEPLAY_NETWORK_CONTRACT.md` includes Carry authority, Drop and Reset reconciliation | Keep it updated if Hilmi changes behavior | Yusuf + Hilmi |
| Reusable Carry component/actor and dedicated technical harness | **DONE** | `UOMCarryComponent`, `AOMCarryableActor`, `L_Sprint2_CarryTest` and validation scripts are merged | None | Yusuf |
| Searchable rejection/recovery log evidence | **PARTIAL** | Reasoned Carry/Interaction logs exist in code | Capture one concrete accepted/rejected/recovery log set during Hilmi's network pass | Yusuf + Hilmi |
| Simultaneous Grab gives exactly one winner and no duplicate/permanent state | **PENDING** | Server-side first-winner implementation exists | No recorded simultaneous Host/Client contention test | Hilmi |
| Distant, occluded, invalid and wrong-holder requests reject safely | **PENDING** | Validation and rejection paths exist | No manual/network evidence with reason logs | Hilmi |
| Holder disconnect releases/resets critical loot without stale state | **PENDING** | Character EndPlay cleanup exists | No recorded disconnect-while-carrying test; critical-loot uniqueness is unproven | Hilmi |
| Out-of-bounds critical-loot recovery restores exactly one usable object | **PARTIAL** | Manual Reset synchronization passed | No actual out-of-world/inaccessible-object recovery evidence | Yusuf + Hilmi |
| `OM4-018`/`OM4-020` 2/3/4-player regression | **PARTIAL** | Two-player Host + Client passed | Three-player and four-player evidence is not recorded | Yusuf + Hilmi |
| Physical two-PC evidence | **PENDING** | Two-player test passed, but machine topology was not recorded | Hilmi must record whether the pass used two separate PCs; run it if not | Hilmi |
| Physical gamepad evidence | **PENDING** | Bindings and automated validation pass | No physical gamepad test | Yusuf |

## Gate-critical now versus later network hardening

### Required to close Sprint 2 fully

1. Run simultaneous Host/Client Grab contention and prove one winner, no
   duplicate and no permanently locked state.
2. Disconnect a Client while it carries the critical test object; prove the
   object releases or resets once and remains usable.
3. Capture invalid/distant/occluded/wrong-holder rejection reason logs.
4. Complete the controlled `OM4-018`/`OM4-020` three-player and four-player
   regression requirement, or record an explicit production waiver.
5. Record a real two-PC result for Hilmi's network evidence if the accepted test
   was same-machine PIE.
6. Hilmi signs off authority, RPC, replication, recovery and evidence.

### Not a Sprint 2 Gate 2 blocker

- **Active-mission late join Carry synchronization:** controlled v5.1 currently
  says a new join during a mission is rejected. Test persistent Carry state only
  after the session/mission join policy supports it.
- **Reconnect to the same active mission:** explicitly V1 OUT in `OM4-024`.
- **Host migration:** V1 OUT in the Production GDD.
- **Real-internet impairment testing:** ONB-017 allows it later after the shared
  emulation method exists.

### Required before full network approval, but schedulable with Hilmi hardening

- Emulated high-ping and packet-loss testing is not written into the short
  `VS Gate 2` sentence. It is nevertheless required by the programmer hand-off,
  network task acceptance and project working rules for a network-critical
  milestone. It may be batched with Hilmi's pre-Sprint-3 network hardening, but
  Sprint 2 must not be called fully network-approved before that evidence exists.

## Scope guard

- Heavy Carry remains Sprint 3 and was not started.
- Throw/push polish is not a `VS-Y02` blocker under the controlled fail/cut rule;
  Cat/Robot AI, missions, Kitchen production art and polish remain outside this
  closeout.
- `GAMEPAD MANUAL TEST: PENDING` remains unchanged.
- The preserved Phase 5 stash was not applied, popped, dropped or modified.
