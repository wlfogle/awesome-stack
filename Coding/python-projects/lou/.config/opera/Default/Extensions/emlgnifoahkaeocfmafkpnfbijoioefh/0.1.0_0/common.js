'use strict';

var Native = function () {
  this.callback = null;
  this.channel = chrome.runtime.connectNative('com.add0n.node');

  function onDisconnect () {
    chrome.tabs.create({
      url: './data/helper/index.html'
    });
  }

  this.channel.onDisconnect.addListener(onDisconnect);
  this.channel.onMessage.addListener(res => {
    if (!res) {
      chrome.tabs.create({
        url: './data/helper/index.html'
      });
    }
    else if (res.code && (res.code !== 0 && (res.code !== 1 || res.stderr !== ''))) {
      window.alert(`Something went wrong!

-----
Code: ${res.code}
Output: ${res.stdout}
Error: ${res.stderr}`
      );
    }
    else if (this.callback) {
      this.callback(res);
    }
    else {
      console.error(res);
    }
  });
};
Native.prototype.exec = function (command, args, callback = function () {}) {
  this.callback = function (res) {
    callback(res);
  };
  this.channel.postMessage({
    cmd: 'exec',
    command,
    arguments: args
  });
};
// for Linux and Mac only
Native.prototype.ps = function (callback = function () {}) {
  this.callback = function (res) {
    if (res.stdout && res.stdout.indexOf('TorBrowser') !== -1) {
      callback(true);
    }
    else {
      callback(false);
    }
  };
  this.channel.postMessage({
    cmd: 'exec',
    command: 'ps',
    arguments: ['aux']
  });
};

function open (url, native) {
  // Mac
  if (navigator.userAgent.indexOf('Mac') !== -1) {
    native.ps(running => {
      if (running) {
        native.exec('open', ['-a', 'TorBrowser', url]);
      }
      else {
        native.exec('open', ['-a', 'TorBrowser', '--args', '-url', url]);
      }
    });
  }
  // Linux
  else if (navigator.userAgent.indexOf('Linux') !== -1) {
    chrome.storage.local.get({
      path: null
    }, prefs => {
      native.exec(prefs.path || 'TorBrowser', ['-allow-remote', '-url', url]);
    });
  }
  // Windows
  else {
    chrome.storage.local.get({
      path: 'C:\\Program Files (x86)\\Tor Browser\\Browser\\firefox.exe'
    }, prefs => {
      if (prefs.path) {
        native.exec(prefs.path, ['-allow-remote', '-url', url]);
      }
      else {
        native.exec('cmd', ['/s/c', `start /d "" TorBrowser -allow-remote -url "${url}"`]);
      }
    });
  }
}

(function (callback) {
  if (navigator.userAgent.indexOf('Firefox') === -1) {
    chrome.runtime.onInstalled.addListener(callback);
  }
  else {
    callback();
  }
})(function () {
  chrome.contextMenus.create({
    id: 'open-in',
    title: 'Open in Tor Browser',
    contexts: ['link'],
    documentUrlPatterns: ['*://*/*']
  });
});

function analyze (url) {
  let native = new Native();

  open(url, native);
}

chrome.contextMenus.onClicked.addListener(info => {
  analyze(info.linkUrl);
});
chrome.runtime.onMessage.addListener(request => {
  if (request.cmd === 'open-in') {
    analyze(request.url);
  }
});
chrome.browserAction.onClicked.addListener(tab => {
  analyze(tab.url);
});

// FAQs & Feedback
chrome.storage.local.get({
  'version': null,
  'faqs': navigator.userAgent.toLowerCase().indexOf('firefox') === -1 ? true : false
}, prefs => {
  let version = chrome.runtime.getManifest().version;

  if (prefs.version ? (prefs.faqs && prefs.version !== version) : true) {
    chrome.storage.local.set({version}, () => {
      chrome.tabs.create({
        url: 'http://add0n.com/open-in.html?from=tor&version=' + version +
          '&type=' + (prefs.version ? ('upgrade&p=' + prefs.version) : 'install')
      });
    });
  }
});

(function () {
  let {name, version} = chrome.runtime.getManifest();
  chrome.runtime.setUninstallURL('http://add0n.com/feedback.html?name=' + name + '&version=' + version);
})();
