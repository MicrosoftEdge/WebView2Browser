# WebView2Browser
A web browser built with the [Microsoft Edge WebView2](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2) control.

![WebView2Browser](https://github.com/MicrosoftEdge/WebView2Browser/raw/master/screenshots/WebView2Browser.png)

WebView2Browser is a sample Windows desktop application demonstrating the WebView2 control capabilities. It is built as a Win32 [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) project and makes use of both C++ and JavaScript in the WebView2 environment to power its features.

WebView2Browser shows some of the simplest uses of WebView2 -such as creating and navigating a WebView, but also some more complex workflows like using the [PostWebMessageAsJson API](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2webview#postwebmessageasjson) to communicate WebViews in separate environments. It is intended as a rich code sample to look at how you can use WebView2 APIs to build your own app.

## Requisites
- [Microsoft Edge (Chromium)](https://www.microsoftedgeinsider.com/en-us/download/) installed on a supported OS.
- [Visual Studio](https://visualstudio.microsoft.com/vs/) with C++ support installed.

The Edge Canary channel is recommended for the installation and the minimum version is 78.0.240.0.

## Build the browser
Clone the repository and open the solution in Visual Studio. WebView2 is already included as a NuGet package* in the project!

- Clone this repository
- Open the solution in Visual Studio 2019**
- Make the changes listed below if you're using a Windows version below Windows 10.
- Set the target you want to build (Debug/Release, x86/x64)
- Build the solution

That's it. Everything should be ready to just launch the app.

*You can get the WebView2 NugetPackage through the Visual Studio NuGet Package Manager.
<br />
**You can also use Visual Studio 2017 by changing the project's Platform Toolset in Project Properties/Configuration properties/General/Platform Toolset. You might also need to change the Windows SDK to the latest version available to you.

## Using versions below Windows 10
There's a couple of changes you need to make if you want to build and run the browser in other versions of Windows. This is because of how DPI is handled in Windows 10 vs previous versions of Windows.

In `WebViewBrowserApp.cpp`, you will need to replace the call to `SetProcessDpiAwarenessContext` and call `SetProcessDPIAware` instead.
```cpp
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Call SetProcessDPIAware() instead when using Windows 7 or any version
    // below 1703 (Windows 10).
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    BrowserWindow::RegisterClass(hInstance);

    // ...
```

In `WebViewBrowser.cpp`, you will need to remove the call to `GetDpiForWindow`.
```cpp
int BrowserWindow::GetDPIAwareBound(int bound)
{
    // Remove the GetDpiForWindow call when using Windows 7 or any version
    // below 1607 (Windows 10). You will also have to make sure the build
    // directory is clean before building again.
    return (bound * GetDpiForWindow(m_hWnd) / DEFAULT_DPI);
}
```

## Browser layout
WebView2Browser has a multi-WebView approach to integrate web content and application UI into a Windows Desktop application. This allows the browser to use standard web technologies (HTML, CSS, JavaScript) to light up the interface but also enables the app to fetch favicons from the web and use IndexedDB for storing favorites and history.

The multi-WebView approach involves using two separate WebView environments (each with its own user data directory): one for the UI WebViews and the other for all content WebViews. UI WebViews (controls and options dropdown) use the UI environment while web content WebViews (one per tab) use the content environment.

![Browser layout](https://github.com/MicrosoftEdge/WebView2Browser/raw/master/screenshots/layout.png)

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

## Implementing the features
The sections below describe how some of the features in WebView2Browser were implemented. You can look at the source code for more details about how everything works here.

- [The basics](#the-basics)
    - [Set up the environment, create a WebView](#set-up-the-environment-create-a-webview)
    - [Navigate to web page](#navigate-to-web-page)
    - [Updating the address bar](#updating-the-address-bar)
    - [Going back, going forward](#going-back-going-forward)
- [Some interesting features](#some-interesting-features)
    - [Communicating the WebViews](#communicating-the-webviews)
    - [Tab handling](#tab-handling)
    - [Updating the security icon](#updating-the-security-icon)
    - [Populating the history](#populating-the-history)
- [Handling JSON and URIs](#handling-json-and-uris)

## The basics
### Set up the environment, create a WebView
WebView2 allows you to host web content in your Windows app. It exposes the globals [CreateWebView2Environment](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/webview2.idl#createwebview2environment) and [CreateWebView2EnvironmentWithDetails](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/webview2.idl#createwebview2environmentwithdetails) from which we can create the two separate environments for the browser's UI and content.

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

We use the [IWebView2CreateWebView2EnvironmentCompletedHandler](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2createwebview2environmentcompletedhandler#interface_i_web_view2_create_web_view2_environment_completed_handler) to create the UI WebViews once the environment is ready.

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
You can navigate to a web page by entering its URI in the address bar. When pressing Enter, the controls WebView will post a web message to the host app so it can navigate the active tab to the specified location. Code below shows how the host Win32 application will handle that message.

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
WebView2Browser will check the URI against browser pages (i.e. favorites, settings, history) and navigate to the requested location or use the provided URI to search Bing as a fallback.

### Updating the address bar
The address bar is updated every time there is a change in the active tab's document source and along with other controls when switching tabs. Each WebView will fire an event when the state of the document changes, we can use this event to get the new source on updates and forward the change to the controls WebView (we'll also update the go back and go forward buttons).

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

    // ...

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
We have sent the `MG_UPDATE_URI` message along with the URI to the controls WebView. Now we want to reflect those changes on the tab state and udpate the UI if necessary.
```javascript
        case commands.MG_UPDATE_URI:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);
                let previousURI = tab.uri;

                // Update the tab state
                tab.uri = args.uri;
                tab.uriToShow = args.uriToShow;
                tab.canGoBack = args.canGoBack;
                tab.canGoForward = args.canGoForward;

                // If the tab is active, update the controls UI
                if (args.tabId == activeTabId) {
                    updateNavigationUI(message);
                }

                // ...
            }
            break;
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
We use the `NavigationStarting` event fired by a content WebView to udpate its associated tab loading state in the controls WebView. Similarly, when a WebView fires the `NavigationCompleted` event, we use that event to instruct the controls WebView to update the tab state. The active tab state in the controls WebView will determine whether to show the reload or the cancel button. Each of those will post a message back to the host application when clicked, so that the WebView for that tab can be reloaded or have its navigation canceled, accordingly.
```javascript
function reloadActiveTabContent() {
    var message = {
        message: commands.MG_RELOAD,
        args: {}
    };
    window.chrome.webview.postMessage(message);
}

 // ...

    document.querySelector('#btn-reload').addEventListener('click', function(e) {
        var btnReload = document.getElementById('btn-reload');
        if (btnReload.className === 'btn-cancel') {
            var message = {
                message: commands.MG_CANCEL,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        } else if (btnReload.className === 'btn') {
            reloadActiveTabContent();
        }
    });
```
```cpp
        case MG_RELOAD:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Reload(), L"");
        }
        break;
        case MG_CANCEL:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->CallDevToolsProtocolMethod(L"Page.stopLoading", L"{}", nullptr), L"");
        }
```

## Some interesting features
### Communicating the WebViews
We need to communicate the WebViews powering tabs and UI so that user interactions in one have the desired effect in the other. WebView2Browser makes use of set of very useful WebView2 APIs for this purpose, including [PostWebMessageAsJson](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2webview#postwebmessageasjson), [add_WebMessageReceived](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2webview#add_webmessagereceived) and [IWebView2WebMessageReceivedEventHandler](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2webmessagereceivedeventhandler). On the JavaScript side, we're making use of the `window.chrome.webview` object exposed to call the `postMessage` method and add an event lister for received messages.
```cpp
HRESULT BrowserWindow::CreateBrowserControlsWebView()
{
    return m_uiEnv->CreateWebView(m_hWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
        [this](HRESULT result, IWebView2WebView* webview) -> HRESULT
    {
        // ...

        RETURN_IF_FAILED(m_controlsWebView->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_controlsUIMessageBrokerToken));

        // ...

        return S_OK;
    }).Get());
}
```
```cpp
HRESULT BrowserWindow::PostJsonToWebView(web::json::value jsonObj, IWebView2WebView* webview)
{
    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    return webview->PostWebMessageAsJson(stream.str().c_str());
}

// ...

HRESULT BrowserWindow::HandleTabNavStarting(size_t tabId, IWebView2WebView* webview)
{
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_STARTING);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}
```
```javascript
function init() {
    window.chrome.webview.addEventListener('message', messageHandler);
    refreshControls();
    refreshTabs();

    createNewTab(true);
}

// ...

function reloadActiveTabContent() {
    var message = {
        message: commands.MG_RELOAD,
        args: {}
    };
    window.chrome.webview.postMessage(message);
}
```
### Tab handling
A new tab will be created whenever the user clicks on the new tab button to the right of the open tabs. The controls WebView will post a message to the host application to create the WebView for that tab and create an object tracking its state.
```javascript
function createNewTab(shouldBeActive) {
    const tabId = getNewTabId();

    var message = {
        message: commands.MG_CREATE_TAB,
        args: {
            tabId: parseInt(tabId),
            active: shouldBeActive || false
        }
    };

    window.chrome.webview.postMessage(message);

    tabs.set(parseInt(tabId), {
        title: 'New Tab',
        uri: '',
        uriToShow: '',
        favicon: 'img/favicon.png',
        isFavorite: false,
        isLoading: false,
        canGoBack: false,
        canGoForward: false,
        securityState: 'unknown',
        historyItemId: INVALID_HISTORY_ID
    });

    loadTabUI(tabId);

    if (shouldBeActive) {
        switchToTab(tabId, false);
    }
}
```
On the host app side, the registered [IWebView2WebMessageReceivedEventHandler](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2webmessagereceivedeventhandler) will catch the message and create the WebView for that tab.
```cpp
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
                m_tabs.at(id)->m_contentWebView->Close();
                it->second = std::move(newTab);
            }
        }
        break;
```
```cpp
std::unique_ptr<Tab> Tab::CreateNewTab(HWND hWnd, IWebView2Environment* env, size_t id, bool shouldBeActive)
{
    std::unique_ptr<Tab> tab = std::make_unique<Tab>();

    tab->m_parentHWnd = hWnd;
    tab->m_tabId = id;
    tab->SetMessageBroker();
    tab->Init(env, shouldBeActive);

    return tab;
}

HRESULT Tab::Init(IWebView2Environment* env, bool shouldBeActive)
{
    return env->CreateWebView(m_parentHWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
        [this, shouldBeActive](HRESULT result, IWebView2WebView* webview) -> HRESULT {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Tab WebView creation failed\n");
            return result;
        }
        m_contentWebView = webview;
        BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
        RETURN_IF_FAILED(m_contentWebView->add_WebMessageReceived(m_messageBroker.Get(), &m_messageBrokerToken));

        // Register event handler for doc state change
        RETURN_IF_FAILED(m_contentWebView->add_DocumentStateChanged(Callback<IWebView2DocumentStateChangedEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2DocumentStateChangedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabURIUpdate(m_tabId, webview), L"Can't update address bar");

            return S_OK;
        }).Get(), &m_uriUpdateForwarderToken));

        RETURN_IF_FAILED(m_contentWebView->add_NavigationStarting(Callback<IWebView2NavigationStartingEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2NavigationStartingEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabNavStarting(m_tabId, webview), L"Can't update reload button");

            return S_OK;
        }).Get(), &m_navStartingToken));

        RETURN_IF_FAILED(m_contentWebView->add_NavigationCompleted(Callback<IWebView2NavigationCompletedEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2NavigationCompletedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabNavCompleted(m_tabId, webview, args), L"Can't udpate reload button");
            return S_OK;
        }).Get(), &m_navCompletedToken));

        // Handle security state updates

        RETURN_IF_FAILED(m_contentWebView->Navigate(L"https://www.bing.com"));
        browserWindow->HandleTabCreated(m_tabId, shouldBeActive);

        return S_OK;
    }).Get());
}
```
The tab registers all handlers so it can forward updates to the controls WebView when events fire. The tab is ready and will be shown on the content area of the browser. Clicking on a tab in the controls WebView will post a message to the host application, which will in turn hide the WebView for the previously active tab and show the one for the clicked tab.
```cpp
HRESULT BrowserWindow::SwitchToTab(size_t tabId)
{
    size_t previousActiveTab = m_activeTabId;

    RETURN_IF_FAILED(m_tabs.at(tabId)->ResizeWebView());
    RETURN_IF_FAILED(m_tabs.at(tabId)->m_contentWebView->put_IsVisible(TRUE));
    m_activeTabId = tabId;

    if (previousActiveTab != INVALID_TAB_ID && previousActiveTab != m_activeTabId)
    {
        RETURN_IF_FAILED(m_tabs.at(previousActiveTab)->m_contentWebView->put_IsVisible(FALSE));
    }

    return S_OK;
}
```

### Updating the security icon
We use the [CallDevToolsProtocolMethod](https://docs.microsoft.com/en-us/microsoft-edge/hosting/webview2/reference/iwebview2webview#calldevtoolsprotocolmethod) to enable listening for security events. Whenever a `securityStateChanged` event is fired, we will use the new state to update the security icon on the controls WebView.
```cpp
        // Enable listening for security events to update secure icon
        RETURN_IF_FAILED(m_contentWebView->CallDevToolsProtocolMethod(L"Security.enable", L"{}", nullptr));

        // Forward security status updates to browser
        RETURN_IF_FAILED(m_contentWebView->add_DevToolsProtocolEventReceived(L"Security.securityStateChanged", Callback<IWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this, browserWindow](IWebView2WebView* webview, IWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabSecurityUpdate(m_tabId, webview, args), L"Can't udpate security icon");
            return S_OK;
        }).Get(), &m_securityUpdateToken));
