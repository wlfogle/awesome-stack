/* globals chrome, widget, WORDS, is_edge, SendAjaxRequest, ToolbarUIItemProperties, popupMinimumSize */
var theButton;
var iconImgSrc = "icons/icon-18.png";		// Opera Classic + default
var iconPreferedSize = null;
if (chrome)
{
	// https://developer.mozilla.org/en-US/Add-ons/WebExtensions/manifest.json/browser_action#Choosing_icon_sizes
	var req_size = 16 * window.devicePixelRatio;
	if (is_edge)
		req_size = 20 * window.devicePixelRatio;
	else if (chrome && chrome.i18n)	// Chrome
		req_size = 19 * window.devicePixelRatio;

	var possible_icons = chrome.runtime.getManifest().browser_action.default_icon;
	req_size = Math.round(req_size);
	while ((req_size > 0) && (typeof possible_icons[req_size] == "undefined"))
		req_size--;
	iconPreferedSize = req_size;
	if (typeof possible_icons[req_size] != "undefined")
		iconImgSrc = possible_icons[req_size];
}

var msAppIdWasRefreshed = 0;
var yaAppIdWasRefreshed = 0;
var ptAppIdWasRefreshed = 0;
var udAppIdWasRefreshed = 0;
var bdAppIdWasRefreshed = 0;
var sgAppIdWasRefreshed = 0;
var stAppIdWasRefreshed = 0;
var nvAppIdWasRefreshed = 0;
var lnAppIdWasRefreshed = 0;
//var msAppId = "4A3CABC2D69C013FAB95D4E92B66F1AC2D6D2E11";
var msAppId = "";
var msSignId = "";
var yaAppId = "5c313937";
var yaSignId = "6958ce92.608d7da8.70bbe1c3.74722d74657874";
var ptAppId = "CfDJ8DH3ktSSPPxDoy6ijXj_1i78LTnP7q6S0CQ22mFydVkQLGC7-aLwa3hwxbMoHoT5TfKjmkK_XA3kfmZ97mEQxeyrd6pFxH_hJoNGnu9cRd610VP-9nHPQQt-0Ap2rlGVTE_Pkr3ACu2e11aQPcxnWjU";
var ptSignId = "2s21SAjZVNAMwmHFb-0_lIDYZw8-4wHOOCapY2AcH9IfkiQWkqR4BXJYGyvIif8FffRYjYGAg0jNoW6-";
var udAppId = "54dfbdc4e47a0f59e23b186668684cd4";
var bdAppId = "";
var bdSignId = "";
var sgAppId = "";
var sgSignId = "";
var stAppId = "";
var nvAppId = "v1.5.6_97f6918302";
var lnAppId = "";

var activeTab_UUID = 0;													// TODO: remove it with code
var cmForSelection = 0;
var cmForPage = 0;

var lastUsedStat = 0;
var tabs_ports = [];
var lastActiveTab = null;
var lastActivePopup = null;

var popupOpenerData = null;

