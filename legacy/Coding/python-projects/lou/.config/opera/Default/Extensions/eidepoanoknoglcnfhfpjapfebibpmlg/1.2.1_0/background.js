
whoisTools();
function whoisTools() {
  chrome.contextMenus.create({
      "id": "whois",
      "title": "Who is this domain",
      "contexts": ["page"],
      "onclick" : onWhoisdomain
    });
}

waybackMachine();
function waybackMachine() {
  chrome.contextMenus.create({
      "id": "wayback",
      "title": "Way back machine this page",
      "contexts": ["page"],
      "onclick" : onWayback
    });
}
var extractHostname = function(url) {
  var hostname;
  if (url.indexOf("://") > -1) {
    hostname = url.split('/')[2];
  } else {
    hostname = url.split('/')[0];
  }
  hostname = hostname.split(':')[0];
  hostname = hostname.split('?')[0];
  return hostname;
}
function onWayback(info, tab) {
  if (info.menuItemId == "wayback"){
    var getURL = "https://web.archive.org/web/*/" + decodeURIComponent(tab.url);
    chrome.tabs.create({ url: getURL });
  }
}

function onWhoisdomain(info, tab) {
  if (info.menuItemId == "whois"){
    var wURL = "http://whois.domaintools.com/" + decodeURIComponent(extractHostname(tab.url));
    chrome.tabs.create({ url: wURL });
  }
}
