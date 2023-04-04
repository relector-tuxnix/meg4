/*
 * meg4/platform/common.h
 *
 * Copyright (C) 2023 bzt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @brief Common on all platforms, mostly OS specific file management stuff
 *
 */

#include <stdio.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef FILENAME_MAX
#define FILENAME_MAX 255
#endif

/* command line flags */
int verbose = 0, zenity = 0, nearest = 0;
int strcasecmp(const char *, const char *);
char *main_floppydir = NULL;
void main_hdr(void);
void meg4_export(char *name, int binary);

/**
 * Windows workaround
 */
#ifdef __WIN32__
#define CLIFLAG "/"
#define SEP "\\"
#include <windows.h>
#include <winnls.h>
#include <fileapi.h>
#include <shlobj.h>
#include <shellapi.h>

static HINSTANCE hWinInstance;
static char *main_lng = NULL;

/* these two functions were borrowed from sdl_windows_main.c */
static void UnEscapeQuotes(char *arg)
{
    char *last = NULL, *c_curr, *c_last;

    while (*arg) {
        if (*arg == '"' && (last != NULL && *last == '\\')) {
            c_curr = arg;
            c_last = last;

            while (*c_curr) {
                *c_last = *c_curr;
                c_last = c_curr;
                c_curr++;
            }
            *c_last = '\0';
        }
        last = arg;
        arg++;
    }
}

/* Parse a command line buffer into arguments */
static int ParseCommandLine(WCHAR *cmdline, char **argv)
{
    char *bufp, *args;
    char *lastp = NULL;
    int argc, last_argc, l;

    l = WideCharToMultiByte(CP_UTF8, 0, cmdline, -1, NULL, 0, NULL, NULL);
    if(!l || !(args = malloc(l))) return 0;
    WideCharToMultiByte(CP_UTF8, 0, cmdline, -1, args, l, NULL, NULL);
    argc = last_argc = 0;
    for (bufp = args; *bufp;) {
        /* Skip leading whitespace */
        while (*bufp == ' ') {
            ++bufp;
        }
        /* Skip over argument */
        if (*bufp == '"') {
            ++bufp;
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            lastp = bufp;
            while (*bufp && (*bufp != '"' || *lastp == '\\')) {
                lastp = bufp;
                ++bufp;
            }
        } else {
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            while (*bufp && *bufp != ' ') {
                ++bufp;
            }
        }
        if (*bufp) {
            if (argv) {
                *bufp = '\0';
            }
            ++bufp;
        }

        /* Strip out \ from \" sequences */
        if (argv && last_argc != argc) {
            UnEscapeQuotes(argv[last_argc]);
        }
        last_argc = argc;
    }
    if (argv) {
        argv[argc] = NULL;
    }
    return (argc);
}

