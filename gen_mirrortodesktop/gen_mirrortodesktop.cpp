#include <Windows.h>
#include "Winamp/wa_ipc.h"
#include "Winamp/GEN.H"
#include <thread>

#define MIRROR_FPS 30

//////////////////////////
// FORWARD DECLARATIONS //
//////////////////////////

// These are callback functions/events which will be called by Winamp
int  init(void);
void config(void);
void quit(void);

void mainThreadFunc(void);

////////////
// PLUGIN //
////////////

// This structure contains plugin information
// GPPHDR_VER is the version of the winampGeneralPurposePlugin (GPP) structure
winampGeneralPurposePlugin plugin = {
  GPPHDR_VER,  // version of the plugin, defined in "GEN.H"
  const_cast<char*>("Mirror Vis to Desktop"), // name/title of the plugin,
  init,        // function name which will be executed on init event
  config,      // function name which will be executed on config event
  quit,        // function name which will be executed on quit event
  0,           // handle to Winamp main window, loaded by winamp when this dll is loaded
  0            // hinstance to this dll, loaded by winamp when this dll is loaded
};

int init() {
    std::thread mainThread(mainThreadFunc);
    mainThread.detach();
    return 0;
}

void config() {
    // Nope.
}

void quit() {
    // Nothing to do.
}

// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
    return &plugin;
}

///////////////
// MIRRORING //
///////////////

// https://stackoverflow.com/a/56132585
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    HWND p = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", NULL);
    HWND* ret = (HWND*)lParam;

    if (p)
    {
        // Gets the WorkerW Window after the current one.
        *ret = FindWindowEx(NULL, hwnd, L"WorkerW", NULL);
    }
    return true;
}

// https://stackoverflow.com/a/56132585
HWND get_wallpaper_window()
{
    // Fetch the Progman window
    HWND progman = FindWindow(L"ProgMan", NULL);
    // Send 0x052C to Progman. This message directs Progman to spawn a 
    // WorkerW behind the desktop icons. If it is already there, nothing 
    // happens.
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);
    // We enumerate all Windows, until we find one, that has the SHELLDLL_DefView 
    // as a child. 
    // If we found that window, we take its next sibling and assign it to workerw.
    HWND wallpaper_hwnd = nullptr;
    EnumWindows(EnumWindowsProc, (LPARAM)&wallpaper_hwnd);
    // Return the handle you're looking for.
    return wallpaper_hwnd;
}

void mainThreadFunc() {
    // Getting wallpaper HDC
    HWND hwnd_Desktop = get_wallpaper_window();
    HDC hDC_Desktop = GetDC(hwnd_Desktop);

    // Clearing wallpaper
    RECT desktopRect;
    GetWindowRect(hwnd_Desktop, &desktopRect);
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));

    int cx_Desktop = desktopRect.right - desktopRect.left;
    int cy_Desktop = desktopRect.bottom - desktopRect.top;

    while (TRUE)
    {
        FillRect(hDC_Desktop, &desktopRect, blackBrush);

        while (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISVISRUNNING) == 0)
        {
            Sleep(1000);
        }
        HWND copyWnd = (HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVISWND);

        // https://stackoverflow.com/a/14407301
        HDC TargetDC = GetDC(copyWnd);
        RECT rect;

        while (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISVISRUNNING) == 1)
        {
            GetWindowRect(copyWnd, &rect);

            int cx = rect.right - rect.left;
            int cy = rect.bottom - rect.top;
            int x = (cx_Desktop - cx) / 2;
            int y = (cy_Desktop - cy) / 2;

            FillRect(hDC_Desktop, &desktopRect, blackBrush);
            
            // https://stackoverflow.com/a/14407301
            BitBlt(hDC_Desktop, x, y, cx, cy, TargetDC, 0, 0, SRCCOPY);
            Sleep(1000 / MIRROR_FPS);
        }
    }
}
