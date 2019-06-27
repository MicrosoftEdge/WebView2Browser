// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"

using namespace Microsoft::WRL;

WCHAR BrowserWindow::s_windowClass[] = { 0 };
WCHAR BrowserWindow::s_title[] = { 0 };

//
//  FUNCTION: RegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM BrowserWindow::RegisterClass(_In_ HINSTANCE hInstance)
{
    // Initialize window class string
    LoadStringW(hInstance, IDC_WEBVIEWBROWSERAPP, s_windowClass, MAX_LOADSTRING);
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProcStatic;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBVIEWBROWSERAPP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WEBVIEWBROWSERAPP);
    wcex.lpszClassName = s_windowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//  FUNCTION: WndProcStatic(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Redirect messages to approriate instance or call default proc
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK BrowserWindow::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Get the ptr to the BrowserWindow instance who created this hWnd
    // The pointer was set when the hWnd was created during InitInstance
    BrowserWindow* browser_window = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (browser_window != nullptr)
    {
        return browser_window->WndProc(hWnd, message, wParam, lParam); // forward message to instance-aware WndProc
    }
    else
    {
        OutputDebugString(L"hWnd does not have associated BrowserWindow instance\n");
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for each browser window instance.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK BrowserWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* minmax = reinterpret_cast<MINMAXINFO*>(lParam);
        minmax->ptMinTrackSize.x = m_minWindowWidth;
        minmax->ptMinTrackSize.y = m_minWindowHeight;
    }
    break;
    case WM_DPICHANGED:
    {
        UpdateMinWindowSize(hWnd);
        if (m_uiWebview != nullptr)
        {
            ResizeUIWebView(hWnd);
        }
        if (m_activeTab != nullptr)
        {
            m_activeTab->ResizeWebView();
        }
    }
    break;
    case WM_SIZE:
    {
        if (m_uiWebview != nullptr)
        {
            ResizeUIWebView(hWnd);
        }
        if (m_activeTab != nullptr)
        {
            m_activeTab->ResizeWebView();
        }
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
    {
        // TO DO: close only the interacted browser window
        PostQuitMessage(0);
    }
        break;
    default:
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;
    }
    return 0;
}


BOOL BrowserWindow::LaunchWindow(_In_ HINSTANCE hInstance, _In_ int nCmdShow)
{
    BrowserWindow* window = new BrowserWindow();
    if (!window->InitInstance(hInstance, nCmdShow))
    {
        delete window;
        return FALSE;
    }
    // TO DO: make sure the instance is destroyed properly
    return TRUE;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL BrowserWindow::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    m_hInst = hInstance; // Store app instance handle
    LoadStringW(m_hInst, IDS_APP_TITLE, s_title, MAX_LOADSTRING);

    SetUIMessageBroker();

    HWND hWnd = CreateWindowW(s_windowClass, s_title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_hInst, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    // Make the BrowserWindow instance ptr available through the hWnd
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // TO DO: remove the menu from resources
    SetMenu(hWnd, nullptr);
    UpdateMinWindowSize(hWnd);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    InitUIWebView(hWnd, m_hInst);

    // TO DO: use a separate env for content webviews
    m_activeTab = Tab::CreateNewTab(hWnd, m_uiEnv.Get());

    return TRUE;
}

void BrowserWindow::InitUIWebView(_In_ HWND hWnd, _In_ HINSTANCE hInst)
{
    THROW_IF_FAILED(CreateWebView2EnvironmentWithDetails(L".", nullptr, WEBVIEW2_RELEASE_CHANNEL_PREFERENCE_STABLE,
        L"", Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
        [hWnd, this](HRESULT result, IWebView2Environment* env) -> HRESULT {
            if (!SUCCEEDED(result))
            {
                OutputDebugString(L"WebView environment creation failed\n");
                return result;
            }
            // Environment is ready, create the WebView
            m_uiEnv = env;

            THROW_IF_FAILED(env->CreateWebView(hWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
                [hWnd, this](HRESULT result, IWebView2WebView* webview) -> HRESULT {
                    if (!SUCCEEDED(result))
                    {
                        OutputDebugString(L"UI WebView creation failed\n");
                        return result;
                    }
                    // WebView created
                    m_uiWebview = webview;
                    RETURN_IF_FAILED(m_uiWebview->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_uiMessageBrokerToken));
                    ResizeUIWebView(hWnd);
                    WCHAR path[MAX_PATH];
                    GetModuleFileNameW(m_hInst, path, MAX_PATH);
                    std::wstring pathName(path);

                    std::size_t index = pathName.find_last_of(L"\\") + 1;
                    pathName.replace(index, pathName.length(), L"ui_bar\\bar.html");

                    RETURN_IF_FAILED(m_uiWebview->Navigate(pathName.c_str()));

                return S_OK;
                }).Get()));

            return S_OK;
        }).Get()));
}

