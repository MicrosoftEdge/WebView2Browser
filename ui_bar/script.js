const WORD_REGEX = /^[^//][^.]*$/;
const VALID_URI_REGEX = /^[-:.&#+()[\]$'*;@~!,?%=\/\w]+$/; // Will check that only RFC3986 allowed characters are included
const SCHEMED_URI_REGEX = /^\w+:.+$/;

var tabs = new Map();
var tabIdCounter = 0;
var activeTabId = 0;
const INVALID_TAB_ID = 0;

const messageHandler = event => {
    var message = event.data.message;
    var args = event.data.args;

    switch (message) {
        case commands.MG_UPDATE_URI:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);

                // Update the tab state
                tab.uri = args.uri;
                tab.canGoBack = args.canGoBack;
                tab.canGoForward = args.canGoForward;

                // If the tab is active, update the controls UI
                if (args.tabId == activeTabId) {
                    updateNavigationUIForTab(args.tabId, commands.MG_UPDATE_URI);
                }
            }
            break;
        case commands.MG_NAV_STARTING:
            if (isValidTabId(args.tabId)) {
                // Update the tab state
                tabs.get(args.tabId).isLoading = true;

                // If the tab is active, udpate the controls UI
                if (args.tabId == activeTabId) {
                    updateNavigationUIForTab(args.tabId, commands.MG_NAV_STARTING);
                }
            }
            break;
        case commands.MG_NAV_COMPLETED:
            if (isValidTabId(args.tabId)) {
                // Update tab state
                tabs.get(args.tabId).isLoading = false;

                // If the tab is active, update the controls UI
                if (args.tabId == activeTabId) {
                    updateNavigationUIForTab(args.tabId, commands.MG_NAV_COMPLETED);
                }
            }
            break;
        case commands.MG_UPDATE_TAB:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);
                const tabElement = document.getElementById(`tab-${args.tabId}`);

                // Update tab label
                if (tabElement) {
                    // use given title or fall back to a generic tab title
                    tab.title = args.title || 'Tab';

                    const tabLabel = tabElement.firstChild;
                    if (!tabLabel) {
                        console.log(`tab-${args.tabId} doesn't have a label`);
                        break;
                    }

                    const tabLabelSpan = tabLabel.firstChild;
                    if (!tabLabel) {
                        console.log(`Label for tab-${args.tabId} doesn't have a span`);
                        break;
                    }

                    tabLabelSpan.innerText = tab.title;
                }
            }
            break;
    }
};

function isValidTabId(tabId) {
    return tabId != INVALID_TAB_ID && tabs.has(tabId);
}

function processAddressBarInput() {
    var text = document.querySelector('#address-field').value;
    tryNavigate(text);
}

function tryNavigate(text) {
    try {
        var uriParser = new URL(text);

        // URL creation succeeded, verify protocol is allowed
        switch (uriParser.protocol) {
            case 'http:':
            case 'https:':
            case 'file:':
            case 'ftp:':
                // allowed protocol, navigate
                navigateActiveTab(uriParser.href, false);
                break;
            default:
                // protocol not allowed, search Bing
                navigateActiveTab(getSearchURI(text), true);
                break;
        }
    } catch (e) {
        // URL creation failed, check for invalid characters
        if (containsIlegalCharacters(text) || isSingleWord(text)) {
            // search Bing
            navigateActiveTab(getSearchURI(text), true);
        } else {
            // try with HTTP
            if (!hasScheme(text)) {
                tryNavigate(`http:${text}`);
            } else {
                navigateActiveTab(getSearchURI(text), true);
            }
        }
    }
}

function navigateActiveTab(uri, isSearch) {
    var message = {
        message: commands.MG_NAVIGATE,
        args: {
            uri: uri,
            encodedSearchURI: isSearch ? uri : getSearchURI(uri)
        }
    };

    window.chrome.webview.postMessage(message);
}

function containsIlegalCharacters(query) {
    return !VALID_URI_REGEX.test(query);
}

function isSingleWord(query) {
    return WORD_REGEX.test(query);
}

function hasScheme(query) {
    return SCHEMED_URI_REGEX.test(query);
}

function getSearchURI(query) {
    return `https://www.bing.com/search?q=${encodeURIComponent(query)}`;
}

function getNewTabId() {
    return ++tabIdCounter;
}

