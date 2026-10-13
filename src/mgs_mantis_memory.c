#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "mgs_mantis_memory.h"

#include <SDL3/SDL_filesystem.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#define MANTIS_MAX_PATH 1024
#define MANTIS_MAX_ROOTS 64
#ifndef MANTIS_MAX_ENTRIES
#define MANTIS_MAX_ENTRIES 8192
#endif
#define MANTIS_MAX_DEPTH 2
#define MANTIS_MAX_VDF 65536

static unsigned int profile_seed;
static unsigned int installed_mask;
static int installed_enabled;

static int join_path(char *out, size_t size, const char *a, const char *b)
{
    size_t len = strlen(a);
    int slash = len && a[len - 1] != '/' && a[len - 1] != '\\';
    int n = snprintf(out, size, "%s%s%s", a, slash ? "/" : "", b);
    return n >= 0 && (size_t)n < size;
}

static int real_directory(const char *path)
{
#ifdef _WIN32
    wchar_t wide[MANTIS_MAX_PATH];
    DWORD attr;
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                             wide, MANTIS_MAX_PATH)) return 0;
    attr = GetFileAttributesW(wide);
    return attr != INVALID_FILE_ATTRIBUTES &&
           (attr & FILE_ATTRIBUTE_DIRECTORY) &&
           !(attr & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat st;
    return lstat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static void normalize_name(const char *name, char *out, size_t size)
{
    size_t i, used = 0;
    size_t length = strlen(name);
    const char *suffixes[] = { ".exe", ".lnk", ".desktop", NULL };
    for (i = 0; suffixes[i]; ++i)
    {
        size_t n = strlen(suffixes[i]);
        size_t j;
        if (length < n) continue;
        for (j = 0; j < n; ++j)
        {
            if (tolower((unsigned char)name[length - n + j]) != suffixes[i][j]) break;
        }
        if (j == n) { length -= n; break; }
    }
    for (i = 0; i < length && used + 1 < size; ++i)
    {
        unsigned char c = (unsigned char)name[i];
        if (c < 128 && isalnum(c)) out[used++] = (char)tolower(c);
    }
    out[used] = '\0';
}

unsigned int MgsMantisMemoryIdentifyName(const char *name)
{
    char value[256];
    if (!name) return 0;
    normalize_name(name, value, sizeof(value));
    if (!strcmp(value, "suikoden") || !strcmp(value, "suikoden1") ||
        !strcmp(value, "suikodeni") || !strcmp(value, "suikoden2") ||
        !strcmp(value, "suikodenii") ||
        !strncmp(value, "suikodeniiihdremaster", strlen("suikodeniiihdremaster")) ||
        !strcmp(value, "gensousuikoden") ||
        !strcmp(value, "gensousuikodenii")) return MGS_MANTIS_SUIKODEN;
    if (!strcmp(value, "azuredreams") || !strcmp(value, "otherlife") ||
        !strcmp(value, "otherlifeazuredreams"))
        return MGS_MANTIS_AZURE_DREAMS;
    if (!strcmp(value, "castlevania") || !strcmp(value, "castlevaniasymphonyofthenight") ||
        !strcmp(value, "castlevaniasotn") ||
        !strcmp(value, "akumajoudraculaxgekkanoyasoukyoku"))
        return MGS_MANTIS_CASTLEVANIA;
    if (!strcmp(value, "vandalhearts") ||
        !strcmp(value, "vandalheartsushinawaretakodaibunmei"))
        return MGS_MANTIS_VANDAL_HEARTS;
    return 0;
}

typedef struct ScanState {
    unsigned int mask;
    unsigned int entries;
    unsigned int depth;
    unsigned int depth_limit;
    unsigned int entry_limit;
} ScanState;

static void scan_directory(const char *path, ScanState *state);

static SDL_EnumerationResult SDLCALL scan_entry(void *userdata,
                                                const char *dirname,
                                                const char *fname)
{
    ScanState *state = (ScanState *)userdata;
    char child[MANTIS_MAX_PATH];
    unsigned int depth;
    if (state->entries >= state->entry_limit) return SDL_ENUM_SUCCESS;
    ++state->entries;
    state->mask |= MgsMantisMemoryIdentifyName(fname);
    if (state->mask == MGS_MANTIS_INSTALLED_MASK) return SDL_ENUM_SUCCESS;
    if (state->depth >= state->depth_limit || !join_path(child, sizeof(child), dirname, fname) ||
        !real_directory(child)) return SDL_ENUM_CONTINUE;
    depth = state->depth;
    state->depth = depth + 1;
    scan_directory(child, state);
    state->depth = depth;
    return SDL_ENUM_CONTINUE;
}

static void scan_directory(const char *path, ScanState *state)
{
    if (state->entries < state->entry_limit &&
        state->mask != MGS_MANTIS_INSTALLED_MASK && real_directory(path))
        SDL_EnumerateDirectory(path, scan_entry, state);
}

unsigned int MgsMantisMemoryScanRoots(const char *const *roots, size_t count)
{
    ScanState state = { 0, 0, 0, 0, 0 };
    const char *unique[MANTIS_MAX_ROOTS];
    size_t i, j, unique_count = 0;
    unsigned int phase_limit = MANTIS_MAX_ENTRIES / 2;
    if (!roots) return 0;
    /* Public callers can supply repeated or empty roots. Dedupe before
     * splitting the finite budget, with an explicit bound on input work. */
    if (count > MANTIS_MAX_ROOTS * 4) count = MANTIS_MAX_ROOTS * 4;
    for (i = 0; i < count && unique_count < MANTIS_MAX_ROOTS; ++i) {
        if (!roots[i] || !roots[i][0]) continue;
        for (j = 0; j < unique_count; ++j) {
#ifdef _WIN32
            if (!_stricmp(roots[i], unique[j])) break;
#else
            if (!strcmp(roots[i], unique[j])) break;
#endif
        }
        if (j == unique_count) unique[unique_count++] = roots[i];
    }
    /* Give every root a bounded share of each pass. Reserve the second half
     * for nested names so a large first library cannot starve later roots. */
    for (i = 0; i < unique_count && state.entries < phase_limit; ++i)
    {
        unsigned int share = (phase_limit - state.entries) / (unsigned int)(unique_count - i);
        state.entry_limit = state.entries + (share ? share : 1);
        state.mask |= MgsMantisMemoryIdentifyName(unique[i]);
        state.depth = 0;
        scan_directory(unique[i], &state);
    }
    state.depth_limit = MANTIS_MAX_DEPTH;
    for (i = 0; i < unique_count && state.entries < MANTIS_MAX_ENTRIES; ++i) {
        unsigned int share = (MANTIS_MAX_ENTRIES - state.entries) / (unsigned int)(unique_count - i);
        state.entry_limit = state.entries + (share ? share : 1);
        state.depth = 0;
        scan_directory(unique[i], &state);
    }
    return state.mask;
}

typedef struct RootList {
    char values[MANTIS_MAX_ROOTS][MANTIS_MAX_PATH];
    size_t count;
} RootList;

static void add_root(RootList *list, const char *path)
{
    size_t i, n;
    if (!path || !path[0] || list->count == MANTIS_MAX_ROOTS) return;
    n = strlen(path);
    if (n >= MANTIS_MAX_PATH || !real_directory(path)) return;
    for (i = 0; i < list->count; ++i)
    {
#ifdef _WIN32
        if (_stricmp(list->values[i], path) == 0) return;
#else
        if (strcmp(list->values[i], path) == 0) return;
#endif
    }
    memcpy(list->values[list->count++], path, n + 1);
}

static void add_child(RootList *list, const char *parent, const char *child)
{
    char path[MANTIS_MAX_PATH];
    if (parent && join_path(path, sizeof(path), parent, child)) add_root(list, path);
}

/* Only Steam's library path configuration is opened. Installed game files are never read. */
static void add_steam_libraries(RootList *list, const char *steam)
{
    char config[MANTIS_MAX_PATH], path[MANTIS_MAX_PATH];
    char line[2048], *key, *end;
    FILE *file;
    size_t total = 0;
    if (!steam) return;
    add_child(list, steam, "steamapps/common");
    if (!join_path(config, sizeof(config), steam, "steamapps/libraryfolders.vdf")) return;
#ifdef _WIN32
    { wchar_t wide[MANTIS_MAX_PATH];
      if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, config, -1,
                               wide, MANTIS_MAX_PATH)) return;
      file = _wfopen(wide, L"rb"); }
