/* globals browser */
if (typeof opera == "undefined")
	var opera = null;
if (typeof chrome == "undefined")
	var chrome = false;

var is_chrome_store_app = (window.chrome && window.chrome.runtime && window.chrome.runtime.id == "blndkmebkmenignoajhoemebccmmfjib");

var is_edge = false;
var is_firefox = false;

// old Edge compatibility
if (!opera && (!chrome || !chrome.runtime))
{
	is_edge = true;
	chrome = (typeof browser!="undefined") ? browser : chrome;
}

var is_chropera;
var is_chredge;
var is_chrome;
var is_original_chrome;

function SetupBrowserVariables(browser_ua)
{
	browser_ua = browser_ua.toLowerCase();
	is_firefox = (browser_ua.indexOf(' firefox/') > -1);	// better method than getBrowserInfo(), because Waterfox return "Waterfox" as vendor
	is_chropera = (browser_ua.indexOf(' opr/') > -1);
	is_chredge = (browser_ua.indexOf(' edg/') > -1);
	is_chrome = !!window.chrome && !is_chropera && !is_chredge;		// TODO: make better detection, because now any Chromium browsers is_chrome :/
	is_original_chrome = (browser_ua.match(/[^ ]+\/[\d\.]+/g).filter(function(v) { return !v.match(/(mozilla|applewebkit|chrome|safari)/) } ).length === 0);
}
SetupBrowserVariables(window.navigator.userAgent);


if (is_original_chrome && window.fetch)
{
	// detect fake chrome browsers
	var funcDetectUAbyRequest = function(url)
	{
		chrome.tabs.create({url:url, active:false}, function(temp_tab)
		{
			var gotNewUA = false;
			var funcTabListener = function(tabId, info)
			{
				if (!gotNewUA && (temp_tab.id == tabId) && (info.status == "complete" || (info.url && info.url != url)))	// another url => redirect => original was loaded
				{
					gotNewUA = true;
					chrome.tabs.onUpdated.removeListener(funcTabListener);
					chrome.tabs.remove(tabId);
				}
			};
	
			chrome.tabs.onUpdated.addListener(funcTabListener);
			funcTabListener(temp_tab.id, temp_tab);		// for case when tab was already loaded before listener
		});
	};

	if (document.location.href.indexOf("/index.html") > 0)	// from background
	{
		//funcDetectUAbyRequest("https://duckduckgo.com/post2.html");	// this page load faster
	}
	else if (chrome && chrome.runtime && chrome.runtime.getBackgroundPage)	// from popup and other pages
	{
		// get from background
		chrome.runtime.getBackgroundPage(function(bg_page)
		{
			if (bg_page.is_original_chrome !== undefined)
				is_original_chrome = bg_page.is_original_chrome;
		});
	}
}

// Emulate `widget` with default preferences for new Opera and Chrome
if (typeof widget == "undefined")
	var widget = { };

// setup widget.cfg
widget.cfg = {};
if (chrome && chrome.storage && typeof chrome.storage.local != "undefined")	// new Chrome, Edge, ChrOpera and similar browsers
{
	widget.cfg.setAsync = function(key_values, handler)	{ chrome.storage.local.set(key_values, handler); };
	widget.cfg.getAsync = function(keys, handler)		{ chrome.storage.local.get(keys, handler); };

	// copy values from old storage (except Opera Classic, because it was discontinued)
	if (typeof localStorage != "undefined")
		widget.cfg.getAsync("buttonPosition", function(cfg)
		{
			if (cfg.buttonPosition == null)
			{
				var key, ls_data = {};
				for (key in localStorage)
					ls_data[key] = localStorage.getItem(key);
				widget.cfg.setAsync(ls_data);
			}
		});
}
else if ((widget && typeof widget.preferences != "undefined") || (typeof localStorage != "undefined"))	// Opera Classic or other old browser
{
	if (typeof widget.preferences == "undefined")
		widget.preferences = localStorage;

	widget.cfg.setAsync = function(key_values, handler)
	{
		for (var key in key_values)
			widget.preferences.setItem(key, JSON.stringify( key_values[key] ) );
		if (handler)
			handler();
	};
	widget.cfg.getAsync = function(keys, handler)
	{
		var TryToDecodeValue = function(val)
		{
			try
			{
				return JSON.parse( val );
			}
			catch (ex)
			{
				return null;
			}
		};

		var i, key, result = {};
		if (typeof keys == "string")									// string
			result[keys] = TryToDecodeValue( widget.preferences.getItem(keys) );
		else if (typeof keys.concat == "function")						// array of names
			for (key in keys)
				result[ keys[key] ] = TryToDecodeValue( widget.preferences.getItem(keys[key]) );
		else if (keys === null)											// null
			for (i=0; i<widget.preferences.length; i++)
				result[ widget.preferences.key(i) ] = TryToDecodeValue( widget.preferences.getItem( widget.preferences.key(i) ) );
		else
			for (key in keys)											// object like {key: defaultValue}
			{
				result[key] = TryToDecodeValue( widget.preferences.getItem(key) );
				if (result[key] === null)
					result[key] = keys[key];
			}
		if (handler)
			handler(result);
	};
}
else
{
	console.log("Can't setup storage connection.");
}

// additional function
widget.cfg.setIfUndefinedAsync = function(key_values)
{
	var key;
	var funcSetIfNull = function(cfg)
	{
		if (cfg[key] == null)
		{
			var new_set = {};
			new_set[key] = key_values[key];
			widget.cfg.setAsync( new_set );
		}
	};

	for (key in key_values)
		widget.cfg.getAsync(key, funcSetIfNull);
};


// setup default preferences
widget.cfg.getAsync(["buttonPosition", "useExtensionButton", "translator_uuid"], function(cfg)
{
	widget.cfg.uuid = cfg.translator_uuid;

	if (cfg.buttonPosition == null || (typeof cfg.useExtensionButton == "boolean"))
	{
		// not setIfUndefinedAsync() because we need reset these values when format will be changed
		widget.cfg.setAsync({
							"buttonPosition":		"right",
							"maxStoreLangPairs":	"3",
							"useSelMarker":			"true",
							"useExtensionButton":	"true",
							});
	}
});
