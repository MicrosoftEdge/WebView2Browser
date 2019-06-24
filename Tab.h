// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "framework.h"

class Tab
{
public:
    Microsoft::WRL::ComPtr<IWebView2WebView> m_contentWebview;

    static std::unique_ptr<Tab> CreateNewTab(HWND hWnd, IWebView2Environment* env);
    void ResizeWebView();
protected:
    HWND m_parentHWnd = nullptr;
    EventRegistrationToken m_uriUpdateForwarderToken = {};

    void Init(IWebView2Environment* env);
};