#else
    file = fopen(config, "rb");
#endif
    if (!file) return;
    while (total < MANTIS_MAX_VDF && fgets(line, sizeof(line), file))
    {
        total += strlen(line);
        key = strchr(line, '"');
        if (!key) continue;
        end = strchr(++key, '"');
        if (!end) continue;
        *end++ = '\0';
        if (strcmp(key, "path") != 0) continue;
        key = strchr(end, '"');
        if (!key) continue;
        end = strchr(++key, '"');
        if (!end) continue;
        *end = '\0';
#ifdef _WIN32
        /* Valve's VDF escapes Windows separators as double backslashes. */
        { char *read = key, *write = key;
          while (*read) { *write++ = *read; if (*read == '\\' && read[1] == '\\') ++read; ++read; }
          *write = '\0'; }
#else
        if (getenv("WSL_DISTRO_NAME") && isalpha((unsigned char)key[0]) && key[1] == ':') {
            char converted[MANTIS_MAX_PATH];
            size_t j = 0, k;
            converted[j++] = '/'; converted[j++] = 'm'; converted[j++] = 'n';
            converted[j++] = 't'; converted[j++] = '/';
            converted[j++] = (char)tolower((unsigned char)key[0]);
            for (k = 2; key[k] && j + 1 < sizeof(converted); ++k) {
                if (key[k] == '\\' && key[k + 1] == '\\') ++k;
                converted[j++] = key[k] == '\\' ? '/' : key[k];
            }
            converted[j] = '\0';
            if (join_path(path, sizeof(path), converted, "steamapps/common")) add_root(list, path);
            continue;
        }
#endif
        if (join_path(path, sizeof(path), key, "steamapps/common")) add_root(list, path);
    }
    fclose(file);
}