/* Windows entry point */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WCHAR *cmdline = GetCommandLineW();
    int ret, argc = ParseCommandLine(cmdline, NULL);
    char **argv = calloc(argc+2, sizeof(char*));
    char *lng = getenv("LANG");
    FILE *f;
    int lid;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    hWinInstance = hInstance;
    if(!lng) {
        lid = GetUserDefaultLangID(); /* GetUserDefaultUILanguage(); */
        /* see https://docs.microsoft.com/en-us/windows/win32/intl/language-identifier-constants-and-strings */
        switch(lid & 0xFF) {
            case 0x01: lng = "ar"; break;   case 0x02: lng = "bg"; break;   case 0x03: lng = "ca"; break;
            case 0x04: lng = "zh"; break;   case 0x05: lng = "cs"; break;   case 0x06: lng = "da"; break;
            case 0x07: lng = "de"; break;   case 0x08: lng = "el"; break;   case 0x0A: lng = "es"; break;
            case 0x0B: lng = "fi"; break;   case 0x0C: lng = "fr"; break;   case 0x0D: lng = "he"; break;
            case 0x0E: lng = "hu"; break;   case 0x0F: lng = "is"; break;   case 0x10: lng = "it"; break;
            case 0x11: lng = "jp"; break;   case 0x12: lng = "ko"; break;   case 0x13: lng = "nl"; break;
            case 0x14: lng = "no"; break;   case 0x15: lng = "pl"; break;   case 0x16: lng = "pt"; break;
            case 0x17: lng = "rm"; break;   case 0x18: lng = "ro"; break;   case 0x19: lng = "ru"; break;
            case 0x1A: lng = "hr"; break;   case 0x1B: lng = "sk"; break;   case 0x1C: lng = "sq"; break;
            case 0x1D: lng = "sv"; break;   case 0x1E: lng = "th"; break;   case 0x1F: lng = "tr"; break;
            case 0x20: lng = "ur"; break;   case 0x21: lng = "id"; break;   case 0x22: lng = "uk"; break;
            case 0x23: lng = "be"; break;   case 0x24: lng = "sl"; break;   case 0x25: lng = "et"; break;
            case 0x26: lng = "lv"; break;   case 0x27: lng = "lt"; break;   case 0x29: lng = "fa"; break;
            case 0x2A: lng = "vi"; break;   case 0x2B: lng = "hy"; break;   case 0x2D: lng = "bq"; break;
            case 0x2F: lng = "mk"; break;   case 0x36: lng = "af"; break;   case 0x37: lng = "ka"; break;
            case 0x38: lng = "fo"; break;   case 0x39: lng = "hi"; break;   case 0x3A: lng = "mt"; break;
            case 0x3C: lng = "gd"; break;   case 0x3E: lng = "ms"; break;   case 0x3F: lng = "kk"; break;
            case 0x40: lng = "ky"; break;   case 0x45: lng = "bn"; break;   case 0x47: lng = "gu"; break;
            case 0x4D: lng = "as"; break;   case 0x4E: lng = "mr"; break;   case 0x4F: lng = "sa"; break;
            case 0x53: lng = "kh"; break;   case 0x54: lng = "lo"; break;   case 0x56: lng = "gl"; break;
            case 0x5E: lng = "am"; break;   case 0x62: lng = "fy"; break;   case 0x68: lng = "ha"; break;
            case 0x6D: lng = "ba"; break;   case 0x6E: lng = "lb"; break;   case 0x6F: lng = "kl"; break;
            case 0x7E: lng = "br"; break;   case 0x92: lng = "ku"; break;   case 0x09: default: lng = "en"; break;
        }
    }
    main_lng = lng;
    /* restore stdout to console */
    AttachConsole(ATTACH_PARENT_PROCESS);
    f = _fdopen(_open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), 0x4000/*_O_TEXT*/), "w");
    if(f) { *stdout = *f; *stderr = *f; }
    printf("\r\n");
    ParseCommandLine(cmdline, argv);
#ifdef SDL_h_
    SDL_SetMainReady();
#endif
    ret = main(argc, argv);
    free(argv);
    exit(ret);
    return ret;
}

/**
 * Read a file entirely into memory
 */
uint8_t* main_readfile(char *file, int *size)
{
#ifdef __EMSCRIPTEN__
    (void)file; (void)size;
    return NULL;
#else
    wchar_t szFile[PATH_MAX + FILENAME_MAX + 1];
    uint8_t *data = NULL;
    HANDLE f;
    DWORD r, t;

    if(!file || !*file || !size) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, file, -1, szFile, PATH_MAX + FILENAME_MAX);
    f = CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(f) {
        r = GetFileSize(f, NULL);
        *size = r;
        data = (uint8_t*)malloc(*size);
        if(data) {
            memset(data, 0, *size);
            if(!ReadFile(f, data, r, &t, NULL) || r != t) { free(data); data = NULL; *size = 0; }
        }
        CloseHandle(f);
    }
    return data;
#endif
}

/**
 * Write to a file
 */
int main_writefile(char *file, uint8_t *buf, int size)
{
#ifdef __EMSCRIPTEN__
    (void)file; (void)buf; (void)size;
    return 0;
#else
    int ret = 0;
    wchar_t szFile[PATH_MAX + FILENAME_MAX + 1];
    HANDLE f;
    DWORD w = size, t;

    if(!file || !*file || !buf || size < 1) return 0;
    MultiByteToWideChar(CP_UTF8, 0, file, -1, szFile, PATH_MAX + FILENAME_MAX);
    f = CreateFileW(szFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(f) {
        ret = (WriteFile(f, buf, w, &t, NULL) && w == t);
        CloseHandle(f);
    }
    return ret;
#endif
}

/**
 * Open open file modal and import selected file
 */
void main_openfile(void)
{
#ifndef EMBED
#ifdef __EMSCRIPTEN__
    EM_ASM({ document.getElementById('meg4_upload').click(); });
#else
    OPENFILENAMEW ofn;
    wchar_t szFile[PATH_MAX + FILENAME_MAX + 1];
    char fn[PATH_MAX + FILENAME_MAX + 1], *n;
    int len;
    uint8_t *buf;

    memset(&szFile,0,sizeof(szFile));
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = NULL;
    ofn.hInstance       = hWinInstance;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof(szFile)-1;
    ofn.Flags           = OFN_EXPLORER | OFN_FILEMUSTEXIST;
    if(GetOpenFileNameW(&ofn)) {
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, fn, PATH_MAX + FILENAME_MAX, NULL, NULL);
        if((buf = main_readfile(fn, &len))) {
            n = strrchr(fn, SEP[0]); if(!n) n = fn; else n++;
            meg4_insert(fn, buf, len);
            free(buf);
        }
    }
#endif
#endif
}