var opera_gx = false;
var svgLogo = '<?xml version="1.0" encoding="UTF-8" standalone="no"?><svg width="100%" height="100%" viewBox="0 0 512 512" version="1.1" xml:space="preserve" style="fill-rule:evenodd;clip-rule:evenodd;stroke-linejoin:round;stroke-miterlimit:1.41421;" id="svg29" xmlns="http://www.w3.org/2000/svg" xmlns:svg="http://www.w3.org/2000/svg" xmlns:serif="http://www.serif.com/"> <g id="bg" transform="matrix(1.10814,0,0,1.09148,-28.6287,-20.5078)"><path d="M487.339,93.757C487.339,52.381 454.253,18.789 413.499,18.789L99.676,18.789C58.922,18.789 25.835,52.381 25.835,93.757L25.835,412.371C25.835,453.747 58.922,487.339 99.676,487.339L413.499,487.339C454.253,487.339 487.339,453.747 487.339,412.371L487.339,93.757Z" style="fill:white;stroke:rgb(0,132,12);stroke-opacity:0;stroke-width:0.91px;" id="path2" /></g><g id="left_bg1" serif:id="left_bg" transform="matrix(1.10814,0,0,1.09148,-28.6287,-20.5078)" style="filter:url(#filter303)"><clipPath id="_clip2"><path d="M308.317,3.382L-33.565,-70.396L-142.914,451.904L198.968,525.683L308.317,3.382Z" id="path6" /></clipPath> <g clip-path="url(#_clip2)" id="g11"><path d="M487.339,93.757C487.339,52.381 454.253,18.789 413.499,18.789L99.676,18.789C58.922,18.789 25.835,52.381 25.835,93.757L25.835,412.371C25.835,453.747 58.922,487.339 99.676,487.339L413.499,487.339C454.253,487.339 487.339,453.747 487.339,412.371L487.339,93.757Z" style="fill:rgb(51,146,255);stroke:rgb(51,146,255);stroke-opacity:0;stroke-width:0.91px;" id="path9" /></g> </g><g transform="matrix(1.58892,0,0,1.58892,-231.185,-67.6208)" id="g18"> <g transform="matrix(251.743,0,0,251.743,166.477,214.872)" id="g16"><path d="M0.453,-0.026C0.44,-0.018 0.429,-0.012 0.418,-0.007C0.408,-0.003 0.399,-0.001 0.391,-0.001C0.377,-0.001 0.367,-0.005 0.361,-0.014C0.354,-0.023 0.351,-0.036 0.35,-0.056C0.325,-0.035 0.301,-0.019 0.277,-0.009C0.253,0.001 0.229,0.006 0.204,0.006C0.155,0.006 0.116,-0.008 0.086,-0.035C0.056,-0.063 0.041,-0.098 0.041,-0.143C0.041,-0.194 0.062,-0.234 0.103,-0.264C0.144,-0.295 0.199,-0.31 0.269,-0.31C0.28,-0.31 0.293,-0.31 0.305,-0.309C0.318,-0.308 0.332,-0.307 0.347,-0.305C0.347,-0.308 0.348,-0.311 0.348,-0.314C0.348,-0.317 0.348,-0.321 0.348,-0.325C0.348,-0.359 0.339,-0.385 0.32,-0.403C0.301,-0.42 0.273,-0.429 0.237,-0.429C0.207,-0.429 0.182,-0.424 0.163,-0.414C0.143,-0.404 0.128,-0.389 0.116,-0.368C0.104,-0.371 0.095,-0.376 0.089,-0.383C0.083,-0.39 0.08,-0.4 0.08,-0.411C0.08,-0.435 0.095,-0.455 0.125,-0.47C0.155,-0.485 0.195,-0.493 0.243,-0.493C0.303,-0.493 0.347,-0.479 0.377,-0.45C0.407,-0.421 0.422,-0.378 0.422,-0.321C0.422,-0.31 0.421,-0.291 0.42,-0.265C0.419,-0.238 0.419,-0.216 0.419,-0.2C0.419,-0.155 0.422,-0.119 0.427,-0.092C0.432,-0.064 0.441,-0.042 0.453,-0.026ZM0.346,-0.243C0.331,-0.245 0.317,-0.247 0.305,-0.248C0.294,-0.249 0.283,-0.25 0.275,-0.25C0.224,-0.25 0.186,-0.242 0.161,-0.225C0.135,-0.208 0.122,-0.183 0.122,-0.15C0.122,-0.123 0.131,-0.102 0.149,-0.087C0.167,-0.072 0.192,-0.064 0.223,-0.064C0.247,-0.064 0.269,-0.068 0.29,-0.077C0.31,-0.086 0.329,-0.1 0.347,-0.118C0.346,-0.134 0.346,-0.147 0.345,-0.157C0.345,-0.168 0.345,-0.178 0.345,-0.187C0.345,-0.193 0.345,-0.202 0.346,-0.215C0.346,-0.227 0.346,-0.237 0.346,-0.243Z" style="fill:white;fill-rule:nonzero;" id="path14" /></g> </g> <g transform="matrix(1.23189,0,0,1.23189,9.99737,201.596)" id="g24"> <g transform="matrix(259.763,0,0,259.763,166.477,214.872)" id="g22"><path d="M0.324,-0.099C0.375,-0.121 0.409,-0.146 0.426,-0.175C0.422,-0.206 0.417,-0.251 0.414,-0.312C0.367,-0.29 0.33,-0.257 0.301,-0.213C0.278,-0.177 0.266,-0.144 0.266,-0.113C0.265,-0.097 0.272,-0.089 0.287,-0.089C0.297,-0.089 0.309,-0.093 0.324,-0.099ZM0.487,-0.221C0.513,-0.255 0.534,-0.304 0.552,-0.368C0.53,-0.366 0.505,-0.36 0.477,-0.348C0.476,-0.32 0.479,-0.278 0.487,-0.221ZM0.22,-0.603C0.281,-0.603 0.35,-0.608 0.429,-0.617C0.439,-0.703 0.446,-0.759 0.451,-0.785L0.518,-0.772C0.512,-0.74 0.505,-0.692 0.496,-0.626C0.573,-0.638 0.636,-0.651 0.686,-0.667L0.685,-0.604C0.667,-0.6 0.601,-0.587 0.488,-0.566C0.481,-0.508 0.478,-0.458 0.477,-0.415C0.503,-0.422 0.533,-0.428 0.568,-0.433C0.569,-0.437 0.571,-0.458 0.575,-0.497L0.642,-0.489C0.641,-0.476 0.639,-0.457 0.635,-0.432C0.697,-0.425 0.746,-0.4 0.781,-0.356C0.814,-0.316 0.83,-0.268 0.83,-0.21C0.83,-0.149 0.81,-0.096 0.77,-0.05C0.727,-0.001 0.668,0.031 0.593,0.047L0.553,-0.015C0.607,-0.023 0.655,-0.044 0.695,-0.077C0.741,-0.114 0.764,-0.156 0.764,-0.203C0.764,-0.305 0.716,-0.36 0.62,-0.367C0.596,-0.278 0.558,-0.205 0.505,-0.147C0.51,-0.125 0.519,-0.099 0.531,-0.069L0.462,-0.057C0.46,-0.063 0.454,-0.079 0.445,-0.105C0.38,-0.05 0.318,-0.022 0.259,-0.022C0.223,-0.022 0.203,-0.042 0.199,-0.081C0.198,-0.088 0.197,-0.098 0.197,-0.109C0.197,-0.134 0.202,-0.158 0.211,-0.181C0.227,-0.223 0.25,-0.26 0.28,-0.292C0.31,-0.325 0.354,-0.358 0.41,-0.392C0.411,-0.444 0.415,-0.499 0.421,-0.556C0.339,-0.545 0.277,-0.54 0.233,-0.54L0.22,-0.603Z" style="fill:rgb(36,135,32);fill-rule:nonzero;" id="path20" /></g> </g> <defs id="defs27"><filter style="color-interpolation-filters:sRGB;" id="filter303" x="-0.016587072" y="-0.016337637" width="0.65944607" height="1.0369438"><feFlood flood-opacity="0.498039" flood-color="rgb(0,0,0)" result="flood" id="feFlood293" /><feComposite in="flood" in2="SourceGraphic" operator="in" result="composite1" id="feComposite295" /><feGaussianBlur in="composite1" stdDeviation="3" result="blur" id="feGaussianBlur297" /><feOffset dx="7" dy="2" result="offset" id="feOffset299" /><feComposite in="SourceGraphic" in2="offset" operator="over" result="composite2" id="feComposite301" /></filter></defs></svg>';

