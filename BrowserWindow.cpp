// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"
#include "shlobj.h"

using namespace Microsoft::WRL;

WCHAR BrowserWindow::s_windowClass[] = { 0 };
WCHAR BrowserWindow::s_title[] = { 0 };
size_t BrowserWindow::s_windowInstanceCount = 0;

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
    // Get the ptr to the BrowserWindow instance who created this hWnd.
    // The pointer was set when the hWnd was created during InitInstance.
    BrowserWindow* browser_window = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (browser_window != nullptr)
    {
        return browser_window->WndProc(hWnd, message, wParam, lParam); // forward message to instance-aware WndProc
    }
    else
    {
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
    }
    case WM_SIZE:
    {
        if (m_uiWebview != nullptr)
        {
            ResizeUIWebView(hWnd);
        }
        if (m_tabs.find(m_activeTabId) != m_tabs.end())
        {
            m_tabs.at(m_activeTabId)->ResizeWebView();
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
    case WM_NCDESTROY:
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        --s_windowInstanceCount;
        delete this;
        if (s_windowInstanceCount == 0)
        {
            PostQuitMessage(0);
        }
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
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
    // BrowserWindow keeps a reference to itself in its host window and will
    // delete itself when the window is destroyed.
    BrowserWindow* window = new BrowserWindow();
    if (!window->InitInstance(hInstance, nCmdShow))
    {
        delete window;
        return FALSE;
    }
    ++s_windowInstanceCount;
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

    m_hWnd = CreateWindowW(s_windowClass, s_title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_hInst, nullptr);

    if (!m_hWnd)
    {
        return FALSE;
    }

    // Make the BrowserWindow instance ptr available through the hWnd
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // TO DO: remove the menu from resources
    SetMenu(m_hWnd, nullptr);
    UpdateMinWindowSize(m_hWnd);
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Get directory for user data. This will be kept separated from the
    // directory for the browser UI data.
    std::wstring userDataDirectory = GetAppDataDirectory();
    userDataDirectory.append(L"\\User Data");

    // Create WebView environment for web content requested by the user. All
    // tabs will be created from this environment and kept isolated from the
    // browser UI. This enviroment is created first so the UI can request new
    // tabs when it's ready.
    HRESULT hr = CreateWebView2EnvironmentWithDetails(nullptr, userDataDirectory.c_str(), WEBVIEW2_RELEASE_CHANNEL_PREFERENCE_STABLE,
        L"", Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, IWebView2Environment* env) -> HRESULT
    {
        RETURN_IF_FAILED(result);

        m_contentEnv = env;
        InitUIWebView();
        return S_OK;
    }).Get());

    if (!SUCCEEDED(hr))
    {
        return FALSE;
    }

    return TRUE;
}

