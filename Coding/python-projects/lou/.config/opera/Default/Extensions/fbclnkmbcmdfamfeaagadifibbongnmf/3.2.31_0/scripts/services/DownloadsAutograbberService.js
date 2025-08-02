'use strict';

angular.module('myjdWebextensionApp')
    .service('DownloadsAutograbberService', ["FilterService", "PopupIconService", "ExtensionMessagingService", function (FilterService, PopupIconService, ExtensionMessagingService) {

        var GrabbedLink = function (linkText) {
            this.linkText = linkText;
            this.cookies;
            this.authHeader;
            this.referer;

            var withCookies = function (cookies) {
                this.cookies = cookies;
                return this;
            };

            var withAuthHeader = function (authHeader) {
                this.authHeader = authHeader;
                return this;
            };

            var withReferer = function (referer) {
                this.referer = referer;
                return this;
            };

            return {
                linkText: linkText,
                cookies: this.cookies,
                authHeader: this.authHeader,
                referer: this.referer,
                withCookies: withCookies,
                withAuthHeader: withAuthHeader,
                withReferer: withReferer
            }
        };

        var TabGrabberData = function () {
            var links = [];

            var addLink = function (link, optionals) {
                var dupes = this.links.filter(function (el) {
                    return el.linkText == link;
                });

                // Remove dupes
                if (dupes.length == 0) {
                    // Filter
                    var whitelisted = FilterService.getFilters().some(function (filter) {
                        if (!filter.active) return false;
                        return filter.getRules().some(function (rule) {
                            if (rule.test(link)) {
                                return true;
                            }
                        });
                    });

                    if (whitelisted) {
                        var grabbedLink = new GrabbedLink(link);
                        if (optionals != null) {
                            grabbedLink
                                .withCookies(optionals.cookies)
                                .withAuthHeader(optionals.authHeader)
                                .withReferer(optionals.referer);
                        }
                        this.links.push(grabbedLink);
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            };

            return {links: links, addLink: addLink};
        };

        var activeTabs = Object.create(null);
        var popupIconTimer;

        function updatePopupIcon() {
            if (hasActiveTab()) {
                if (popupIconTimer != null) {
                    clearTimeout(popupIconTimer);
                }
                popupIconTimer = setInterval(function () {
                    this.toggle = this.toggle == null ? false : !this.toggle;
                    PopupIconService.updateBadge({text: this.toggle == false ? " " : "", color: "#cc0000"})
                }, 800);
            } else {
                PopupIconService.updateBadge({text: ""});
                clearInterval(popupIconTimer);
            }
        }

        var clearAllAutograbbers = function () {
            var keys = Object.keys(activeTabs);
            keys.map(function (key) {
                try {
                    chrome.tabs.sendMessage(Number.parseInt(key), {
                        action: "autograbber-stopped-in-tab",
                        tabId: key
                    });
                } catch (e) {
                    console.error(e);
                }
            });
            activeTabs = Object.create(null);
            updatePopupIcon();
        };

        var hasActiveTab = function (noCleanUp) {
            if (noCleanUp == null || noCleanUp == false) {
                cleanUpActiveTabs();
            }
            return Object.keys(activeTabs).length !== 0;
        };

        var isTabActive = function (tabId) {
            if (isNaN(tabId)) return false;

            var keys = Object.keys(activeTabs);
            if (keys != null) {
                var foundKey = undefined;
                for (var idx = 0; idx < keys.length; idx++) {
                    if (keys[idx] == tabId) {
                        foundKey = keys[idx];
                        return true;
                    }
                }
            }
            return false;
        };

        var getGrabbedData = function (tabId) {
            if (isNaN(tabId)) return null;
            return activeTabs[tabId] == null ? null : activeTabs[tabId];
        };

        var cleanUpActiveTabs = function () {
            return new Promise(function (resolve, reject) {
                var keys = Object.keys(activeTabs);
                var orphanedKeysPromises = [];
                keys.filter(
                    function (key) {
                        orphanedKeysPromises.push(new Promise(function (resolve, reject) {
                            try {
                                chrome.tabs.get(Number.parseInt(key), function (tab) {
                                    if (tab == null) {
                                        resolve(key);
                                    } else {
                                        resolve();
                                    }
                                })
                            } catch (e) {
                                console.error(e);
                                resolve();
                            }
                        }));
                    }
                );
                Promise.all(orphanedKeysPromises).then(function (keys) {
                    keys.map(function (key) {
                        toggleTab(key);
                    });
                    resolve();
                }).catch(function (e) {
                    console.error(e);
                    resolve();
                });
            });
        };

        var getActiveTabsData = function (noCleanUp) {
            if (noCleanUp == null || noCleanUp == false) {
                cleanUpActiveTabs();
            }
            console.log("" + Date.now() + " | " + JSON.stringify(activeTabs));
            return activeTabs;
        };

        var addLink = function (tabId, link, optionals) {
            if (isNaN(tabId)) return false;
            var grabberData = getGrabbedData(tabId);
            return grabberData.addLink(link, optionals);
        };

        var toggleTab = function (tabId) {
            // returns new activation state as boolean
            if (isNaN(tabId)) return false;

            var keys = Object.keys(activeTabs);
            if (keys != null) {
                var foundKey = undefined;
                for (var idx = 0; idx < keys.length; idx++) {
                    if (keys[idx] == tabId) {
                        foundKey = keys[idx];
                        break;
                    }
                }
                if (foundKey != null) {
                    delete activeTabs[foundKey];
                    updatePopupIcon();
                    console.log(activeTabs);
                    return false;
                }
            }

            activeTabs[tabId] = new TabGrabberData();
            updatePopupIcon();
            console.log(activeTabs);
            return true;
        };

        /*chrome.tabs.onRemoved.addListener(function (tab) {
            console.log("CHROME.TABS.ONREMOVED | id " + tab.id);
            toggleTab(tab);
        });

        chrome.tabs.onReplaced.addListener(function (addedTabId, removedTabId) {
            console.log("CHROME.TABS.ONREPLACED | id " + removedTabId);
            if (activeTabs[removedTabId]) {
                activeTabs[addedTabId] = activeTabs[removedTabId];
                delete activeTabs[removedTabId];
            }
        });*/

        return {
            toggleTab: toggleTab,
            isTabActive: isTabActive,
            clearAllAutograbbers: clearAllAutograbbers,
            hasActiveTab: hasActiveTab,
            getGrabbedData: getGrabbedData,
            getActiveTabsData: getActiveTabsData,
            addLink: addLink
        }
    }]);