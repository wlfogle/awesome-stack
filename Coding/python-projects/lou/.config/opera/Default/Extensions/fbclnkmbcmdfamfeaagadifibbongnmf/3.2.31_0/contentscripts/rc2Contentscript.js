if (typeof browser !== 'undefined') { chrome = browser; }

var log = [];
var mouseMoved = Date.now();

if (document.location.hash.startsWith("#rc2jd")) {
    var Params = function (url) {
        this.params = url.split("?")[1].split("&");
    };

    Params.prototype.get = function (name) {
        var ret;
        this.params.forEach(function (param) {
            if (param.indexOf(name + "=") !== -1) {
                var split = param.split("=");
                ret = split[1];
                return false;
            }
        });
        return ret;
    };

    function ResultPoll(job) {
        var self = this;

        function _poll() {
            var sTokenElement = document.getElementById('g-recaptcha-response');
            var fallbackElement = document.getElementById('captcha-response');
            if (sTokenElement != null && sTokenElement.value != null && sTokenElement.value.length > 0) {
                log.push(Date.now() + " | result poll SUCCESSFULL, value: " + sTokenElement.value);
                self.cancel();
                CaptchaFormInjector.success(sTokenElement.value, job);
            } else if (fallbackElement != null && fallbackElement.value != null && fallbackElement.value.length > 0) {
                log.push(Date.now() + " | result poll SUCCESSFULL (FALLBACK ELEMENT), value: " + fallbackElement.value);
                self.cancel();
                CaptchaFormInjector.success(fallbackElement.value, job);
            }
        }

        this.poll = function (interval) {
            log.push(Date.now() + " | starting result poll");
            this.intervalHandle = setInterval(_poll, interval || 500);
        };

        this.cancel = function () {
            if (self.intervalHandle !== undefined) {
                clearInterval(self.intervalHandle);
            }
        }
    }

    var CaptchaFormInjector = (function () {
        var tabMode = document.location.hash.startsWith("#rc2jdt");
        var state = {
            inserted: false
        };

        function loadSolverTemplate(callback, error, templateUrl) {
            var xhr = new XMLHttpRequest();
            xhr.onload = function () {
                log.push(Date.now() + " | solver template loaded");
                if (callback !== undefined && typeof callback === "function") {
                    callback(this.response);
                }
            };
            xhr.onerror = function () {
                log.push(Date.now() + " | failed to load solver template");
                if (error !== undefined && typeof error === "function") {
                    error(this.response);
                }
            };
            xhr.open("GET", templateUrl === undefined ? chrome.runtime.getURL("./res/browser_solver_template.html") : templateUrl);
            xhr.responseType = "text";
            xhr.send();
        }

        var sendLoadedEvent = function (element, callbackUrl) {
            var bounds = element.getBoundingClientRect();

            var w = Math.max(document.documentElement.clientWidth, window.innerWidth || 0);
            var h = Math.max(document.documentElement.clientHeight, window.innerHeight || 0);
            /*
                 * If the browser does not support screenX and screen Y, use screenLeft and
                 * screenTop instead (and vice versa)
                 */
            var winLeft = window.screenX ? window.screenX : window.screenLeft;
            var winTop = window.screenY ? window.screenY : window.screenTop;
            var windowWidth = window.outerWidth;
            var windowHeight = window.outerHeight;
            var ie = getInternetExplorerVersion();
            if (ie > 0) {
                if (ie >= 10) {
                    // bug in ie 10 and 11
                    var zoom = screen.deviceXDPI / screen.logicalXDPI;
                    winLeft *= zoom;
                    winTop *= zoom;
                    windowWidth *= zoom;
                    windowHeight *= zoom;
                }
            }
            var loadedParams = Object.create(null);
            loadedParams.x = winLeft;
            loadedParams.y = winTop;
            loadedParams.w = windowWidth;
            loadedParams.h = windowHeight;
            loadedParams.vw = w;
            loadedParams.vh = h;
            loadedParams.eleft = bounds.left;
            loadedParams.etop = bounds.top;
            loadedParams.ew = bounds.width;
            loadedParams.eh = bounds.height;

            chrome.runtime.sendMessage({
                name: "myjdrc2",
                action: "loaded",
                callbackUrl: callbackUrl,
                params: loadedParams
            });
        };

        var sendMouseMovedEvent = function (callbackUrl, currentTime) {
            chrome.runtime.sendMessage({
                name: "myjdrc2",
                action: "mouse-move",
                callbackUrl: callbackUrl,
                timestamp: currentTime
            });
        };

        var init = function () {
            var injectionMsg = {type: "myjdrc2", name: "injected"};
            var callbackUrl;
            log.push(Date.now() + " | posting to parent " + JSON.stringify(injectionMsg));
            window.parent.postMessage(injectionMsg, "*");
            if (tabMode) {
                log.push(Date.now() + " | tab mode inited");
                var params = new Params(document.location.hash);
                var siteKey = decodeURIComponent(params.get("k"));
                callbackUrl = decodeURIComponent(params.get("u"));
                var captchaId = decodeURIComponent(params.get("c"));
                var hoster = decodeURIComponent(params.get("h"));
                log.push(Date.now() + " | [params] sitekey: " + siteKey + " callbackUrl: " + callbackUrl + " captchaId: " + captchaId + " hoster: " + hoster);
                writeCaptchaFormFirefoxCompat({
                    siteKey: siteKey,
                    callbackUrl: callbackUrl,
                    captchaId: captchaId,
                    hoster: hoster
                });
                chrome.runtime.sendMessage({
                    name: "myjdrc2",
                    action: "tabmode-init",
                    data: {
                        callbackUrl: callbackUrl,
                        captchaId: captchaId
                    }
                });

                var searchElementTimeout = setTimeout(function () {
                    var captchaContainer = document.getElementById("captchaContainer");
                    if (captchaContainer != null) {
                        clearInterval(searchElementTimeout);
                        sendLoadedEvent(captchaContainer, callbackUrl);
                    }
                }, 300);

                chrome.runtime.onMessage.addListener(function (msg) {
                    if (msg.name && msg.name === "myjdrc2") {
                        if (msg.action && msg.action === "captcha-done") {
                            if (msg.data && msg.data.captchaId === captchaId) {
                                chrome.runtime.sendMessage({
                                    name: "close-me",
                                    data: {"tabId": "self"}
                                });
                            }
                        }
                    }
                });
            } else {
                log.push(Date.now() + " | frame mode inited");
                listenToParent();
            }

            document.addEventListener("mousemove", function (event) {
                var currentTime = Date.now();
                if (currentTime - mouseMoved > 3 * 1000) {
                    mouseMoved = currentTime;
                    sendMouseMovedEvent(callbackUrl, currentTime);
                }
            });
        };

        var i18n = function () {
            var header_please_solve_el = document.getElementById("header_please_solve");
            header_please_solve_el.innerText = chrome.i18n.getMessage("header_please_solve");

            var help_whats_happening_header_el = document.getElementById("help_whats_happening_header");
            help_whats_happening_header_el.innerText = chrome.i18n.getMessage("help_whats_happening_header");

            var help_whats_happening_description_el = document.getElementById("help_whats_happening_description");
            help_whats_happening_description_el.innerText = chrome.i18n.getMessage("help_whats_happening_description");

            var help_whats_happening_link_el = document.getElementById("help_whats_happening_link");
            help_whats_happening_link_el.innerText = chrome.i18n.getMessage("help_whats_happening_link");

            var help_need_help_header_el = document.getElementById("help_need_help_header");
            help_need_help_header_el.innerText = chrome.i18n.getMessage("help_need_help_header");

            var help_need_help_description_el = document.getElementById("help_need_help_description");
            help_need_help_description_el.innerText = chrome.i18n.getMessage("help_need_help_description");

            var help_need_help_link_el = document.getElementById("help_need_help_link");
            help_need_help_link_el.innerText = chrome.i18n.getMessage("help_need_help_link");
        };

        var listenToParent = function () {
            window.addEventListener("message", function (e) {
                if (e.data.type === "rc2_payload") {
                    if (!state.inserted) {
                        state.inserted = true;
                        writeCaptchaFormFirefoxCompat(e.data.job);
                    }
                }
            }, false);

            var lastKnownHeight;
            var lastKnownWidth;
            setInterval(function () {
                if (document.documentElement && document.documentElement.scrollHeight && document.documentElement.scrollWidth) {
                    // Firefox: document.body.scrollHeight not returning correct values if body contains position:fixed elements -> using document.documentElement
                    var currentHeight = Math.max(document.documentElement.scrollHeight, document.body.scrollHeight);
                    var currentWidth = Math.max(document.documentElement.scrollWidth, document.body.scrollWidth);
                    if (lastKnownHeight !== (currentHeight - 32) || lastKnownWidth !== (currentWidth - 16)) {
                        lastKnownHeight = currentHeight;
                        lastKnownWidth = currentWidth;
                        window.parent.postMessage({
                            type: "myjdrc2",
                            name: "window-dimensions-update",
                            data: {height: currentHeight + 32, width: currentWidth + 16}
                        }, "*");
                    }
                }
            }, 500);
        };

        var success = function (result, job) {
            if (result && result.length > 0) {
                sendSolution(result, job);
            }
        };

        var sendSolution = function (token, job) {
            if (tabMode) {
                var resultMsg = {
                    name: "myjdrc2",
                    action: "response",
                    data: {
                        token: token,
                        callbackUrl: job.callbackUrl,
                        captchaId: job.captchaId
                    }
                };
                log.push(Date.now() + " | sending solution message to extension background " + JSON.stringify(resultMsg));
                chrome.runtime.sendMessage(resultMsg);

                setTimeout(function () {
                    chrome.runtime.sendMessage({
                        name: "close-me",
                        data: {"tabId": "self"}
                    });
                }, 2000);
            } else if (!tabMode) {
                window.parent.postMessage({type: "myjdrc2", name: "response", data: {token: token}}, "*");
            }
        };

        var insertHosterName = function (hosterName) {
            if (hosterName != null && hosterName != "" && hosterName != "undefined") {
                log.push(Date.now() + " | inserting hostername into DOM for job " + JSON.stringify(hosterName));
                var hosterNameContainer = document.getElementsByClassName("hosterName");
                for (var i = 0; i < hosterNameContainer.length; i++) {
                    hosterNameContainer[i].textContent = hosterName.replace(/^(https?):\/\//, "");
                }
            } else {
                var shouldHideContainer = document.getElementsByClassName("hideIfNoHoster");
                for (var i = 0; i < shouldHideContainer.length; i++) {
                    shouldHideContainer[i].style.visibility = "hidden";
                }
            }
        };

        var insertFavIcon = function (favicon) {
            if (favicon != null && favicon.startsWith("data:image/png;base64,")) {
                var favIconImg = document.getElementsByClassName("hideIfNoFavicon");
                for (var i = 0; i < favIconImg.length; i++) {
                    favIconImg[i].src = favicon;
                }
            } else {
                var favIconImg = document.getElementsByClassName("hideIfNoFavicon");
                for (var i = 0; i < favIconImg.length; i++) {
                    favIconImg[i].style.visibility = "hidden";
                }
            }
        };

        var insertRc2ScriptIntoDOM = function (job) {
            log.push(Date.now() + " | inserting rc2 script into DOM for job " + JSON.stringify(job));
            var captchaContainer = document.getElementById("captchaContainer");
            captchaContainer.innerHTML = "<div id=\"recaptcha_container\"><form action=\"\" method=\"post\"> <div class=\"placeholder\"> <div id=\"recaptcha_widget\"> \
                    <form action=\"?\" method=\"POST\"> \
                    <div class=\"g-recaptcha\" data-callback=\"onResponse\"></div> \
                    </form></div>";
            captchaContainer.querySelector('.g-recaptcha').setAttribute("data-sitekey", job.siteKey);
            var rc2Script = document.createElement('script');
            rc2Script.type = 'text/javascript';
            rc2Script.src = "https://www.google.com/recaptcha/api.js";
            rc2Script.onload = function () {
                log.push(Date.now() + " | rc2script onload fired, letting window.parent know");
                window.parent.postMessage({type: "myjdrc2", name: "content_loaded"}, "*");
            };
            var dataCallbackScript = document.createElement('script');
            dataCallbackScript.type = 'text/javascript';
            dataCallbackScript.text = 'var onResponse = function (response) {\n' +
                '            document.getElementById(\'captcha-response\').value = response;\n' +
                '        }';
            rc2Script.src = "https://www.google.com/recaptcha/api.js";
            document.body.appendChild(dataCallbackScript);
            document.body.appendChild(rc2Script);
            var resultPoll = new ResultPoll(job);
            resultPoll.poll();
        };

        var writeCaptchaFormFirefoxCompat = function (job) {
            try {
                // block document load
                document.open();
                document.write("");
                document.close();
            } catch (exception) {
            }
            if (document.head !== undefined && document.head !== null) {
                document.head.innerHTML = "";
            }
            document.body = document.createElement("body");
            if (!tabMode) {
                log.push(Date.now() + " | firefox compat: frame mode");
                document.body.innerHTML = "<style>html {height:100%} #captchaContainer{margin: auto}</style><div id=\"captchaContainer\"></div>";
                insertRc2ScriptIntoDOM(job);
                insertHosterName(job.hoster);
                insertFavIcon(job.favIcon);
                i18n();
            } else {
                log.push(Date.now() + " | firefox compat: tab mode");
                loadSolverTemplate(function (template) {
                    document.body.innerHTML = template;
                    insertRc2ScriptIntoDOM(job);
                    insertHosterName(job.hoster);
                    insertFavIcon(job.favIcon);
                    i18n();
                }, function (error) {
                    console.log(error);
                });
            }
        };

        return {
            init: init,
            success: success
        }
    })();

    log.push(Date.now() + " | rc2 detected on " + document.location.hash);
    CaptchaFormInjector.init();
}

function getInternetExplorerVersion() {
    var rv = -1;
    if (navigator.appName == 'Microsoft Internet Explorer') {
        var ua = navigator.userAgent;
        var re = new RegExp("MSIE ([0-9]{1,}[\.0-9]{0,})");
        if (re.exec(ua) != null) rv = parseFloat(RegExp.$1);
    } else if (navigator.appName == 'Netscape') {
        var ua = navigator.userAgent;
        var re = new RegExp("Trident/.*rv:([0-9]{1,}[\.0-9]{0,})");
        if (re.exec(ua) != null) rv = parseFloat(RegExp.$1);
    }
    return rv;
}

var debug = function () {
    if (log !== undefined && log.length > 0) {
        for (var i = 0; i < log.length; i++) {
            console.log(log[i]);
        }
    } else {
        console.log("no logs available");
    }
};