```
```cpp
HRESULT BrowserWindow::HandleTabSecurityUpdate(size_t tabId, IWebView2WebView* webview, IWebView2DevToolsProtocolEventReceivedEventArgs* args)
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
```
```javascript
        case commands.MG_SECURITY_UPDATE:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);
                tab.securityState = args.state;

                if (args.tabId == activeTabId) {
                    updateNavigationUI(message);
                }
            }
            break;
```

### Populating the history
WebView2Browser uses IndexedDB in the controls WebView to store history items, just an example of how WebView2 enables you to access standard web technologies as you would in the browser. The item for a navigation will be created as soon as the URI is updated. These items are then retrieved by the history UI in a tab making use of `window.chrome.postMessage`.

In this case, most functionality is implemented using JavaScript on both ends (controls WebView and content WebView loading the UI) so the host application is only acting as a message broker to communicate those ends.
```javascript
        case commands.MG_UPDATE_URI:
            if (isValidTabId(args.tabId)) {
                // ...

                // Don't add history entry if URI has not changed
                if (tab.uri == previousURI) {
                    break;
                }

                // Filter URIs that should not appear in history
                if (!tab.uri || tab.uri == 'about:blank') {
                    tab.historyItemId = INVALID_HISTORY_ID;
                    break;
                }

                if (tab.uriToShow && tab.uriToShow.substring(0, 10) == 'browser://') {
                    tab.historyItemId = INVALID_HISTORY_ID;
                    break;
                }

                addHistoryItem(historyItemFromTab(args.tabId), (id) => {
                    tab.historyItemId = id;
                });
            }
            break;
