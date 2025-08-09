'use strict';

angular.module('myjdWebextensionApp')
    .directive('myAutograbberListitem', function () {
        return {
            controller: 'AutograbberListitemController',
            scope: {
                tabData: '='
            },
            templateUrl: 'partials/directives/myautograbberlistitem.html'
        }
    });