/**
 * Open save file modal and write buffer to selected file
 */
int main_savefile(const char *name, uint8_t *buf, int len)
{
#ifdef __EMSCRIPTEN__
    if(name && buf && len > 0) {
        EM_ASM({ meg4_savefile($0, $1, $2, $3); }, name, strlen(name), buf, len);
        return 1;
    }
    return 0;
#else
    OPENFILENAMEW ofn;
    wchar_t szFile[PATH_MAX + FILENAME_MAX + 1], szExt[FILENAME_MAX];
    char fn[PATH_MAX + FILENAME_MAX + 1];
    int l = 0, ret = 0;

    if(!name || !*name || !buf || len < 1) return 0;
    memset(&szFile,0,sizeof(szFile));
    if(main_floppydir && *main_floppydir) {
        strcpy(fn, main_floppydir);
        if(fn[strlen(fn) - 1] == SEP[0]) fn[strlen(fn) - 1] = 0;
        MultiByteToWideChar(CP_UTF8, 0, fn, -1, szFile, sizeof(szFile)-1);
        _wmkdir(szFile);
        strcat(fn, SEP);
        strcat(fn, name);
        return main_writefile(fn, buf, len);
    }
#ifndef EMBED
    else {
        memset(&szExt,0,sizeof(szExt));
        memcpy(szExt, L"All\0*.*\0", 18);
        l = MultiByteToWideChar(CP_UTF8, 0, name, -1, szFile, sizeof(szFile)-1);
        while(l > 0 && szFile[l - 1] != L'.') l--;
        if(l > 0) wsprintfW(szExt, L"%s\0*.%s\0", szFile + l, szFile + l);
        memset(&ofn,0,sizeof(ofn));
        ofn.lStructSize     = sizeof(ofn);
        ofn.hwndOwner       = NULL;
        ofn.hInstance       = hWinInstance;
        ofn.lpstrFilter     = szExt;
        ofn.lpstrFile       = szFile;
        ofn.nMaxFile        = sizeof(szFile)-1;
        ofn.lpstrDefExt     = l > 0 ? &szFile[l] : NULL;
        ofn.Flags           = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if(buf && len > 0 && GetSaveFileNameW(&ofn)) {
            WideCharToMultiByte(CP_UTF8, 0, szFile, -1, fn, PATH_MAX + FILENAME_MAX + 1, NULL, NULL);
            ret = main_writefile(fn, buf, len);
        }
    }
#endif
    return ret;
#endif
}

/**
 * Return floppy list
 */
char **main_getfloppies(void)
{
#ifndef __EMSCRIPTEN__
    wchar_t szFile[PATH_MAX + FILENAME_MAX + 1];
    WIN32_FIND_DATAW ffd;
    HANDLE h;
    int n = 0, j, l;
    char **ret = NULL;

    if(!main_floppydir || !*main_floppydir) return NULL;
    ret = (char**)malloc(sizeof(char*));
    if(!ret) return NULL; else ret[0] = NULL;
    memset(&szFile,0,sizeof(szFile));
    l = MultiByteToWideChar(CP_UTF8, 0, main_floppydir, -1, szFile, sizeof(szFile)-1);
    if(!l) return NULL;
    if(szFile[l - 1] != L'\\') szFile[l++] = L'\\';
    wcscpy_s(szFile + l, FILENAME_MAX, L"*.PNG");
    h = FindFirstFileW(szFile, &ffd);
    if(h != INVALID_HANDLE_VALUE) {
        do {
            wcscpy_s(szFile + l, FILENAME_MAX, ffd.cFileName);
            j = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, NULL, PATH_MAX + FILENAME_MAX + 2, NULL, NULL);
            if(ffd.cFileName[0] == L'.' || !j) continue;
            ret = (char**)realloc(ret, (n + 2) * sizeof(char*));
            if(!ret) break;
            ret[n + 1] = NULL;
            ret[n] = (char*)malloc(j + 1);
            if(!ret[n]) continue;
            WideCharToMultiByte(CP_UTF8, 0, szFile, -1, ret[n], j + 1, NULL, NULL);
            n++;
        } while(FindNextFileW(h, &ffd) != 0);
        FindClose(h);
    }
    return ret;
