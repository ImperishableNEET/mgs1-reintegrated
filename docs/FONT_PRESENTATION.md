# Optional high-resolution text pilot

Status: an optional English text runtime candidate uses TeX Gyre Heros Regular,
a Helvetica-compatible face, with a locally generated 8x coverage atlas.
Actual Helvetica was not found locally. The Graphics tab's **Sharp English text**
option and `Play.ps1 -SharpCodecFont` are off by default. Original character
advances, wrapping, centering, line spacing and dialogue timing are retained.
Unsupported messages fall back as a complete board to the original font.

The atlas is loaded from `fonts/texgyreheros_regular_ascii8x_coverage.bmp`
beside the executable. Its GUST Font License and source attribution accompany
the private candidate. No commercial Helvetica font or game glyph bitmap is
bundled. English Codec dialogue, contact names, cutscene captions and location labels are replaced; HUD and icons retain
the original assets. This is a stylistic candidate, not an exact font match.

The option raises the internal rendering scale only while Codec or caption text is drawing,
with an 8x cap. Gameplay retains its selected scale. Changes wait for the first
new drawing frame, preserving held frames during Codec transitions. Reallocating
the target can cause a transition hitch; a first newly drawn backbuffer can also
resample the preceding front buffer. An independent font target remains a future
improvement. Coverage geometry currently favors correctness over throughput.

Focused validation covers immutable layout snapshots, antialiasing, PS1 mask
behavior, unsupported/untagged fallback and order-table occlusion at 2x/8x.
A bounded actual-game Codec capture also confirms the replacement is reached.
Full conversation and performance playtesting remain separate from these checks.

The previous private reconstructed specimen covers A–Z and digits but is not
used by this runtime candidate.
The first filtered and literal-contour previews were rejected as too blurry or
pixelated. The next specimen reconstructs English letter shapes from the original
glyphs, retaining descriptor widths, advances, vertical offsets and margins.
The font's intermediate shades represent coverage/intensity; they are not a
separate shadow layer. Preserve Codec's original spacing and 18-pixel line pitch.

## Bounded implementation

Start with English Codec dialogue. The existing font code determines layout and
uploads a bitmap; `draw_radio_message` submits that board as one sprite. Record
glyph placements during the existing layout, freeze them when the bitmap updates,
and explicitly associate that snapshot with the submitted sprite. Render an
authored high-resolution atlas at the sprite's original order-table position.
Do not alter wrapping, centering, dialogue timing or game logic.

Keep the original complete sprite for messages containing unsupported glyphs.
Reset the sidecar on font clear and preserve snapshot lifetime through deferred
renderer submission. Avoid replacing shared VRAM or identifying text by UV/CLUT
alone. The language-pack glyph override currently excludes ASCII.

Judge the specimen at normal display size, then check one ordinary conversation,
turbo dialogue and an aspect transition after integration. Preserve original
font mode as a fallback. Icons are a separate silhouette-reconstruction pilot.

## Reference practices

- [FontForge bitmap-font guidance](https://fontforge.org/docs/tutorial/editexample8.html):
  automatic tracing is unreliable for very small bitmap sources.
- [FreeType glyph metrics](https://freetype.org/freetype2/docs/glyphs/glyphs-3.html):
  bearings, advances, grid fitting and final pixel placement require separate care.
- [MSDFgen](https://github.com/Chlumsky/msdfgen): an optional scalable representation
  for good outlines; it does not reconstruct source detail.
- [Depixelizing Pixel Art](https://johanneskopf.de/publications/pixelart/):
  connectivity-aware reconstruction is worth assessing for small item silhouettes.

Private source-derived specimens remain outside public payloads. A visual
specimen is not evidence that a replacement works in the game renderer.

## Asset-tool reference

[Arms Depot / tools-mgs](https://github.com/Joy-Division/tools-mgs) provides
MGS1 STAGE.DIR, PSX DAR and FACE.DAT extractors plus an incomplete GCX decoder.
Evaluate its format handling as a reference for importer validation and portrait
asset inspection. The archived project documents limited input checking and
16-bit filename-hash collisions; retain stage-scoped identities and validate
bounds before reusing code. Its README does not establish Master Collection
container support or a font reconstruction solution.

## Other font candidates

- Credits, ordinary captions and location names: compare the existing Heros
  face against each original. `menu/jimaku.c` uses the same KCB bitmap-font
  system for ordinary captions/location labels. Credits need their own resource
  audit before assuming identical metrics or weight.
- PAUSE and large interface lettering: [Mission Genome Sans by V. Sarela]
  (https://www.fontstruct.com/fontstructions/show/1968440/mission-genome-sans)
  is explicitly a non-pixelated MGS interface recreation, not Konami's original
  source typeface. Its listed license links to CC BY-NC-ND 3.0; do not treat it
  as a freely modifiable font. The download endpoint returned a sign-in page.
- Small LIFE/item/interface and ammunition numerals: [Gleeson's MGS1 Fonts]
  (https://gleeson.itch.io/mgs1-fonts) contains HUD (5x5), wide menu/Codec (8x6)
  and Ammo (5x7) recreations under CC BY-SA 4.0. These deliberately retain
  bitmap shapes, so increasing their size alone does not smooth stair steps.
- PAUSE uses `_menu_number_draw_string2` in `menu/jimaku.c`; the location label
  uses KCB text plus four offset black sprite copies for its outline. Preserve
  that outline and original placement when adding high-resolution rendering.

These references establish useful recreations; none establishes the original
commercial source face for the custom blocky HUD alphabet.

Codec contact names retain their original row crops, green tint and selected-row additive glow. Frequency digits retain the original assets. Opening credits use pre-rendered texture images and are not covered by this text bridge.

Save/load slot names and bitmap-font status messages also use the sharp-text bridge, with separate 128x14 row crops. Block interface headings, difficulty/time numerals and selection highlight geometry remain original.
