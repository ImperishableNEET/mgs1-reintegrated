# Importing locally owned Integral discs

The inherited importer extracts an existing disc image; it does not control an
optical drive. Dump physical discs separately to supported CUE/BIN files.
Currently supported: one local BIN, one MODE2/2352 track, INDEX 01 00:00:00.
CHD, compressed images, multi-track layouts and other editions are not supported.

```powershell
python tools/import_content.py --source 'D:/IntegralImages' --output '.local/content-import'
```

Place one or more supported Integral disc pairs in the source directory. Recognized
serials: SLPM_862.47 (Disc 1), SLPM_862.48 (Disc 2), SLPM_862.49 (VR).
Recognition does not establish playable coverage of those discs or modes.
Executable SHA-256 and format bounds are checked; input image hashes are recorded.
This is not a claim that every accepted image matches a known whole-disc hash.
Output must be new and separate from input; incomplete output is preserved on error.
Each disc receives a receipt of source and extracted-file hashes. XA/Form2 files
also retain raw sectors, since logical bytes alone cannot preserve their streams.

The runtime still uses the original CUE/BIN, e.g. `./Play.ps1 -Disc ...`.
Asset extraction is optional for playing; useful for inspection and future mod work.
All output belongs in ignored local directories, never Git or release archives.
No downloads of game data or circumvention mechanisms are provided.
Master Collection extraction is not part of this supported import workflow.
