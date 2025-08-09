/* global Parser */

self.importScripts('termlib/termlib_parser.js');

const storage = {
  get(prefs) {
    return new Promise(resolve => chrome.storage.local.get(prefs, resolve));
  }
};

const isRunning = () => storage.get({
  remote: 'http://127.0.0.1:9666/'
}).then(prefs => {
  const controller = new AbortController();
  setTimeout(() => controller.abort(), 2000);

  return fetch(prefs.remote + 'flash/', {
    signal: controller.signal
  }).then(() => true, () => false);
});

const config = {};

config.command = {
  executable: {
    Mac: 'open',
    Win: '%LocalAppData%\\JDownloader 2.0\\JDownloader2.exe',
    Lin: 'JDownloader2'
  },
  args: {
    Mac: '-a "JDownloader2"',
    Win: '',
    Lin: ''
  },
  get guess() {
    const key = navigator.platform.substr(0, 3);
    return {
      executable: config.command.executable[key],
      args: config.command.args[key]
    };
  }
};

config.post = {
  method: 'POST',
  action: (d, tab) => getCookies(d.referrer).then(async cookies => {
    const prefs = await storage.get({
      'delay': 5000,
      'autostart': 1,
      'engine': 'flash/add',
      'remote': 'http://127.0.0.1:9666/'
    });

    let index = 0;
    const delay = () => new Promise(resolve => setTimeout(resolve, prefs.delay));

    const body = new URLSearchParams();

    body.append('urls', d.finalUrl || d.url);
    body.append('referer', d.referrer || '');
    body.append('autostart', prefs.autostart);
    body.append('package', tab.title || '');
    body.append('description', 'Initiated by ' + chrome.runtime.getManifest().name);
    body.append('cookies', cookies);
    body.append('fnames', (d.filename || '').split(/[/\\]/).pop());
    body.append('source', chrome.runtime.getURL(''));

    const once = () => fetch(config.post.url, {
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8'
      },
      method: 'post',
      body
    }).then(r => {
      r.text().then(c => console.info(c));
      if (!r.ok) {
        throw new Error('Connection is rejected by JDownloader');
      }
    }).catch(e => {
      index += 1;
      if (index < 20 && e.message !== 'Connection is rejected by JDownloader') {
        return delay().then(once);
      }
      throw new Error(
        'Cannot send command to JDownloader; Make sure ' +
        config.post.url +
        ' is accessible'
      );
    });

    config.post.url = prefs.remote + prefs.engine;
    return once();
  })
};

const locales = {
  extract: 'Cannot extract any link from selected text',
  empty: 'Command box is empty. Use options page to define one!',
  path: 'JDownloader cannot be located. Go to the options page and fix its path.',
  permission: 'To extract links from all iframes of this page, the permission is needed',
  help: `
    JDownloader -status queue
    JDownloader -status downloads
    JDownloader -status all
    JDownloader -status watchfolder
  `
};

const notify = e => chrome.notifications.create({
  type: 'basic',
  title: chrome.runtime.getManifest().name,
  iconUrl: '/data/icons/48.png',
  message: e.message || e
});

const openWithJD = async (d, tab = {}) => {
  try {
    if (await isRunning() !== true) {
      await execute(d);
    }
    await config.post.action(d, tab);
    if (d.id) {
      chrome.downloads.erase({
        id: d.id
      });
    }
  }
  catch (e) {
    notify(e);
  }
};

const getCookies = url => {
  if (!url || !chrome.cookies) {
    return Promise.resolve('');
  }
  return storage.get({
    cookies: false
  }).then(prefs => {
    if (prefs.cookies) {
      return new Promise(resolve => chrome.cookies.getAll({
        url
      }, arr => resolve(arr.map(o => o.name + '=' + o.value).join('; '))));
    }
    return '';
  });
};

