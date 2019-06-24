// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"
#include "Tab.h"

using namespace Microsoft::WRL;

std::unique_ptr<Tab> Tab::CreateNewTab(HWND hWnd, IWebView2Environment* env)
{
    OutputDebugString(L"Should launch new tab");
    std::unique_ptr<Tab> tab = std::make_unique<Tab>();

    tab->m_parentHWnd = hWnd;
    tab->Init(env);

    return tab;
}

void Tab::Init(IWebView2Environment* env)
{
    THROW_IF_FAILED(env->CreateWebView(m_parentHWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
        [this](HRESULT result, IWebView2WebView* webview) -> HRESULT {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Tab WebView creation failed\n");
            return result;
        }
        m_contentWebview = webview;

        // Register event handler for doc state change
        THROW_IF_FAILED(m_contentWebview->add_DocumentStateChanged(Callback<IWebView2DocumentStateChangedEventHandler>(
            [this](IWebView2WebView* webview, IWebView2DocumentStateChangedEventArgs* args) -> HRESULT
        {
            BrowserWindow* browser_window = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
            browser_window->HandleTabURIUpdate(webview);

            return S_OK;
        }).Get(), &m_uriUpdateForwarderToken));
        ResizeWebView();

        THROW_IF_FAILED(m_contentWebview->Navigate(L"https://www.bing.com"));

        return S_OK;
    }).Get()));
}

void Tab::ResizeWebView()
{
    RECT bounds;
    GetClientRect(m_parentHWnd, &bounds);

    // TO DO: remove hacky offset once put_Bounds is fixed
    bounds.top += (BrowserWindow::c_uiBarHeight * GetDpiForWindow(m_parentHWnd) / DEFAULT_DPI) / 2;
    bounds.bottom -= bounds.top;
    THROW_IF_FAILED(m_contentWebview->put_Bounds(bounds));
}
