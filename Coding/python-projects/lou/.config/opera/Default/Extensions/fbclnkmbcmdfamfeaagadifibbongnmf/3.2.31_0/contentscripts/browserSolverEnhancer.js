if (typeof browser !== 'undefined') { chrome = browser; }

var BrowserSolverEnhancer = function () {
    var IFRAME_VISIBLE_STYLE = "display:block;border:none; width: 100%;height: 600px;";
    var IFRAME_INVISIBLE_STYLE = "display:none;border:none; width: 100%;height: 600px;";

    var Elements = Object.create(null);

    var captchaParams = {
        siteKey: document.getElementsByName("sitekey")[0].content,
        siteDomain: document.getElementsByName("siteDomain")[0].content,
        //favIcon: document.getElementsByName("favIcon")[0].content,
        siteUrl: document.getElementsByName("siteUrl")[0].content,
        isValid: function () {
            return this.siteKey !== undefined && this.siteKey.length > 0 && this.siteDomain !== undefined && this.siteDomain.length > 0;
        },
        getSanitizedContextUrl: function () {
            if (this.siteDomain.indexOf("http://") === -1 && this.siteDomain.indexOf("https://") === -1) {
                return "http://" + this.siteDomain;
            }
            return this.siteDomain;
        },
        getSanitizedSiteUrl: function () {
            if (this.siteUrl.indexOf("http://") === -1 && this.siteUrl.indexOf("https://") === -1) {
                return "http://" + this.siteUrl;
            }
            return this.siteUrl;
        },
        getFinalUrl: function () {
            if (this.siteUrl != null && this.siteUrl != "") {
                return this.getSanitizedSiteUrl();
            } else {
                return this.getSanitizedContextUrl();
            }
        }
    };

    var hideExtensionInfoContainers = function () {
        var noExtensionElements = document.getElementsByClassName("no_extension");
        if (noExtensionElements.length && noExtensionElements.length !== 0) {
            for (var i = 0; i < noExtensionElements.length; i++) {
                noExtensionElements[i].setAttribute("style", "display:none");
            }
        }
    };

    var isBoundToDomain = function () {
        return document.getElementsByName("boundToDomain") !== undefined && document.getElementsByName("boundToDomain").length > 0 && document.getElementsByName("boundToDomain")[0].content === "true";
    };

    var isFrameOptionsSameOrigin = function () {
        return document.getElementsByName("sameOrigin") !== undefined && document.getElementsByName("sameOrigin").length > 0 && document.getElementsByName("sameOrigin")[0].content === "true";
    };

    var injectCaptchaIntoFrame = function () {
        Elements.wrapper = document.getElementsByClassName("wrap")[1];
        Elements.wrapper.appendChild(Elements.loadingElement);

        var rc2Parameter = {
            "siteKey": "" + captchaParams.siteKey,
            "contextUrl": "" + captchaParams.getSanitizedContextUrl(),
            "siteUrl": "" + captchaParams.getSanitizedSiteUrl(),
            "favIcon": "" + captchaParams.favIcon
        };

        Elements.iframe = document.createElement("iframe");
        Elements.iframe.setAttribute("sandbox", "allow-scripts allow-same-origin allow-forms");
        Elements.iframe.src = captchaParams.getFinalUrl() + "#rc2jd";
        Elements.iframe.setAttribute("style", IFRAME_INVISIBLE_STYLE);
        Elements.iframe.onload = function () {
            var payload = Object.create(null);
            payload.job = rc2Parameter;
            payload.type = "rc2_payload";
            Elements.iframe.contentWindow.postMessage(payload, "*");
        };
        Elements.wrapper.appendChild(Elements.iframe);
    };

    var injectCaptchaIntoTab = function () {
        window.location = captchaParams.getFinalUrl() + "#rc2jdt?k=" + encodeURIComponent(captchaParams.siteKey) + "&u=" + encodeURIComponent(window.location.href) + "&h=" + encodeURIComponent(captchaParams.getSanitizedContextUrl());
    };

    var init = function () {
        Elements.loadingElement = document.createElement("div");
        Elements.loadingElement.setAttribute("style", "font-weight: bold; margin-top: 20px;margin-bottom: 8px;margin-left: 8px;")
        Elements.loadingElement.textContent = "Please wait...";

        if (!captchaParams.isValid()) {
            Elements.loadingElement.setAttribute("style", "color:red;font-weight: bold; margin-top: 20px;margin-bottom: 8px;margin-left: 8px;");
            Elements.loadingElement.textContent = "Can not load captcha. This is a bug, please contact us!";
        }

        if (isFrameOptionsSameOrigin()) {
            injectCaptchaIntoTab();
        } else {
            listenToFrame();
            injectCaptchaIntoFrame();
        }
    };

    var listenToFrame = function () {
        window.addEventListener("message", function (event) {
            var eventData = event.data;
            if (eventData && eventData.type === 'myjdrc2') {
                if (eventData.name === 'response') {
                    sendSolution(eventData.data.token);
                } else if (eventData.name === 'injected') {
                    // nothing to do
                } else if (eventData.name === 'content_loaded') {
                    Elements.loadingElement.textContent = "Please solve this captcha";
                    if (Elements.iframe) {
                        Elements.iframe.setAttribute("style", IFRAME_VISIBLE_STYLE);
                    }
                } else if (eventData.name === 'window-height-update') {
                    if (data.height !== undefined) {
                        Elements.iframe.setAttribute("height", "" + data.height + "px");
                    }
                }
            }
        }, false);
    };

    return {
        init: init,
        isBoundToDomain: isBoundToDomain,
        isFrameOptionsSameOrigin: isFrameOptionsSameOrigin,
        hideExtensionInfoContainers: hideExtensionInfoContainers
    }
};

window.alert = function () {
};

window.addEventListener('DOMContentLoaded', function () {
    var enhancer = new BrowserSolverEnhancer();
    enhancer.hideExtensionInfoContainers();

    if (enhancer.isBoundToDomain() || enhancer.isFrameOptionsSameOrigin()) {
        enhancer.init();
    }
}, true);

function sendSolution(token) {
    chrome.runtime.sendMessage(chrome.runtime.id, {
        name: "myjdrc2",
        action: "response",
        data: {
            token: token,
            callbackUrl: window.location.href
        }
    });
}