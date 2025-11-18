#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_ENTRIES 256
#define MAX_PATH_LEN 512
#define STATE_FILE "sdmc:/.clownsec_state"
#define FILES_LIST "sdmc:/.clownsec_files"
#define BASE_PATH "sdmc:/MOFLEX/"
#define ROOT_PATH "sdmc:/"

typedef struct {
    char name[256];
    bool isDirectory;
    int moflexCount; // -1 means not scanned yet
} DirectoryEntry;

typedef struct {
    DirectoryEntry *entries;
    int count;
    int capacity;
    int selected;
    int scrollOffset;
    char currentPath[MAX_PATH_LEN];
} DirectoryList;

// State management (kept small to avoid stack overflow)
typedef struct {
    char sourceDir[MAX_PATH_LEN];
    bool filesActive;
} AppState;

// Function prototypes
void initGfx(void);
void exitGfx(void);
bool loadDirectory(DirectoryList *list, const char *path);
void freeDirectoryList(DirectoryList *list);
void displayDirectory(const DirectoryList *list);
int countMoflexFiles(const char *path);
bool moveFiles(const char *sourceDir, const char *destDir);
void saveState(const AppState *state);
bool loadState(AppState *state);
void clearState(void);
bool launchMoviePlayer(void);
bool isMoflexFile(const char *filename);
void cleanupOldMoflexFiles(void);