function setupBandgeIcon(badge, stat)
{
	if (!badge)
		return;

	if (badge["getPanel"])	// for sidebar
	{
		if (typeof stat == "undefined")
			stat = 0;
	}
	else
	{
		if (typeof stat == "undefined")
			stat = lastUsedStat;
		else
			lastUsedStat = stat;
	}

	if (opera)
	{
		if (stat > 0)
			badge.icon = iconImgSrc.replace("icons/", "icons/selection/");
		else if (stat < 0)
			badge.icon = iconImgSrc.replace("icons/", "icons/disabled/");
		else
			badge.icon = iconImgSrc;
	}
	else
	{
/*
		if (stat > 0)
			badge.setIcon( {path: iconImgSrc.replace("icons/", "icons/selection/")} );
		else if (stat < 0)
			badge.setIcon( {path: iconImgSrc.replace("icons/", "icons/disabled/")} );
		else
			badge.setIcon( {path: iconImgSrc} );
*/

		widget.cfg.getAsync(["iconTheme", "iconColor"], function(cfg)
		{
			var logo2display = svgLogo;

			if ((cfg.iconTheme == 1) && cfg.iconColor)
			{
				// preparations for Opera GX
				logo2display = logo2display.replace("fill:rgb(51,146,255);", "fill:"+cfg.iconColor+";");	// left bg
				logo2display = logo2display.replace("stroke:rgb(51,146,255);", "stroke:"+cfg.iconColor+";");	// left part stroke
				logo2display = logo2display.replace("fill:rgb(36,135,32);", "fill:"+cfg.iconColor+";");	// green letter
			}

			if (stat > 0)
				logo2display = logo2display.replace('style="fill:white;stroke:', 'style="fill:#FFFF97;stroke:');
			else if (stat < 0)
				logo2display = logo2display.replace('style="fill:white;stroke:', 'style="fill:#FFE5E6;stroke:');

			badge.setIcon( {path: "data:image/svg+xml;base64," + btoa(logo2display)} );
		});
	}
}

function SetupCookiesByUrl(url, oncomplete)
{
	// fix bug with recursive reloading popup on trying open new tab while popup is opened
	if (opera)
	{
		var temp_tab = opera.extension.tabs.create({url: url, focused: false});
		if (temp_tab.readyState)
		{
			var loop = setInterval(function()
			{
				if (temp_tab.readyState === 'complete')
				{
					clearInterval(loop);
//					temp_tab.close();	// wait for fix. Now it close popup :(
					if (oncomplete)
						oncomplete();
				}
			}, 100);
		}
		else if (oncomplete)
			setTimeout( oncomplete, 2000 );
	}
	else
	{
		chrome.tabs.create({url: url, active: false}, function(temp_tab)
		{
			var gotNewId = false;
			var funcTabListener = function(tabId, info)
			{
				if (!gotNewId && (temp_tab.id == tabId) && (info.status == "complete"))
				{
					gotNewId = true;
					chrome.tabs.onUpdated.removeListener(funcTabListener);
					if (oncomplete)
						oncomplete();
					chrome.tabs.remove(tabId);
				}
			};

			chrome.tabs.onUpdated.addListener(funcTabListener);
			funcTabListener(temp_tab.id, temp_tab);		// for case when tab was already loaded before listener
		});
	}
}

function ExtractAppId(srcUrl, extractRules, callback, post_data)
{
	var redirect_cnt = 0;
	var xhr = SendAjaxRequest(srcUrl,
								(post_data ? "POST" : "GET"),
								null,
								post_data,
								function()
								{
									if (this.readyState == 4)
									{
										if (this.status == 200)
										{
											if (!redirect_cnt)
											{
												if (/\s*location\.href=['"]([^'"]+)['"]/.test(this.responseText))
												{
													console.log("Anti-fraud redirect? " + RegExp.$1);
													CreateTabWithUrl(RegExp.$1);
												}
											}

											for (var targetVar in extractRules)
											{
												var re = extractRules[targetVar];
												if (re.test(this.responseText))
													window[targetVar] = RegExp.$1;
												else
													console.log("using default "+targetVar+"!");
											}
											window[targetVar+"WasRefreshed"] = (new Date())-0;
										}
										xhr = null;

										if (callback)
											callback();
									}
								}
								);
}

function RefreshMsAppId(callback)
{
	widget.cfg.getAsync(["bingClientId", "bingClientSecret"], function(cfg)
	{
		// get public access token
		SetupCookiesByUrl("https://www.bing.com/translator", function()
		{
			ExtractAppId("https://www.bing.com/translator",
						{
							"msSignId": new RegExp("params_AbusePreventionHelper\\s*=\\s*\\[\\s*(\\d+)\\s*,['\"]", "im"),
							"msAppId": new RegExp("params_AbusePreventionHelper\\s*=\\s*\\[\\s*\\d+\\s*,['\"]([^'\"]+)['\"]\\s*,", "im")				// main var has to be last!
						},
						callback);
		});
	});
}

function RefreshYaAppId(callback)
{
	ExtractAppId("https://translate.yandex.com",
				{
					"yaSignId": new RegExp("Ya.reqid\\s*=\\s*['\"]([^'\"]+)['\"];", "im"),
					"yaAppId": new RegExp(",\\s+SID:\\s*['\"]([^'\"]+)['\"],\\s+", "im")				// main var has to be last!
				},
				callback);
}

function RefreshPtAppId(callback)
{
	ExtractAppId("https://m.online-translator.com/translation",
				{
					"ptSignId": new RegExp("[\"']_paft[\"'][^>]+value=[\"']([^\"']+)[\"']", "im"),
					"ptAppId": new RegExp("[\"']_xsrf[\"'][^>]+value=[\"']([^\"']+)[\"']", "im")				// main var has to be last!
				},
				callback);
}

function RefreshUdAppId(callback)
{
	ExtractAppId("https://m.urbandictionary.com/javascripts/application.js",
				{"udAppId": new RegExp("\\?key=([0-9a-z]+)", "im") },
				callback);
}

function RefreshBdAppId(callback)
{
	ExtractAppId("https://fanyi.baidu.com/",
				{
					"bdSignId": new RegExp("window.gtk[\\s]*=[\\s]*['\"]([0-9\\.]+)['\"]", "im"),
					"bdAppId": new RegExp("token:[\\s]*['\"]([0-9a-z]+)['\"]", "im")				// main var has to be last!
				},
				callback);
}

function RefreshSgAppId(callback)
{
	// Require referer!!! from https://fanyi.sogou.com/
	ExtractAppId("https://fanyi.sogou.com/text",
				{
					"sgSignId": new RegExp("[\"']ServerSgtkn[\"'][\\s]*:[\\s]*['\"]([0-9a-z]+)['\"]", "im"),
					"sgAppId": new RegExp("[\"']secretCode[\"'][\\s]*\:[\\s]*([0-9]+)", "im")				// main var has to be last!
				},
				callback);
}

