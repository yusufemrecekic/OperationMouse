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
| Base pivot height | current capsule half-height x 1.2 |
| Provisional crouch arm | standing desired arm x 0.60 (102 uu) |
| Posture blend rate | 8 per second |
| Character gameplay mass | 10 kg |
| Initial / sustained push force | 50 / 500 |
| Touch force | 0 (disables artificial upward touch impulse) |
| Repulsion force | 0.25 |

Candidate B opts into `UOMCloseSpaceCameraComponent`; the reusable component is
disabled by default on the production Character. It owns a local-only 5-uu
camera-channel sphere sweep and keeps the desired 170-uu open distance separate
from the obstruction-limited distance. Retraction is fast and collision-clamped,
while expansion is slower to avoid wall-edge popping. Compression below 0.55
smoothly raises the pivot by at most one scaled capsule half-height. The minimum
readable distance derives from scaled capsule radius, so later mouse-scale
calibration does not require absolute collision distances in C++.

Candidate B now resolves camera composition in two collision-safe stages. The
base pivot follows the current scaled capsule half-height (`x 1.2`), producing
18 uu standing and 9.6 uu crouched. Pivot height and the provisional crouch arm
(170 x 0.60 = 102 uu) blend at 8/s. Before the existing camera-distance sweep,
the requested base/adaptive pivot is sphere-swept from the Character reference
on `ECC_Camera`; overhead furniture or roofs clamp it to the player's side with
the existing 2-uu padding. Initial penetration uses the sweep MTD normal and
depth, while a camera-distance sweep that starts penetrating clamps to zero
instead of accepting a cross-geometry result.

Whole-mesh hiding is now narrowed to a true emergency collapse:
the trigger is the smaller of half the 5-uu probe and one quarter of the
capsule-derived readable distance (2.5 uu here), restoring at 5 uu. Ordinary
crouch and low-clearance compression therefore retain the visible mouse. No
camera state, RPC, gameplay transform or remote mesh visibility is changed.
Final production materials/fade remain an Ali visual decision if ever needed.

The UE 5.8 inherited perspective near clip was 10 uu. A 5-uu sweep therefore
left the camera center collision-safe while its near plane could begin beyond
the contacted wall, especially during oblique yaw at thin edges. The project
now overrides near clip to 2 uu and keeps an additional configurable 2-uu gap
behind sphere-sweep contact. This remains comfortably above an extreme
sub-unit clip distance, limits the depth-precision cost, and fits the current
27-uu character / 25–40-uu passage calibration context. Close-space fixtures
continue to block `ECC_Camera`; no passage dimensions were changed.

The human-reference chair uses an explicit collision-policy exception for camera
composition: its seat, back and four leg components ignore only `ECC_Camera`.
They remain rendered and retain their blocking Pawn, WorldStatic, WorldDynamic
and PhysicsBody responses. Walls, floors, roofs, passage fixtures and the table
remain hard camera blockers. This is a per-fixture calibration decision, not an
automatic furniture classifier or a global production collision change. Future
Ali/Yusuf integration will choose Block or Ignore per production asset.

Because an always-rendered Camera-Ignore chair can still cut across the view,
the calibration fixture adds two invisible, query-only camera proxy boxes: a
74 x 74 x 10-uu seat mass centered at `(-1770, -900, 42)` and a 10 x 74 x
76-uu back mass centered at `(-1802, -900, 78)`. Two shapes are intentional:
one bounding box around the L-shaped seat/back would incorrectly block the
large empty volume above the seat. The proxies block only `ECC_Camera`; all
gameplay/physics channels ignore them. Individual legs have no camera proxy.
This explicit visual collision plus simplified Camera-only proxy is the future
Ali/Yusuf authoring guideline for selected complex or porous furniture.

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
- **J — Physics contact:** equal-size simulated Light/Medium/Heavy cubes with
  deliberate 2/10/50-kg mass overrides, 0.35/0.55/0.75 linear and angular
  damping, straight/side/repeated-contact lanes and nearby platform edges.

## Candidate B Physics Contact Profile

The inherited UE profile was Character mass 100 kg, InitialPushForceFactor 500,
PushForceFactor 750000, TouchForceFactor 1, unlimited minimum touch force,
MaxTouchForce 250 and RepulsionForce 2.5. `CapsuleTouched` also builds its
impulse direction with a positive gravity-space Z component. Exact repeatable
PIE displacement measurements are not claimed because contact angle, solver
substeps and frame timing make this graybox test nondeterministic; the confirmed
baseline symptom was occasional upward/forward missile-like launch.

Candidate B alone now uses a 10-kg gameplay mass, 50 initial push, 500 sustained
push, zero touch force, Min/MaxTouchForce -1/0 and 0.25 repulsion. Push and touch
mass scaling are disabled; velocity scaling remains enabled. Zero touch force
removes the artificial upward overlap impulse while ordinary blocking contact
still supplies controlled physical comedy. Leaving push mass scaling disabled
is intentional: UE multiplies the applied force by body mass when enabled,
which would defeat the desired Light > Medium > Heavy acceleration response.
No engine, production Character, Carry, Heavy Carry, RPC or replication code
was changed. Listen Server and later latency/physical 2-PC behavior remain a
Hilmi evidence responsibility.

The map uses movable daylight, fixed exposure, neutral graybox materials,
positive-scale readable labels and walkable route links. It does not modify the
Kitchen or any production map.

## Preserved Production Tuning

No production Blueprint, C++ gameplay value or existing map was edited.
Movement speed/acceleration, jump, `MaxStepHeight`, mantle thresholds, camera
arm/FOV, Interaction range, Carry offsets, Heavy Carry spacing, physics feel,
collision tolerances and network behavior retain their existing values.

## Post-scale Tuning Backlog

1. **Physics contact profile:** Candidate B built-in tuning is ready for manual
   Light/Medium/Heavy contact review; production adoption is not yet approved.
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
