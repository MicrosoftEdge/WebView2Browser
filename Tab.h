// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "framework.h"

class Tab
{
public:
    Microsoft::WRL::ComPtr<IWebView2WebView> m_contentWebview;

    static std::unique_ptr<Tab> CreateNewTab(HWND hWnd, IWebView2Environment* env, size_t id, bool shouldBeActive);
    void ResizeWebView();
protected:
    HWND m_parentHWnd = nullptr;
    size_t m_tabId = INVALID_TAB_ID;
    EventRegistrationToken m_uriUpdateForwarderToken = {};
    EventRegistrationToken m_navStartingToken = {};
    EventRegistrationToken m_navCompletedToken = {};
    EventRegistrationToken m_securityUpdateToken = {};
    EventRegistrationToken m_messageBrokerToken = {};  // Message broker for browser pages loaded in a tab
    Microsoft::WRL::ComPtr<IWebView2WebMessageReceivedEventHandler> m_messageBroker;

    void Init(IWebView2Environment* env, bool shouldBeActive);
    void SetMessageBroker();
};