const execute = d => {
  return new Promise((resolve, reject) => {
    storage.get({
      ...config.command.guess,
      delay: 5000
    }).then(prefs => {
      if (!prefs.executable) {
        return notify(locales.empty);
      }
      const p = new Parser();
      p.escapeExpressions = {}; // do not escape expressions
      getCookies(d.referrer).then(cookies => {
        // remove args that are not provided
        if (!d.referrer) {
          prefs.args = prefs.args.replace(/\s[^\s]*\[REFERRER\][^\s]*\s/, ' ');
        }
        if (!d.filename) {
          prefs.args = prefs.args.replace(/\s[^\s]*\[FILENAME\][^\s]*\s/, ' ');
          prefs.args = prefs.args.replace(/\s[^\s]*\[DISK][^\s]*\s/, ' ');
        }

        const url = d.finalUrl || d.url;

        if (!cookies) {
          prefs.args = prefs.args.replace('--load-cookies=[COOKIES]', '');
        }
        const termref = {
          lineBuffer: prefs.args
            .replace(/\[URL\]/g, url)
            .replace(/\[REFERRER\]/g, d.referrer)
            .replace(/\[USERAGENT\]/g, navigator.userAgent)
            .replace(/\[FILENAME\]/g, (d.filename || '').split(/[/\\]/).pop())
            .replace(/\[DISK\]/g, (d.filename || ''))
            .replace(/\\/g, '\\\\')
        };

        p.parseLine(termref);

        setTimeout(resolve, prefs.delay);
        const script = String.raw`
          const cookies = args[0];
          const command = args[1].replace(/%([^%]+)%/g, (_, n) => {
            if (n === 'ProgramFiles(x86)' && !env[n]) {
              return env['ProgramFiles'];
            }
            return env[n];
          });
          function execute () {
            const exe = require('child_process').spawn(command, args.slice(2), {
              detached: true
            });
            let stdout = '', stderr = '';
            exe.stdout.on('data', data => stdout += data);
            exe.stderr.on('data', data => stderr += data);

            stdout += command;
            exe.on('close', code => {
              push({code, stdout, stderr});
              done();
            });
          }

          if (cookies) {
            const filename = require('path').join(
              require('os').tmpdir(),
              'download-with-' + require('crypto').randomBytes(4).readUInt32LE(0) + ''
            );
            require('fs').writeFile(filename, cookies, e => {
              if (e) {
                push({code: 1, stderr: 'cannot create tmp file'});
                done();
              }
              else {
                args = args.map(s => s.replace(/\[COOKIES\]/g, filename));
                execute();
              }
            });
          }
          else {
            args = args.map(s => s.replace(/\[COOKIES\]/g, '.'));
            execute();
          }
        `;
        const args = [cookies, prefs.executable, ...termref.argv];
        chrome.runtime.sendNativeMessage('com.add0n.native_client', {
          permissions: ['fs', 'path', 'os', 'child_process', 'crypto'],
          args,
          script
        }, res => {
          if (!res) {
            chrome.tabs.create({url: '/data/guide/index.html'});
            return reject(Error('got empty response'));
          }
          else if (res.code !== 0) {
            const msg = res.stderr || res.error || res.err;
            console.warn(msg);
            if (msg && msg.indexOf('ENOENT') !== -1) {
              return reject(Error(locales.path));
            }
            return reject(Error(msg));
          }
          else {
            resolve();
          }
        });
      });
    });
  });
};

let id;
const dObserver = (d, response = () => {}) => {
  storage.get({
    mimes: [],
    whitelist: []
  }).then(prefs => {
    if (prefs.mimes.includes(d.mime)) {
      return false;
    }
    response();
    const url = d.finalUrl || d.url;

    if (d.url.includes('belaviyo/native-client')) {
      return false;
    }
    if (id === d.id || d.error) {
      return false;
    }
    if (url.startsWith('http') || url.startsWith('ftp')) {
      if (prefs.whitelist.length) {
        const matches = [d.url, d.finalUrl, d.referrer].reduce((p, href) => {
          try {
            const {hostname} = new URL(href);

            if (prefs.whitelist.some(s => s.endsWith(hostname) || hostname.endsWith(s))) {
              return true;
            }
          }
          catch (e) {}

          return p;
        }, false);

        if (matches === false) {
          return false;
        }
      }

      chrome.downloads.pause(d.id, () => chrome.tabs.query({
        active: true,
        currentWindow: true
      }, tabs => {
        openWithJD(d, tabs && tabs.length ? tabs[0] : {});
      }));
    }
  });
};

