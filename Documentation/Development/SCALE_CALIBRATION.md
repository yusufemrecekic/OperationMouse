# Mouse Scale Calibration Harness

## Status

- Candidate A comparison profile: **PRESERVED / NOT ACCEPTED**
- Candidate B active profile: **IMPLEMENTED / MANUAL REVIEW PENDING**
- Targeted structural validation: **PASSED**
- Map Check: **0 errors / 0 warnings**
- Manual scale review: **PENDING**
- Neither profile is approved production tuning.

## Candidate A

`BP_ScaleCalibrationMouse_A` derives from
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

Candidate A remains available as a direct comparison asset, but the calibration
GameMode no longer spawns it by default.

## Candidate B (Active)

`BP_ScaleCalibrationMouse_B` also derives directly from
`BP_OMMouseCharacter_Prototype`; actor scale remains 1.0.

| Property | Test-only value |
| --- | ---: |
| Capsule radius | 7.5 uu |
| Capsule half-height | 15 uu |
| Capsule total height | 30 uu |
| Prototype visual scale | 0.15 |
| Mesh relative location | X 0, Y 0, Z -15 uu |
| Approximate rendered height | 27.07 uu |
| Crouched capsule half-height | 8 uu |
| Walk / Sprint | 270 / 400 uu/s |
| MaxAcceleration | 1400 uu/s² |
| BrakingDecelerationWalking | 1600 uu/s² |
| MaxStepHeight | 14 uu |
| JumpZVelocity | 245 uu/s |
| Expected jump apex at default 980 gravity | approximately 30.6 uu |
| Camera TargetArmLength | 170 uu |
| SpringArm probe size | 5 uu |
| SpringArm target offset Z | 18 uu |

The smaller camera probe keeps collision enabled while allowing a technically
useful retest of 25–40-uu passages. The target pivot sits 18 uu above the Actor
origin, just above the 30-uu standing capsule, so full retraction does not put
the test camera inside the Character body. Standard SpringArm collision can
still retract abruptly when there is no valid camera space; advanced
obstruction, fade or alternate camera modes remain post-scale-lock work.

## Map

Package: `/Game/OperationMouse/Tests/Scale/L_ScaleCalibration`

- **A — Human references:** recognizable 90-uu kitchen counter/base/toe-kick,
  75-uu dining table with legs, chair with a 45-uu seat/back/legs, 180-uu
  human silhouette and a 90 x 210-uu doorway reference.
- **B — Passage width:** 12, 16, 20, 25, 30 and 40 uu.
- **C — Step / ledge / mantle:** 5, 10, 15, 20, 25, 30 and 40 uu.
- **D — Safe gap / jump:** 10, 20, 30, 40, 50 and 60 uu, with lower safety
  floors.
- **E — Camera / low clearance:** under-table route, 55-uu-clear low roof,
  close corridor walls and turns.
- **F — Interaction range:** real interaction actors at 40, 60, 80, 100 and
  120-uu surface-clearance distances. Each value is measured from the marked
  Candidate B capsule front to the near surface of the 75-uu target cube.
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

## Post-scale Tuning Backlog

1. **Physics prop launch:** ordinary Character contact can create unrealistic
   vertical prop launches. Later tuning must inspect CharacterMovement physics
   interaction forces, mass ratios, push/touch impulse and vertical impulse.
2. **Heavy Carry startup clearance:** Zone H exposes the pre-existing case where
   state reaches 2/2 before lift when a holder is extremely close. Stepping
   backward remains the recovery.
3. Candidate B camera settings are test-only; final production camera behavior
   remains undecided.
4. Production Interaction range remains undecided. Current qualitative target:
   approximately 1.5–2 mouse body lengths.
5. Production Carry distance/clearance and Heavy Carry spacing remain undecided.
6. Production movement, jump and mantle tuning remain undecided.

## Manual Review

1. Run `Scripts/LaunchScaleCalibration.ps1` from the repository root.
2. Use normal PIE for the first Candidate B review.
3. Judge Candidate B beside the human dining table and chair.
4. Judge Walk/Sprint acceleration, top speed and stopping.
5. Inspect the camera under the table and low-clearance route.
6. Retest the 25, 30 and 40-uu passages.
7. Compare 5, 10, 15 and 20-uu step/ledge behavior.
8. Judge Interaction feel at the 40–120-uu surface-distance markers.

Do not mark a final mouse scale until Ali and Yusuf record the visual and
mechanical results together. Gamepad manual evidence remains pending.