#else
    return NULL;
#endif
}

#ifndef __EMSCRIPTEN__
/**
 * Save a config file
 */
int main_cfgsave(char *cfg, uint8_t *buf, int len)
{
    HANDLE f;
    DWORD w = len, t;
    wchar_t szFile[PATH_MAX + FILENAME_MAX + 1], szCfg[FILENAME_MAX];
    int ret = 0, i;

    if(!SHGetFolderPathW(HWND_DESKTOP, CSIDL_PROFILE, NULL, 0, szFile)) {
        MultiByteToWideChar(CP_UTF8, 0, cfg, -1, szCfg, sizeof(szCfg)-1);
        wcscat(szFile, L"\\AppData"); _wmkdir(szFile);
        wcscat(szFile, L"\\Local"); _wmkdir(szFile);
        wcscat(szFile, L"\\MEG-4"); _wmkdir(szFile);
        wcscat(szFile, L"\\"); i = wcslen(szFile); wcscat(szFile, szCfg);
        for(; szFile[i]; i++) if(szFile[i] == L'/' || szFile[i] == L'\\') { szFile[i] = 0; _wmkdir(szFile); szFile[i] = L'\\'; }
        f = CreateFileW(szFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if(f) {
            ret = (WriteFile(f, buf, w, &t, NULL) && w == t);
            CloseHandle(f);
        }
    }
    return ret;
}

/**
 * Load a config file
 */
uint8_t *main_cfgload(char *cfg, int *len)
{
    uint8_t *data = NULL;
    HANDLE f;
    DWORD r, t;
    wchar_t szFile[PATH_MAX + FILENAME_MAX + 1], szCfg[FILENAME_MAX];
    int i;

    if(!SHGetFolderPathW(HWND_DESKTOP, CSIDL_PROFILE, NULL, 0, szFile)) {
        MultiByteToWideChar(CP_UTF8, 0, cfg, -1, szCfg, sizeof(szCfg)-1);
        wcscat(szFile, L"\\AppData\\Local\\MEG-4\\"); wcscat(szFile, szCfg);
        for(i = 0; szFile[i]; i++) if(szFile[i] == L'/') szFile[i] = '\\';
        f = CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if(f) {
            r = GetFileSize(f, NULL);
            *len = r;
            data = (uint8_t*)malloc(*len + 1);
            if(data) {
                memset(data, 0, *len + 1);
                if(!ReadFile(f, data, r, &t, NULL) || r != t) { free(data); data = NULL; *len = 0; }
            }
            CloseHandle(f);
        }
    }
    return data;
}
#endif

#else
#define CLIFLAG "-"
#define SEP "/"
#include <dirent.h>
#ifdef _MACOS_
#include <AppKit/AppKit.h>
#else
void main_delay(int msec);
#include <signal.h>     /* including the glib version of this with __USE_MISC doesn't compile... */
#define __USE_MISC      /* needed for MAP_ANONYMOUS */
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>   /* mkdir... */
#include <sys/types.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
/* some moron hide these behind feature defines which don't work if you have __USE_MISC... */
int kill(pid_t pid, int sig);
FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);
/* gtk is sooo fucked up, DO NOT include its headers... */
/*#include <gtk/gtk.h>*/
void (*gtk_init)(int *argc, char ***argv);
void* (*gtk_file_chooser_dialog_new)(const char *title, void *parent, int action, const char *first_button_text, ...);
int (*gtk_dialog_run)(void *dialog);
void (*gtk_widget_destroy)(void *widget);
char* (*gtk_file_chooser_get_filename)(void *chooser);
void (*gtk_file_chooser_set_do_overwrite_confirmation)(void *chooser, int do_overwrite_confirmation);
void (*gtk_file_chooser_set_create_folders)(void *chooser, int create_folders);
int (*gtk_file_chooser_set_current_name)(void *chooser, const char *filename);
#endif

