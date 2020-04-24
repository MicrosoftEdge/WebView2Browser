// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"
#include "shlobj.h"
#include <Urlmon.h>
#pragma comment (lib, "Urlmon.lib")

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
    // Get the ptr to the BrowserWindow instance who created this hWnd.
    // The pointer was set when the hWnd was created during InitInstance.
    BrowserWindow* browser_window = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (browser_window != nullptr)
    {
        return browser_window->WndProc(hWnd, message, wParam, lParam);  // Forward message to instance-aware WndProc
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
        UpdateMinWindowSize();
    }
    case WM_SIZE:
    {
        ResizeUIWebViews();
        if (m_tabs.find(m_activeTabId) != m_tabs.end())
        {
            m_tabs.at(m_activeTabId)->ResizeWebView();
        }
    }
    break;
    case WM_CLOSE:
    {
        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_CLOSE_WINDOW);
        jsonObj[L"args"] = web::json::value::parse(L"{}");

        CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Try again.");
    }
    break;
    case WM_NCDESTROY:
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        delete this;
        PostQuitMessage(0);
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
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

    UpdateMinWindowSize();
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
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataDirectory.c_str(),
        nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
    {
        RETURN_IF_FAILED(result);

        m_contentEnv = env;
        HRESULT hr = InitUIWebViews();

        if (!SUCCEEDED(hr))
        {
            OutputDebugString(L"UI WebViews environment creation failed\n");
        }

        return hr;
    }).Get());

    if (!SUCCEEDED(hr))
    {
        OutputDebugString(L"Content WebViews environment creation failed\n");
        return FALSE;
    }

    return TRUE;
}

HRESULT BrowserWindow::InitUIWebViews()
{
    // Get data directory for browser UI data
    std::wstring browserDataDirectory = GetAppDataDirectory();
    browserDataDirectory.append(L"\\Browser Data");

    // Create WebView environment for browser UI. A separate data directory is
    // used to isolate the browser UI from web content requested by the user.
    return CreateCoreWebView2EnvironmentWithOptions(nullptr, browserDataDirectory.c_str(),
        nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
    {
        // Environment is ready, create the WebView
        m_uiEnv = env;

        RETURN_IF_FAILED(CreateBrowserControlsWebView());
        RETURN_IF_FAILED(CreateBrowserOptionsWebView());

        return S_OK;
    }).Get());
}

HRESULT BrowserWindow::CreateBrowserControlsWebView()
{
    return m_uiEnv->CreateCoreWebView2Controller(m_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* host) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Controls WebView creation failed\n");
            return result;
        }
        // WebView created
        m_controlsController = host;
        CheckFailure(m_controlsController->get_CoreWebView2(&m_controlsWebView), L"");

        wil::com_ptr<ICoreWebView2Settings> settings;
        RETURN_IF_FAILED(m_controlsWebView->get_Settings(&settings));
        RETURN_IF_FAILED(settings->put_AreDevToolsEnabled(FALSE));

        RETURN_IF_FAILED(m_controlsController->add_ZoomFactorChanged(Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [](ICoreWebView2Controller* host, IUnknown* args) -> HRESULT
        {
            host->put_ZoomFactor(1.0);
            return S_OK;
        }
        ).Get(), &m_controlsZoomToken));

        RETURN_IF_FAILED(m_controlsWebView->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_controlsUIMessageBrokerToken));
        RETURN_IF_FAILED(ResizeUIWebViews());

        std::wstring controlsPath = GetFullPathFor(L"wvbrowser_ui\\controls_ui\\default.html");
        RETURN_IF_FAILED(m_controlsWebView->Navigate(controlsPath.c_str()));

        return S_OK;
    }).Get());
}