function RefreshStAppId(callback)
{
	ExtractAppId("https://translate.systran.net/translationTools/text",
				{"stAppId": new RegExp("window.csrfToken[\\s]*=[\\s]*['\"]([^'\"]+)['\"]", "im")},
				callback);
}

function RefreshNvAppId(callback)
{
	var base_url = "https://papago.naver.com";
	var xhr = SendAjaxRequest(base_url,
								"GET",
								null,
								null,
								function()
								{
									if (this.readyState == 4)
									{
										if (this.status == 200)
										{
											if (/\"([^\"]*home\.[^\"]+.js)\"/.test(this.responseText))
											{
												var page_url = RegExp.$1;
												if (page_url.substr(0, 1) == "/")
													page_url = base_url + page_url;
												ExtractAppId(page_url,
															{"nvAppId": new RegExp("AUTH_KEY\:[\"\']([^'\"]+)[\"\']", "im")},
															callback);
											}
											else
											{
												console.log("Error: Can't detect AppId source document!");
												callback();
											}
										}
										xhr = null;
									}
								}
								);
}

function RefreshLnAppId(callback)
{
	ExtractAppId("https://lingvanex.com/lingvanex_demo_page/js/api-base.js",
				{"lnAppId": new RegExp("B2B_AUTH_TOKEN[\\s]*=[\\s]*['\"]([^'\"]+)['\"]", "im")},
				callback);
}


function IsTabCanReceiveMessage(tab, tab_port)
{
	if (!opera && (typeof tab_port == "undefined"))
		tab_port = false;	// old Opera can be `undefined`, but new Opera(Chrome) can't

	return tab
			&& (tab_port || (typeof tab_port == "undefined"))	// Back compatibility with old Operas
			&& (typeof tab.url != "undefined")
			&& (tab.url !== "" || (typeof tab_port == "object" && tab_port.sender && tab_port.sender.url != ""))	// tab can has old url. tab_port has newer
			&& tab.url.indexOf("operaemail:/") < 0;	// if we can work with active page
}

function updateBandgeStatus(activeTab, activeTabPort)
{
	if (IsTabCanReceiveMessage(activeTab, activeTabPort))
	{
		setupBandgeIcon(theButton, 0);
		(activeTabPort || activeTab).postMessage({ action:"get_active_page_selection" });
	}
	else	// Can't detect active page
	{
		setupBandgeIcon(theButton, -1);
	}
}

function CreateTabWithUrl(url)
{
	if (opera)
	{
		var newTab = opera.extension.tabs.create({ url:url, focused:true });
//		tab.focus();
	}
	else
	{
		if (lastActiveTab)
			var newTab = chrome.tabs.create({ url:url, active:true, index:lastActiveTab.index+1 });		// always next to active?
		else
			var newTab = chrome.tabs.create({ url:url, active:true });
	}

	if (opera && (opera.version()-0 < 12.1))
	{
		widget.cfg.getAsync("openNewTabsNextToActive", function(cfg)
		{
			if (cfg.openNewTabsNextToActive == "true")
			{
				var activeTab = opera.extension.tabs.getFocused();
				if (activeTab.tabGroup)
				{
					var allTabs = activeTab.tabGroup.tabs.getAll();
					var targetList = activeTab.tabGroup;
				}
				else
				{
					var allTabs = opera.extension.tabs.getAll();
					var targetList = activeTab.browserWindow;
				}

				var nextTab = null;
				var i, cnt = allTabs.length;
				for (i=0; i<cnt; i++)
					if ((allTabs[i].id == activeTab.id) && (i < cnt-1))
					{
						nextTab = allTabs[i+1];
						break;
					}

				targetList.insert(newTab, nextTab);
			}
		});
	}

	return newTab;
}


var onMessageHandler;
window.addEventListener("DOMContentLoaded", function()
{
	onMessageHandler = function(msg, sender, respFunc)		// global for possiblity call it from popup (Chrome)
	{
		if (opera)
		{
			sender = msg.source;
			msg = msg.data;
		}
		else if ((typeof sender == "undefined") && msg.source)
		{
			sender = msg.source;
		}

		var appIdName;
		var getAppIdName;
		var appIdWasRefreshed;
		var resp_msg;

		switch (msg.action)
		{
			case "bg_console_log":
				console.log([sender, msg.data]);
				break;

			case "real_user_agent":
				if (is_original_chrome)
				{
					// fix fake chrome browser detection
					SetupBrowserVariables(msg.user_agent);
					if (!is_original_chrome && window.dispatchEvent)
						window.dispatchEvent(new Event("DOMContentLoaded"));
				}
				break;

			case "get_sender_tab_info":
				respFunc( sender.tab );
				break;

			case "get_content_script_cfgs":	// from userJs, but only by Chrome
				widget.cfg.getAsync(msg.names, function(cfg)
				{
					respFunc( cfg );
				});
				break;

			case "tab_changed_url":	// from userJs
				if (opera)
					opera.extension.tabs.onfocus(null);
				// else Chrome has tabs.onActivated handler
				break;


			case "get_msAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_msAppId", value:msAppId, value2:msSignId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "msAppId";
					appIdName2 = "msSignId";
					getAppIdName = "RefreshMsAppId";
					appIdWasRefreshedName = "msAppIdWasRefreshed";
				}
			case "get_yaAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_yaAppId", value:yaAppId, value2:yaSignId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "yaAppId";
					appIdName2 = "yaSignId";
					getAppIdName = "RefreshYaAppId";
					appIdWasRefreshedName = "yaAppIdWasRefreshed";
				}
			case "get_ptAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_ptAppId", value:ptAppId, value2:ptSignId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "ptAppId";
					appIdName2 = "ptSignId";
					getAppIdName = "RefreshPtAppId";
					appIdWasRefreshedName = "ptAppIdWasRefreshed";
				}
			case "get_udAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_udAppId", value:udAppId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "udAppId";
					getAppIdName = "RefreshUdAppId";
					appIdWasRefreshedName = "udAppIdWasRefreshed";
				}
			case "get_bdAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_bdAppId", value:bdAppId, value2:bdSignId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "bdAppId";
					appIdName2 = "bdSignId";
					getAppIdName = "RefreshBdAppId";
					appIdWasRefreshedName = "bdAppIdWasRefreshed";
				}
			case "get_sgAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_sgAppId", value:sgAppId, value2:sgSignId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "sgAppId";
					appIdName2 = "sgSignId";
					getAppIdName = "RefreshSgAppId";
					appIdWasRefreshedName = "sgAppIdWasRefreshed";
				}
			case "get_stAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_stAppId", value:stAppId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "stAppId";
					getAppIdName = "RefreshStAppId";
					appIdWasRefreshedName = "stAppIdWasRefreshed";
				}
			case "get_nvAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_nvAppId", value:nvAppId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "nvAppId";
					getAppIdName = "RefreshNvAppId";
					appIdWasRefreshedName = "nvAppIdWasRefreshed";
				}
			case "get_lnAppId":
				if (typeof appIdName == "undefined")
				{
					resp_msg = { action:"setup_lnAppId", value:lnAppId, next_action:msg["next_action"], next_action_args:msg["next_action_args"] };
					appIdName = "lnAppId";
					getAppIdName = "RefreshLnAppId";
					appIdWasRefreshedName = "lnAppIdWasRefreshed";
				}
			case "get_xxAppId":	// used previous cases
				if (((resp_msg.value === "") && ((new Date()-window[appIdWasRefreshedName]) > 1000*5))							// not faster, than 5 seconds
					|| (msg.refresh && (resp_msg.value != "-")  && ((new Date()-window[appIdWasRefreshedName]) > 1000*5*60))	// not faster, than 5 minutes
					|| ((new Date()-window[appIdWasRefreshedName]) > 1000*3*60*60)												// not older, than 3 hours (TODO: test interval)
					)
				{
					// Warning! this function can be called from injected script => new temporary tabs can be loaded in the loop.
					window[getAppIdName](function()
					{
						window[appIdWasRefreshedName] = (new Date())-0;
						resp_msg.refreshed = 1;
						resp_msg.value = window[appIdName];		// new value
						if (typeof appIdName2 != "undefined")
							resp_msg.value2 = window[appIdName2];
						if (sender["onMessageHandler"])
							sender.onMessageHandler( resp_msg );
						else if (sender.postMessage)
							sender.postMessage( resp_msg );
						else if (respFunc)
							respFunc( resp_msg );
					});
				}
				else
				{
					if (sender["onMessageHandler"])
						sender.onMessageHandler( resp_msg );
					else if (sender.postMessage)
						sender.postMessage( resp_msg );
					else if (respFunc)
						respFunc( resp_msg );
				}
				break;


			// Deprecated?
			case "refresh_msAppId":
				RefreshMsAppId(function()
				{
					sender.postMessage({ action:"setup_msAppId", value:msAppId, refreshed:1 });
				});
				break;

			case "selection_changed":	// from userJs
				widget.cfg.getAsync("useSelMarker", function(cfg)
				{
					if ((cfg.useSelMarker == null) || (cfg.useSelMarker == "true"))
						setupBandgeIcon(theButton, (msg.selection === "" ? 0 : 1) );
				});
				if (opera)
					tabs_ports[opera.extension.tabs.getFocused().id] = sender;		// fix for Opera. Sometimes it remember wrong port :/
				activeTab_UUID = msg.uuid;	// selection can change only in active page
				break;

			case "get_active_page_data":	// from popup
				// always update lastActivePopup
				if (opera)
					lastActivePopup = (opera.extension.popup ? opera.extension.popup : sender);	// temporary, while Opera don't has it
				else
					lastActivePopup = chrome.extension.getViews({ "type":"popup" })[0];

				if (popupOpenerData)
				{
					var new_msg = {
						action:		"set_active_page_data",
						url:		popupOpenerData.pageUrl,
						title:		popupOpenerData.pageTitle,
						selection:	popupOpenerData.selectionText
					};
					popupOpenerData = null;	// one time object

					if (opera)
					{
						if (lastActivePopup)
							lastActivePopup.postMessage(new_msg);
					}
					else if (sender && (typeof sender.onMessageHandler == "function"))
					{
						sender.onMessageHandler(new_msg, window);
					}
					else if (lastActivePopup && lastActivePopup.onMessageHandler)
					{
						lastActivePopup.onMessageHandler(new_msg, window);
					}
					else if (sender && sender.tab)
					{
						var senderWindow = chrome.extension.getViews({ "windowId":sender.tab.windowId, "tabId":sender.tab.id })[0]
						if (senderWindow && senderWindow.onMessageHandler)
							senderWindow.onMessageHandler(new_msg, window);
						else if (respFunc)
							respFunc(new_msg, window);
					}
				}
				else
				{
	//				var activeTab = opera.extension.tabs.getFocused();
					var activeTab = lastActiveTab;
					if (activeTab && IsTabCanReceiveMessage(activeTab, (opera ? activeTab.port : tabs_ports[activeTab.id])))
					{
						if (activeTab_UUID)
						{
							if (opera)
								opera.extension.broadcastMessage({ action:"get_active_page_data", uuid:activeTab_UUID });	// activeTab is not enough. Page can has a frames...
							else if (tabs_ports[activeTab.id])
								tabs_ports[activeTab.id].postMessage({ action:"get_active_page_data", uuid:activeTab_UUID });
						}
						else if (!opera && tabs_ports[activeTab.id])
							tabs_ports[activeTab.id].postMessage({ action:"get_active_page_data" });
						else if (opera && activeTab)
							(tabs_ports[activeTab.id] || activeTab.port).postMessage({ action:"get_active_page_data" });
					}
					else	// no tab? just return empty answer
					{
						if (opera)
						{
							if (lastActivePopup)
								lastActivePopup.postMessage({ action:"set_active_page_data" });
						}
						else if (sender && (typeof sender.onMessageHandler == "function"))
						{
							sender.onMessageHandler({ action:"set_active_page_data" }, window);
						}
						else if (lastActivePopup && lastActivePopup.onMessageHandler)
						{
							lastActivePopup.onMessageHandler({ action:"set_active_page_data" }, window);
						}
						else if (respFunc)
							respFunc({ action:"set_active_page_data" }, window);
					}
				}
				break;

			case "set_active_page_data":	// from userJs
				if (!activeTab_UUID || (activeTab_UUID && (msg.uuid == activeTab_UUID)))
				{
					if (opera)
						lastActivePopup.postMessage(msg);
					else
						lastActivePopup.onMessageHandler(msg, window);
				}
