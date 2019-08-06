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
**You can also use Visual Studio 2017 by changing the project's Platform Toolset in Project Properties/Configuration properties/General/Platform Toolset.

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

## Used APIs
WebView2Browser makes use of a handful of the APIs available in WebView2. For the APIs not used here, you can find more about them in the Microsoft Edge WebView2 documentation. The following is a list of the most interesting APIs WebView2Browser uses and the feature(s) they enable.

- CreateWebView2EnvironmentWithDetails
Used to create the environments for UI and content WebViews. Different user data directories are passed to isolate UI from web content.

- IWebView2DevToolsProtocolEventReceivedEventHandler
Used along with add_DevToolsProtocolEventReceived to listen for CDP security events to update the lock icon in the browser UI.

- IWebView2DocumentStateChangedEventHandler
Used along with add_DocumentStateChanged to udpate the address bar and navigation buttons in the browser UI.

- IWebView2ExecuteScriptCompletedHandler
Used along with ExecuteScript to get the title and favicon from the visited page.

- IWebView2FocusChangedEventHandler
Used along with add_LostFocus to hide the browser options dropdown when it loses focus.

- IWebView2NavigationCompletedEventHandler
Used along with add_NavigationCompleted to udpate the reload button in the browser UI.

- IWebView2Settings
Used to disable DevTools in the browser UI.

- IWebView2WebMessageReceivedEventHandler
This is one of the most important APIs to WebView2Browser. Most functionalities involving communication across WebViews use this.

- IWebView2WebView
There are several WebViews in WebView2Browser.

## The basics
### Set up the environment, create a WebView
### Navigate to web page
### Updating the address bar
### Going back, going forward
### Reloading, stop navigation

## Some interesting features
### Communicating the WebViews
### Tab handling
### Updating the security icon
### Populating the history

## Handling JSON and URIs
WebView2Browser uses Microsoft's cpprestsdk (Casablanca) to handle all JSON in the C++ side of thigs. IUri and CreateUri are also used to parse file paths into URIs and can be used to for other URIs as well.