/**
 * Read a file entirely into memory
 */
uint8_t* main_readfile(char *file, int *size)
{
#ifdef __EMSCRIPTEN__
    (void)file; (void)size;
    return NULL;
#else
    uint8_t *data = NULL;
    FILE *f;

    if(!file || !*file || !size) return NULL;
    f = fopen(file, "rb");
    if(f) {
        fseek(f, 0L, SEEK_END);
        *size = (int)ftell(f);
        fseek(f, 0L, SEEK_SET);
        data = (uint8_t*)malloc(*size);
        if(data) {
            memset(data, 0, *size);
            if((int)fread(data, 1, *size, f) != *size) { free(data); data = NULL; *size = 0; }
        }
        fclose(f);
    }
    return data;
#endif
}

/**
 * Write to a file
 */
int main_writefile(char *file, uint8_t *buf, int size)
{
#ifdef __EMSCRIPTEN__
    (void)file; (void)buf; (void)size;
    return 0;
#else
    int ret = 0;
    FILE *f;

    if(!file || !*file || !buf || size < 1) return 0;
    f = fopen(file, "wb");
    if(f) {
        ret = ((int)fwrite(buf, 1, size, f) == size);
        fclose(f);
    }
    return ret;
#endif
}

/**
 * Open open file modal and import selected file
 */
void main_openfile(void)
{
#ifndef EMBED
#ifdef __EMSCRIPTEN__
    EM_ASM({ document.getElementById('meg4_upload').click(); });
#else
    char *fn = NULL, *n;
    int len;
    uint8_t *buf;
#ifdef __ANDROID__
    /* TODO */
#else
#ifdef _MACOS_
    const char *utf8Path;
    NSURL *url;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
    NSOpenPanel *dialog = [NSOpenPanel openPanel];
    [dialog setAllowsMultipleSelection:NO];

    if([dialog runModal] == NSModalResponseOK) {
        url = [dialog URL];
        utf8Path = [[url path] UTF8String];
        len = strlen(utf8Path);
        fn = (char*)malloc(len + 1);
        if(fn) strcpy(ret, utf8Path);
    }
    [pool release];
    [keyWindow makeKeyAndOrderFront:nil];
#else
    FILE *p = NULL;
    pid_t pid;
    void *chooser;
    void *handle;
    char *tmp1, *tmp2;
    tmp1 = mmap(NULL, PATH_MAX, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(!tmp1) return;
    memset(tmp1, 0, PATH_MAX);
    meg4_showcursor();
    if(zenity) p = popen("/usr/bin/zenity --file-selection", "r");
    if(p) {
        main_log(1, "using zenity hack");
        if(!fgets(tmp1 + 1, PATH_MAX - 1, p)) main_log(1, "fgets error");
        pclose(p);
        for(tmp2 = tmp1 + 1; *tmp2 && *tmp2 != '\n' && *tmp2 != '\r'; tmp2++);
        *tmp2 = 0;
        tmp1[0] = 1;
    } else {
        main_log(1, "trying to dlopen gtk");
        if(!(pid = fork())) {
            handle = dlopen("libgtk-3.so", RTLD_LAZY | RTLD_LOCAL);
            if(handle) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
                if( (gtk_init = dlsym(handle, "gtk_init")) &&
                    (gtk_file_chooser_dialog_new = dlsym(handle, "gtk_file_chooser_dialog_new")) &&
                    (gtk_dialog_run = dlsym(handle, "gtk_dialog_run")) &&
                    (gtk_file_chooser_set_create_folders = dlsym(handle, "gtk_file_chooser_set_create_folders")) &&
                    (gtk_file_chooser_set_do_overwrite_confirmation = dlsym(handle, "gtk_file_chooser_set_do_overwrite_confirmation")) &&
                    (gtk_file_chooser_set_current_name = dlsym(handle, "gtk_file_chooser_set_current_name")) &&
                    (gtk_file_chooser_get_filename = dlsym(handle, "gtk_file_chooser_get_filename")) &&
                    (gtk_widget_destroy = dlsym(handle, "gtk_widget_destroy")) ) {
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
                    (*gtk_init)(NULL, NULL);
                    chooser = (*gtk_file_chooser_dialog_new)(NULL, NULL, 0, "gtk-cancel", -6, "gtk-open", -3, NULL);
                    if((*gtk_dialog_run)(chooser) == -3) {
                        tmp2 = (*gtk_file_chooser_get_filename)(chooser);
                        if(tmp2) strncpy(tmp1 + 1, tmp2, PATH_MAX - 2);
                    }
                    (*gtk_widget_destroy)(chooser);
                } else fprintf(stderr, "meg4: unable to load GTK symbols\n");
                dlclose(handle);
            } else fprintf(stderr, "meg4: unable to run-time link the GTK3 library\n");
            tmp1[0] = 1;
            /* this will actually hang if gtk_init was called... */
            exit(0);
        }
        if(pid < 0) fprintf(stderr, "meg4: unable to fork\n");
        else {
            /* waitpid(pid, &ret, 0); */
            while(!tmp1[0] && !kill(pid, SIGCONT)) main_delay(100);
            kill(pid, SIGKILL);
        }
    }
    meg4_hidecursor();
    if(tmp1[1]) {
        fn = (char*)malloc(strlen(tmp1 + 1) + 1);
        if(fn) strcpy(fn, tmp1 + 1);
    }
    munmap(tmp1, PATH_MAX);
#endif
#endif
    if(fn) {
        if((buf = main_readfile(fn, &len))) {
            n = strrchr(fn, SEP[0]); if(!n) n = fn; else n++;
            meg4_insert(fn, buf, len);
            free(buf);
        }
        free(fn);
    }
#endif
#endif
}