HRESULT BrowserWindow::CreateBrowserOptionsWebView()
{
    return m_uiEnv->CreateCoreWebView2Controller(m_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* host) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Options WebView creation failed\n");
            return result;
        }
        // WebView created
        m_optionsController = host;
        CheckFailure(m_optionsController->get_CoreWebView2(&m_optionsWebView), L"");

        wil::com_ptr<ICoreWebView2Settings> settings;
        RETURN_IF_FAILED(m_optionsWebView->get_Settings(&settings));
        RETURN_IF_FAILED(settings->put_AreDevToolsEnabled(FALSE));

        RETURN_IF_FAILED(m_optionsController->add_ZoomFactorChanged(Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [](ICoreWebView2Controller* host, IUnknown* args) -> HRESULT
        {
            host->put_ZoomFactor(1.0);
            return S_OK;
        }
        ).Get(), &m_optionsZoomToken));

        // Hide by default
        RETURN_IF_FAILED(m_optionsController->put_IsVisible(FALSE));
        RETURN_IF_FAILED(m_optionsWebView->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_optionsUIMessageBrokerToken));

        // Hide menu when focus is lost
        RETURN_IF_FAILED(m_optionsController->add_LostFocus(Callback<ICoreWebView2FocusChangedEventHandler>(
            [this](ICoreWebView2Controller* sender, IUnknown* args) -> HRESULT
        {
            web::json::value jsonObj = web::json::value::parse(L"{}");
            jsonObj[L"message"] = web::json::value(MG_OPTIONS_LOST_FOCUS);
            jsonObj[L"args"] = web::json::value::parse(L"{}");

            PostJsonToWebView(jsonObj, m_controlsWebView.Get());

            return S_OK;
        }).Get(), &m_lostOptionsFocus));

        RETURN_IF_FAILED(ResizeUIWebViews());

        std::wstring optionsPath = GetFullPathFor(L"wvbrowser_ui\\controls_ui\\options.html");
        RETURN_IF_FAILED(m_optionsWebView->Navigate(optionsPath.c_str()));

        return S_OK;
    }).Get());
}

// Set the message broker for the UI webview. This will capture messages from ui web content.
// Lambda is used to capture the instance while satisfying Microsoft::WRL::Callback<T>()
void BrowserWindow::SetUIMessageBroker()
{
    m_uiMessageBroker = Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs) -> HRESULT
    {
        wil::unique_cotaskmem_string jsonString;
        CheckFailure(eventArgs->get_WebMessageAsJson(&jsonString), L"");  // Get the message from the UI WebView as JSON formatted string
        web::json::value jsonObj = web::json::value::parse(jsonString.get());

        if (!jsonObj.has_field(L"message"))
        {
            OutputDebugString(L"No message code provided\n");
            return S_OK;
        }

        if (!jsonObj.has_field(L"args"))
        {
            OutputDebugString(L"The message has no args field\n");
            return S_OK;
        }

        int message = jsonObj.at(L"message").as_integer();
        web::json::value args = jsonObj.at(L"args");

        switch (message)
        {
        case MG_CREATE_TAB:
        {
            size_t id = args.at(L"tabId").as_number().to_uint32();
            bool shouldBeActive = args.at(L"active").as_bool();
            std::unique_ptr<Tab> newTab = Tab::CreateNewTab(m_hWnd, m_contentEnv.Get(), id, shouldBeActive);

            std::map<size_t, std::unique_ptr<Tab>>::iterator it = m_tabs.find(id);
            if (it == m_tabs.end())
            {
                m_tabs.insert(std::pair<size_t,std::unique_ptr<Tab>>(id, std::move(newTab)));
            }
            else
            {
                m_tabs.at(id)->m_contentController->Close();
                it->second = std::move(newTab);
            }
        }
        break;
        case MG_NAVIGATE:
        {
            std::wstring uri(args.at(L"uri").as_string());
            std::wstring browserScheme(L"browser://");

            if (uri.substr(0, browserScheme.size()).compare(browserScheme) == 0)
            {
                // No encoded search URI
                std::wstring path = uri.substr(browserScheme.size());
                if (path.compare(L"favorites") == 0 ||
                    path.compare(L"settings") == 0 ||
                    path.compare(L"history") == 0)
                {
                    std::wstring filePath(L"wvbrowser_ui\\content_ui\\");
                    filePath.append(path);
                    filePath.append(L".html");
                    std::wstring fullPath = GetFullPathFor(filePath.c_str());
                    CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Navigate(fullPath.c_str()), L"Can't navigate to browser page.");
                }
                else
                {
                    OutputDebugString(L"Requested unknown browser page\n");
                }
            }
            else if (!SUCCEEDED(m_tabs.at(m_activeTabId)->m_contentWebView->Navigate(uri.c_str())))
            {
                CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Navigate(args.at(L"encodedSearchURI").as_string().c_str()), L"Can't navigate to requested page.");
            }
        }
        break;
        case MG_GO_FORWARD:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->GoForward(), L"");
        }
        break;
        case MG_GO_BACK:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->GoBack(), L"");
        }
        break;
        case MG_RELOAD:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Reload(), L"");
        }
        break;
        case MG_CANCEL:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->CallDevToolsProtocolMethod(L"Page.stopLoading", L"{}", nullptr), L"");
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
            size_t id = args.at(L"tabId").as_number().to_uint32();
            m_tabs.at(id)->m_contentController->Close();
            m_tabs.erase(id);
        }
        break;
        case MG_CLOSE_WINDOW:
        {
            DestroyWindow(m_hWnd);
        }
        break;
        case MG_SHOW_OPTIONS:
        {
            CheckFailure(m_optionsController->put_IsVisible(TRUE), L"");
            m_optionsController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        break;
        case MG_HIDE_OPTIONS:
        {
            CheckFailure(m_optionsController->put_IsVisible(FALSE), L"Something went wrong when trying to close the options dropdown.");
        }
        break;
        case MG_OPTION_SELECTED:
        {
            m_tabs.at(m_activeTabId)->m_contentController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        break;
        case MG_GET_FAVORITES:
        case MG_GET_SETTINGS:
        case MG_GET_HISTORY:
        {
            // Forward back to requesting tab
            size_t tabId = args.at(L"tabId").as_number().to_uint32();
            jsonObj[L"args"].erase(L"tabId");

            CheckFailure(PostJsonToWebView(jsonObj, m_tabs.at(tabId)->m_contentWebView.Get()), L"Requesting history failed.");
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

HRESULT BrowserWindow::SwitchToTab(size_t tabId)
{
    size_t previousActiveTab = m_activeTabId;

    RETURN_IF_FAILED(m_tabs.at(tabId)->ResizeWebView());
    RETURN_IF_FAILED(m_tabs.at(tabId)->m_contentController->put_IsVisible(TRUE));
    m_activeTabId = tabId;

    if (previousActiveTab != INVALID_TAB_ID && previousActiveTab != m_activeTabId)
    {
        RETURN_IF_FAILED(m_tabs.at(previousActiveTab)->m_contentController->put_IsVisible(FALSE));
    }

    return S_OK;
}

HRESULT BrowserWindow::HandleTabURIUpdate(size_t tabId, ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"uri"] = web::json::value(source.get());

    std::wstring uri(source.get());
    std::wstring favoritesURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\favorites.html"));
    std::wstring settingsURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\settings.html"));
    std::wstring historyURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\history.html"));

    if (uri.compare(favoritesURI) == 0)
    {
        jsonObj[L"args"][L"uriToShow"] = web::json::value(L"browser://favorites");
    }
    else if (uri.compare(settingsURI) == 0)
    {
        jsonObj[L"args"][L"uriToShow"] = web::json::value(L"browser://settings");
    }
    else if (uri.compare(historyURI) == 0)
    {
        jsonObj[L"args"][L"uriToShow"] = web::json::value(L"browser://history");
    }

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}

HRESULT BrowserWindow::HandleTabHistoryUpdate(size_t tabId, ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));
    
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"uri"] = web::json::value(source.get());

    BOOL canGoForward = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoForward(&canGoForward));
    jsonObj[L"args"][L"canGoForward"] = web::json::value::boolean(canGoForward);

    BOOL canGoBack = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoBack(&canGoBack));
    jsonObj[L"args"][L"canGoBack"] = web::json::value::boolean(canGoBack);

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}

