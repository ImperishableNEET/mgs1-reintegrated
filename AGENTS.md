# Reintegrated development

Read README.md, docs/REINTEGRATED_STATUS.md and sources.lock.json first.
Current runtime: recovered C, Windows i686; full x86-64/ARM64 remains unproven.
Preserve original game behavior by default. Presentation/QoL preferences are
launcher preferences and must remain absent from validation commands.
User launchers default Triangle Codec turbo on, with an explicit opt-out.

Prefer one bounded implementation, shortest relevant check, then a concise result.
Reproduce defects before changing semantics. After two attempts without measurable
progress, diagnose or park. Do not repeat unchanged passed gameplay checks.
Use deterministic tools first; delegate only when it saves net effort. No paid
helper by default. No purchases, resets, global setup changes or bulk recovery.

Dependencies are pinned in sources.lock.json. Edits to vendor must be captured as
patches and verified by tools/patch_stack.py --check. Never commit vendor, content,
disc images, BIOS/SDK files, save data, captures, private evidence or local settings.
Use tools/check_public_tree.py before packaging or pushing. Existing upstream
licenses and notices retain their scope; do not blanket-relicense recovered code.
Do not publish, push, create releases, or message maintainers without user direction.

Keep original playable packages and source discs outside this repository untouched.
Report actual architecture/execution, gameplay, fidelity and fallback separately.
The research workspace is intentionally excluded from public history.
