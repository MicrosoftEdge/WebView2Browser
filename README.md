# WebView2Browser
A web browser built with the [Microsoft Edge WebView2](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2) control.

WebView2Browser is a sample Windows desktop application demonstrating the WebView2 control capabilities. It is built as a Win32 [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) project and makes use of both C++ and JavaScript in the WebView2 environment to power its features.

WebView2Browser shows some of the simplest uses of WebView2 -such as creating and navigating a WebView, but also some more complex workflows like using the [PostWebMessageAsJson API](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2webview#postwebmessageasjson) to communicate WebViews in separate environments. It is intended as a rich code sample to look at how you can use WebView2 APIs to build your own app.

## Requisites
- [Microsoft Edge (Chromium)](https://www.microsoftedgeinsider.com/en-us/download/) installed on a supported OS.
- [Visual Studio](https://visualstudio.microsoft.com/vs/) with C++ support installed.

The Canary channel is recommended for the installation and the minimum version is 78.0.240.0.

## Build the browser
Clone the repository and open the solution in Visual Studio. WebView2 is already included as a NuGet package* in the project!

- Clone this repository
- Open the solution in Visual Studio 2019**
- Set the target you want to build (Debug/Release, x86/x64)
- Build the solution

That's it. Everything should be ready to just launch the app.

*You can get the WebView2 NugetPackage through the Visual Studio NuGet Package Manager.
<br />
**You can also use Visual Studio 2017 by changing the project's Platform Toolset in Project Properties/Configuration properties/General/Platform Toolset. You might also need to change the Windows SDK to the latest version available to you.

## Browser layout
WebView2Browser has a multi-WebView approach to integrate web content and application UI into a Windows Desktop application. This allows the browser to use standard web technologies (HTML, CSS, JavaScript) to light up the interface but also enables the app to fetch favicons from the web and use IndexedDB for storing favorites and history.

The multi-WebView approach involves using two separate WebView environments (each with its own user data directory): one for the UI WebViews and the other for all content WebViews. UI WebViews (controls and options dropdown) use the UI environtment while web content WebViews (one per tab) use the content environment.

## Features
WebView2Browser provides all the functionalities to make a basic web browser, but there's plenty of room for you to play around.

- Go back/forward
- Reload page
- Cancel navigation
- Multiple tabs
- History
- Favorites
- Search from the address bar
- Page security status
- Clearing cache and cookies

## WebView2 APIs
WebView2Browser makes use of a handful of the APIs available in WebView2. For the APIs not used here, you can find more about them in the [Microsoft Edge WebView2 Reference](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference-webview2). The following is a list of the most interesting APIs WebView2Browser uses and the feature(s) they enable.

API | Feature(s)
:--- | :---
CreateWebView2EnvironmentWithDetails | Used to create the environments for UI and content WebViews. Different user data directories are passed to isolate UI from web content. |
IWebView2DevToolsProtocolEventReceivedEventHandler | Used along with add_DevToolsProtocolEventReceived to listen for CDP security events to update the lock icon in the browser UI. |
IWebView2DocumentStateChangedEventHandler | Used along with add_DocumentStateChanged to udpate the address bar and navigation buttons in the browser UI. |
IWebView2ExecuteScriptCompletedHandler | Used along with ExecuteScript to get the title and favicon from the visited page. |
IWebView2FocusChangedEventHandler | Used along with add_LostFocus to hide the browser options dropdown when it loses focus.
IWebView2NavigationCompletedEventHandler | Used along with add_NavigationCompleted to udpate the reload button in the browser UI.
IWebView2Settings | Used to disable DevTools in the browser UI.
IWebView2WebMessageReceivedEventHandler | This is one of the most important APIs to WebView2Browser. Most functionalities involving communication across WebViews use this.
IWebView2WebView | There are several WebViews in WebView2Browser and most features make use of members in this interface, the table below shows how they're used.

IWebView2WebView API | Feature(s)
:--- | :---
add_NavigationStarting | Used to display the cancel navigation button in the controls WebView.
add_DocumentStateChanged | Used to update the address bar and go back/forward buttons.
add_NavigationCompleted | Used to display the reload button once a navigation completes.
add_LostFocus | Used to hide the options dropdown when the user clicks away of it.
ExecuteScript | Used to get the title and favicon of a visited page.
PostWebMessageAsJson | Used to communicate WebViews. All messages use JSON to pass parameters needed.
add_WebMessageReceived | Used to handle web messages posted to the WebView.
CallDevToolsProtocolMethod | Used to enable listening for security events, which will notify of security status changes in a document.
<br />

## The basics
### Set up the environment, create a WebView
WebView2 allows you host web content in your Windows app. It exposes the globals [CreateWebView2Environment](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/webview2.idl#createwebview2environment) and [CreateWebView2EnvironmentWithDetails](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/webview2.idl#createwebview2environmentwithdetails) from which we can create the two separate environments for the browser's UI and content.

```cpp
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
        HRESULT hr = InitUIWebViews();

        if (!SUCCEEDED(hr))
        {
            OutputDebugString(L"UI WebViews environment creation failed\n");
        }

        return hr;
    }).Get());
```
```cpp
HRESULT BrowserWindow::InitUIWebViews()
{
    // Get data directory for browser UI data
    std::wstring browserDataDirectory = GetAppDataDirectory();
    browserDataDirectory.append(L"\\Browser Data");

    // Create WebView environment for browser UI. A separate data directory is
    // used to isolate the browser UI from web content requested by the user.
    return CreateWebView2EnvironmentWithDetails(nullptr, browserDataDirectory.c_str(), WEBVIEW2_RELEASE_CHANNEL_PREFERENCE_STABLE,
        L"", Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, IWebView2Environment* env) -> HRESULT
    {
        // Environment is ready, create the WebView
        m_uiEnv = env;

        RETURN_IF_FAILED(CreateBrowserControlsWebView());
        RETURN_IF_FAILED(CreateBrowserOptionsWebView());

        return S_OK;
    }).Get());
}
```

We use the [IWebView2CreateWebView2EnvironmentCompletedHandler](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2createwebview2environmentcompletedhandler#interface_i_web_view2_create_web_view2_environment_completed_handler) to create the UI WebViews once the environment is created.

```cpp
HRESULT BrowserWindow::CreateBrowserControlsWebView()
{
    return m_uiEnv->CreateWebView(m_hWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
        [this](HRESULT result, IWebView2WebView* webview) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Controls WebView creation failed\n");
            return result;
        }
        // WebView created
        m_controlsWebView = webview;

        wil::com_ptr<IWebView2Settings> settings;
        RETURN_IF_FAILED(m_controlsWebView->get_Settings(&settings));
        RETURN_IF_FAILED(settings->put_AreDevToolsEnabled(FALSE));
        RETURN_IF_FAILED(settings->put_IsFullscreenAllowed(FALSE));

        RETURN_IF_FAILED(m_controlsWebView->add_ZoomFactorChanged(Callback<IWebView2ZoomFactorChangedEventHandler>(
            [](IWebView2WebView* webview, IUnknown* args) -> HRESULT
        {
            webview->put_ZoomFactor(1.0);
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
```
We're setting up a few things here. The [IWebView2Settings](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2settings) interface is used to disable DevTools in the WebView powering the browser controls. We're also adding a handler for received web messages. This handler will enable us to do something when the user interacts with the controls in this WebView.

### Navigate to web page
You can navigate to a web page by entering it's URI in the address bar. When pressing Enter, the controls WebView will post a web message to the host app so it can navigate the active tab to the specified location. Code below shows how the host Win32 application will handle that message.

```cpp
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
```
WebView2Browser will check the URI against browser pages (i.e. favorites, settings, history) and navigate to the requested location or use the provided URI for Bing search as a fallback.

### Updating the address bar
The address bar is updated every time there is a change in the active tab's document source and along with other controls when switching tabs. Each WebView will fire an event when the state of the document changes, we can use this to get the new source on updates and forward the change to the controls WebView. We'll also update the go back and go forward buttons when receiving this event.

```cpp
        // Register event handler for doc state change
        RETURN_IF_FAILED(m_contentWebView->add_DocumentStateChanged(Callback<IWebView2DocumentStateChangedEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2DocumentStateChangedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabURIUpdate(m_tabId, webview), L"Can't update address bar");

            return S_OK;
        }).Get(), &m_uriUpdateForwarderToken));
```
```cpp
HRESULT BrowserWindow::HandleTabURIUpdate(size_t tabId, IWebView2WebView* webview)
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

    BOOL canGoForward = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoForward(&canGoForward));
    jsonObj[L"args"][L"canGoForward"] = web::json::value::boolean(canGoForward);

    BOOL canGoBack = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoBack(&canGoBack));
    jsonObj[L"args"][L"canGoBack"] = web::json::value::boolean(canGoBack);

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}
```

### Going back, going forward
Each WebView will keep a history for the navigations it has performed so we only need to connect the browser UI with the corresponding methods. If the active tab's WebView can be navigated back/forward, the buttons will post a web message to the host application when clicked.

The JavaScript side:
```javascript
    document.querySelector('#btn-forward').addEventListener('click', function(e) {
        if (document.getElementById('btn-forward').className === 'btn') {
            var message = {
                message: commands.MG_GO_FORWARD,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-back').addEventListener('click', function(e) {
        if (document.getElementById('btn-back').className === 'btn') {
            var message = {
                message: commands.MG_GO_BACK,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        }
    });
```
The host application side:
```cpp
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
```

### Reloading, stop navigation

## Some interesting features
### Communicating the WebViews
### Tab handling
### Updating the security icon
### Populating the history

## Handling JSON and URIs
WebView2Browser uses Microsoft's cpprestsdk (Casablanca) to handle all JSON in the C++ side of thigs. IUri and CreateUri are also used to parse file paths into URIs and can be used to for other URIs as well.