HRESULT BrowserWindow::HandleTabNavStarting(size_t tabId, ICoreWebView2* webview)
{
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_STARTING);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

HRESULT BrowserWindow::HandleTabNavCompleted(size_t tabId, ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args)
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

    std::wstring getFaviconURI(
        L"(() => {"
        // Let the UI use a fallback favicon
        L"    let faviconURI = '';"
        L"    let links = document.getElementsByTagName('link');"
        // Test each link for a favicon
        L"    Array.from(links).map(element => {"
        L"        let rel = element.rel;"
        // Favicon is declared, try to get the href
        L"        if (rel && (rel == 'shortcut icon' || rel == 'icon')) {"
        L"            if (!element.href) {"
        L"                return;"
        L"            }"
        // href to icon found, check it's full URI
        L"            try {"
        L"                let urlParser = new URL(element.href);"
        L"                faviconURI = urlParser.href;"
        L"            } catch(e) {"
        // Try prepending origin
        L"                let origin = window.location.origin;"
        L"                let faviconLocation = `${origin}/${element.href}`;"
        L"                try {"
        L"                    urlParser = new URL(faviconLocation);"
        L"                    faviconURI = urlParser.href;"
        L"                } catch (e2) {"
        L"                    return;"
        L"                }"
        L"            }"
        L"        }"
        L"    });"
        L"    return faviconURI;"
        L"})();"
    );

    CheckFailure(webview->ExecuteScript(getTitleScript.c_str(), Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [this, tabId](HRESULT error, PCWSTR result) -> HRESULT
    {
        RETURN_IF_FAILED(error);

        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_UPDATE_TAB);
        jsonObj[L"args"] = web::json::value::parse(L"{}");
        jsonObj[L"args"][L"title"] = web::json::value::parse(result);
        jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

        CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Can't update title.");
        return S_OK;
    }).Get()), L"Can't update title.");

    CheckFailure(webview->ExecuteScript(getFaviconURI.c_str(), Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [this, tabId](HRESULT error, PCWSTR result) -> HRESULT
    {
        RETURN_IF_FAILED(error);

        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_UPDATE_FAVICON);
        jsonObj[L"args"] = web::json::value::parse(L"{}");
        jsonObj[L"args"][L"uri"] = web::json::value::parse(result);
        jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

        CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Can't update favicon.");
        return S_OK;
    }).Get()), L"Can't update favicon");

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_COMPLETED);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    BOOL navigationSucceeded = FALSE;
    if (SUCCEEDED(args->get_IsSuccess(&navigationSucceeded)))
    {
        jsonObj[L"args"][L"isError"] = web::json::value::boolean(!navigationSucceeded);
    }

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