function switchToTab(id, updateOnHost) {
    if (!id) {
        console.log('ID not provided');
        return;
    }

    // Check the tab to switch to is valid
    if (!isValidTabId(id)) {
        return;
    }

    // No need to switch if the tab is already active
    if (id == activeTabId) {
        return;
    }

    // Get the tab element to switch to
    var tab = document.getElementById(`tab-${id}`);
    if (!tab) {
        console.log(`Can't switch to tab ${id}: element does not exist`);
        return;
    }

    // Change the style for the previously active tab
    if (isValidTabId(activeTabId)) {
        const activeTabElement = document.getElementById(`tab-${activeTabId}`);

        // Check the previously active tab element does actually exist
        if (activeTabElement) {
            activeTabElement.className = 'tab';
        }
    }

    // Set tab as active
    tab.className = 'tab-active';
    activeTabId = id;

    // Instruct host app to switch tab
    if (updateOnHost) {
        var message = {
            message: commands.MG_SWITCH_TAB,
            args: {
                tabId: parseInt(activeTabId)
            }
        };

        window.chrome.webview.postMessage(message);
    }

    updateNavigationUIForTab(id, commands.MG_SWITCH_TAB);
}

function closeTab(id) {
    // if closing tab was active, switch tab or close window
    if (id == activeTabId) {
        if (tabs.size == 1) {
            // last tab is closing, shut window down
            closeWindow();
            return;
        }

        // other tabs are open, switch to rightmost tab
        var tabsEntries = Array.from(tabs.entries());
        var lastEntry = tabsEntries.pop();
        if (lastEntry[0] == id) {
            lastEntry = tabsEntries.pop();
        }
        switchToTab(lastEntry[0], true);
    }

    // remove tab element
    var tabElement = document.getElementById(`tab-${id}`);
    if (tabElement) {
        tabElement.parentNode.removeChild(tabElement);
    }
    // remove tab from map
    tabs.delete(id);

    var message = {
        message: commands.MG_CLOSE_TAB,
        args: {
            tabId: id
        }
    };

    window.chrome.webview.postMessage(message);
}

function closeWindow() {
    var message = {
        message: commands.MG_CLOSE_WINDOW,
        args: {}
    };

    window.chrome.webview.postMessage(message);
}

function updateNavigationUIForTab(id, reason) {
    const tab = tabs.get(parseInt(id));

    if (!tab) {
        return console.log(`Tab ${id} does not exist in map`);
    }

    // update reload
    var btnReload = document.getElementById('btn-reload');
    btnReload.className = tab.isLoading ? 'btn-cancel' : 'btn';

    // udpate uri
    if (reason == commands.MG_UPDATE_URI || reason == commands.MG_SWITCH_TAB) {
        document.getElementById('address-field').value = tab.uri;
    }

    // update go back/forward
    const btnForward = document.getElementById('btn-forward');
    const btnBack = document.getElementById('btn-back');

    if (tab.canGoForward)
        btnForward.className = 'btn';
    else
        btnForward.className = 'btn-disabled';

    if (tab.canGoBack)
        btnBack.className = 'btn';
    else
        btnBack.className = 'btn-disabled';
}

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
        title: '',
        uri: '',
        favicon: '',
        isLoading: false,
        canGoBack: false,
        canGoForward: false
    });

    var tab = document.createElement('div');
    tab.className = shouldBeActive ? 'tab-active' : 'tab';
    tab.id = `tab-${tabId}`;

    var tabLabel = document.createElement('div');
    tabLabel.className = 'tab-label';

    var labelText = document.createElement('span');
    labelText.innerHTML = 'New Tab';
    tabLabel.appendChild(labelText);

    var closeButton = document.createElement('div');
    closeButton.className = 'btn-tab-close';
    closeButton.addEventListener('click', function(e) {
        closeTab(tabId);
    });

    tab.appendChild(tabLabel);
    tab.appendChild(closeButton);

    var createTabButton = document.getElementById('btn-new-tab');
    document.getElementById('tabs-strip').insertBefore(tab, createTabButton);

    tab.addEventListener('click', function(e) {
        if (e.srcElement.className != 'btn-tab-close') {
            switchToTab(tabId, true);
        }
    });

    if (shouldBeActive) {
        switchToTab(tabId, false);
    }
}

function addUIListeners() {
    document.querySelector('#address-field').addEventListener('keypress', function (e) {
        var key = e.which || e.keyCode;
        if (key === 13) { // 13 is enter
            e.preventDefault();
            processAddressBarInput();
        }
    });

    document.querySelector('#btn-forward').addEventListener('click', function (e) {
        if (document.getElementById('btn-forward').className === 'btn') {
            var message = {
                message: commands.MG_GO_FORWARD,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-back').addEventListener('click', function (e) {
        if (document.getElementById('btn-back').className === 'btn') {
            var message = {
                message: commands.MG_GO_BACK,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-reload').addEventListener('click', function(e) {
        var btnReload = document.getElementById('btn-reload');
        if (btnReload.className === 'btn-cancel') {
            var message = {
                message: commands.MG_CANCEL,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        } else if (btnReload.className === 'btn') {
            var message = {
                message: commands.MG_RELOAD,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-new-tab').addEventListener('click', function(e) {
        createNewTab(true);
    });
}

function init() {
    window.chrome.webview.addEventListener('message', messageHandler);
    addUIListeners();

    createNewTab(true);
}

init();
