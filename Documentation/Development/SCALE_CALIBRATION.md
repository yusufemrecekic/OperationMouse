# Mouse Scale Calibration Harness

## Status

- Candidate A technical harness: **IMPLEMENTED**
- Targeted structural validation: **PASSED**
- Map Check: **0 errors / 0 warnings**
- Manual scale review: **PENDING**
- Candidate A is a calibration candidate, not approved production tuning.

## Candidate A

`BP_ScaleCalibrationMouse` derives from
`BP_OMMouseCharacter_Prototype`, so it keeps the real CharacterMovement,
camera, traversal, interaction and Carry components.

| Property | Test-only value |
| --- | ---: |
| Capsule radius | 11 uu |
| Capsule half-height | 24 uu |
| Capsule total height | 48 uu |
| Prototype visual scale | 0.25 |
| Mesh relative location | X 0, Y 0, Z -24 uu |
| Mesh relative rotation | Yaw -90 degrees |
| Approximate rendered height | 45.12 uu |
| Crouched capsule half-height | 12 uu |

The source Y Bot bounds are approximately 180.47 uu high. Scaling only the
visual to 0.25 produces approximately 45.12 uu while the 48-uu capsule leaves
small collision headroom. The actor itself remains at world scale 1, avoiding
CharacterMovement side effects from whole-actor scaling. The crouched height is
test-only because the inherited 44-uu production value cannot fit inside a
24-uu standing half-height.

## Map

Package: `/Game/OperationMouse/Tests/Scale/L_ScaleCalibration`

- **A — Human references:** 90-uu counter, 75-uu work surface, 10-uu
  toe-kick, 75-uu furniture leg, 220-uu cabinet wall and a 90 x 210-uu
  doorway reference.
- **B — Passage width:** 12, 16, 20, 25, 30 and 40 uu.
- **C — Step / ledge / mantle:** 5, 10, 15, 20, 25, 30 and 40 uu.
- **D — Safe gap / jump:** 10, 20, 30, 40, 50 and 60 uu, with lower safety
  floors.
- **E — Camera / low clearance:** under-table route, 55-uu-clear low roof,
  close corridor walls and turns.
- **F — Interaction range:** real interaction actors at 50, 100, 200 and
  300-uu reference distances.
- **G — Normal Carry:** three real `AOMCarryableActor` fixtures at small,
  medium and large visual sizes.
- **H — Heavy Carry:** one real `AOMHeavyCarryableActor`, spacing markers and
  Reset fixture.
- **I — Two-player spacing:** two PlayerStarts and 25, 35 and 50-uu shared
  passage references.

The map uses movable daylight, fixed exposure, neutral graybox materials,
positive-scale readable labels and walkable route links. It does not modify the
Kitchen or any production map.

## Preserved Production Tuning

No production Blueprint, C++ gameplay value or existing map was edited.
Movement speed/acceleration, jump, `MaxStepHeight`, mantle thresholds, camera
arm/FOV, Interaction range, Carry offsets, Heavy Carry spacing, physics feel,
collision tolerances and network behavior retain their existing values.

## Known Heavy Carry Startup Issue

Zone H intentionally provides close spacing markers so two holders can start
extremely close to the cargo. The known pre-existing case where state reaches
2/2 before the cargo lifts must be observed, not fixed, during this calibration.
Stepping backward is the existing recovery behavior.

## Manual Review

1. Run `Scripts/LaunchScaleCalibration.ps1` from the repository root.
2. Set PIE to **2 Players** and **Play As Listen Server**.
3. Confirm both players spawn as the small upright prototype and remain grounded.
4. Walk Zones A through I; record readable/blocked passage, step, mantle and gap
   thresholds rather than changing values immediately.
5. In Zone E, inspect camera clipping and current arm-distance readability.
6. In Zone F, compare the real Interaction prompt/focus against the distance markers.
7. In Zone G, test Grab, Carry, obstruction and Drop for all three sizes.
8. In Zone H, test Waiting, 2/2 Carry, Reset and the documented close-start case.
9. In Zone I, test side-by-side standing, passing and shared approaches.

Do not mark a final mouse scale until Ali and Yusuf record the visual and
mechanical results together. Gamepad manual evidence remains pending.
