// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "framework.h"
#include "Tab.h"

class BrowserWindow
{
public:
    static const int c_uiBarHeight = 70;

    static ATOM RegisterClass(_In_ HINSTANCE hInstance);
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static BOOL LaunchWindow(_In_ HINSTANCE hInstance, _In_ int nCmdShow);
    void HandleTabURIUpdate(IWebView2WebView* webview);
    void HandleTabNavStarting(IWebView2WebView* webview);
    void HandleTabNavCompleted(IWebView2WebView* webview);
    void CheckFailure(HRESULT hr);
protected:
    HINSTANCE m_hInst = nullptr;                     // current app instance

    static WCHAR s_windowClass[MAX_LOADSTRING];    // the window class name
    static WCHAR s_title[MAX_LOADSTRING];          // The title bar text

    int m_minWindowWidth = 0;
    int m_minWindowHeight = 0;

    Microsoft::WRL::ComPtr<IWebView2Environment> m_uiEnv;
    Microsoft::WRL::ComPtr<IWebView2WebView> m_uiWebview;
    std::unique_ptr<Tab> m_activeTab;

    EventRegistrationToken m_uiMessageBrokerToken = {};  // token for the registered UI message handler of this window
    Microsoft::WRL::ComPtr<IWebView2WebMessageReceivedEventHandler> m_uiMessageBroker;

    BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
    void InitUIWebView(_In_ HWND hWnd, _In_ HINSTANCE hInst);

    void SetUIMessageBroker();
    void ResizeUIWebView(HWND hWnd);
    void UpdateMinWindowSize(HWND hWnd);
};