// Set the message broker for the UI webview. This will capture messages from ui web content.
// Lambda is used to capture the instance while satisfying Microsoft::WRL::Callback<T>()
void BrowserWindow::SetUIMessageBroker()
{
    m_uiMessageBroker = Callback<IWebView2WebMessageReceivedEventHandler>(
        [this](IWebView2WebView* webview, IWebView2WebMessageReceivedEventArgs* eventArgs) -> HRESULT
    {
        wil::unique_cotaskmem_string jsonString;
        eventArgs->get_WebMessageAsJson(&jsonString); // Get the message from the UI WebView as JSON formatted string
        web::json::value jsonObj = web::json::value::parse(jsonString.get());

        // TO DO: validate the message is correctly formatted
        int message = jsonObj.at(L"message").as_integer();
        web::json::value args = jsonObj.at(L"args");

        switch (message)
        {
        case MG_NAVIGATE:
        {
            std::wstring uri(args.at(L"uri").as_string());
            if (!SUCCEEDED(m_activeTab->m_contentWebview->Navigate(uri.c_str())))
            {
                THROW_IF_FAILED(m_activeTab->m_contentWebview->Navigate(args.at(L"encodedSearchURI").as_string().c_str()));
            }
        }
        break;
        case MG_GO_FORWARD:
        {
            THROW_IF_FAILED(m_activeTab->m_contentWebview->GoForward());
        }
        break;
        case MG_GO_BACK:
        {
            THROW_IF_FAILED(m_activeTab->m_contentWebview->GoBack());
        }
        break;
        case MG_RELOAD:
        {
            THROW_IF_FAILED(m_activeTab->m_contentWebview->Reload());
        }
        break;
        case MG_CANCEL:
        {
            THROW_IF_FAILED(m_activeTab->m_contentWebview->CallDevToolsProtocolMethod(L"Page.stopLoading", L"{}", nullptr));
        }
        break;
        default:
        {
            OutputDebugString(L"Unexpected message\n");
        }
        break;
        }

        return S_OK;
    });
}

void BrowserWindow::HandleTabURIUpdate(IWebView2WebView* webview)
{
    wil::unique_cotaskmem_string uri;
    THROW_IF_FAILED(webview->get_Source(&uri));

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"uri"] = web::json::value(uri.get());

    BOOL canGoForward = FALSE;
    THROW_IF_FAILED(webview->get_CanGoForward(&canGoForward));
    jsonObj[L"args"][L"canGoForward"] = web::json::value::boolean(canGoForward);

    BOOL canGoBack = FALSE;
    THROW_IF_FAILED(webview->get_CanGoBack(&canGoBack));
    jsonObj[L"args"][L"canGoBack"] = web::json::value::boolean(canGoBack);

    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    THROW_IF_FAILED(m_uiWebview->PostWebMessageAsJson(stream.str().c_str()));
}

void BrowserWindow::HandleTabNavStarting(IWebView2WebView* webview)
{
    if (webview == m_activeTab->m_contentWebview.Get())
    {
        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_NAV_STARTING);
        jsonObj[L"args"] = web::json::value::parse(L"{}");

        utility::stringstream_t stream;
        jsonObj.serialize(stream);

        THROW_IF_FAILED(m_uiWebview->PostWebMessageAsJson(stream.str().c_str()));
    }
}

void BrowserWindow::HandleTabNavCompleted(IWebView2WebView* webview)
{
    if (webview == m_activeTab->m_contentWebview.Get())
    {
        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_NAV_COMPLETED);
        jsonObj[L"args"] = web::json::value::parse(L"{}");

        utility::stringstream_t stream;
        jsonObj.serialize(stream);

        THROW_IF_FAILED(m_uiWebview->PostWebMessageAsJson(stream.str().c_str()));
    }
}

void BrowserWindow::ResizeUIWebView(HWND hWnd)
{
    RECT bounds;
    GetClientRect(hWnd, &bounds);
    bounds.bottom = bounds.top + (c_uiBarHeight * GetDpiForWindow(hWnd) / DEFAULT_DPI);
    THROW_IF_FAILED(m_uiWebview->put_Bounds(bounds));
}

void BrowserWindow::UpdateMinWindowSize(HWND hWnd)
{
    // TO DO: figure out why these limits are not being applied properly
    m_minWindowWidth = MIN_WINDOW_WIDTH * GetDpiForWindow(hWnd) / DEFAULT_DPI;
    m_minWindowHeight = MIN_WINDOW_HEIGHT * GetDpiForWindow(hWnd) / DEFAULT_DPI;
}

void BrowserWindow::CheckFailure(HRESULT hr)
{
    if (FAILED(hr))
    {
        WCHAR message[512] = L"";
        StringCchPrintf(message, ARRAYSIZE(message), L"Error: 0x%x", hr);
        MessageBoxW(nullptr, message, nullptr, MB_OK);
    }
}
