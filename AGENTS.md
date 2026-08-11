# Operation: Mouse Working Rules

- The Production GDD in `Documentation/Design/` is the primary source of truth.
- Protect the approved V1 scope. Never add stretch goals automatically.
- Classify new proposals as `[GDD]`, `[DERIVED]`, or `[RECOMMENDATION]`.
- The released game and all project-facing names use English. Explain development work to the project owner in Turkish.
- Design multiplayer-first. Gameplay-critical decisions should normally be server-authoritative.
- Put network-critical reusable foundations in C++ and designer-facing mission, level, UI, VFX, SFX, and tuning work in Blueprint when appropriate.
- Keep C++ Blueprint-friendly, readable, incremental, and beginner-friendly. Avoid unnecessary abstractions and over-engineering.
- Use Unreal's built-in `CharacterMovementComponent` first. Add a custom movement component only after a concrete, explained requirement.
- Do not introduce Gameplay Ability System for the initial prototype without a reviewed need.
- Explain important architecture decisions and test each small system before expanding it.
- Multiplayer gameplay must be tested with Host + Client; network-critical milestones must also receive latency testing.
- Reference images guide art direction, silhouette, proportion, and readability. They are not gameplay specifications.
- Debug the first meaningful error and its root cause. Do not apply random fixes.
- Update `DEVELOPMENT_STATUS.md` after meaningful milestones.

## Team Safety

- Assume human developers may be working in parallel. Inspect the current branch, Git status, and relevant files before editing.
- Modify the smallest reasonable set of files and avoid unrelated formatting or refactors.
- Preserve human changes. Never discard, overwrite, or rewrite another developer's work automatically.
- Warn before changes likely to conflict, especially shared C++ framework files, Blueprints, Data Assets, Gameplay Tags, and maps.
- Do not rename or move large groups of Unreal assets without approval.
- Never run destructive Git operations or history rewrites without explicit approval.
- After implementation work, report files created, modified, and deleted, plus build result, test result, and known issues.