void BrowserWindow::InitUIWebView()
{
    // Get data directory for browser UI data
    std::wstring browserDataDirectory = GetAppDataDirectory();
    browserDataDirectory.append(L"\\Browser Data");

    // Create WebView environment for browser UI. A separate data directory is
    // used to isolate the browser UI from web content requested by the user.
    THROW_IF_FAILED(CreateWebView2EnvironmentWithDetails(nullptr, browserDataDirectory.c_str(), WEBVIEW2_RELEASE_CHANNEL_PREFERENCE_STABLE,
        L"", Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, IWebView2Environment* env) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"WebView environment creation failed\n");
            return result;
        }
        // Environment is ready, create the WebView
        m_uiEnv = env;

        THROW_IF_FAILED(env->CreateWebView(m_hWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
            [this](HRESULT result, IWebView2WebView* webview) -> HRESULT
        {
            if (!SUCCEEDED(result))
            {
                OutputDebugString(L"UI WebView creation failed\n");
                return result;
            }
            // WebView created
            m_uiWebview = webview;
            RETURN_IF_FAILED(m_uiWebview->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_uiMessageBrokerToken));
            ResizeUIWebView(m_hWnd);

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
        case MG_CREATE_TAB:
        {
            size_t id = args.at(L"tabId").as_number().to_uint32();
            bool shouldBeActive = args.at(L"active").as_bool();
            std::unique_ptr<Tab> newTab = Tab::CreateNewTab(m_hWnd, m_contentEnv.Get(), id, shouldBeActive);
            m_tabs.insert(std::pair<size_t,std::unique_ptr<Tab>>(id, std::move(newTab)));
        }
        break;
        case MG_NAVIGATE:
        {
            std::wstring uri(args.at(L"uri").as_string());
            if (!SUCCEEDED(m_tabs.at(m_activeTabId)->m_contentWebview->Navigate(uri.c_str())))
            {
                THROW_IF_FAILED(m_tabs.at(m_activeTabId)->m_contentWebview->Navigate(args.at(L"encodedSearchURI").as_string().c_str()));
            }
        }
        break;
        case MG_GO_FORWARD:
        {
            THROW_IF_FAILED(m_tabs.at(m_activeTabId)->m_contentWebview->GoForward());
        }
        break;
        case MG_GO_BACK:
        {
            THROW_IF_FAILED(m_tabs.at(m_activeTabId)->m_contentWebview->GoBack());
        }
        break;
        case MG_RELOAD:
        {
            THROW_IF_FAILED(m_tabs.at(m_activeTabId)->m_contentWebview->Reload());
        }
        break;
        case MG_CANCEL:
        {
            THROW_IF_FAILED(m_tabs.at(m_activeTabId)->m_contentWebview->CallDevToolsProtocolMethod(L"Page.stopLoading", L"{}", nullptr));
        }
        break;
        case MG_SWITCH_TAB:
        {
            size_t tabId = args.at(L"tabId").as_number().to_uint32();

            SwitchToTab(tabId);
        }
        break;
        case MG_CLOSE_TAB:
        {
            m_tabs.erase(args.at(L"tabId").as_number().to_uint32());
        }
        break;
        case MG_CLOSE_WINDOW:
        {
            PostMessage(m_hWnd, WM_CLOSE, 0, 0);
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

void BrowserWindow::SwitchToTab(size_t tabId)
{
    size_t previousActiveTab = m_activeTabId;

    m_activeTabId = tabId;
    m_tabs.at(tabId)->ResizeWebView();
    THROW_IF_FAILED(m_tabs.at(tabId)->m_contentWebview->put_IsVisible(TRUE));

    if (previousActiveTab != INVALID_TAB_ID)
    {
        THROW_IF_FAILED(m_tabs.at(previousActiveTab)->m_contentWebview->put_IsVisible(FALSE));
    }
}

void BrowserWindow::HandleTabURIUpdate(size_t tabId, IWebView2WebView* webview)
{
    wil::unique_cotaskmem_string uri;
    THROW_IF_FAILED(webview->get_Source(&uri));

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
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

void BrowserWindow::HandleTabNavStarting(size_t tabId, IWebView2WebView* webview)
{
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_STARTING);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    THROW_IF_FAILED(m_uiWebview->PostWebMessageAsJson(stream.str().c_str()));
}

void BrowserWindow::HandleTabNavCompleted(size_t tabId, IWebView2WebView* webview)
{
    std::wstring getTitleScript(
        // Look for a title tag
        L"(() => {"
        L"    const titleTag = document.getElementsByTagName('title')[0];"
        L"    if (titleTag) {"
        L"        return titleTag.innerHTML;"
        L"    }"
        // No title tag, look for the file name
        L"    pathname = window.location.pathname;"
        L"    var filename = pathname.split('/').pop();"
        L"    if (filename) {"
        L"        return filename;"
        L"    }"
        // No file name, look for the hostname
        L"    const hostname =  window.location.hostname;"
        L"    if (hostname) {"
        L"        return hostname;"
        L"    }"
        // Fallback: let the UI use a generic title
        L"    return '';"
        L"})();"
    );

    THROW_IF_FAILED(webview->ExecuteScript(getTitleScript.c_str(), Callback<IWebView2ExecuteScriptCompletedHandler>(
        [this, webview, tabId](HRESULT error, PCWSTR result) -> HRESULT
    {
        RETURN_IF_FAILED(error);

        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_UPDATE_TAB);
        jsonObj[L"args"] = web::json::value::parse(L"{}");
        jsonObj[L"args"][L"title"] = web::json::value::parse(result);
        jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

        utility::stringstream_t stream;
        jsonObj.serialize(stream);

        THROW_IF_FAILED(m_uiWebview->PostWebMessageAsJson(stream.str().c_str()));
        return S_OK;
    }).Get()));

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_COMPLETED);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    THROW_IF_FAILED(m_uiWebview->PostWebMessageAsJson(stream.str().c_str()));
}

void BrowserWindow::HandleTabCreated(size_t tabId, bool shouldBeActive)
{
    if (shouldBeActive)
    {
        SwitchToTab(tabId);
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

std::wstring BrowserWindow::GetAppDataDirectory()
{
    TCHAR path[MAX_PATH];
    std::wstring dataDirectory;
    HRESULT hr = SHGetFolderPath(nullptr, CSIDL_APPDATA, NULL, 0, path);
    if (SUCCEEDED(hr))
    {
        dataDirectory = std::wstring(path);
        dataDirectory.append(L"\\Microsoft\\");
    }
    else
    {
        dataDirectory = std::wstring(L".\\");
    }

    dataDirectory.append(s_title);
    return dataDirectory;
}
