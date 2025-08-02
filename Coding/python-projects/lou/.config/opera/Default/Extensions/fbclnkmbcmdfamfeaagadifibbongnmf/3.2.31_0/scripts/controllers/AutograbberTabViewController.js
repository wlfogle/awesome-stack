"use strict";

angular.module('myjdWebextensionApp')
    .controller('AutograbberTabViewController', ['$scope', '$routeParams', '$rootScope', '$location', '$timeout', 'ExtensionMessagingService', 'FilterService', function ($scope, $routeParams, $rootScope, $location, $timeout, ExtensionMessagingService, FilterService) {
        $rootScope.bodyClass = "autograbber-view-container";
        $scope.inited = false;
        $scope.FilterTags = FilterService.ExtensionGroupTags;
        $scope.filterState = Object.create(null);

        $scope.dataView = {
            selectionCount: 0,
            raw: undefined,
            filtered: undefined
        };

        var tabId = $routeParams["tabId"];
        if (!isNaN(tabId)) {
            $scope.tabId = tabId;
        }

        $scope.createView = function (rawData) {
            // fills the $scope.dataView object with data from rawData

            var keys = Object.keys($scope.filterState);
            for (var idx = 0; idx < keys.length; idx++) {
                // reset filter counts
                $scope.filterState[keys[idx]].count = 0;
            }

            $scope.dataView.raw = rawData;
            for (var i = 0; i < $scope.dataView.raw.data.active.length; i++) {
                if ($scope.dataView.raw.data.active[i].tab.id == $scope.tabId) {
                    $scope.dataView.filtered = {
                        tab: $scope.dataView.raw.data.active[i].tab,
                        links: $scope.dataView.raw.data.active[i].data
                            .map(function (link) {
                                var filter = FilterService
                                    .getMatchingFilters(link.linkText)[0];
                                link.selected = link.selected == null ? false : link.selected;
                                if (filter != null) {
                                    link.filterTag = filter.tag;
                                } else {
                                    link.filterTag = FilterService.ExtensionGroupTags.OTHER;
                                }

                                if ($scope.filterState[link.filterTag] != null) {
                                    $scope.filterState[link.filterTag].count++;
                                } else {
                                    $scope.filterState[link.filterTag] = {
                                        active: true,
                                        count: 1
                                    }
                                }
                                return link;
                            })
                    };
                    $scope.dataView.unfiltered = $scope.dataView.raw.data.active[i].data;
                }
            }

            if ($scope.dataView.filtered != null) {
                // apply filters on links
                $scope.dataView.filtered.links = $scope.dataView.filtered.links.filter(function (link) {
                    var filter = FilterService
                        .getMatchingFilters(link.linkText)[0];
                    if (filter == null && $scope.filterState[FilterService.ExtensionGroupTags.OTHER].active == true) {
                        return link;
                    } else if (filter != null) {
                        if ($scope.filterState[filter.tag] != null
                            && $scope.filterState[filter.tag].active == true) {
                            return link;
                        }
                    }
                });
            }
            $scope.updateSelectionCount();
        };

        $scope.toggleFilterState = function (filterTag) {
            if ($scope.filterState[filterTag] != null) {
                $scope.filterState[filterTag].active = !$scope.filterState[filterTag].active;
            } else {
                $scope.filterState[filterTag] = {active: false, count: 0};
            }

            $scope.createView($scope.dataView.raw);
        };

        $scope.showTab = function (tabId) {
            $timeout(function () {
                $scope.tabId = tabId;
                $scope.createView($scope.dataView.raw);
            });
        };

        $scope.selectAll = function () {
            $timeout(function () {
                for (var i = 0; i < $scope.dataView.raw.data.active.length; i++) {
                    if ($scope.dataView.raw.data.active[i].tab.id == $scope.tabId) {
                        $scope.dataView.raw.data.active[i].data.map(
                            function (link) {
                                link.selected = true;
                            });
                    }
                }
                $scope.updateSelectionCount();
            });
        };

        var getSelectedLinks = function () {
            if ($scope.dataView.filtered == null || $scope.dataView.filtered.links.length == 0) return [];

            return [].concat($scope.dataView.filtered.links.filter(
                function (link) {
                    if (link.selected) {
                        var filters = FilterService.getMatchingFilters(link.linkText);
                        var excluded = filters != null && $scope.filterState[filters[0].tag] != null && !$scope.filterState[[filters[0].tag]].active;
                        if (!excluded) {
                            return link;
                        }
                    }
                }));
        };

        $scope.updateSelectionCount = function () {
            $scope.dataView.selectionCount = getSelectedLinks().length;
        };

        $scope.clearSelection = function () {
            $timeout(function () {
                for (var i = 0; i < $scope.dataView.raw.data.active.length; i++) {
                    if ($scope.dataView.raw.data.active[i].tab.id == $scope.tabId) {
                        $scope.dataView.raw.data.active[i].data.map(
                            function (link) {
                                link.selected = false;
                            });
                    }
                }
                $scope.updateSelectionCount();
            });
        };

        ExtensionMessagingService.sendMessage("autograbber", "get-active-tabs")
            .then(function (response) {
                $scope.createView(response);
            });

        ExtensionMessagingService.addListener("autograbber-view", "switch-tab", function (request, sender, sendResponse) {
            if (request.data != null && request.data.id != null) {
                $scope.showTab(request.data.id);
            }
        });
    }
    ]);