//				if (activeTab_UUID == msg.uuid)	// this message is consequence of direct message to require tab
//					opera.extension.broadcastMessage(request.data);	// TODO: send directly to popup, when it will be possible
				break;

			case "translate_url":	// from popup
				// fix bug with recursive reloading popup on trying open new tab while popup is opened
				theButton.disabled = true;

				if (opera)
					var activeTab = opera.extension.tabs.getFocused();
				else
					var activeTab = lastActiveTab;
				if (!msg.url && activeTab && (typeof activeTab.url != "undefined"))
					msg.url = activeTab.url;

				if (msg.url)
					CreateTabWithUrl(msg.prefixUrl + encodeURIComponent(msg.url));

				// fix bug with recursive reloading popup on trying open new tab while popup is opened
				theButton.disabled = false;
				break;

			case "setup_pragma_cookies":	// from popup
				SetupCookiesByUrl("http://online.translate.ua", function()
				{
					if (opera)
						sender.postMessage({ action:"retranslate_text" });
					else
						sender.onMessageHandler({ action:"retranslate_text" });
				});
				break;

			case "get_translated_selection":	// from userJs
				// TODO: open popup with translation, when it will be possible :/
				break;

			case "change_popup_size":	// from popup (Opera Classic)
				if (popupMinimumSize.width < ToolbarUIItemProperties.popup.width + msg.deltaX)
					theButton.popup.width = ToolbarUIItemProperties.popup.width + msg.deltaX;
				else
					theButton.popup.width = popupMinimumSize.width;

				if (popupMinimumSize.height < ToolbarUIItemProperties.popup.height + msg.deltaY)
					theButton.popup.height = ToolbarUIItemProperties.popup.height + msg.deltaY;
				else
					theButton.popup.height = popupMinimumSize.height;

				lastActivePopup = (opera.extension.popup ? opera.extension.popup : sender);	// temporary, while Opera don't has it
				lastActivePopup.postMessage({
					action: "fix_elements_size",
					deltaX: theButton.popup.width-popupMinimumSize.width,
					deltaY: theButton.popup.height-popupMinimumSize.height
				});
				break;

			case "save_popup_size":
				ToolbarUIItemProperties.popup.width = theButton.popup.width;
				ToolbarUIItemProperties.popup.height = theButton.popup.height;
				widget.cfg.setAsync( {"popupSize": theButton.popup.width + "x" + theButton.popup.height} );
				break;

			case "update_extension_button":
				widget.cfg.getAsync("useExtensionButton", function(cfg)
				{
					var useExtensionButton = ((cfg.useExtensionButton == null) || (cfg.useExtensionButton == "true"));

					if (opera.contexts.toolbar.length)
					{
						if (!useExtensionButton)
							opera.contexts.toolbar.removeItem( theButton );
					}
					else
					{
						if (useExtensionButton)
							opera.contexts.toolbar.addItem( theButton );
					}
				});
				break;

			case "refresh_extesion_icons":
				setupBandgeIcon(theButton);
				if ((typeof chrome == "object") && chrome.sidebarAction)
					setupBandgeIcon(chrome.sidebarAction, 0);
				else if ((typeof opr == "object") && opr.sidebarAction)
					setupBandgeIcon(opr.sidebarAction, 0);
				break;

			case "create_tab_with_url":
				CreateTabWithUrl(msg.url);
				break;
		}

		return true;
	};


	widget.cfg.getAsync([
							"useExtensionButton",
							"msAppId", "yaAppId", "ptAppId", "udAppId",
							"msAppIdWasRefreshed", "yaAppIdWasRefreshed", "ptAppIdWasRefreshed", "udAppIdWasRefreshed",
							"translator_uuid", "last_check", "last_verified", "check_tries", "mode", "useBgProcess"
						], function(cfg)
	{
		// cases when need reset verification mode
		var updateCfg = {};
		if ((cfg.translator_uuid == null) || (cfg.translator_uuid.length != 36) || (cfg.translator_uuid.indexOf("-") != 8))
		{
			cfg.translator_uuid = MEL.getUUID("4utc");
			updateCfg["translator_uuid"] = cfg.translator_uuid;
			widget.cfg.uuid = cfg.translator_uuid;
		}

		if (JSON.stringify({}) !== JSON.stringify(updateCfg))
		{
			updateCfg["last_verified"]	= cfg.last_verified	= 0;
			updateCfg["last_check"]		= cfg.last_check	= 0;
			updateCfg["check_tries"]	= cfg.check_tries	= 0;
			updateCfg["mode"]			= cfg.mode			= "";			// reset demo-mode
			widget.cfg.setAsync(updateCfg);
		}
		//

		if (cfg.msAppId != null)	msAppId = cfg.msAppId;
		if (cfg.yaAppId != null)	yaAppId = cfg.yaAppId;
		if (cfg.ptAppId != null)	ptAppId = cfg.ptAppId;
		if (cfg.udAppId != null)	udAppId = cfg.udAppId;
		if (cfg.msAppIdWasRefreshed != null)	msAppIdWasRefreshed = cfg.msAppIdWasRefreshed;
		if (cfg.yaAppIdWasRefreshed != null)	yaAppIdWasRefreshed = cfg.yaAppIdWasRefreshed;
		if (cfg.ptAppIdWasRefreshed != null)	ptAppIdWasRefreshed = cfg.ptAppIdWasRefreshed;
		if (cfg.udAppIdWasRefreshed != null)	udAppIdWasRefreshed = cfg.udAppIdWasRefreshed;

		var useExtensionButton = ((cfg.useExtensionButton == null) || (cfg.useExtensionButton == "true"));
		var useBgProcess = ((cfg.useBgProcess == null) || (cfg.useBgProcess == "true"));

		// chrome do not allow to run background scripts
		// I add to description info about this process. Hope this is not break the rules anymore.
		//if (is_original_chrome)
		//	useBgProcess = false;

		// Firefox doesn't allow to use background process
//		if (is_firefox)
//			useBgProcess = false;

		if (opera)
		{
			theButton = opera.contexts.toolbar.createItem(ToolbarUIItemProperties);
			if (useExtensionButton)
				opera.contexts.toolbar.addItem(theButton);

			opera.extension.onmessage = onMessageHandler;
			opera.extension.tabs.onfocus = function(evt, port)
			{
				var activeTab = opera.extension.tabs.getFocused();
				if (activeTab)	// in Opera 11.xx it can be null
				{
					if (port)
						tabs_ports[activeTab.id] = port;									// fix port after tab change content
					updateBandgeStatus(activeTab, port || activeTab.port);
					lastActiveTab = activeTab;
				}
				else
					updateBandgeStatus(null);
			};
			opera.extension.tabs.onfocus();
		}
		else	// Chrome
		{
			if (!widget.has_handlers)
			{
				if (chrome.action)				// Manifest 3
					theButton = chrome.action
				else
					theButton = chrome.browserAction;

				chrome.runtime.onConnect.addListener(function(port) {
					if (!port.sender.tab || (port.sender.url != port.sender.tab.url)) // skip start screen and [i]frames
						return;

					if (tabs_ports[port.sender.tab.id])
						port.onMessage.removeListener( onMessageHandler );

					tabs_ports[port.sender.tab.id] = port;
					port.onDisconnect.addListener(function()
					{
						port.onMessage.removeListener( onMessageHandler );
						delete tabs_ports[port.sender.tab.id];
						updateBandgeStatus(null, null);
					});
					port.onMessage.addListener( onMessageHandler );
					if (port.sender.tab.active)
						updateBandgeStatus(port.sender.tab, port);
				});

				chrome.runtime.onMessage.addListener( onMessageHandler );

				chrome.tabs.onActivated.addListener(function(activeInfo) {
					// tabs_ports setup at onConnect handler
					// TODO: check activeInfo.tabId on disconnect status (back action do not restore connection. Opera Next bug?)
					chrome.tabs.get(activeInfo.tabId, function(activeTab)
					{
						lastActiveTab = activeTab;
						if (lastActiveTab.url.indexOf("http") !== 0)					// we don't have access to internal pages
							updateBandgeStatus(activeTab, tabs_ports[activeTab.id]);
						else
						{
							// if this is new tab without connection => manual include script (extension was restarted)
							if (!tabs_ports[activeTab.id])
							{
								// tabs_ports will be filled in onConnect
								chrome.tabs.executeScript(lastActiveTab.id, { file:"includes/user_js.js", allFrames:true, runAt:"document_end" }, function(result)
								{
									if (chrome.runtime.lastError)
										console.log("Can't inject content script. " + chrome.runtime.lastError.message + " Address: " + lastActiveTab.url);

									// Warning: even if page has selection on this step, we ignore it, because we don't know source element, but we have to ignore some elements
									updateBandgeStatus(activeTab, tabs_ports[activeTab.id]);
								});
							}
							else
								updateBandgeStatus(activeTab, tabs_ports[activeTab.id]);
						}
					});
				});

				var UpdateActiveWindowBadge = function()
				{
					chrome.tabs.query({active:true, currentWindow:true}, function(activeTabs){
						if (activeTabs.length)
						{
							lastActiveTab = activeTabs[0];
							updateBandgeStatus(activeTabs[0], tabs_ports[activeTabs[0].id]);
						}
					});
				};

				chrome.windows.onFocusChanged.addListener(function(windowId)
				{
					if (windowId < 0)
						return;

					UpdateActiveWindowBadge();
				});
				UpdateActiveWindowBadge();

				// setup icon for Sidebar
				if ((typeof chrome == "object") && chrome.sidebarAction)
					setupBandgeIcon(chrome.sidebarAction, 0);
				else if ((typeof opr == "object") && opr.sidebarAction)
					setupBandgeIcon(opr.sidebarAction, 0);

				widget.has_handlers = true;
			}


		}

/*
		// Temporary off, while server not ready
		var ws = new MEL.websocket.Client("ws://translator.sailormax.net/websockets/translator_users", "json-rpc");
		ws.onopen = function(event)
		{
			ws.send( {method: "auth", params: {client_uuid: cfg.translator_uuid}} );
		};
		ws.onmessage = function(event)
		{
			var msg = event.data;
			if (msg.substr(0, 1) == "{")
			{
				try
				{
					msg = JSON.parse(msg);
				}
				catch (e)
				{
					msg = {};
				}

				if ((msg.method == "verify") && (msg.params.client_uuid == cfg.translator_uuid))
				{
					VerifyWidgetAsync();
				}
			}
		};
		ws.onerror = function(event)
		{
		};
		ws.onclose = function(event)
		{
			if ((event.reason == "") && (event.code == 1006))		// Abnormal Closure (bad handshake?) -- https://tools.ietf.org/html/rfc6455#section-11.7
				ws.disconnect();		// do not retry until next start
		};

		window.setTimeout(function()
		{
			ws.connect();
		}, 30000);	// after some pause
*/
	});

