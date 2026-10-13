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
and cosmetic Reintegrated title branding. Scanlines are optional. Hold Triangle
for Codec dialogue turbo (default on); disable it in Gameplay or launch Play.ps1
with -CodecTurbo:$false. Cutscene behavior is unchanged. Loading an
older save can restore the save's language/caption options.

Settings and saves use %LOCALAPPDATA%/MGS-Reintegrated. Existing upstream
RE-INTEGRAL profiles are left untouched. Back up saves before testing candidates.
Alt+Enter toggles borderless windowed fullscreen. Exclusive fullscreen is not
available in the packaged SDL GPU backend; Ctrl+Alt+Enter reports this limitation.
This Windows recovered-code candidate uses Direct3D 12. The earlier hybrid
Direct3D 11 implementation is separate.

The player cleared Mantis and confirmed DualSense Edge / Xbox controller hot
swapping in this candidate. Disconnect either cable end, then connect the other
controller. Same-controller USB socket changes remain reconnects. F2 or L3+R3
explicitly switches virtual ports; the original statue alternative is retained.
No full campaign playthrough is claimed. Remaining intermittent floor/shadow
flicker is tracked for future renderer work.

Installed-game Mantis memory reading defaults on for new launcher profiles;
select Off to opt out. Saved explicit choices persist. Scanning reads bounded
local file/folder names, plus Steam library configuration for paths only.
Optional Sharp English text uses the bundled Heros atlas and adjacent notices.

See THIRD_PARTY.md, LICENSE, SOURCE-AND-NOTICES.txt and the accompanying source
archive for provenance and license scopes. Packaging is not a blanket license
clearance. No affiliation with Konami is implied.
