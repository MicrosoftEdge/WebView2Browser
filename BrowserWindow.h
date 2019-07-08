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
    static const int c_optionsDropdownHeight = 108;
    static const int c_optionsDropdownWidth = 200;

    static ATOM RegisterClass(_In_ HINSTANCE hInstance);
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static BOOL LaunchWindow(_In_ HINSTANCE hInstance, _In_ int nCmdShow);
    static std::wstring GetAppDataDirectory();
    void HandleTabURIUpdate(size_t tabId, IWebView2WebView* webview);
    void HandleTabNavStarting(size_t tabId, IWebView2WebView* webview);
    void HandleTabNavCompleted(size_t tabId, IWebView2WebView* webview);
    void HandleTabCreated(size_t tabId, bool shouldBeActive);
    void CheckFailure(HRESULT hr);
protected:
    HINSTANCE m_hInst = nullptr;                     // current app instance
    HWND m_hWnd = nullptr;
    static size_t s_windowInstanceCount;

    static WCHAR s_windowClass[MAX_LOADSTRING];    // the window class name
    static WCHAR s_title[MAX_LOADSTRING];          // The title bar text

    int m_minWindowWidth = 0;
    int m_minWindowHeight = 0;

    Microsoft::WRL::ComPtr<IWebView2Environment> m_uiEnv;
    Microsoft::WRL::ComPtr<IWebView2Environment> m_contentEnv;
    Microsoft::WRL::ComPtr<IWebView2WebView> m_controlsWebView;
    Microsoft::WRL::ComPtr<IWebView2WebView> m_optionsWebView;
    std::map<size_t,std::unique_ptr<Tab>> m_tabs;
    size_t m_activeTabId = 0;

    EventRegistrationToken m_controlsUIMessageBrokerToken = {};  // token for the UI message handler in controls WebView
    EventRegistrationToken m_optionsUIMessageBrokerToken = {};  // token for the UI message handler in options WebView
    EventRegistrationToken m_lostOptionsFocus = {};  // token for the lost focus handler in options WebView
    Microsoft::WRL::ComPtr<IWebView2WebMessageReceivedEventHandler> m_uiMessageBroker;

    std::wstring GetFullPathFor(LPCWSTR relativePath);
    BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
    void InitUIWebViews();
    void CreateBrowserControlsWebView();
    void CreateBrowserOptionsWebView();

    void SetUIMessageBroker();
    void ResizeUIWebViews();
    void UpdateMinWindowSize();
    void SwitchToTab(size_t tabId);
    int GetDPIAwareBound(int bound);
};