const state = enabled => {
  chrome.downloads.onCreated.removeListener(dObserver);
  if (enabled) {
    chrome.downloads.onCreated.addListener(dObserver);
  }
  const path = {
    '16': 'data/icons/' + (enabled ? '' : 'disabled/') + '16.png',
    '32': 'data/icons/' + (enabled ? '' : 'disabled/') + '32.png',
    '48': 'data/icons/' + (enabled ? '' : 'disabled/') + '48.png'
  };
  chrome.action.setIcon({path});
  chrome.action.setTitle({
    title: `${chrome.runtime.getManifest().name} (Integration is "${enabled ? 'enabled' : 'disabled'}")`
  });
};

const onCommand = (toggle = true) => {
  storage.get({
    enabled: false
  }).then(prefs => {
    const enabled = toggle ? !prefs.enabled : prefs.enabled;
    chrome.storage.local.set({
      enabled
    });
    state(enabled);
  });
};
chrome.action.onClicked.addListener(onCommand);
onCommand(false);

// contextMenus
const buildContexts = () => storage.get({
  'context.open-link': true,
  'context.open-video': true,
  'context.grab': true,
  'context.extract': true,
  'context.test': true
}).then(prefs => {
  chrome.contextMenus.removeAll(() => {
    if (prefs['context.test']) {
      chrome.contextMenus.create({
        id: 'test',
        title: 'Open Download Test Page',
        contexts: ['action', 'page'],
        documentUrlPatterns: ['*://*/*']
      });
    }
    if (prefs['context.open-video']) {
      chrome.contextMenus.create({
        id: 'open-video',
        title: 'Download Media or Image',
        contexts: ['video', 'audio', 'image'],
        documentUrlPatterns: ['*://*/*']
      });
    }
    if (prefs['context.open-link']) {
      chrome.contextMenus.create({
        id: 'open-link',
        title: 'Download Link',
        contexts: ['link'],
        documentUrlPatterns: ['*://*/*']
      });
    }
    if (prefs['context.grab']) {
      chrome.contextMenus.create({
        id: 'grab',
        title: 'Download all Links',
        contexts: ['page', 'action'],
        documentUrlPatterns: ['*://*/*']
      });
    }
    if (prefs['context.extract']) {
      chrome.contextMenus.create({
        id: 'extract',
        title: 'Extract Links from Selection',
        contexts: ['selection'],
        documentUrlPatterns: ['*://*/*']
      });
    }
  });
});
chrome.runtime.onStartup.addListener(buildContexts);
chrome.runtime.onInstalled.addListener(buildContexts);
chrome.storage.onChanged.addListener(prefs => {
  if (Object.keys(prefs).some(s => s.startsWith('context.'))) {
    buildContexts();
  }
});

const links = {};
chrome.tabs.onRemoved.addListener(id => delete links[id]);

const grab = async (mode, tabId) => {
  try {
    const target = {tabId};

    await chrome.scripting.executeScript({
      target,
      func: mode => window.mode = mode,
      args: [mode]
    });
    await chrome.scripting.executeScript({
      target,
      files: ['/data/grab/inject.js']
    });
  }
  catch (e) {
    notify(e);
  }
};

