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
