"use strict";

angular.module('myjdWebextensionApp')
    .controller('AutograbberListitemController', ['$scope', 'ExtensionMessagingService', function ($scope, ExtensionMessagingService) {
        $scope.goToTab = function () {
            chrome.tabs.get($scope.tabData.tab.id, function (tab) {
                chrome.tabs.update(tab.id, {active: true});
            });
        };

        $scope.stopAutoGrabber = function () {
            ExtensionMessagingService.sendMessage('autograbber', 'stop-single-tab', {id: $scope.tabData.tab.id});
        };

        $scope.openDataView = function () {
            chrome.tabs.query({url: chrome.runtime.getURL("index.html")},
                function (tabs) {
                    var matchingTabs = tabs.filter(function (tab) {
                        if (tab.url.indexOf("#!/autograbber") !== -1) {
                            return tab;
                        }
                    });
                    if (matchingTabs.length === 0) {
                        chrome.tabs.create({url: chrome.runtime.getURL("index.html") + "#!/autograbber" + "/" + $scope.tabData.tab.id});
                    } else {
                        chrome.tabs.update(matchingTabs[0].id, {active: true});
                        ExtensionMessagingService.sendMessage('autograbber-view', 'switch-tab', {id: $scope.tabData.tab.id});
                    }
                });
        }
    }]);