/**
 * Open save file modal and write buffer to selected file
 */
int main_savefile(const char *name, uint8_t *buf, int len)
{
#ifdef __EMSCRIPTEN__
    if(name && buf && len > 0) {
        EM_ASM({ meg4_savefile($0, $1, $2, $3); }, name, strlen(name), buf, len);
        return 1;
    }
    return 0;
#else
    char path[PATH_MAX + FILENAME_MAX + 2];
    int ret = 0;
#ifndef EMBED
    char *fn = NULL;
    int i;
#ifdef __ANDROID__
    /* TODO */
#else
#ifdef _MACOS_
    const char *utf8Path;
    NSURL *url;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
#else
    FILE *p = NULL;
    pid_t pid;
    void *chooser;
    void *handle;
    char *tmp1, *tmp2;
#endif
#endif
#endif
    if(!name || !*name || !buf || len < 1) return 0;
    if(main_floppydir && *main_floppydir) {
        strcpy(path, main_floppydir);
        if(path[strlen(path) - 1] == SEP[0]) path[strlen(path) - 1] = 0;
        mkdir(path, 0755);
        strcat(path, SEP);
        strcat(path, name);
        return main_writefile(path, buf, len);
    }
#ifndef EMBED
    else {
#ifdef __ANDROID__
    /* TODO */
#else
#ifdef _MACOS_
        NSSavePanel *dialog = [NSSavePanel savePanel];
        [dialog setExtensionHidden:NO];

        if([dialog runModal] == NSModalResponseOK) {
            url = [dialog URL];
            utf8Path = [[url path] UTF8String];
            i = strlen(utf8Path);
            fn = (char*)malloc(i + 1);
            if(fn) strcpy(ret, utf8Path);
        }
        [pool release];
        [keyWindow makeKeyAndOrderFront:nil];
#else
        tmp1 = mmap(NULL, PATH_MAX, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if(!tmp1) return 0;
        memset(tmp1, 0, PATH_MAX);
        meg4_showcursor();
        if(zenity) {
            strcpy(tmp1, "/usr/bin/zenity --file-selection --save --filename=\""); tmp2 = tmp1 + strlen(tmp1);
            for(i = 0; name && name[i]; i++)
                if(name[i] >= ' ' && name[i] != '\"') *tmp2++ = name[i];
            *tmp2 = '\"';
            p = popen(tmp1, "r");
            memset(tmp1, 0, PATH_MAX);
        }
        if(p) {
            main_log(1, "using zenity hack");
            if(!fgets(tmp1 + 1, PATH_MAX - 1, p)) main_log(1, "fgets error");
            pclose(p);
            for(tmp2 = tmp1 + 1; *tmp2 && *tmp2 != '\n' && *tmp2 != '\r'; tmp2++);
            *tmp2 = 0;
            tmp1[0] = 1;
        } else {
            main_log(1, "trying to dlopen gtk");
            if(!(pid = fork())) {
                handle = dlopen("libgtk-3.so", RTLD_LAZY | RTLD_LOCAL);
                if(handle) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
                    if( (gtk_init = dlsym(handle, "gtk_init")) &&
                        (gtk_file_chooser_dialog_new = dlsym(handle, "gtk_file_chooser_dialog_new")) &&
                        (gtk_dialog_run = dlsym(handle, "gtk_dialog_run")) &&
                        (gtk_file_chooser_set_create_folders = dlsym(handle, "gtk_file_chooser_set_create_folders")) &&
                        (gtk_file_chooser_set_do_overwrite_confirmation = dlsym(handle, "gtk_file_chooser_set_do_overwrite_confirmation")) &&
                        (gtk_file_chooser_set_current_name = dlsym(handle, "gtk_file_chooser_set_current_name")) &&
                        (gtk_file_chooser_get_filename = dlsym(handle, "gtk_file_chooser_get_filename")) &&
                        (gtk_widget_destroy = dlsym(handle, "gtk_widget_destroy")) ) {
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
                        (*gtk_init)(NULL, NULL);
                        chooser = (*gtk_file_chooser_dialog_new)(NULL, NULL, 1, "gtk-cancel", -6, "gtk-save", -3, NULL);
                        (*gtk_file_chooser_set_create_folders)(chooser, 1);
                        (*gtk_file_chooser_set_do_overwrite_confirmation)(chooser, 1);
                        (*gtk_file_chooser_set_current_name)(chooser, name);
                        if((*gtk_dialog_run)(chooser) == -3) {
                            tmp2 = (*gtk_file_chooser_get_filename)(chooser);
                            if(tmp2) strncpy(tmp1 + 1, tmp2, PATH_MAX - 2);
                        }
                        (*gtk_widget_destroy)(chooser);
                    } else fprintf(stderr, "meg4: unable to load GTK symbols\n");
                    dlclose(handle);
                } else fprintf(stderr, "meg4: unable to run-time link the GTK3 library\n");
                tmp1[0] = 1;
                /* this will actually hang if gtk_init was called... */
                exit(0);
            }
            if(pid < 0) fprintf(stderr, "meg4: unable to fork\n");
            else {
                /* waitpid(pid, &ret, 0); */
                while(!tmp1[0] && !kill(pid, SIGCONT)) main_delay(100);
                kill(pid, SIGKILL);
            }
        }
        meg4_hidecursor();
        if(tmp1[1]) {
            fn = (char*)malloc(strlen(tmp1 + 1) + 1);
            if(fn) strcpy(fn, tmp1 + 1);
        }
        munmap(tmp1, PATH_MAX);
#endif
#endif
        if(fn) { ret = main_writefile(fn, buf, len); free(fn); }
    }
#endif
    return ret;
#endif
}