chrome.contextMenus.onClicked.addListener((info, tab) => {
  if (info.menuItemId === 'test') {
    chrome.tabs.create({
      url: 'https://webbrowsertools.com/test-download-with/'
    });
  }
  else if (info.menuItemId === 'grab') {
    chrome.permissions.request({
      origins: ['*://*/*']
    }, () => grab('none', tab.id));
  }
  else if (info.menuItemId === 'extract') {
    const re = /(?:(?:https?|ftp|file):\/\/|www\.|ftp\.)(?:\([-A-Z0-9+&@#/%=~_|$?!:,.]*\)|[-A-Z0-9+&@#/%=~_|$?!:,.])*(?:\([-A-Z0-9+&@#/%=~_|$?!:,.]*\)|[A-Z0-9+&@#/%=~_|$])/igm;
    const es = info.selectionText.match(re) || [];
    chrome.permissions.request({
      origins: ['*://*/*']
    }, async () => {
      const target = {
        tabId: tab.id,
        frameIds: [info.frameId]
      };
      try {
        await chrome.scripting.executeScript({
          target,
          args: [es],
          func: es => window.extraLinks = es
        });
        const a = await chrome.scripting.executeScript({
          target,
          files: ['/data/grab/selection.js']
        });
        if (a && a[0]) {
          const es = a[0].result.filter((s, i, l) => s && l.indexOf(s) === i);
          if (es.length) {
            links[tab.id] = es;
            grab('serve', tab.id);
          }
          else {
            notify(locales.extract);
          }
        }
      }
      catch (e) {
        notify(e);
      }
    });
  }
  else {
    openWithJD({
      finalUrl: info.menuItemId === 'open-video' ? info.srcUrl : info.linkUrl,
      referrer: info.pageUrl
    }, tab);
  }
});

chrome.runtime.onMessage.addListener((request, sender, response) => {
  if (request.method === 'notify') {
    notify(request.message);
  }
  else if (request.method === 'exec') {
    chrome.scripting.executeScript({
      target: {
        tabId: sender.tab.id,
        allFrames: true
      },
      func: () => [
        [...document.images].map(i => i.src),
        [...document.querySelectorAll('video')].map(v => v.src),
        [...document.querySelectorAll('a')].map(a => a.href),
        [...document.querySelectorAll('source')].map(v => v.src)
      ].flat().filter(s => s && (s.startsWith('http') || s.startsWith('ftp') || s.startsWith('data:')))
    }).then(r => response(r.map(o => o.result))).catch(notify);
    return true;
  }
  else if (request.method === 'download') {
    openWithJD(Object.assign({
      referrer: sender.tab.url
    }, request.job), sender.tab);
  }
  else if (request.method === 'close-interface') {
    chrome.tabs.sendMessage(sender.tab.id, {
      cmd: 'close-interface'
    });
  }
  else if (request.method === 'extracted-links') {
    response(links[sender.tab.id] || []);
  }
});

/* FAQs & Feedback */
{
  const {management, runtime: {onInstalled, setUninstallURL, getManifest}, storage, tabs} = chrome;
  if (navigator.webdriver !== true) {
    const page = getManifest().homepage_url;
    const {name, version} = getManifest();
    onInstalled.addListener(({reason, previousVersion}) => {
      management.getSelf(({installType}) => installType === 'normal' && storage.local.get({
        'faqs': true,
        'last-update': 0
      }, prefs => {
        if (reason === 'install' || (prefs.faqs && reason === 'update')) {
          const doUpdate = (Date.now() - prefs['last-update']) / 1000 / 60 / 60 / 24 > 45;
          if (doUpdate && previousVersion !== version) {
            tabs.query({active: true, currentWindow: true}, tbs => tabs.create({
              url: page + '&version=' + version + (previousVersion ? '&p=' + previousVersion : '') + '&type=' + reason,
              active: reason === 'install',
              ...(tbs && tbs.length && {index: tbs[0].index + 1})
            }));
            storage.local.set({'last-update': Date.now()});
          }
        }
      }));
    });
    setUninstallURL(page + '&rd=feedback&name=' + encodeURIComponent(name) + '&version=' + version);
  }
}
