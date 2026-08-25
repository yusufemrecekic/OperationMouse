# Mouse Scale Calibration Harness

## Status

- Candidate A comparison profile: **PRESERVED / NOT ACCEPTED**
- Candidate B active profile: **PROVISIONAL SCALE DIRECTION / CAMERA RETEST PENDING**
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
| Collision safety padding | 2 uu |
| Project perspective near clip | 2 uu |
| Open-space camera pivot Z | 18 uu |
| Camera retract / extend rate | 30 / 5 per second |
| Minimum safe distance | capsule radius x 2.0 |
| Close-space threshold | 0.55 compression ratio |
| Maximum adaptive pivot rise | capsule half-height x 1.0 |

Candidate B opts into `UOMCloseSpaceCameraComponent`; the reusable component is
disabled by default on the production Character. It owns a local-only 5-uu
camera-channel sphere sweep and keeps the desired 170-uu open distance separate
from the obstruction-limited distance. Retraction is fast and collision-clamped,
while expansion is slower to avoid wall-edge popping. Compression below 0.55
smoothly raises the pivot by at most one scaled capsule half-height. The minimum
readable distance derives from scaled capsule radius, so later mouse-scale
calibration does not require absolute collision distances in C++.

If geometry leaves less than two scaled capsule radii of camera space, only the
owning local player's mesh is hidden with hysteresis until safe space returns.
No camera state, RPC, gameplay transform or remote mesh visibility is changed.
This fallback is intentionally minimal; final production materials/fade remain
an Ali visual decision after the scale and camera behavior are accepted.

The human-reference chair is the explicit soft-camera-occluder foundation test.
Its seat, back and four leg actors carry the `OMCameraSoftOccluder` tag. Only
tagged actors/components are skipped by obstruction-distance resolution and
hidden from the owning local player's view while they obstruct composition.
Untagged walls, passage fixtures, floors and the table remain hard
`ECC_Camera` blockers. Each sweep skips at most four soft components, so a hard
blocker behind foreground furniture still limits camera distance. Components
restore when they stop obstructing, ownership changes, the camera component
deactivates or play ends. Ali/Yusuf may later replace this foundation hide/show
presentation with dither or material fade without changing its opt-in contract.

The UE 5.8 inherited perspective near clip was 10 uu. A 5-uu sweep therefore
left the camera center collision-safe while its near plane could begin beyond
the contacted wall, especially during oblique yaw at thin edges. The project
now overrides near clip to 2 uu and keeps an additional configurable 2-uu gap
behind sphere-sweep contact. This remains comfortably above an extreme
sub-unit clip distance, limits the depth-precision cost, and fits the current
27-uu character / 25–40-uu passage calibration context. Close-space fixtures
continue to block `ECC_Camera`; no passage dimensions were changed.

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
3. Candidate B camera foundation values are test-only; open-space, wall,
   under-table and 25/30/40-uu passage camera review remains pending.
4. Production Interaction range remains undecided. Current qualitative target:
   approximately 1.5–2 mouse body lengths.
5. Production Carry distance/clearance and Heavy Carry spacing remain undecided.
6. Production movement, jump and mantle tuning remain undecided.

## Manual Review

1. Run `Scripts/LaunchScaleCalibration.ps1` from the repository root.
2. Use normal PIE for the first Candidate B review.
3. Judge Candidate B beside the human dining table and chair.
4. Judge Walk/Sprint acceleration, top speed and stopping.
5. In open space, walk/sprint and rotate yaw through 180 degrees.
6. Back toward a wall, then rotate beside a wall and watch retract/extend.
7. Inspect the camera under the table and low-clearance route.
8. Enter the 40, 30 and 25-uu passages and rotate 180 degrees in each.
9. Exit each obstruction into open space and verify smooth restoration.
10. Compare 5, 10, 15 and 20-uu step/ledge behavior.
11. Judge Interaction feel at the 40–120-uu surface-distance markers.

Do not mark a final mouse scale until Ali and Yusuf record the visual and
mechanical results together. Gamepad manual evidence remains pending.