/**
 * Return floppy list
 */
char **main_getfloppies(void)
{
#ifndef __EMSCRIPTEN__
    DIR *dir;
    struct dirent *ent;
    char fn[PATH_MAX + FILENAME_MAX + 1], *ext, **ret = NULL;
    int n = 0, j, l;

    if(!main_floppydir || !*main_floppydir) return NULL;
    ret = (char**)malloc(sizeof(char*));
    if(!ret) return NULL; else ret[0] = NULL;
    strcpy(fn, main_floppydir); l = strlen(fn);
    if(l > 0 && fn[l - 1] == SEP[0]) fn[--l] = 0;
    if((dir = opendir(fn)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if(ent->d_name[0] == '.' || !(ext = strrchr(ent->d_name, '.')) || strcasecmp(ext, ".png")) continue;
            fn[l] = SEP[0]; strncpy(fn + l + 1, ent->d_name, FILENAME_MAX);
            j = strlen(fn);
            if(!j) continue;
            ret = (char**)realloc(ret, (n + 2) * sizeof(char*));
            if(!ret) break;
            ret[n + 1] = NULL;
            ret[n] = (char*)malloc(j + 1);
            if(!ret[n]) continue;
            strcpy(ret[n], fn);
            n++;
        }
        closedir(dir);
    }
    return ret;
#else
    return NULL;
#endif
}

#ifndef __EMSCRIPTEN__
/**
 * Save a config file
 */
