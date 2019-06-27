const WORD_REGEX = /^[^//][^.]*$/;
const VALID_URI_REGEX = /^[-:.&#+()[\]$'*;@~!,?%=\/\w]+$/; // Will check that only RFC3986 allowed characters are included
const SCHEMED_URI_REGEX = /^\w+:.+$/;

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

window.chrome.webview.addEventListener('message', event => {
    var message = event.data.message;
    var args = event.data.args;

    switch (message) {
        case commands.MG_UPDATE_URI:
            document.getElementById('address-field').value = args.uri;
            const btnForward = document.getElementById('btn-forward');
            const btnBack = document.getElementById('btn-back');

            if (args.canGoForward)
                btnForward.className = 'btn';
            else
                btnForward.className = 'btn-disabled';

            if (args.canGoBack)
                btnBack.className = 'btn';
            else
                btnBack.className = 'btn-disabled';
            break;
        case commands.MG_NAV_STARTING:
            var btnReload = document.getElementById('btn-reload');
            btnReload.className = 'btn-cancel';
            break;
        case commands.MG_NAV_COMPLETED:
            var btnReload = document.getElementById('btn-reload');
            btnReload.className = 'btn';
            break;
    }
});

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
