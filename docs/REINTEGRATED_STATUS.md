# Reintegrated checkpoint â€” 2026-10-12

Base: RE:INTEGRAL a6cf2278b1d54e1d71ebfd286b3098a07cc066fe.
Dependencies and upstream patches: sources.lock.json. Additional Reintegrated
patches capture the isolated candidate's dependency edits. Original builds remain
outside this repository as rollback.

Included work: native scheduler/message and memory-width corrections, selected
GCL scalar/pointer boundaries, explicit-width math, stage-loader pointer arithmetic,
direct and selected indirect KMD shading/transform boundaries. These are incremental
portability changes; the full runtime still enforces a 32-bit host.

Private candidate evidence: ordinary startup/menu/Dock automated smoke passed;
selected components compared on Windows/Linux x86-64 and QEMU AArch64. That does
not establish full 64-bit gameplay, performance improvement, or complete campaign
coverage. Ocelot/persistence evidence from the earlier hybrid cannot be transferred
to this foundation. Upstream campaign reports belong to upstream.

User presentation: title, scanlines, controller support, optional codec turbo.
Latest scanline scissor correction was viewed at normal and maximized sizes;
subjective acceptance and source-row scaling differences remain open. Previous
nearest-neighbor and clipped scanline candidates are not the chosen source.

Repository milestone: reconstruct source from pins and patches, run asset-free
import checks, compile in isolation, audit tracked payloads and record results.
The GitHub fork and source branch are published. The experimental Windows x86 prerelease is published; no full replay
or new architecture claim is implied.
Next implementation: named KMD parent-chain adapter support, when authorized.

## Repository verification

- Existing upstream ancestry retained (eight commits at the pinned base).
- Additional dependency patch stacks validate: 87 MGS patches, 25 PsyZ patches.
- 3,711 dependency files agree with the preserved candidate after newline normalization.
- 14 asset-free importer tests pass, including single-disc import and malformed inputs.
- Launcher PowerShell syntax passes. Real-disc read/import is inherited; no new
  whole-disc extraction was performed just to initialize Git.
- Public-tree audit excludes game payloads/private settings and checks local paths.
  The two reviewed large inherited text files are stage inventory metadata and
  synthetic pocket-back fixtures, not bundled disc files.
- Inherited workflow is disabled. Experimental executable publication and local candidate packaging are complete.

The public-tree check is a practical payload check, not a blanket license clearance.
See the preserved upstream license scopes and THIRD_PARTY.md.

Fresh isolated repository configure/build passed (1,230 build steps, Windows i686, modern renderer). Executable --version passed. Reused the existing local Clang20/SDK toolchain and Python environment; a pristine-machine setup/install has not been tested. No additional gameplay claim was needed for this repository migration. Build SHA-256: `786aa92bce3188efcf37eee7faa04b470baf55f8c5a6741d04ef2705a5cc516b`.


## Branded player candidate — 2026-10-12

Version 0.2.0-reintegrated.1; built from d5d277d0aa42. Engine SHA-256:
`39efd859d8ff2d11f96c6a072d8991abe6616d1cd5f3d66a3588b856e88f1636`.
Launcher, runtime version/help and packaging identify Reintegrated. The accepted
logo is included. New profiles use 640x480, original aspect and scanlines off;
English captions and cosmetic title branding apply on user launches.
Upstream profiles and accepted playable packages are preserved.

Focused settings/quoting checks passed. Unpacked package checks passed version
identity, synthetic disc validation (including Unicode paths and read-only input),
self-contained importer operation and GUI smoke. A 2,400-VSync ordinary startup
probe reached controllable Dock, passed all three transitions and sustained
movement. Validation was unfiltered. A separate 120-VSync presentation startup
passed and logged English captions enabled. This was not a persistence or full
campaign test. Audio used the dummy device; listening remains pending. Controller
connection was detected, but physical input review remains pending for this package.
Manual first-run disc browsing/save-and-play interaction remains pending.

The recovered Windows renderer uses SDL GPU Direct3D12. Alt+Enter uses borderless
windowed fullscreen. Exclusive fullscreen is unavailable in this backend; the
previous hybrid Direct3D11/exclusive evidence is not transferable.

