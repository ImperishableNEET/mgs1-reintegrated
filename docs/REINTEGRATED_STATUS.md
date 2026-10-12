# Reintegrated checkpoint — 2026-10-12

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
No GitHub push, public release, full replay or new architecture claim is implied.
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
- Inherited workflow is disabled. Public publishing and release packaging are pending.

The public-tree check is a practical payload check, not a blanket license clearance.
See the preserved upstream license scopes and THIRD_PARTY.md.