```
```javascript
function addHistoryItem(item, callback) {
    queryDB((db) => {
        let transaction = db.transaction(['history'], 'readwrite');
        let historyStore = transaction.objectStore('history');

        // Check if an item for this URI exists on this day
        let currentDate = new Date();
        let year = currentDate.getFullYear();
        let month = currentDate.getMonth();
        let date = currentDate.getDate();
        let todayDate = new Date(year, month, date);

        let existingItemsIndex = historyStore.index('stampedURI');
        let lowerBound = [item.uri, todayDate];
        let upperBound = [item.uri, currentDate];
        let range = IDBKeyRange.bound(lowerBound, upperBound);
        let request = existingItemsIndex.openCursor(range);

        request.onsuccess = function(event) {
            let cursor = event.target.result;
            if (cursor) {
                // There's an entry for this URI, update the item
                cursor.value.timestamp = item.timestamp;
                let updateRequest = cursor.update(cursor.value);

                updateRequest.onsuccess = function(event) {
                    if (callback) {
                        callback(event.target.result.primaryKey);
                    }
                };
            } else {
                // No entry for this URI, add item
                let addItemRequest = historyStore.add(item);

                addItemRequest.onsuccess = function(event) {
                    if (callback) {
                        callback(event.target.result);
                    }
                };
            }
        };

    });
}
```
## Handling JSON and URIs
WebView2Browser uses Microsoft's [cpprestsdk (Casablanca)](https://github.com/Microsoft/cpprestsdk) to handle all JSON in the C++ side of thigs. IUri and CreateUri are also used to parse file paths into URIs and can be used to for other URIs as well.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