//	RefreshMsAppId();
//	RefreshUDAppId();
}, false );

function SetupUIBasedOnMode(mode)
{
	// send to all extension pages, including popup, but excluding current (background)
	if (chrome && chrome.runtime)
		chrome.runtime.SendMessage({"action":"SetupUIBasedOnMode"});
	else if (browser && browser.runtime)
		browser.runtime.SendMessage({"action":"SetupUIBasedOnMode"});
}

function CreateOrRemoveContextMenus()
{
	widget.cfg.getAsync(["useContextMenuForPages", "useSelectionContextMenu"], function(cfg)
	{
		if (cmForPage)
			chrome.contextMenus.remove(cmForPage);
		if (cmForSelection)
			chrome.contextMenus.remove(cmForSelection);

		if ((cfg.useContextMenuForPages != null) && (cfg.useContextMenuForPages !== ""))
		{
			cmForPage = chrome.contextMenus.create({
													"id": "mn_translate_this_page",
													"title": WORDS.cmTranslatePage,
													"contexts": ["page"],
													"onclick": function(args)
														{
															// translate only external pages (Firefox can show this menu on extension's pages)
															if (args.pageUrl.indexOf("http") === 0)
															{
																widget.cfg.getAsync(["useContextMenuForPages", "useGoogleCn", "defTargetLang"], function(cfg)
																{
																	if ((cfg.useContextMenuForPages != null) && (cfg.useContextMenuForPages !== ""))
																	{
																		useGoogleCn = (cfg.useGoogleCn == "true");
																		var urlPrefix = GetTranslatedPageUrlPrefix(cfg.useContextMenuForPages, "", cfg.defTargetLang);
																		CreateTabWithUrl(urlPrefix + encodeURIComponent(args.pageUrl));
																	}
																});
															}
														}
													});
		}

		if ((cfg.useSelectionContextMenu == null) || (cfg.useSelectionContextMenu === "true"))
		{
			cmForSelection = chrome.contextMenus.create({
														"id": "mn_translate_this_selection",
														"title": WORDS.cmTranslateIt,
														"contexts": ["selection"],
														"onclick": function(info, tab)
															{
																popupOpenerData = {
																	pageUrl:		info.pageUrl,
																	pageTitle:		tab.title,
																	selectionText:	info.selectionText
																};

																if ((typeof chrome == "object") && chrome.action && chrome.action["openPopup"])	// Manifest 3
																{
																	if (!is_firefox)
																	{
																		// but Chrome support only this kind of using
																		chrome.action.openPopup(function(args)						// undocumeted function
																		{
																			// error cases?
																		});
																	}
																	else
																		chrome.action.openPopup();	// firefox support this method without arguments
																}
																else if ((typeof chrome == "object") && chrome.browserAction && chrome.browserAction["openPopup"])
																{
																	if (!is_firefox)
																	{
																		// but Chrome support only this kind of using
																		chrome.browserAction.openPopup(function(args)						// undocumeted function
																		{
																			// error cases?
																		});
																	}
																	else
																		chrome.browserAction.openPopup();	// firefox support this method without arguments
																}
																else // regular popup window
																{
																	widget.cfg.getAsync(["popupWindowSize", "popupWindowPosition"], function(cfg)
																	{
																		if (cfg.popupWindowSize)
																			var popupWindowSize = cfg.popupWindowSize.split("x");
																		else
																			var popupWindowSize = [430, 450];

																		if (cfg.popupWindowPosition)
																			var popupWindowPosition = cfg.popupWindowPosition.split("x");
																		else
																			var popupWindowPosition = [235, 185];

																		var popup_address = "popup.html?window=1&selection=1";
																		var wnd = window.open(popup_address, "Translator.extension","width="+popupWindowSize[0]+",height="+popupWindowSize[1]+",left="+popupWindowPosition[0]+",top="+popupWindowPosition[1]+",menubar=off,toolbar=off,location=off,personalbar=off,status=on,dependent=off,noopener=on,resizable=on,scrollbars=off");
																		if (!wnd)
																			CreateTabWithUrl(popup_address);
																	});
																}
															}
														});
		}
	});
}

