# Metal Gear Solid: Reintegrated — experimental Windows package

This is a Windows x86 (32-bit) recovered-code candidate based on RE:INTEGRAL,
FoxdieTeam and PsyZ. Full 64-bit gameplay and complete campaign compatibility
have not been established for this fork.

Extract the whole ZIP into a writable folder. Open **MGS-Reintegrated.exe** and
select your own Integral Disc 1 and Disc 2 CUE/BIN images. No retail assets or
private saves are included. Do not select another game's images.

Alternatively use **Play.cmd** (unfiltered) or **Play-Scanlines.cmd**. These
shortcuts ask for Disc 1 on first use; Play.ps1 also accepts -DiscTwo. The GUI
does not require PowerShell. The shortcuts use Windows PowerShell.

New profiles default to original aspect and 640x480 output, English captions,
and cosmetic Reintegrated title branding. Scanlines are optional. Loading an
older save can restore the save's language/caption options.

Settings and saves use %LOCALAPPDATA%/MGS-Reintegrated. Existing upstream
RE-INTEGRAL profiles are left untouched. Back up saves before testing candidates.
Alt+Enter toggles borderless windowed fullscreen; Ctrl+Alt+Enter selects exclusive
fullscreen. Direct3D 11 is the current default presentation backend.

Xbox Elite Series 2 and DualSense Edge, including hot swapping, were tested by
the user on an earlier candidate. This package still needs physical-controller
and listening review; no new full playthrough is claimed.

See THIRD_PARTY.md, LICENSE, SOURCE-AND-NOTICES.txt and the accompanying source
archive for provenance and license scopes. Packaging is not a blanket license
clearance. No affiliation with Konami is implied.
