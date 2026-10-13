using System;
using System.IO;
using System.Runtime.InteropServices;
using ReIntegral;

internal static class LauncherSettingsTest
{
    [DllImport("shell32.dll", SetLastError = true)]
    private static extern IntPtr CommandLineToArgvW([MarshalAs(UnmanagedType.LPWStr)] string command, out int count);
    [DllImport("kernel32.dll")]
    private static extern IntPtr LocalFree(IntPtr pointer);

    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    private static void CheckQuoting()
    {
        foreach (string path in new[] { "", "C:\\path with spaces\\disc.cue", "D:\\日本語\\ä\\", "a\"b", "trailing\\\\" })
        {
            int count;
            IntPtr arguments = CommandLineToArgvW("program " + GameLaunch.Quote(path), out count);
            Require(arguments != IntPtr.Zero, "Cannot parse command line");
            try
            {
                Require(count == 2 && Marshal.PtrToStringUni(Marshal.ReadIntPtr(arguments, IntPtr.Size)) == path,
                    "Argument changed during Windows quoting: " + path);
            }
            finally { LocalFree(arguments); }
        }
    }

    private static void CheckSettings(string directory)
    {
        string path = Path.Combine(directory, "settings.xml");
        var value = Settings.Load(path);
        Require(!value.Invulnerable && !value.BonusItems && !value.AutoCard, "Gameplay aids must default off");
        Require(!value.FastLoading, "Experimental fast loading must default off");
        Require(!value.FirstPersonMoveShoot && GameLaunch.FirstPersonMoveShootEnvironment(value.FirstPersonMoveShoot) == "0",
            "Integral first-person move/shoot must default off");
        Require(value.CodecTurbo, "User launcher Codec turbo must default on");
        Require(value.MissionLogFast, "User launcher mission-log speedup must default on");
        Require(value.MantisMemory == "installed", "New launcher profiles must use installed-game memory reading");
        Require(GameLaunch.Arguments(value).Contains("--mantis-memory installed"), "Installed mode was not passed to the engine");
        Require(!value.Scanlines, "Optional scanlines must default off");
        Require(!value.OriginalResolution, "Display-resolution rendering must default on");
        Require(value.Resolution == "640x480" && !value.Widescreen,
            "New profiles must default to the original display format");
        Require(!value.TextureCorrection && !value.GeometryPrecision && !value.DepthBuffer,
            "Experimental graphics must default off");
        value.DiscOne = "C:\\日本語\\disc 1.cue";
        value.Resolution = "1920x1080";
        value.Widescreen = true;
        value.TextPack = "C:\\my languages\\german.mgslang";
        value.VoicePack = "C:\\my languages\\english.mgslang";
        value.Advanced = "--mods-root \"C:\\my mods\"";
        value.Save(path);
        value.Fullscreen = true;
        value.MantisMemory = "off";
        value.Scanlines = true;
        value.CodecTurbo = false;
        value.MissionLogFast = false;
        value.FastLoading = true;
        value.FirstPersonMoveShoot = true;
        value.TextureCorrection = value.GeometryPrecision = value.DepthBuffer = true;
        value.OriginalResolution = true;
        value.Save(path);
        var restored = Settings.Load(path);
        Require(restored.MantisMemory == "off" && !GameLaunch.Arguments(restored).Contains("--mantis-memory"),
            "Explicit memory-reading opt-out did not persist");
        Require(restored.DiscOne == value.DiscOne && restored.Advanced == value.Advanced && restored.Fullscreen && restored.Scanlines &&
            restored.Resolution == value.Resolution && restored.Widescreen,
            "Settings round-trip or replacement failed");
        Require(restored.TextureCorrection && restored.GeometryPrecision && restored.DepthBuffer,
            "Graphics settings did not round-trip");
        Require(restored.OriginalResolution, "Explicit original-resolution preference did not persist");
        Require(!restored.CodecTurbo, "Codec turbo opt-out did not persist");
        Require(!restored.MissionLogFast, "Mission-log opt-out did not persist");
        Require(restored.FastLoading, "Fast loading setting did not round-trip");
        Require(restored.FirstPersonMoveShoot && GameLaunch.FirstPersonMoveShootEnvironment(restored.FirstPersonMoveShoot) == "1",
            "Integral first-person move/shoot did not round-trip");
        Require(restored.TextPack == value.TextPack && restored.VoicePack == value.VoicePack,
            "Language selections did not round-trip");
        string languages = GameLaunch.Arguments(restored);
        Require(languages.Contains("--text-pack " + GameLaunch.Quote(value.TextPack)) &&
            languages.Contains("--voice-pack " + GameLaunch.Quote(value.VoicePack)), "Language paths were not quoted");
        Require(Directory.GetFiles(directory, "*.tmp").Length == 0, "Temporary settings file leaked");
        File.WriteAllText(path, "<Settings><Version>1</Version><SharpCodecFont>true</SharpCodecFont></Settings>");
        var legacy = Settings.Load(path);
        Require(legacy.SharpCodecFont && !legacy.OriginalResolution,
            "Older profiles must keep sharp text and adopt display-resolution rendering");
        File.WriteAllText(path, "<Settings><Version>99</Version></Settings>");
        try { Settings.Load(path); throw new InvalidOperationException("Unknown settings version accepted"); }
        catch (InvalidDataException) { }
    }

    private static void CheckArguments()
    {
        var settings = new Settings { BonusItems = true, Invulnerable = true, AutoCard = true, FastLoading = true,
            DebugOverlay = true, Fullscreen = true, Widescreen = true,
            TextureCorrection = true, GeometryPrecision = true, DepthBuffer = true,
            Stage = "s00a", Frames = "600", MantisMemory = "42",
            Advanced = "--capture \"C:\\frame capture.bmp\"" };
        string command = GameLaunch.Arguments(settings);
        foreach (string flag in new[] { "--bonus-items 1", "--invulnerable 1", "--auto-card 1", "--debug-overlay",
            "--fullscreen", "--widescreen", "--texture-correction", "--geometry-precision", "--depth-buffer", "--fast-loading 1",
            "--stage s00a", "--frames 600", "--mantis-memory 42", settings.Advanced })
            Require(command.Contains(flag), "Missing option: " + flag);
        foreach (string resolution in new[] { "3840x2160", "7680x4320", "2560x1920" })
        {
            settings.Resolution = resolution;
            Require(GameLaunch.Arguments(settings).Contains("--resolution " + resolution), "Supported output rejected");
        }
        foreach (string resolution in new[] { "7681x4320", "7680x4321", "0x0", "1920x1080 --help" })
        {
            settings.Resolution = resolution;
            try { GameLaunch.Arguments(settings); throw new InvalidOperationException("Invalid resolution accepted"); }
            catch (ArgumentException) { }
        }
    }

    public static int Main()
    {
        string directory = Path.Combine(Path.GetTempPath(), "reintegral-settings-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(directory);
        try { CheckQuoting(); CheckSettings(directory); CheckArguments(); }
        finally { Directory.Delete(directory, true); }
        Console.WriteLine("Launcher settings, Windows argument quoting and option checks passed.");
        return 0;
    }
}