HRESULT BrowserWindow::HandleTabSecurityUpdate(size_t tabId, ICoreWebView2* webview, ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args)
{
    wil::unique_cotaskmem_string jsonArgs;
    RETURN_IF_FAILED(args->get_ParameterObjectAsJson(&jsonArgs));
    web::json::value securityEvent = web::json::value::parse(jsonArgs.get());

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_SECURITY_UPDATE);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"state"] = securityEvent.at(L"securityState");

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

void BrowserWindow::HandleTabCreated(size_t tabId, bool shouldBeActive)
{
    if (shouldBeActive)
    {
        CheckFailure(SwitchToTab(tabId), L"");
    }
}

HRESULT BrowserWindow::HandleTabMessageReceived(size_t tabId, ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs)
{
    wil::unique_cotaskmem_string jsonString;
    RETURN_IF_FAILED(eventArgs->get_WebMessageAsJson(&jsonString));
    web::json::value jsonObj = web::json::value::parse(jsonString.get());

    wil::unique_cotaskmem_string uri;
    RETURN_IF_FAILED(webview->get_Source(&uri));

    int message = jsonObj.at(L"message").as_integer();
    web::json::value args = jsonObj.at(L"args");

    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    switch (message)
    {
    case MG_GET_FAVORITES:
    case MG_REMOVE_FAVORITE:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\favorites.html"));
        // Only the favorites UI can request favorites
        if (fileURI.compare(source.get()) == 0)
        {
            jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
            CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Couldn't perform favorites operation.");
        }
    }
    break;
    case MG_GET_SETTINGS:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\settings.html"));
        // Only the settings UI can request settings
        if (fileURI.compare(source.get()) == 0)
        {
            jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
            CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Couldn't retrieve settings.");
        }
    }
    break;
    case MG_CLEAR_CACHE:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\settings.html"));
        // Only the settings UI can request cache clearing
        if (fileURI.compare(uri.get()) == 0)
        {
            jsonObj[L"args"][L"content"] = web::json::value::boolean(false);
            jsonObj[L"args"][L"controls"] = web::json::value::boolean(false);

            if (SUCCEEDED(ClearContentCache()))
            {
                jsonObj[L"args"][L"content"] = web::json::value::boolean(true);
            }

            if (SUCCEEDED(ClearControlsCache()))
            {
                jsonObj[L"args"][L"controls"] = web::json::value::boolean(true);
            }

            CheckFailure(PostJsonToWebView(jsonObj, m_tabs.at(tabId)->m_contentWebView.Get()), L"");
        }
    }
    break;
    case MG_CLEAR_COOKIES:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\settings.html"));
        // Only the settings UI can request cookies clearing
        if (fileURI.compare(uri.get()) == 0)
        {
            jsonObj[L"args"][L"content"] = web::json::value::boolean(false);
            jsonObj[L"args"][L"controls"] = web::json::value::boolean(false);

            if (SUCCEEDED(ClearContentCookies()))
            {
                jsonObj[L"args"][L"content"] = web::json::value::boolean(true);
            }


            if (SUCCEEDED(ClearControlsCookies()))
            {
                jsonObj[L"args"][L"controls"] = web::json::value::boolean(true);
            }

            CheckFailure(PostJsonToWebView(jsonObj, m_tabs.at(tabId)->m_contentWebView.Get()), L"");
        }
    }
    break;
    case MG_GET_HISTORY:
    case MG_REMOVE_HISTORY_ITEM:
    case MG_CLEAR_HISTORY:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\history.html"));
        // Only the history UI can request history
        if (fileURI.compare(uri.get()) == 0)
        {
            jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
            CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Couldn't perform history operation");
        }
    }
    break;
    default:
    {
        OutputDebugString(L"Unexpected message\n");
    }
    break;
    }

    return S_OK;
}