window.addEventListener("load", function()
{
	// only if browser support manual open popups
	if (typeof chrome == "object")
	{
		CreateOrRemoveContextMenus();
	}
}, false);


// my request headers safeguard
if ((typeof chrome == "object") && chrome.webRequest)
{
	var options = chrome.webRequest.OnBeforeSendHeadersOptions;
	var extraInfoSpec = [
							options.REQUEST_HEADERS || options.REQUESTHEADERS,	// Firefox has own name
							options.BLOCKING
						];
	if (options.EXTRA_HEADERS || options.EXTRAHEADERS)	// some browsers do not support this option
		extraInfoSpec.push(options.EXTRA_HEADERS || options.EXTRAHEADERS);

	chrome.webRequest.onBeforeSendHeaders.addListener(
		function(details)
		{
			var header, fixed_headers = [];
			var headers_name_idx = {}
			var is_my_request = false;
			var my_referer = details.url.match(/^https?\:\/\/[^\/]+/)[0]+"/";

			for (var k in details.requestHeaders)
			{
				var header = details.requestHeaders[k];
				if ((header.name == "Request-From-Extension") && (header.value == widget.cfg.uuid))
				{
					if (details.url.substr(0, 7) == "http://")
					{
						// block not secured requests
						if (details.url.indexOf("http://addon.translate.ua/") !== 0
							&& details.url.indexOf("http://info.babylon.com/") !== 0
							)
							return { cancel: true };
					}

					is_my_request = true;
				}
				else if (header.name == "My-Referer")
				{
					my_referer = header.value;
				}
				else if ((header.name == "Origin") && (header.value.substr(0, 4) !== "http"))
				{
					// skip Origins like "chrome-extension://..."
				}
				else
				{
					headers_name_idx[ header.name ] = fixed_headers.length;
					fixed_headers.push(header);
				}
			}

			if (!is_my_request)
				return;

			// check secure headers
			var idx;
			if (details.url.substr(0, 7) == "http://")
			{
				if (idx = headers_name_idx["Upgrade-Insecure-Requests"])
					fixed_headers[ idx ].value = "1";
				else
					fixed_headers.push({name:"Upgrade-Insecure-Requests", value:"1"});
			}

			if (idx = headers_name_idx["DNT"])
				fixed_headers[ idx ].value = "1";
			else
				fixed_headers.push({name:"DNT", value:"1"});

			if (!headers_name_idx["Referer"])
				fixed_headers.push({ name:"Referer", value:my_referer });

			if (!headers_name_idx["Origin"])
				fixed_headers.push({ name:"Origin", value:my_referer.match(/^https?\:\/\/[^\/]+/)[0] });	// no slash at end!

			return {
		      requestHeaders: fixed_headers
	        }
		},
		{
			urls: ['<all_urls>'],
			types: ['xmlhttprequest'],
			tabId: chrome.tabs.TAB_ID_NONE
		},
		extraInfoSpec
	);
}
//