int main(int argc, char **argv) {
    // Initialize services
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    // Initialize filesystem access
    Result rc = fsInit();
    if (R_FAILED(rc)) {
        printf("Failed to initialize filesystem\n");
        printf("Error: 0x%08lX\n", rc);
        printf("Press START to exit\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 1;
    }

    // Initialize AM service for launching apps
    rc = amInit();
    if (R_FAILED(rc)) {
        printf("Failed to initialize AM service\n");
        printf("Error: 0x%08lX\n", rc);
        printf("Press START to exit\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        fsExit();
        gfxExit();
        return 1;
    }

    // Check if we need to restore files
    AppState state = {0};
    if (loadState(&state) && state.filesActive) {
        consoleClear();
        printf("Clownsec Moflex Launcher\n");
        printf("========================\n\n");
        printf("Restoring files from root...\n");
        printf("Source: %s\n\n", state.sourceDir);

        if (moveFiles(ROOT_PATH, state.sourceDir)) {
            printf("Files restored successfully!\n");
            clearState();
        } else {
            printf("ERROR: Failed to restore files!\n");
            printf("Please manually move files back.\n");
        }

        printf("\nPress START to continue\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
    }

    // Create MOFLEX folder if it doesn't exist
    mkdir("sdmc:/MOFLEX", 0777);

    // Check for and cleanup old moflex files in root
    cleanupOldMoflexFiles();

    consoleClear();
    printf("Clownsec Moflex Launcher\n");
    printf("========================\n\n");
    printf("Loading directory...\n");
    printf("Path: %s\n", BASE_PATH);
    gfxFlushBuffers();
    gfxSwapBuffers();

    // Main directory browser
    DirectoryList dirList = {0};
    strncpy(dirList.currentPath, BASE_PATH, MAX_PATH_LEN - 1);

    if (!loadDirectory(&dirList, dirList.currentPath)) {
        consoleClear();
        printf("\x1b[1;1H");
        printf("MOFLEX Folder Not Found\n");
        printf("=======================\n\n");
        printf("Please create a 'MOFLEX' folder\n");
        printf("on your SD card and put your\n");
        printf("moflex collections inside it.\n\n");
        printf("Example structure:\n");
        printf("  sdmc:/MOFLEX/\n");
        printf("    Action/\n");
        printf("    Comedy/\n");
        printf("    SciFi/\n\n");
        printf("Press START to exit\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        freeDirectoryList(&dirList);
        amExit();
        fsExit();
        gfxExit();
        return 1;
    }

    consoleClear();
    printf("Clownsec Moflex Launcher\n");
    printf("========================\n\n");
    printf("Directory loaded successfully!\n");
    printf("Found %d directories\n\n", dirList.count);
    printf("Starting browser...\n");
    gfxFlushBuffers();
    gfxSwapBuffers();
    svcSleepThread(1000000000LL); // Wait 1 second

    bool running = true;
    bool needsRedraw = true;

    while (running && aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) {
            running = false;
        }

        if (kDown & KEY_UP) {
            if (dirList.selected > 0) {
                dirList.selected--;
                if (dirList.selected < dirList.scrollOffset) {
                    dirList.scrollOffset = dirList.selected;
                }
                needsRedraw = true;
            }
        }

        if (kDown & KEY_DOWN) {
            if (dirList.selected < dirList.count - 1) {
                dirList.selected++;
                int visibleLines = 25;
                if (dirList.selected >= dirList.scrollOffset + visibleLines) {
                    dirList.scrollOffset = dirList.selected - visibleLines + 1;
                }
                needsRedraw = true;
            }
        }

        if (kDown & KEY_A) {
            if (dirList.count > 0) {
                DirectoryEntry *entry = &dirList.entries[dirList.selected];

                if (entry->isDirectory) {
                    // First, count the moflex files if not already counted
                    if (entry->moflexCount == -1) {
                        char fullPath[MAX_PATH_LEN];
                        snprintf(fullPath, MAX_PATH_LEN, "%s%s", dirList.currentPath, entry->name);
                        entry->moflexCount = countMoflexFiles(fullPath);
                    }

                    // Show confirmation dialog
                    consoleClear();
                    printf("Clownsec Moflex Launcher\n");
                    printf("========================\n\n");
                    printf("Selected: %s\n", entry->name);
                    printf("Moflex files: %d\n\n", entry->moflexCount);

                    if (entry->moflexCount > 126) {
                        printf("WARNING: More than 126 files!\n");
                        printf("3D Movie Player may crash.\n\n");
                    }

                    if (entry->moflexCount == 0) {
                        printf("No moflex files found!\n");
                        printf("\nPress B to go back\n");

                        while (aptMainLoop()) {
                            hidScanInput();
                            if (hidKeysDown() & KEY_B) break;
                            gfxFlushBuffers();
                            gfxSwapBuffers();
                            gspWaitForVBlank();
                        }
                        needsRedraw = true;
                        continue;
                    }

                    printf("Press A to move files and launch\n");
                    printf("Press B to cancel\n");

                    bool waitingConfirm = true;
                    bool confirmed = false;

                    while (waitingConfirm && aptMainLoop()) {
                        hidScanInput();
                        u32 kConfirm = hidKeysDown();

                        if (kConfirm & KEY_A) {
                            confirmed = true;
                            waitingConfirm = false;
                        }
                        if (kConfirm & KEY_B) {
                            waitingConfirm = false;
                        }

                        gfxFlushBuffers();
                        gfxSwapBuffers();
                        gspWaitForVBlank();
                    }

                    if (confirmed) {
                        char sourcePath[MAX_PATH_LEN];
                        snprintf(sourcePath, MAX_PATH_LEN, "%s%s", dirList.currentPath, entry->name);

                        consoleClear();
                        printf("Clownsec Moflex Launcher\n");
                        printf("========================\n\n");
                        printf("Moving files to root...\n");
                        printf("From: %s\n\n", sourcePath);

                        if (moveFiles(sourcePath, ROOT_PATH)) {
                            printf("Files moved successfully!\n\n");

                            // Save state
                            AppState newState = {0};
                            strncpy(newState.sourceDir, sourcePath, MAX_PATH_LEN - 1);
                            newState.filesActive = true;
                            saveState(&newState);

                            printf("Launching 3D Movie Player...\n");
                            printf("When done, exit and relaunch\n");
                            printf("this app to restore files.\n\n");

                            gfxFlushBuffers();
                            gfxSwapBuffers();
                            gspWaitForVBlank();

                            svcSleepThread(2000000000LL); // Wait 2 seconds

                            if (launchMoviePlayer()) {
                                // App will exit here to launch Movie Player
                                running = false;
                            } else {
                                printf("Failed to launch Movie Player!\n");
                                printf("Restoring files...\n");
                                moveFiles(ROOT_PATH, sourcePath);
                                clearState();
                                printf("\nPress START to exit\n");

                                while (aptMainLoop()) {
                                    hidScanInput();
                                    if (hidKeysDown() & KEY_START) {
                                        running = false;
                                        break;
                                    }
                                    gfxFlushBuffers();
                                    gfxSwapBuffers();
                                    gspWaitForVBlank();
                                }
                            }
                        } else {
                            printf("ERROR: Failed to move files!\n");
                            printf("\nPress B to go back\n");

                            while (aptMainLoop()) {
                                hidScanInput();
                                if (hidKeysDown() & KEY_B) break;
                                gfxFlushBuffers();
                                gfxSwapBuffers();
                                gspWaitForVBlank();
                            }
                            needsRedraw = true;
                        }
                    } else {
                        needsRedraw = true;
                    }
                }
            }
        }

        if (kDown & KEY_B) {
            // Go back to parent directory
            if (strcmp(dirList.currentPath, BASE_PATH) != 0) {
                // Remove last directory component
                char *lastSlash = strrchr(dirList.currentPath, '/');
                if (lastSlash != NULL && lastSlash != dirList.currentPath) {
                    // Find second to last slash
                    *lastSlash = '\0';
                    lastSlash = strrchr(dirList.currentPath, '/');
                    if (lastSlash != NULL) {
                        *(lastSlash + 1) = '\0';
                    }
                } else {
                    strncpy(dirList.currentPath, BASE_PATH, MAX_PATH_LEN - 1);
                }

                freeDirectoryList(&dirList);
                loadDirectory(&dirList, dirList.currentPath);
                needsRedraw = true;
            }
        }

        if (needsRedraw) {
            displayDirectory(&dirList);
            needsRedraw = false;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    // Cleanup
    freeDirectoryList(&dirList);
    amExit();
    fsExit();
    gfxExit();

    return 0;
}

bool loadDirectory(DirectoryList *list, const char *path) {
    // Allocate memory for entries
    list->capacity = MAX_ENTRIES;
    list->entries = (DirectoryEntry *)linearAlloc(sizeof(DirectoryEntry) * list->capacity);
    if (!list->entries) {
        return false;
    }

    list->count = 0;
    list->selected = 0;
    list->scrollOffset = 0;
    strncpy(list->currentPath, path, MAX_PATH_LEN - 1);

    DIR *dir = opendir(path);
    if (!dir) {
        linearFree(list->entries);
        list->entries = NULL;
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && list->count < MAX_ENTRIES) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Skip hidden files
        if (entry->d_name[0] == '.') {
            continue;
        }

        struct stat st;
        char fullPath[MAX_PATH_LEN];
        snprintf(fullPath, MAX_PATH_LEN, "%s%s", path, entry->d_name);

        if (stat(fullPath, &st) == 0) {
            // Only show directories, skip files
            if (!S_ISDIR(st.st_mode)) {
                continue;
            }

            DirectoryEntry *dirEntry = &list->entries[list->count];
            strncpy(dirEntry->name, entry->d_name, 255);
            dirEntry->name[255] = '\0';
            dirEntry->isDirectory = true; // Always true now
            dirEntry->moflexCount = -1; // Not scanned yet

            list->count++;
        }
    }

    closedir(dir);

    // Sort: directories first, then alphabetically
    for (int i = 0; i < list->count - 1; i++) {
        for (int j = i + 1; j < list->count; j++) {
            bool swap = false;

            if (list->entries[i].isDirectory && !list->entries[j].isDirectory) {
                continue;
            } else if (!list->entries[i].isDirectory && list->entries[j].isDirectory) {
                swap = true;
            } else if (strcasecmp(list->entries[i].name, list->entries[j].name) > 0) {
                swap = true;
            }

            if (swap) {
                DirectoryEntry temp = list->entries[i];
                list->entries[i] = list->entries[j];
                list->entries[j] = temp;
            }
        }
    }

    return true;
}

void freeDirectoryList(DirectoryList *list) {
    if (list->entries) {
        linearFree(list->entries);
        list->entries = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

void displayDirectory(const DirectoryList *list) {
    consoleClear();
    printf("Clownsec Moflex Launcher\n");
    printf("========================\n\n");
    printf("Current: %s\n\n", list->currentPath);

    if (list->count == 0) {
        printf("(Empty directory)\n\n");
    } else {
        int visibleLines = 25;
        int endIdx = list->scrollOffset + visibleLines;
        if (endIdx > list->count) {
            endIdx = list->count;
        }

        for (int i = list->scrollOffset; i < endIdx; i++) {
            const DirectoryEntry *entry = &list->entries[i];

            if (i == list->selected) {
                printf("> ");
            } else {
                printf("  ");
            }

            if (entry->isDirectory) {
                printf("[DIR] %s\n", entry->name);
            } else {
                printf("      %s\n", entry->name);
            }
        }

        if (list->count > visibleLines) {
            printf("\n(%d-%d of %d)\n",
                   list->scrollOffset + 1,
                   endIdx,
                   list->count);
        }
    }

    printf("\nA: Select  B: Back  START: Exit\n");
}

int countMoflexFiles(const char *path) {
    int count = 0;

    DIR *dir = opendir(path);
    if (!dir) {
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        if (isMoflexFile(entry->d_name)) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

bool isMoflexFile(const char *filename) {
    size_t len = strlen(filename);
    if (len < 8) { // ".moflex" is 7 characters
        return false;
    }

    const char *ext = filename + len - 7;
    return (strcasecmp(ext, ".moflex") == 0);
}

void cleanupOldMoflexFiles() {
    // Check if state file exists - if it does, we don't need to cleanup
    AppState state = {0};
    if (loadState(&state)) {
        return; // State exists, don't cleanup
    }

    // Check if there are any moflex files in root
    DIR *dir = opendir(ROOT_PATH);
    if (!dir) return;

    bool foundMoflex = false;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (isMoflexFile(entry->d_name)) {
            foundMoflex = true;
            break;
        }
    }
    closedir(dir);

    if (!foundMoflex) {
        return; // No moflex files in root, nothing to cleanup
    }

    // Found old moflex files, move them to OLDMOFLEX
    consoleClear();
    printf("Clownsec Moflex Launcher\n");
    printf("========================\n\n");
    printf("Found existing moflex files in root!\n");
    printf("Moving them to /MOFLEX/OLDMOFLEX/...\n\n");

    // Create OLDMOFLEX folder
    mkdir("sdmc:/MOFLEX/OLDMOFLEX", 0777);

    // Move files
    if (moveFiles(ROOT_PATH, "sdmc:/MOFLEX/OLDMOFLEX")) {
        printf("Files moved successfully!\n");
    } else {
        printf("Warning: Some files may not have moved.\n");
    }

    printf("\nPress START to continue\n");
    gfxFlushBuffers();
    gfxSwapBuffers();

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
}

bool moveFiles(const char *sourceDir, const char *destDir) {
    DIR *dir = opendir(sourceDir);
    if (!dir) {
        return false;
    }

    bool success = true;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        if (!isMoflexFile(entry->d_name)) {
            continue;
        }

        char sourcePath[MAX_PATH_LEN];
        char destPath[MAX_PATH_LEN];

        // Add slash only if sourceDir doesn't end with one
        size_t sourceLen = strlen(sourceDir);
        if (sourceLen > 0 && sourceDir[sourceLen - 1] == '/') {
            snprintf(sourcePath, MAX_PATH_LEN, "%s%s", sourceDir, entry->d_name);
        } else {
            snprintf(sourcePath, MAX_PATH_LEN, "%s/%s", sourceDir, entry->d_name);
        }

        // Add slash only if destDir doesn't end with one
        size_t destLen = strlen(destDir);
        if (destLen > 0 && destDir[destLen - 1] == '/') {
            snprintf(destPath, MAX_PATH_LEN, "%s%s", destDir, entry->d_name);
        } else {
            snprintf(destPath, MAX_PATH_LEN, "%s/%s", destDir, entry->d_name);
        }

        if (rename(sourcePath, destPath) != 0) {
            printf("Failed to move: %s\n", entry->d_name);
            success = false;
        } else {
            // Force filesystem sync after each file on real hardware
            FS_Archive sdmcArchive = {ARCHIVE_SDMC, {PATH_EMPTY, 0, (u8*)""}};
            FSUSER_ControlArchive(sdmcArchive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
            svcSleepThread(50000000LL); // 50ms delay for hardware
        }
    }

    closedir(dir);

    // Final sync to ensure all operations are committed
    FS_Archive sdmcArchive = {ARCHIVE_SDMC, {PATH_EMPTY, 0, (u8*)""}};
    FSUSER_ControlArchive(sdmcArchive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
    svcSleepThread(100000000LL); // 100ms final delay

    return success;
}

void saveState(const AppState *state) {
    FILE *f = fopen(STATE_FILE, "wb");
    if (f) {
        fwrite(state, sizeof(AppState), 1, f);
        fflush(f);
        fclose(f);

        // Force filesystem sync to ensure state is written on real hardware
        FS_Archive sdmcArchive = {ARCHIVE_SDMC, {PATH_EMPTY, 0, (u8*)""}};
        FSUSER_ControlArchive(sdmcArchive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
        svcSleepThread(100000000LL); // 100ms delay
    }
}

bool loadState(AppState *state) {
    FILE *f = fopen(STATE_FILE, "rb");
    if (!f) {
        return false;
    }

    size_t read = fread(state, sizeof(AppState), 1, f);
    fclose(f);

    return (read == 1);
}

void clearState(void) {
    remove(STATE_FILE);

    // Force filesystem sync after removing state file
    FS_Archive sdmcArchive = {ARCHIVE_SDMC, {PATH_EMPTY, 0, (u8*)""}};
    FSUSER_ControlArchive(sdmcArchive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
    svcSleepThread(100000000LL); // 100ms delay
}

bool launchMoviePlayer(void) {
    // 3D Movie Player title IDs for different regions
    u64 titleIds[] = {
        0x0004000000036A00ULL,  // CIA installed version (most common on emulator/CFW)
        0x0004001000021A00ULL,  // USA (preinstalled)
        0x0004001000021B01ULL,  // EUR (preinstalled)
        0x0004001000020F00ULL   // JPN (preinstalled)
    };

    // Try both SD and NAND media types
    FS_MediaType mediaTypes[] = {MEDIATYPE_SD, MEDIATYPE_NAND};

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 4; i++) {
            // Check if title is actually installed before trying to launch
            AM_TitleEntry titleEntry;
            Result rc = AM_GetTitleInfo(mediaTypes[m], 1, &titleIds[i], &titleEntry);

            if (R_FAILED(rc)) {
                // Title not installed on this media type, try next
                continue;
            }

            // Title is installed, try to launch it
            u8 buf[0x300];
            u8 hmac[0x20];
            memset(buf, 0, sizeof(buf));
            memset(hmac, 0, sizeof(hmac));

            rc = APT_PrepareToDoApplicationJump(0, titleIds[i], mediaTypes[m]);
            if (R_SUCCEEDED(rc)) {
                rc = APT_DoApplicationJump(buf, sizeof(buf), hmac);
                if (R_SUCCEEDED(rc)) {
                    return true;
                }
            }
        }
    }

    return false;
}
