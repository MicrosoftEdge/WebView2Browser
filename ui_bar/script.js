const URI_REGEX = /([http|https]:\/\/)[a-z]/i;

function processAddressBarInput() {
    var text = document.querySelector('#address-field').value;
    if (!text) {
        return;
    }

    if (!isURI(text)) {
        text = getSearchURI(text);
    }

    navigateTopWebView(text);
}

function navigateTopWebView(uri) {
    var message = {
        message: commands.MG_NAVIGATE,
        args: {
            uri: uri
        }
    };

    window.chrome.webview.postMessage(message);
}

function isURI(query) {
    return URI_REGEX.test(query);
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
