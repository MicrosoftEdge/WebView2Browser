// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WebViewBrowserApp.cpp : Defines the entry point for the application.
//

#include "BrowserWindow.h"
#include "WebViewBrowserApp.h"

using namespace Microsoft::WRL;

void tryLaunchWindow(HINSTANCE hInstance, int nCmdShow);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Call SetProcessDPIAware() instead when using Windows 7 or any version
    // below 1703 (Windows 10).
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    BrowserWindow::RegisterClass(hInstance);

    tryLaunchWindow(hInstance, nCmdShow);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WEBVIEWBROWSERAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

void tryLaunchWindow(HINSTANCE hInstance, int nCmdShow)
{
    BOOL launched = BrowserWindow::LaunchWindow(hInstance, nCmdShow);
    if (!launched)
    {
        int msgboxID = MessageBox(NULL, L"Could not launch the browser", L"Error", MB_RETRYCANCEL);

        switch (msgboxID)
        {
        case IDRETRY:
            tryLaunchWindow(hInstance, nCmdShow);
            break;
        case IDCANCEL:
        default:
            PostQuitMessage(0);
        }
    }
}
