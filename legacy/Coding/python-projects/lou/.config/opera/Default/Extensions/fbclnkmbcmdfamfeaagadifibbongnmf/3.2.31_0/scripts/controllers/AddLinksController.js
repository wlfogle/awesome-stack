"use strict";

angular.module('myjdWebextensionApp')
    .controller('AddLinksCtrl', ['$rootScope', '$routeParams', '$scope', '$timeout', '$interval', 'RequestQueueEventService', 'CnlService', 'ApiErrorService', 'StorageService', 'BackgroundScriptService', 'ExtensionI18nService', function ($rootScope, $routeParams, $scope, $timeout, $interval, requestQueueEventService, cnlService, apiErrorService, storageService, BackgroundScriptService, ExtensionI18nService) {
        $scope.SaveForLaterDevice = storageService.SaveForLaterDevice;

        $scope.priorityValues = storageService.remotePriorityValues;

        $scope.optionalDefaults = {};

        $scope.devices = [];

        $scope.requestStates = {
            IDLE: "IDLE",
            SUCCESS: "SUCCESS",
            RUNNING: "RUNNING",
            ERROR: "ERROR"
        };

        $scope.$on(requestQueueEventService.Events.COUNTDOWN_FINISHED_SEND_LINKS, function () {
            $timeout(function () {
                $scope.selection.showOptionalValues = false;
                $scope.send();
            }, 0);
        });

        function resetScope() {
            $scope.state = {
                loading: false,
                sending: {state: $scope.requestStates.IDLE},
                loggedIn: false,
                removeAfterSend: true,
                requestQueue: []
            };

            $scope.selection = {};

            $scope.history = {archivepw: [], downloadpw: [], packageName: [], saveto: [], priority: []};
        }

        chrome.runtime.onMessage.addListener(function (msg) {
            if (msg) {
                if (msg.type && msg.type === "CONNECTION_STATE_CHANGE") {
                    if (msg.value && msg.value === "CONNECTED") {
                        $timeout(function () {
                            $scope.state.loggedIn = true;
                        }, 0);
                    } else if (msg.value && msg.value === "DISCONNECTED") {
                        $timeout(function () {
                            $scope.state.loggedIn = true;
                        }, 0);
                    }
                }
            }
        });

        var loadingIndicatorTimeout;

        var loadDeviceList = function () {
            BackgroundScriptService.getDevices().then(function (result) {
                setLoading(false);
                $timeout(function () {
                    if (!result.data.error && result.data.devices) {
                            $scope.$emit(requestQueueEventService.Events.DEVICES_RECEIVED, {});
                        $scope.devices.splice(0, $scope.devices.length);
                        if ($.isArray(result.data.devices)) {
                            $scope.state.lastDeviceReloadResult = result.data.devices.length + " JDs " + ExtensionI18nService.getMessage("ui_add_links_refresh_jd_found");
                            var devices = [].concat(result.data.devices);
                            $scope.devices = $scope.devices.concat(devices);
                            if ($scope.showSaveForLater && $scope.devices.indexOf($scope.SaveForLaterDevice) === -1) {
                                $scope.devices.push($scope.SaveForLaterDevice);
                            }
                        } else {
                            $scope.state.lastDeviceReloadResult = ExtensionI18nService.getMessage("ui_add_links_refresh_no_jd_found");
                        }
                        $timeout(function () {
                            $scope.state.lastDeviceReloadResult = "";
                        }, 2000);
                    } else {
                        $scope.error = result.data.error || ExtensionI18nService.getMessage("ui_add_links_refresh_failed_to_get_jds");
                    }
                }, 0);
            }).catch(function () {
                setLoading(false);
                $timeout(function () {
                    $scope.error = ExtensionI18nService.getMessage("ui_add_links_refresh_failed_to_get_jds");
                }, 0);
            });
        };

        var loadSessionInfo = function () {
            BackgroundScriptService.getSessionInfo().then(function (result) {
                $scope.state.isConnecting = false;
                $timeout(function () {
                    $scope.$emit(requestQueueEventService.Events.SESSION_RECEIVED, {});
                }, 0);
                setLoading(false);
                if (result.data.isLoggedIn) {
                    $timeout(function () {
                        $scope.state.isLoggedIn = true;
                    }, 0);
                    storageService.get(storageService.STORAGE_DEVICE_LIST_KEY, function (result) {
                        $timeout(function () {
                            $scope.devices = result[storageService.STORAGE_DEVICE_LIST_KEY] || [];
                            if ($scope.showSaveForLater && $scope.devices.indexOf($scope.SaveForLaterDevice) === -1) {
                                $scope.devices.push($scope.SaveForLaterDevice);
                            }
                            if (!$scope.selection.device) {
                                $scope.selection.device = $scope.devices[0];
                            }
                            $timeout(function () {
                                $scope.$emit(requestQueueEventService.Events.DEVICES_RECEIVED, {});
                            }, 0);
                            restoreOptionsAndHistory(function () {

                            });
                        }, 0);
                    });
                } else {
                    $timeout(function () {
                        $scope.state.isLoggedIn = false;
                    }, 0);
                    resetScope();
                }
            }).catch(function () {
                $timeout(function () {
                    $scope.state.isLoggedIn = false;
                }, 0);
                resetScope();
            });
        };

        function initDefaultOptionalValues() {
            storageService.getSettings(function (settings) {
                if (settings) {
                    $scope.optionalDefaults = settings;
                }
            });
        }

        function init() {
            setLoading(true);
            $scope.selection.device = $scope.device;
            initDefaultOptionalValues();
            loadSessionInfo();
            loadDeviceList();
        }


        function setLoading(loading) {
            if (loading) {
                loadingIndicatorTimeout = $timeout(function () {
                    $scope.state.loading = true;
                }, 1000);
            } else {
                if (loadingIndicatorTimeout) {
                    $timeout.cancel(loadingIndicatorTimeout);
                }
                $timeout(function () {
                    $scope.state.loading = false;
                }, 0);
            }
        }

        resetScope();
        init();

        $scope.selectDevice = function (device) {
            $scope.selection.device = device;
            if ($scope.cachedHistory === undefined) {
                $scope.cachedHistory = {};
            }
            $scope.history = $scope.cachedHistory[$scope.selection.device.id] || {
                archivepw: [],
                downloadpw: [],
                packageName: [],
                saveto: [],
                priority: []
            };
        };

        $scope.toggleOptionalValues = function () {
            var prevValue = $scope.selection.showOptionalValues || false;
            $scope.selection.showOptionalValues = !prevValue;
        };

        $scope.refreshDevices = function () {
            loadDeviceList();
        };

        function addToHistory(key, value) {
            if ($scope.history[key] === undefined) {
                $scope.history[key] = [];
            }
            if ($.inArray(value, $scope.history[key]) === -1) {
                $scope.history[key].unshift(value);
            }
        }

        $scope.saveOptions = function () {
            var options = {
                removeAfterSend: $scope.selection.removeAfterSend,
                autoFill: $scope.selection.autoFill,
                showOptionalValues: $scope.selection.showOptionalValues
            };
            storageService.set(storageService.ADD_LINK_CACHED_OPTIONS, options, function () {
                // Nothing to do
            });
        };

        function saveOptionsAndHistory(callback) {
            if ($scope.selection.comment !== undefined) {
                addToHistory("comment", $scope.selection.comment);
            }
            if ($scope.selection.packageName !== undefined) {
                addToHistory("packageName", $scope.selection.packageName);
            }
            if ($scope.selection.saveto !== undefined) {
                addToHistory("saveto", $scope.selection.saveto);
            }
            if ($scope.selection.archivepw !== undefined) {
                addToHistory("archivepw", $scope.selection.archivepw);
            }
            if ($scope.selection.downloadpw !== undefined) {
                addToHistory("downloadpw", $scope.selection.downloadpw);
            }

            storageService.get([storageService.ADD_LINK_CACHED_OPTIONS, storageService.ADD_LINK_CACHED_HISTORY], function (result) {
                // Save options
                var options = {
                    deepDecrypt: $scope.selection.deepDecrypt,
                    autoExtract: $scope.selection.autoExtract,
                    autoStart: $scope.selection.autoStart,
                    overwritePackagizer: $scope.selection.overwritePackagizer,
                    device: $scope.selection.device,
                    removeAfterSend: $scope.selection.removeAfterSend,
                    autoFill: $scope.selection.autoFill,
                    showOptionalValues: $scope.selection.showOptionalValues
                };
                storageService.set(storageService.ADD_LINK_CACHED_OPTIONS, options, function () {
                    // Save value history
                    if (!result.ADD_LINK_CACHED_HISTORY) {
                        result.ADD_LINK_CACHED_HISTORY = {};
                    }
                    if (!result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id]) {
                        result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id] = {};
                    }
                    result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].comment = $scope.history.comment;
                    result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].archivepw = $scope.history.archivepw;
                    result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].downloadpw = $scope.history.downloadpw;
                    result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].packageName = $scope.history.packageName;
                    result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].saveto = $scope.history.saveto;
                    result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].priority = $scope.selection.priority;
                    storageService.set(storageService.ADD_LINK_CACHED_HISTORY, result.ADD_LINK_CACHED_HISTORY, callback);
                });
            });
        }

        var restoreDefaultValues = function () {
            var autoExtract = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_AUTOEXTRACT.key];
            if (autoExtract !== 'unset') {
                $scope.selection.autoExtract = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_AUTOEXTRACT.key];
            }
            var autoStart = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_AUTOSTART.key];
            if (autoStart !== 'unset') {
                $scope.selection.autoStart = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_AUTOSTART.key];
            }
            var deepDecrypt = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_DEEPDECRYPT.key];
            if (deepDecrypt !== 'unset') {
                $scope.selection.deepDecrypt = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_DEEPDECRYPT.key];
            }
            var overwritePackagizer = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_OVERWRITE_PACKAGIZER.key];
            if (overwritePackagizer !== 'unset') {
                $scope.selection.overwritePackagizer = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_OVERWRITE_PACKAGIZER.key];
            }
            var priority = $scope.optionalDefaults[storageService.settingsKeys.DEFAULT_PRIORITY.key];
            if (priority && priority.id != storageService.priorityValues.UNSET.id) {
                $scope.selection.priority = priority;
            } else {
                $scope.selection.priority = storageService.priorityValues.DEFAULT;
            }
        };

        function restoreOptionsAndHistory(callback) {
            storageService.get([storageService.ADD_LINK_CACHED_OPTIONS, storageService.ADD_LINK_CACHED_HISTORY], function (result) {
                $timeout(function () {
                    if (result && result.ADD_LINK_CACHED_OPTIONS) {
                        $scope.selection.autoFill = result.ADD_LINK_CACHED_OPTIONS.autoFill;
                        //$scope.selection.showOptionalValues = result.ADD_LINK_CACHED_OPTIONS.showOptionalValues;
                        $scope.selection.removeAfterSend = result.ADD_LINK_CACHED_OPTIONS.removeAfterSend;
                        if ($scope.selection.autoFill) {
                            $scope.selection.autoExtract = result.ADD_LINK_CACHED_OPTIONS.autoExtract;
                            $scope.selection.autoStart = result.ADD_LINK_CACHED_OPTIONS.autoStart;
                            $scope.selection.deepDecrypt = result.ADD_LINK_CACHED_OPTIONS.deepDecrypt;
                            $scope.selection.overwritePackagizer = result.ADD_LINK_CACHED_OPTIONS.overwritePackagizer;
                            //$scope.selection.showOptionalValues = result.ADD_LINK_CACHED_OPTIONS.showOptionalValues;
                        } else {
                            // Fall back to defaults, if any
                            restoreDefaultValues();
                        }
                        $scope.optionsHistory = result.ADD_LINK_CACHED_OPTIONS;
                    }

                    if ($scope.selection.device) {
                        if (result.ADD_LINK_CACHED_HISTORY && result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id]) {
                            if ($scope.selection.autoFill && result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id]) {
                                $scope.selection.saveto = (result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].saveto && result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].saveto.length > 0) ? result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].saveto[0] : undefined;
                                $scope.selection.packageName = (result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].packageName && result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].packageName.length > 0) ? result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].packageName[0] : undefined;
                                $scope.selection.archivepw = (result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].archivepw && result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].archivepw.length > 0) ? result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].archivepw[0] : undefined;
                                $scope.selection.downloadpw = (result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].downloadpw && result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].downloadpw.length > 0) ? result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].downloadpw[0] : undefined;
                                $scope.selection.comment = (result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].comment && result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].comment.length > 0) ? result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].comment[0] : undefined;
                                $scope.selection.priority = setIfUndefined(result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id].priority, $scope.priorityValues.DEFAULT);
                            }
                            $scope.history = result.ADD_LINK_CACHED_HISTORY[$scope.selection.device.id];
                        }
                        $scope.cachedHistory = result.ADD_LINK_CACHED_HISTORY || {};
                    }
                    if (callback) {
                        callback();
                    }
                }, 0);
            });
        }

        $scope.clearAllOptionalValues = function () {
            $timeout(function () {
                $scope.selection.saveto = undefined;
                $scope.selection.comment = undefined;
                $scope.selection.packageName = undefined;
                $scope.selection.archivepw = undefined;
                $scope.selection.downloadpw = undefined;
                $scope.selection.priority = $scope.priorityValues.DEFAULT;
                $scope.selection.autoExtract = undefined;
                $scope.selection.autoStart = undefined;
                $scope.selection.deepDecrypt = undefined;
                $scope.selection.overwritePackagizer = undefined;
            }, 0);
        };

        $scope.autoFillOptionalValues = function (checkIfActive) {
            if (checkIfActive && !$scope.selection.autoFill) {
                return;
            }
            $timeout(function () {
                if ($scope.selection.device && $scope.history) {
                    $scope.selection.saveto = setIfUndefined($scope.selection.saveto, $scope.history.saveto[0]);
                    $scope.selection.packageName = setIfUndefined($scope.selection.packageName, $scope.history.packageName[0]);
                    $scope.selection.archivepw = setIfUndefined($scope.selection.archivepw, $scope.history.archivepw[0]);
                    $scope.selection.downloadpw = setIfUndefined($scope.selection.downloadpw, $scope.history.downloadpw[0]);
                    if ($scope.history.priority) {
                        $scope.selection.priority = $scope.history.priority;
                    }
                }

                if ($scope.optionsHistory) {
                    $scope.selection.autoExtract = setIfUndefined($scope.selection.autoExtract, $scope.optionsHistory.autoExtract);
                    $scope.selection.autoStart = setIfUndefined($scope.selection.autoStart, $scope.optionsHistory.autoStart);
                    $scope.selection.deepDecrypt = setIfUndefined($scope.selection.deepDecrypt, $scope.optionsHistory.deepDecrypt);
                    $scope.selection.overwritePackagizer = setIfUndefined($scope.selection.overwritePackagizer, $scope.optionsHistory.overwritePackagizer);
                }
            }, 0);
        };

        $scope.autoFillDefaultValues = function () {
            $scope.clearAllOptionalValues();
            $timeout(function () {
                restoreDefaultValues();
            }, 0);
        };

        function setIfUndefined(variable, value) {
            return variable === undefined ? value : variable;
        }

        function sendAddLinkQueries(addLinksQueries, callback) {
            if (addLinksQueries.length > 0) {
                BackgroundScriptService.addLink($scope.selection.device, addLinksQueries[0]).then(function () {
                    addLinksQueries.splice(0, 1);
                    sendAddLinkQueries(addLinksQueries, callback);
                }).catch(function (e) {
                    $scope.state.sending.state = $scope.requestStates.ERROR;
                    if (e !== undefined) {
                        $scope.state.errorMessage = JSON.stringify(e);
                        $scope.state.sending.log = JSON.stringify(e);
                    }
                });
            } else if (callback) {
                callback();
            }
        }

        function sendCnlQueries(cnlQueries, callback) {
            if (cnlQueries.length > 0) {
                BackgroundScriptService.addCnl($scope.selection.device, cnlQueries[0]).then(function () {
                    cnlQueries.splice(0, 1);
                    sendCnlQueries(cnlQueries, callback);
                }).catch(function (e) {
                    $scope.state.sending.state = $scope.requestStates.ERROR;
                    $scope.state.sending.log = JSON.stringify(e);
                });
            } else if (callback) {
                callback();
            }
        }

        var successClose = function (idsToRemove) {
            $timeout(function () {
                $scope.state.sending.state = $scope.requestStates.SUCCESS;
                $scope.$emit(requestQueueEventService.Events.HIDE_REQUEST_QUEUE);
            }, 0);
            $timeout(function () {
                $scope.$emit(requestQueueEventService.Events.ADD_LINKS_DIALOG_CLOSE, idsToRemove);
            }, 0);
        };

        $scope.send = function (device) {
            if (device !== undefined) {
                $scope.selection.device = device;
            }
            if ($scope.selection.device === undefined) {
                $scope.selection.device = $scope.SaveForLaterDevice;
            }
            if ($scope.state.sending.state === $scope.requestStates.RUNNING) {
                return;
            }
            if ($scope.selection.device.id === $scope.SaveForLaterDevice.id) {
                saveOptionsAndHistory(function () {
                    addRequestsToSaveForLater($scope.requests, function () {
                        $scope.clearAllOptionalValues();
                        $scope.state.sending.state = $scope.requestStates.SUCCESS;
                        successClose([]);
                    });
                });
            } else {
                $timeout(function () {
                    $scope.state.sending.state = $scope.requestStates.RUNNING
                }, 0);

                var addLinksQueries = [];
                var cnlQueries = [];

                $.each($scope.requests, function (index, request) {
                    var query = {};

                    if ($scope.selection.comment !== undefined) {
                        query.comment = $scope.selection.comment;
                    }
                    if ($scope.selection.saveto !== undefined) {
                        query.destinationFolder = $scope.selection.saveto;
                    }
                    if ($scope.selection.archivepw !== undefined) {
                        query.extractPassword = $scope.selection.archivepw;
                    }
                    if ($scope.selection.downloadpw !== undefined) {
                        query.downloadPassword = $scope.selection.downloadpw;
                    }
                    if ($scope.selection.deepDecrypt !== undefined) {
                        query.deepDecrypt = $scope.selection.deepDecrypt;
                    }
                    if ($scope.selection.autoExtract !== undefined) {
                        query.autoExtract = $scope.selection.autoExtract;
                    }
                    if ($scope.selection.autoStart !== undefined) {
                        query.autostart = $scope.selection.autoStart;
                    }
                    if ($scope.selection.priority !== undefined) {
                        query.priority = $scope.selection.priority.value;
                    }
                    if ($scope.selection.packageName !== undefined) {
                        query.packageName = $scope.selection.packageName;
                    }
                    if ($scope.selection.overwritePackagizer !== undefined) {
                        query.overwritePackagizerRules = $scope.selection.overwritePackagizer;
                    }

                    if (request.type === 'cnl') {
                        if (request.content.requestBody && request.content.requestBody.formData) {
                            var data = request.content.requestBody.formData;
                            var dummyCnl = Object.create(null);
                            if (Array.isArray(data.crypted) && data.crypted[0] !== undefined) {
                                dummyCnl["crypted"] = data.crypted[0];
                            } else {
                                dummyCnl["crypted"] = data.crypted;
                            }
                            if (Array.isArray(data.jk) && data.jk[0] !== undefined) {
                                dummyCnl["jk"] = data.jk[0];
                            } else {
                                dummyCnl["jk"] = data.jk;
                            }
                            if (Array.isArray(data.k) && data.k[0] !== undefined) {
                                dummyCnl["k"] = data.k[0];
                            } else {
                                dummyCnl["k"] = data.k;
                            }
                            if (Array.isArray(data.source) && data.source[0] !== undefined) {
                                dummyCnl["source"] = data.source[0];
                            } else {
                                dummyCnl["source"] = data.source;
                            }
                            var hexDummyCnl = CryptoJS.enc.Utf8.parse(JSON.stringify(dummyCnl)).toString(CryptoJS.enc.Hex);
                            var dummyCnlUrl = "http://dummycnl.jdownloader.org/" + encodeURIComponent(hexDummyCnl);
                            query.links = dummyCnlUrl;
                        }

                        if (request.content.url !== undefined) {
                            query.sourceUrl = request.content.url;
                        }
                        if (request.parent.url !== undefined) {
                            query.sourceUrl = request.parent.url;
                        }
                        if (request.content.source !== undefined) {
                            query.sourceUrl = request.content.source;
                        }
                        if (request.content.urls !== undefined) {
                            query.links = query.links + "\r\n" + request.content.urls;
                        }
                        if (request.content.passwords !== undefined) {
                            if (query.downloadPassword !== undefined) {
                                query.downloadPassword = query.downloadPassword.concat(request.content.passwords).join(" ");
                            } else {
                                query.downloadPassword = (request.content.passwords != null && $.isArray(request.content.passwords)) ? request.content.passwords.join(" ") : "";
                            }
                        }
                        query.permission = true;
                        addLinksQueries.push(query);
                    } else if (request.type === 'text' || request.type === 'link') {
                        query.links = "";
                        if (request.previewText) {
                            query.links = request.previewText + "\n\n";
                        }
                        query.links += request.content;

                        if (request.parent.url !== undefined) {
                            query.sourceUrl = request.parent.url;
                        }
                        addLinksQueries.push(query);
                    }
                });

                saveOptionsAndHistory(function () {
                    if (addLinksQueries.length > 0 || cnlQueries.length > 0) {
                        var requestsState = {
                            ADD_LINKS: addLinksQueries.length > 0 ? $scope.requestStates.RUNNING : $scope.requestStates.SUCCESS,
                            CNL: cnlQueries.length > 0 ? $scope.requestStates.RUNNING : $scope.requestStates.SUCCESS
                        };

                        var donecallback = function () {
                            var idsToRemove = [];
                            if ($scope.selection.removeAfterSend) {
                                $.each($scope.requests, function (index, request) {
                                    idsToRemove.push(request.id);
                                });
                            }

                            successClose(idsToRemove);
                        };

                        if (cnlQueries.length > 0) {
                            sendCnlQueries(cnlQueries, function () {
                                requestsState.CNL = $scope.requestStates.SUCCESS;
                                if (requestsState.ADD_LINKS !== $scope.requestStates.RUNNING) {
                                    donecallback();
                                }
                            });
                        }
                        if (addLinksQueries.length > 0) {
                            sendAddLinkQueries(addLinksQueries, function () {
                                requestsState.ADD_LINKS = $scope.requestStates.SUCCESS;
                                if (requestsState.CNL !== $scope.requestStates.RUNNING) {
                                    donecallback();
                                }
                            });
                        }
                    }
                });
            }
        };

        function addRequestsToSaveForLater(requests, callback) {
            storageService.get(storageService.CLIPBOARD_HISTORY, function (result) {
                if (!result.CLIPBOARD_HISTORY) {
                    result.CLIPBOARD_HISTORY = {};
                }

                $.each(requests, function (index, value) {
                    result.CLIPBOARD_HISTORY[value.id] = value;
                });

                storageService.set(storageService.CLIPBOARD_HISTORY, result.CLIPBOARD_HISTORY, function () {
                    if (callback) {
                        callback();
                    }
                });
            });
        }

        $scope.translatedPriority = function (prio) {
            return ExtensionI18nService.getMessage("ui_add_links_prio_" + prio.toLowerCase());
        };

        $scope.$on("$destroy", function () {
            if (loadingIndicatorTimeout) {
                $timeout.cancel(loadingIndicatorTimeout);
            }
            if ($scope.connectionSubscription) {
                $scope.connectionSubscription.dispose();
                delete $scope.connectionSubscription;
            }
        }.bind(this));
    }]);