#ifdef _WIN32
static void add_registered_steam(RootList *list)
{
    wchar_t wide[MANTIS_MAX_PATH];
    char utf8[MANTIS_MAX_PATH];
    DWORD bytes = sizeof(wide);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath",
                     RRF_RT_REG_SZ, NULL, wide, &bytes) != ERROR_SUCCESS) return;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1,
                             utf8, sizeof(utf8), NULL, NULL)) return;
    add_steam_libraries(list, utf8);
}
#endif

unsigned int MgsMantisMemoryScanInstalled(void)
{
    RootList list = { { { 0 } }, 0 };
    const char *home = getenv("HOME");
    const char *documents = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    const char *paths[MANTIS_MAX_ROOTS];
    size_t i;
#ifdef _WIN32
    const char *profile = getenv("USERPROFILE");
    const char *local = getenv("LOCALAPPDATA");
    add_registered_steam(&list);
    add_steam_libraries(&list, "C:/Program Files (x86)/Steam");
    add_steam_libraries(&list, "C:/Program Files/Steam");
    if (local) {
        char steam[MANTIS_MAX_PATH];
        if (join_path(steam, sizeof(steam), local, "Steam")) add_steam_libraries(&list, steam);
    }
    add_root(&list, documents);
    add_root(&list, getenv("ProgramFiles"));
    add_root(&list, getenv("ProgramFiles(x86)"));
    add_root(&list, getenv("ProgramW6432"));
    add_child(&list, profile, "Games");
    add_child(&list, profile, "AppData/Local/Programs");
#else
    if (home) {
        char steam[MANTIS_MAX_PATH];
        if (join_path(steam, sizeof(steam), home, ".local/share/Steam")) add_steam_libraries(&list, steam);
        if (join_path(steam, sizeof(steam), home, ".steam/steam")) add_steam_libraries(&list, steam);
    }
    if (getenv("WSL_DISTRO_NAME")) {
        char drive;
        for (drive = 'a'; drive <= 'z'; ++drive) {
            char steam[MANTIS_MAX_PATH];
            snprintf(steam, sizeof(steam), "/mnt/%c/Program Files (x86)/Steam", drive);
            add_steam_libraries(&list, steam);
            snprintf(steam, sizeof(steam), "/mnt/%c/Program Files/Steam", drive);
            add_steam_libraries(&list, steam);
        }
    }
    add_root(&list, documents);
    add_child(&list, home, ".local/share");
    add_child(&list, home, "Games");
    if (getenv("WSL_DISTRO_NAME")) {
        const char *drive;
        for (drive = "cdef"; *drive; ++drive) {
            char root[] = "/mnt/c/Program Files";
            root[5] = *drive;
            add_root(&list, root);
            { char x86[MANTIS_MAX_PATH];
              snprintf(x86, sizeof(x86), "/mnt/%c/Program Files (x86)", *drive);
              add_root(&list, x86); }
            { char users[MANTIS_MAX_PATH];
              snprintf(users, sizeof(users), "/mnt/%c/Users", *drive);
              add_root(&list, users); }
        }
    }
    add_root(&list, "/opt");
    add_root(&list, "/usr/local/games");
    add_root(&list, "/usr/games");
#endif
    for (i = 0; i < list.count; ++i) paths[i] = list.values[i];
    return MgsMantisMemoryScanRoots(paths, list.count);
}

void MgsMantisMemoryConfigure(unsigned int seed)
{
    profile_seed = seed;
    installed_enabled = 0;
    installed_mask = 0;
}

unsigned int MgsMantisMemorySeed(void)
{
    return profile_seed;
}

void MgsMantisMemoryConfigureInstalled(unsigned int mask)
{
    profile_seed = 0;
    installed_enabled = 1;
    installed_mask = mask & MGS_MANTIS_INSTALLED_MASK;
}

int MgsMantisMemoryInstalledEnabled(void) { return installed_enabled; }
unsigned int MgsMantisMemoryInstalledMask(void) { return installed_mask; }
