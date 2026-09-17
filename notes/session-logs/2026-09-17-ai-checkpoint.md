# AI checkpoint, 17 September 2026

Checkpoint requested by the user after the current task. The broader engine-completion goal remains unfinished.

## Completed in this task

- `d2a13355` restores `CCop::Guard`: idle animation transitions and countdowns.
- `59b1bd08` restores `CCop::LookConfused`: randomized search turns, turn completion and return to fight selection.
- `eb428c18` restores `CCop::PlaySounds`: animation-frame sound cues and sprite-ring effects for both cop types.
- `94547544` fixes `CThug::PlaySounds`: four calls used `0x80` instead of the original `0x8000` sound flag. Source layout was also adjusted to reproduce the original instructions.

All four functions match the original instruction stream, operands and branch destinations, allowing relocated calls/data. Jump tables and compact switch-selector tables were compared separately. Function names were checked against the symbol-bearing Mac binary.

## Verification

- Clean Windows DLL build, Linux build, layout validator, SDL build and tag checker passed on the combined changes.
- Guard and search-turn handlers each passed 40,000 comparisons against original machine code.
- Cop and thug sound handlers each passed 71,680 animation/frame cases against original machine code. These tests compare object changes and ordered calls; external callees are controlled stubs.
- The old thug sound code demonstrably failed: sound ID 160 where the original requested 32800. The fixed code passes.
- Live patrol sound test passed: 1,200 player-update frames, 106 thug sound calls, zero invalid sound IDs/pitches and no crash. This verifies emitted calls, not audible output quality.

Local verification artifacts are under `out/cop-check/` and `out/modern-check/thug-sounds-runtime/`. Original IDA database: `~/Documents/spidey-work/idbs/new_idbs/SpideyPC.i64`.

## Remaining work

The previous checkpoint restored thug AI, Hit and grenade throwing and verified one level-one thug chasing, attacking, taking damage and dying after mouse punches. It did not verify all levels or enemy types.

Cop AI and Hit overrides remain absent. Restore their missing dependencies before enabling the full dispatcher: fight selection, movement/combat states, messages, hit/slide/death and web reactions. Original dispatcher addresses are `0x42FA10` (AI), `0x42F810` (switch logic), and `0x429C60` (Hit). No new Windows hooks were added.

Other NPC/boss AI, later-level progression, saves/restarts, remaining rendering/web issues and the Windows DLL global-access crash chain still require work. Existing web-swing and other unrelated worktree drafts were preserved. The CD-check bypass remains disabled.