int main_cfgsave(char *cfg, uint8_t *buf, int len)
{
    char file[PATH_MAX + FILENAME_MAX + 1], *tmp = getenv("HOME");
    int ret = 0, i;
    if(tmp) {
        strcpy(file, tmp);
        strcat(file, "/.config"); mkdir(file, 0755);
        strcat(file, "/MEG-4"); mkdir(file, 0700);
        strcat(file, "/"); i = strlen(file); strcat(file, cfg);
        for(; file[i]; i++) if(file[i] == '/') { file[i] = 0; mkdir(file, 0755); file[i] = '/'; }
        ret = main_writefile(file, buf, len);
    }
    return ret;
}

/**
 * Load a config file
 */
uint8_t *main_cfgload(char *cfg, int *len)
{
    uint8_t *ret = NULL;
    char file[PATH_MAX + FILENAME_MAX + 1], *tmp = getenv("HOME");
    if(tmp) {
        strcpy(file, tmp);
        strcat(file, "/.config/MEG-4/"); strcat(file, cfg);
        ret = main_readfile(file, len);
    }
    return ret;
}
#endif

#endif

#ifdef __EMSCRIPTEN__
/**
 * Save a config file
 */
int main_cfgsave(char *cfg, uint8_t *buf, int len)
{
    EM_ASM({
        var name = new TextDecoder("utf-8").decode(new Uint8Array(Module.HEAPU8.buffer,$0,$1));
        var data = new TextDecoder("utf-8").decode(new Uint8Array(Module.HEAPU8.buffer,$2,$3));
        localStorage.setItem("MEG-4/"+name, data);
    }, cfg, strlen(cfg), buf, len);
    return 1;
}

/**
 * Load a config file
 */
uint8_t *main_cfgload(char *cfg, int *len)
{
    uint8_t *buf = EM_ASM_PTR({
        var name = new TextDecoder("utf-8").decode(new Uint8Array(Module.HEAPU8.buffer,$0,$1));
        var data = localStorage.getItem("MEG-4/"+name);
        if(data != undefined) {
            const buf = Module._malloc(data.length + 1);
            Module.HEAPU8.set(new TextEncoder("utf-8").encode(data + String.fromCharCode(0)), buf);
            return buf;
        }
        return 0;
    }, cfg, strlen(cfg));
    if(buf) *len = strlen((const char*)buf);
    return buf;
}
#endif

/**
 * Log messages
 */
void main_log(int lvl, const char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    if(verbose >= lvl) { printf("meg4: "); vprintf(fmt, args); printf("\r\n"); }
    __builtin_va_end(args);
}

/**
 * Parse the command line
 */
void main_parsecommandline(int argc, char **argv, char **lng, char **infile)
{
    int i, j;

    *infile = NULL;
    for(i = 1; i < argc && argv && argv[i]; i++) {
        if(!memcmp(argv[i], "--help", 6) || !strcmp(argv[i], CLIFLAG "h") || !strcmp(argv[i], CLIFLAG "?")) goto usage;
        if(argv[i][0] == CLIFLAG[0]) {
            for(j = 1; j < 16 && argv[i][j]; j++)
                switch(argv[i][j]) {
                    case 'L': if(j == 1 && argv[i + 1]) { *lng = argv[++i]; j = 16; } else goto usage; break;
                    case 'd': if(j == 1 && argv[i + 1]) { main_floppydir = argv[++i]; j = 16; } else goto usage; break;
                    case 'v': verbose++; break;
#ifndef __WIN32__
                    case 'z': zenity++; break;
#endif
                    case 'n': nearest++; break;
                    default:
usage:                  main_hdr();
                        printf("  meg4 [" CLIFLAG "L <xx>] "
#ifndef __WIN32__
                            "[" CLIFLAG "z] "
#endif
                            "[" CLIFLAG "n] [" CLIFLAG "v|" CLIFLAG "vv|" CLIFLAG "vvv] "
#ifndef EMBED
                            "["
#endif
                            CLIFLAG "d <dir>"
#ifndef EMBED
                            "]"
#endif
                            " [floppy]\r\n\r\n");
                        exit(0);
                    break;
                }
        } else
        if(!*infile) *infile = argv[i];
    }
#ifdef EMBED
    if(!main_floppydir) goto usage;
#endif
    if(!*lng) *lng = "en";
}
