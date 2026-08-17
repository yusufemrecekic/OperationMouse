# Debug and Logging Standard

## Goal

A teammate must be able to reproduce a reported bug from the evidence package
without asking what happened.

## Log format

Use `LogOperationMouse` until a system becomes large enough to justify its own
category. Gameplay log messages follow this shape:

```text
[System][Result] Key=Value Key=Value Reason=StableReasonName
```

Examples:

```text
[Interaction][Completed] Interactor=OMMouseCharacter_0 Target=Sprint1_ButtonProxy Type=Instant
[Interaction][Rejected] Interactor=OMMouseCharacter_0 Target=Sprint1_FailProxy Reason=BeginInteractionRejected
```

Rules:

- name the system and result first;
- include the controlled Character and target when relevant;
- use stable, searchable failure reasons;
- log authoritative gameplay decisions on the server;
- do not log every Tick or replicate logs;
- do not include passwords, tokens, account identifiers, or private data.

## On-screen state

Sprint 1 uses deliberately simple on-screen evidence:

- the local interaction prompt shows the available action;
- each harness proxy displays `READY`, `COMPLETE`, or `EXPECTED FAIL`;
- `UOMInteractionComponent::bDrawDebug` may temporarily draw the focus sweep;
- `stat net` is used only during multiplayer/network review.

Debug drawing must be disabled for normal play and must not change gameplay.

## Reproduction evidence template

```text
Title:
Branch / commit:
Map:
Build target:
Net mode: Standalone / Listen Server Host / Client
Input device: Keyboard+mouse / Gamepad
Preconditions:
1.
2.
Steps:
1.
2.
3.
Expected:
Actual:
Relevant log lines:
Screenshot/video:
Reproduction count: __ / __
```

## Sprint 1 reproducible example

**Expected failure proxy**

1. Open `L_Phase5_InteractionTest` in Standalone PIE.
2. Approach the red `FAIL: EXPECTED FAIL` proxy.
3. Aim until the Interact prompt appears.
4. Press `E` or Gamepad Face Button Left.
5. Confirm the proxy does not complete.
6. Search the server log for `Reason=BeginInteractionRejected`.

The test is reproducible when the proxy remains in the expected-fail state and
the log identifies the Character, target, and rejection reason.

