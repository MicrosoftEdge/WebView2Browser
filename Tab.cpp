// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"
#include "Tab.h"

using namespace Microsoft::WRL;

std::unique_ptr<Tab> Tab::CreateNewTab(HWND hWnd, IWebView2Environment* env, size_t id, bool shouldBeActive)
{
    std::unique_ptr<Tab> tab = std::make_unique<Tab>();

    tab->m_parentHWnd = hWnd;
    tab->m_tabId = id;
    tab->SetMessageBroker();
    tab->Init(env, shouldBeActive);

    return tab;
}

void Tab::Init(IWebView2Environment* env, bool shouldBeActive)
{
    THROW_IF_FAILED(env->CreateWebView(m_parentHWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
        [this, shouldBeActive](HRESULT result, IWebView2WebView* webview) -> HRESULT {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Tab WebView creation failed\n");
            return result;
        }
        m_contentWebview = webview;
        BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
        THROW_IF_FAILED(m_contentWebview->add_WebMessageReceived(m_messageBroker.Get(), &m_messageBrokerToken));

        // Register event handler for doc state change
        THROW_IF_FAILED(m_contentWebview->add_DocumentStateChanged(Callback<IWebView2DocumentStateChangedEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2DocumentStateChangedEventArgs* args) -> HRESULT
        {
            browserWindow->HandleTabURIUpdate(m_tabId, webview);

            return S_OK;
        }).Get(), &m_uriUpdateForwarderToken));

        THROW_IF_FAILED(m_contentWebview->add_NavigationStarting(Callback<IWebView2NavigationStartingEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2NavigationStartingEventArgs* args) -> HRESULT
        {
            browserWindow->HandleTabNavStarting(m_tabId, webview);

            return S_OK;
        }).Get(), &m_navStartingToken));

        THROW_IF_FAILED(m_contentWebview->add_NavigationCompleted(Callback<IWebView2NavigationCompletedEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2NavigationCompletedEventArgs* args) -> HRESULT
        {
            browserWindow->HandleTabNavCompleted(m_tabId, webview, args);
            return S_OK;
        }).Get(), &m_navCompletedToken));

        // Enable listening for security events to update secure icon
        THROW_IF_FAILED(m_contentWebview->CallDevToolsProtocolMethod(L"Security.enable", L"{}", nullptr));

        // Forward security status updates to browser
        THROW_IF_FAILED(m_contentWebview->add_DevToolsProtocolEventReceived(L"Security.securityStateChanged", Callback<IWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
        {
            browserWindow->HandleTabSecurityUpdate(m_tabId, webview, args);
            return S_OK;
        }).Get(), &m_securityUpdateToken));

        THROW_IF_FAILED(m_contentWebview->Navigate(L"https://www.bing.com"));
        browserWindow->HandleTabCreated(m_tabId, shouldBeActive);

        return S_OK;
    }).Get()));
}

void Tab::SetMessageBroker()
{
    m_messageBroker = Callback<IWebView2WebMessageReceivedEventHandler>(
        [this](IWebView2WebView* webview, IWebView2WebMessageReceivedEventArgs* eventArgs) -> HRESULT
    {
        BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
        browserWindow->HandleTabMessageReceived(m_tabId, webview, eventArgs);

        return S_OK;
    });
}

void Tab::ResizeWebView()
{
    RECT bounds;
    GetClientRect(m_parentHWnd, &bounds);

    // TO DO: remove hacky offset once put_Bounds is fixed
    BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
    bounds.top += browserWindow->GetDPIAwareBound(BrowserWindow::c_uiBarHeight) / 2;
    bounds.bottom -= bounds.top;

    THROW_IF_FAILED(m_contentWebview->put_Bounds(bounds));
}