HRESULT BrowserWindow::ClearContentCache()
{
    return m_tabs.at(m_activeTabId)->m_contentWebView->CallDevToolsProtocolMethod(L"Network.clearBrowserCache", L"{}", nullptr);
}

HRESULT BrowserWindow::ClearControlsCache()
{
    return m_controlsWebView->CallDevToolsProtocolMethod(L"Network.clearBrowserCache", L"{}", nullptr);
}

HRESULT BrowserWindow::ClearContentCookies()
{
    return m_tabs.at(m_activeTabId)->m_contentWebView->CallDevToolsProtocolMethod(L"Network.clearBrowserCookies", L"{}", nullptr);
}

HRESULT BrowserWindow::ClearControlsCookies()
{
    return m_controlsWebView->CallDevToolsProtocolMethod(L"Network.clearBrowserCookies", L"{}", nullptr);
}

HRESULT BrowserWindow::ResizeUIWebViews()
{
    if (m_controlsWebView != nullptr)
    {
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        bounds.bottom = bounds.top + GetDPIAwareBound(c_uiBarHeight);
        bounds.bottom += 1;

        RETURN_IF_FAILED(m_controlsController->put_Bounds(bounds));
    }

    if (m_optionsWebView != nullptr)
    {
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        bounds.top = GetDPIAwareBound(c_uiBarHeight);
        bounds.bottom = bounds.top + GetDPIAwareBound(c_optionsDropdownHeight);
        bounds.left = bounds.right - GetDPIAwareBound(c_optionsDropdownWidth);

        RETURN_IF_FAILED(m_optionsController->put_Bounds(bounds));
    }

    // Workaround for black controls WebView issue in Windows 7
    HWND wvWindow = GetWindow(m_hWnd, GW_CHILD);
    while (wvWindow != nullptr)
    {
        UpdateWindow(wvWindow);
        wvWindow = GetWindow(wvWindow, GW_HWNDNEXT);
    }

    return S_OK;
}

void BrowserWindow::UpdateMinWindowSize()
{
    RECT clientRect;
    RECT windowRect;

    GetClientRect(m_hWnd, &clientRect);
    GetWindowRect(m_hWnd, &windowRect);

    int bordersWidth = (windowRect.right - windowRect.left) - clientRect.right;
    int bordersHeight = (windowRect.bottom - windowRect.top) - clientRect.bottom;

    m_minWindowWidth = GetDPIAwareBound(MIN_WINDOW_WIDTH) + bordersWidth;
    m_minWindowHeight = GetDPIAwareBound(MIN_WINDOW_HEIGHT) + bordersHeight;
}

void BrowserWindow::CheckFailure(HRESULT hr, LPCWSTR errorMessage)
{
    if (FAILED(hr))
    {
        std::wstring message;
        if (!errorMessage || !errorMessage[0])
        {
            message = std::wstring(L"Something went wrong.");
        }
        else
        {
            message = std::wstring(errorMessage);
        }

        MessageBoxW(nullptr, message.c_str(), nullptr, MB_OK);
    }
}

int BrowserWindow::GetDPIAwareBound(int bound)
{
    // Remove the GetDpiForWindow call when using Windows 7 or any version
    // below 1607 (Windows 10). You will also have to make sure the build
    // directory is clean before building again.
    return (bound * GetDpiForWindow(m_hWnd) / DEFAULT_DPI);
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

std::wstring BrowserWindow::GetFullPathFor(LPCWSTR relativePath)
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(m_hInst, path, MAX_PATH);
    std::wstring pathName(path);

    std::size_t index = pathName.find_last_of(L"\\") + 1;
    pathName.replace(index, pathName.length(), relativePath);

    return pathName;
}

std::wstring BrowserWindow::GetFilePathAsURI(std::wstring fullPath)
{
    std::wstring fileURI;
    ComPtr<IUri> uri;
    DWORD uriFlags = Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME;
    HRESULT hr = CreateUri(fullPath.c_str(), uriFlags, 0, &uri);

    if (SUCCEEDED(hr))
    {
        wil::unique_bstr absoluteUri;
        uri->GetAbsoluteUri(&absoluteUri);
        fileURI = std::wstring(absoluteUri.get());
    }

    return fileURI;
}

HRESULT BrowserWindow::PostJsonToWebView(web::json::value jsonObj, ICoreWebView2* webview)
{
    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    return webview->PostWebMessageAsJson(stream.str().c_str());
}