Local output includes player ZIP, matching symbols and corresponding MPL source
ZIP, with SHA256SUMS. Retail payloads/private saves are excluded. The user authorized and published the experimental Windows prerelease.
Smallest next task: user smoke/listening/controller review of the published package.


Published [0.2.0-reintegrated.1](https://github.com/ImperishableNEET/mgs1-reintegrated/releases/tag/v0.2.0-reintegrated.1)
from 7f80090ce25a03df5908dd5eeb65dec92ab562e8. This final package enables
Triangle Codec turbo by default in user launchers with a persisted GUI opt-out;
engine/validation defaults remain original. Launcher settings checks and unpacked
package checks passed. Four release assets were uploaded and their sizes verified;
the tag matches the package source commit. The Dock check above belongs to the
preceding readiness candidate, not a new full replay of this launcher-only revision.

## Display candidate - 2026-10-12

Published prerelease preserved. Local candidate matches launcher output limits to
engine limits (7680x4320), adds a 4K preset, selects 1280x720 on newly enabling
widescreen from 4:3/narrower selections, and preserves saved choices on load.
Projection follows selected output aspect; menus/Codec stay 4:3. Fullscreen label
now explicitly says borderless windowed. Exclusive remains unavailable in SDL GPU.
Scanlines cover every source row with fractional coverage, removing the abrupt
2x scaling density threshold. Launcher content area enlarged with scroll fallback.
Focused launcher settings and renderer configuration tests passed. 240-VSync
1280x720 widescreen window smoke entered 2560x1440 borderless with SDL fullscreen
off, then restored exact original bounds. 120-VSync 3840x2160 startup passed.
Final UI screenshot shows the previously clipped gameplay note. No campaign,
listening, physical-controller or mixed-DPI acceptance claim. Subjective scanline
appearance remains pending. No published assets replaced.
Evidence: .local/display-window, .local/display-4k, .local/display-ui-final,
.local/display-build.log. Candidate: output/Reintegrated-display-candidate-20261012.

Follow-up widescreen HUD defect: forced 1x raster compresses thin HUD strokes
below one pixel in widescreen. A failing native-wide contract fixture was retained
in .local/wide-hud-ab/fixture-before.log. Corrected original-resolution preference
to use the minimum raster scale that preserves horizontal source-pixel sampling:
ceil(3*output_width/(4*output_height)) for widescreen, 1x for original aspect.
1280x720 therefore uses 2x. Five focused video contracts pass. Ordinary 2400-VSync
startup/menu/Dock probe passes all three transitions and sustained movement.
Direct-stage 240-VSync captures were black and are retained as failed evidence;
no visual comparison claim is based on them. Subjective flicker/HUD acceptance
remains pending. Local corrected candidate will be separate from the prior one.

Astra independently confirmed horizontal 1x sampling loss. Focused GPU textured
stripe fixture reproduces missing blue column at1x (color mask b), with all four
colors surviving at2x (mask f). Both complete widescreen GPU fixtures pass after
correcting fixture output dimensions/letterboxing coordinates; failed harness
runs retained. Diagnostic captures omit presentation overlays, so cannot prove
scanline appearance or temporal flicker. Two-x can retain uneven texel widths;
visual acceptance still pending. README separators replaced with ASCII pipes and
mixed text encoding normalized to UTF-8 locally; not pushed.

Follow-up internal VRAM transfer defect reproduced: presentation smoothing had
also selected LINEAR filtering for native-to-scaled uploads, scaled-to-native
resolve, and target initialization. Alternating red/black native VRAM uploads
failed exact2x replication. These three data transfers now always use NEAREST;
only final display presentation retains smoothing. The same fixture now passes
256/256 pixel checks. Existing 2x widescreen suite passes, including thin strokes
and alternating-buffer FT4/SPRT equality. Flicker attribution remains pending
human reproduction; no temporal quality claim. Prior candidates preserved.
Evidence: .local/wide-hud-ab/upload-before.log, upload-after.log,
transfer-fix-build.log. Candidate output/Reintegrated-vram-filter-fix-20261012.
Unrelated LINE_F4 geometry initialization finding is parked because user's
precision settings were off; not represented as this symptom's established cause.

Follow-up sprite sampling defect: exact recovered number packets (6x7, UV210/232)
showed ten differing pixels across alternating framebuffer origins at2x for one
horizontal placement phase. Generic UV0 fixtures had missed it. One-step bias
left another phase failing; that failed evidence is retained. Second bounded
correction snaps only screen-aligned SPRT UVs within1/32768 texel of integers.
World, polygon and framebuffer sampling rules remain unchanged. Exactnumberfont
nowmatches across both buffers at all fourx phases (zero differing pixels).
Exact PAUSE fixture also matches, but PAUSEdidnot reproduce before the correction;
userPAUSE andnewCodec flicker remain pending ratherthanclaimedfixed.
Full focused widescreen GPUfixtures pass1x/2x/8x, nativeupload256checks pass.
GLSL/SPV/MSL/DXIL fragment headers regenerated with officialglslang16.6.0,
locallybuilt KhronosSPIRV-Cross and existingMicrosoftSDKdxc; tools keptprivate.
Evidence .local/wide-hud-ab/exact-font-before.log, tie-after.log (failed),
snap-after.log, snap-1x.log, snap-8x.log. Candidate:
output/Reintegrated-sprite-tie-fix-20261012. Publishedpackage remains untouched.
Next: playercheck PAUSE/Codec; ifpersistent captureactual consecutivepausedframes,
compare submittedglyphs/texturestate. No furtherparameterguessing.

2026-10-12: Bounded Heliport crash diagnosis (15-minute cap).
Reproduced player evasion-expiry crash under GDB with identical candidate executable
SHA256 F9B836CE55C9C961DAA56400D446AE7524E7436F56920617D544EEFB8D471CD9
and matching PDB. Fault 0x004f30b1 at enemy/think.c:1784; ESI=0xff.
Concrete cause: s00a_command_800CAD84 passes int unk[5] to HZD_NearZones,
which writes up to six outputs, including the 0xff terminal neighbor. Disassembly
shows sixth output overwrites saved ESI; caller then writes work->count3 through it.
Corrected both enemy/think.c and enemy/merylthink.c callers to six entries;
no navigation, timing, renderer or return safeguards changed.
Focused tests/test_near_zones_capacity.py passes (real NearZones extraction,
five neighbors + terminal sentinel + canary; both caller capacities checked).
Vendor stack: all 88 patches applied; engine build and git diff --check pass.
Evidence .local/heliport-crash-20261012: debugger.log, original log/settings,
fix-build.log, capacity-test.log. Original saves copied for reproduction, untouched.
Candidate output/Reintegrated-heliport-crash-fix-20261012; SHA256
03F64BD312315E8DE7601BE41A5222FA6A44B519C5A7000573775B0EB7A3ED05.
Player Heliport evasion-expiry retest pending; published release untouched.
Codec aspect jump separately identified: immediate canvas presentation switch
while DG_UnDrawFrameCount holds the old front buffer. Correct future implementation
must associate aspect with displayed framebuffer; no speculative delay added.
Codec speech-time frequency flicker remains unresolved. Player confirms PAUSE,
MP5 ammo, alert/evasion glyphs and 1920x1080 widescreen Alt+Enter now look good.

2026-10-12: Codec/HUD presentation candidate (bounded 30-minute pass).
User confirmed Heliport evasion expiry survived corrected debug build; debugger
second run exited normally. Crash correction retained in this candidate.
Codec entry/exit: aspect is tagged to completed framebuffer and applied immediately
before that half is selected for display. Held old front buffers retain old aspect;
DG_UnDrawFrameCount and simulation timing are unchanged. All nine video contracts pass.
Corrected exact Codec fixture models real line digits before nine native feedback
sprites, proper large digit0 and decimal coordinates, and speech meter levels6-9.
Before correction: native copy differed2/17 pixels, glow amplified36/216pixels;
changing speech meter altered180frequency pixels despite constant digits.
SyncScaledVramToNative now expands nearest resolve to bounded binary-aligned
rectangles for stable texel ties. Corrected fixture: native/line/glow/speech
invariance differences all0. GPU fixtures pass1x/2x/8x; original glow retained.
Widescreen HUD: life/O2/items shift left, weapons/radar right dynamically;
glyph dimensions and bar fill unchanged. Full HUD clip covers outer edges while
local radar clips retain transform. Focused outer-edge GPU assertions pass2x.
Mission-log prompt/reveal/exit optional4x presentation speedup across threevariants;
reading/input and demo2 page flips untouched. Launcher checkbox defaults on,
opt-out persists; direct engine defaults unchanged. UI height enlarged for full note.
Engine/launcher builds, launcher settings/opt-out tests, UI snapshots and vendor
stack(all91patches) pass. Public audit0findings; git diff --check passes.
Evidence .local/codec-video/exact-feedback-native.log (failed), aligned-resolve.log,
combined-build.log; .local/codec-ui-final/launcher-2.png. No new gameplay route run.
Candidate output/Reintegrated-codec-hud-fix-20261012; executable SHA256
6D00A2C040F7A77D1E985AB2D31A6794C590AAF9A18C3A044A82F18DEE236799.
Player Codec transitions/glow, HUD placement and mission-log visual acceptance pending.
Published release, original saves and prior rollback candidates untouched; nothing pushed.
Composed menus, briefing and mission log retain4:3; font/title expansion parked.
Johnny regional restoration research: no verified Integral toggle/asset established;
private .local/regional-restoration-research-20261012.txt records exact next research.

2026-10-13: local candidate gameplay acceptance and remaining renderer issues.
The player confirms the Tank Hangar supply-tunnel gas trap now reaches its original
Game Over through invulnerability, and Deepthroat's Codec static works. The player
completed Raven's tank battle and reached nuclear warhead storage. These are user
reports for the local candidate, not complete campaign validation.
The supply-tunnel exception matches verified failure callback 0xBB90 in s02c/s02e;
other stages and callbacks retain invulnerability. Focused recovered oxygen-actor
checks cover lethal damage, original callback, recovery and cleanup.
FMV staging appends slice uploads and cycles at capacity/frame boundaries. 1x/4x
GPU checks preserve pixels across capacity wrap. Inspected captured transitions
show no recurrence of the last-frame stretch.
Grey Fox previously caused D3D12 device loss. The player passed the scene after
reducing concurrent LLM GPU load; external contention is plausible, not proven.
Expensive framebuffer-feedback submissions are now bounded; 7x stress checks pass.
Exact crash causality and broad performance remain open. Depth-buffer floor and
lighting flicker remains reported; the optional depth buffer is experimental.

2026-10-13: the player confirms the newest Otacon office overlay fix removes floor
flicker beneath the computer desks. Remaining intermittent texture flicker in the
Nikita room and other locations is deferred to the long-term roadmap. Current
priority is the reported Mantis intro crash and encounter-local controller rules.
The matching-build first-fault capture is prepared; crash cause remains unresolved.

2026-10-13: captured Mantis constructor crash and source-generator correction.
Matching EXE/PDB first fault resolves to psyco.c:3650, a script-driven write to
Mantis's life-level table. Active GCL chara command is 0x9906, constructor
NewPsychoMantis, actor class 0xa76f/name 0xbb89. The table was in a read-only
compiler-generated .mgsst contribution, separate from writable reset markers.
The generator now emits an external mutable data anchor before recovered globals.
The resulting engine has one writable .mgsst; actual table stores lie inside the
correct overlay reset range. Mixed constant-dispatch/mutable-array regression
checks writes and reset; the old executable fails the new PE section audit.
Candidate Reintegrated-Mantis-crash-fix-20261013 engine SHA256:
9C9BCC95AA996B2102BA2610A346330928FFFB9901BD232C2ABF623313B5E79F.
Live intro-to-playable-combat confirmation is pending while the player is away.
Controller resistance and Installed games memory reading remain planned, not
implemented in this crash-fix candidate. Original saves and rollback copies retained.

Mantis controller/memory candidate (2026-10-13):
Built Windows x86 RelWithDebInfo candidate Reintegrated-Mantis-controller-memory-20261013.
Engine SHA256 DFB15D1A12FF1466D576E1FD1133DC6464895B0D8D963BBBF5E7B024AB6C4F0D.
Retains constructor overlay-section fix and original statue damage fallback.
During second-controller scenes, stable serial identity preserves reconnect slots;
a verified replacement may use port 2. Uncertain reconnects remain conservative.
F2/L3+R3 stays available; lone replacement remains usable after encounter exit.
No custom partial-resistance or boss-health/phase changes. Codec hint scripts unchanged;
exact original fallback activation/hint sequence still under source review.
Installed games defaults on for new launcher profiles; saved explicit choices persist.
Bounded scanner reads names only and Steam library path configuration. Four verified
Integral title bits: Suikoden 0x01, Azure Dreams/Other Life 0x02, Castlevania SOTN 0x10,
Vandal Hearts 0x20. Generic title aliases are heuristic matches. Profile mask is logged
for fixed-mask replay; live scan is rejected during replay. No save-format changes.
Six focused CTest checks passed; original recovered matcher tested all mask combinations.
Actual engine PE has one writable overlay-state section; public-tree audit zero findings.
Windows candidate execution/hardware Mantis combat remains pending while player is away.
Linux/WSL root discovery is implemented but full game runtime there remains unproven.
Private Integral script audit confirms statue fallback is wired: s07b ea54.g
procedure 0x9a3d passes the original -f variable reference 040128 to actor 0xa76f;
procedure 0x9c2b assigns that reference to 1 conditionally. Communications script
a242.g also branches on it. Exact Codec wording/count was not decoded; scripts retained.

2026-10-13 player confirmation: Mantis cleared after delivery of the controller/memory
candidate. Encounter completion is now confirmed by gameplay. This does not separately
verify every controller identity case, statue hint sequence or installed-title dialogue.

2026-10-13 Installed-game scanner correction: player Castlevania folder in Program
Files (x86) was missed; launch log showed installed mask 0. Depth-first Steam scans
could exhaust the shared entry budget before later roots. Immediate names in all
roots are now checked before depth-2 traversal, preserving the 8192-entry bound.
Actual host scan now finds the existing Castlevania folder: mask16 (0x10).
Scanner fixture passes. Installed remains the new-profile default; saved choices
are preserved. Separate candidate Reintegrated-Mantis-scan-fix-20261013 packaged.

2026-10-13 Mantis port hint now hidden until F2/L3+R3 is used or two game-over
transitions occur in s07b with second-controller support active. Death count resets
on leaving s07b; manual shortcut visibility persists for the launch. No save changes.
Windows x86 engine builds. Candidate Reintegrated-Mantis-hidden-hint-20261013.

2026-10-13 installed profile follow-up: actual player launch log 082042 reports
installed mask17 (Suikoden + Castlevania), confirming fixed scanner in gameplay.
Player reports physical DualSense Edge / Xbox One hot swapping still ineffective.
Required hardware handling: disconnect either cable end; changing PC USB sockets for
same controller remains a reconnect, while a different controller selects port2.
Serial-only recognition is being broadened conservatively to distinct model evidence.

2026-10-13 hot-swap correction: serial-less different physical controller families
now qualify for replacement-to-port2. SDL real gamepad type groups PlayStation,
Xbox and Nintendo; USB socket, cable end and GUID excluded. Serial identity takes
precedence; same-family unknown reconnect remains conservative. Original family
returns port1. Live slots compact and remembered identities clear outside encounter.
Focused routing test and 27 PsyZ patches pass; Windows x86 engine builds.
Candidate Reintegrated-Mantis-hotswap-fix-20261013, engine SHA256
237D517164307B18DFBE127A8B456484631F3BD93E0B153B89810538582A53B2.
Physical user swap confirmation pending. F2 manual routing remains authoritative.

2026-10-13 player confirms corrected physical controller hot swap works.
Candidate accepted for publication as v0.2.0-reintegrated.2 experimental prerelease.

2026-10-13 autonomous batch completed early within the requested one-hour limit:
Windows wired-HID controller socket switching requires matching serial plus verified
physical USB topology; same-socket reconnect remains normal. Read-only topology probe
resolved29 interfaces; physical socket-movement playtest pending. Family/manual fallbacks
retained, no game launched. Mantis hint counts actual positive GameOverTimer transitions,
not Meryl's negative nonfatal states; manual return prompt persists after fight.
Scanner reserves fair shares for roots and nested pass;32-entry regression catches noisy
root starvation. Sharp-text span buffer best-effort shrink saves5,312,596bytes (~5.07MiB)
for shipped atlas, with unchanged2x/8x render expectations. No FPS claim.
Seven focused checks pass (SDK input/hints, controller policy, fair scanner, font2x/8x,
overlay sections, launcher). SDK expected D-pad bytes corrected to existing analog-stick
projection; production input projection unchanged. No simulation timing/RNG/collision edits.
Residual floor/shadow audit found no additional narrow evidence-backed exception; mesh
identification remains required. No speculative renderer workaround applied.
Separate unpublished Reintegrated-overnight-20261013 candidate packaged with matching
symbols and source delta; published v0.2.0-reintegrated.2 untouched. Engine SHA256:
33B4847CAE383BFEF9213270E37B2F46909D71298BDB3DA311919C910944D210.

2026-10-13 unpublished font/modern-renderer/menu candidate:
Optional licensed interface coverage atlas replaces tagged 8x6 menu lettering
(PAUSE, save/load headings, Codec labels), with native layout/palette/mask and
unsupported/source-modification fallbacks. Straight diagonal variant plus faithful
alternative supplied; all A-Z/0-9 faithful ink verified privately, no retail pixels
bundled. Actual atlas GPU specimen inspected. Heros glyph batching uses bounded
112x48 viewport bands; modelled crouch-board uploads fall from98 to41, not an FPS claim.
Modern renderer now preserves both scaled framebuffer halves directly across
subtitle-driven scale changes. Held2x-to4x regression:288 changed RGB channels before,
zero after; downscale and native synchronization also verified. Recognized16bit
framebuffer polygons use affine screen-space UVs and scaled source sampling while
retaining model subpixel positioning/depth. Controlled warping/detail-loss baselines
fail before fixes and pass afterward. Static page policy is specific to this game;
not every cutscene artifact is claimed resolved. Other texture paths remain tested.
Widescreen title/main menu expands only7 abstract background layers; logos/portrait/
menu lettering remain proportioned. States6/7 only; other composed menus stay4:3.
420-frame title and main-menu captures confirm full-width background/centered layout.
New Game probe was intentionally held in title for visual capture, so campaign
completion criterion is unmet by design; no campaign claim. Original saves untouched.
Torment Nexus LLC ink now matches native op_copy RGB555 color206,173,156, opaque.
Published release unchanged; all104 MGS patches verified.

2026-10-13 player pause/cutscene clarity regression: launchers forced
REI_ORIGINAL_RES=1, selecting2x widescreen gameplay while sharp text raised the
whole frame to7x at2560x1440. Thirty drawn frames after text cleared restored2x.
Both code audits and paused/unpaused recording crops confirm raster-detail change.
Modern launcher/Play.ps1 now default to display-resolution rendering; a persisted
OriginalResolution preference and PowerShell switch keep low-GPU gameplay optional.
Old settings without the new field adopt display-resolution mode without losing
other choices. Direct explicit REI_ORIGINAL_RES=1 engine behavior remains intact.
New140-frame2560x1440 regression covers pause, cooldown, Codec and exit at stable8x
with no target-resize events. Five focused launch/profile/video checks pass.
No simulation, dialogue timing, projection or depth code changed in this fix.

2026-10-13 GPU submission failure / whole-PC restart investigation:
Latest player log ended at SDL_SubmitGPUCommandBuffer at8x in s07c. Its recent
commands contained about1.37million vertices and140fontchunks each. NVIDIA driver
errors occurred near failure; later Kernel-Power41 confirms an unclean restart
with BugcheckCode0. These records do not establish the cause of the whole-PC crash.
Added shared262144-vertex /32M-feedback-pixel submission bound after complete
passes and dirty bookkeeping. Font chunks reacquire command buffers after yields;
RGB, masks and ordering preserved. Agent2x/8x large-font checks passed before
whole-PC clarification. Further GPU/live-game testing then stopped. Engine CPU
build succeeds; default stable display-resolution rendering remains intact.
Window swapchain frame-fence policy unchanged. No driver/registry settings touched.
Patched candidate needs player validation; no system-stability claim.


## Candidate 0.2.0-reintegrated.3 (13 October 2026)
User confirmed widescreen title, PSG1 scope, gas-mask surround and camera surround, and played through the beginning of the communications tower. Camera item behavior remains imperfect and is deferred to the next release. Full campaign completion is not established. Includes stable modern rendering resolution, bounded GPU submissions, interface font work, HIDEO positioning and shared sight mask coverage. Whole-PC restart root cause remains uncertain.
