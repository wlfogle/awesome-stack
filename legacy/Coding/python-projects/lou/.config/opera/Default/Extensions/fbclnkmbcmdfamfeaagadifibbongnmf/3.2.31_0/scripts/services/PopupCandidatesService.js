"use strict";

angular.module('myjdWebextensionApp')
    .service('PopupCandidatesService', ['$q', function ($q) {
        var popupCandidates = Object.create(null);
        var previousActiveTab;
        var removedTabListeners = Object.create(null);

        chrome.tabs.onCreated.addListener(function (tab) {
            popupCandidates[tab.id] = {tab: tab, creationTime: Date.now(), lastActiveTab: previousActiveTab};
            previousActiveTab = tab;
        });

        chrome.tabs.onRemoved.addListener(function (tab) {
            if (removedTabListeners[tab] !== undefined && removedTabListeners[tab].length > 0) {
                for (var i = 0; i < removedTabListeners[tab].length; i++) {
                    removedTabListeners[tab][i](popupCandidates[tab]);
                }
            }
            delete removedTabListeners[tab];
            delete popupCandidates[tab];
        });

        chrome.tabs.onUpdated.addListener(function (tabId, changeInfo, tabInfo) {
            if (popupCandidates[tabId] !== undefined && changeInfo.url !== undefined) {
                popupCandidates[tabId].tab = tabInfo;
            }
        });

        this.checkIfPopup = function (tab, timestampFirstCnlRequest) {
            return new $q(function (resolve, reject) {
                if (popupCandidates[tab.id] && ((timestampFirstCnlRequest - popupCandidates[tab.id].creationTime) < 2000)) {
                    resolve(true);
                } else {
                    resolve(false);
                }
            });
        };

        this.addRemovedTabListener = function (tabId, listener) {
            if (removedTabListeners[tabId] === undefined) {
                removedTabListeners[tabId] = [];
            }
            if (removedTabListeners[tabId].indexOf(listener === -1)) {
                removedTabListeners[tabId].push(listener);
            }
        };

        this.removeRemovedTabListener = function (tabId, listener) {
            if (removedTabListeners[tabId] !== undefined) {
                for (var i = 0; removedTabListeners[tabId].length; i++) {
                    if (removedTabListeners[tabId][i] === listener) {
                        removedTabListeners[tabId] = removedTabListeners[tabId].splice(i, 1);
                        return true;
                    }
                }
            }
            return false;
        };

    }]);