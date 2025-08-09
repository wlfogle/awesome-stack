/* globals chrome, widget, WORDS, is_edge, languages */
function SaveSettings()
{
	if (!widget)
		return;

	var new_cfg = {};

	var defSourceLang = document.getElementsByName("defSourceLang")[0].value;
	if (!languages[defSourceLang]) defSourceLang = "";
	new_cfg["defSourceLang"] = defSourceLang;

	var defTargetLang = document.getElementsByName("defTargetLang")[0].value;
	if (!languages[defTargetLang]) defTargetLang = "";
	new_cfg["defTargetLang"] = defTargetLang;

	new_cfg["useTargetLangAlways"] = document.getElementsByName("useTargetLangAlways")[0].checked ? "true" : "false";

	var defTargetLang2 = document.getElementsByName("defTargetLang2")[0].value;
	if (!languages[defTargetLang2]) defTargetLang2 = "";
	new_cfg["defTargetLang2"] = defTargetLang2;

	var knownLangsList = [];
	var defKnownLangs = document.getElementsByName("knownLangs[]");
	var i, cnt = defKnownLangs.length;
	for (i=0; i<cnt; i++)
		if (languages[defKnownLangs[i].value])
			knownLangsList.push(defKnownLangs[i].value);
	new_cfg["knownLangs"] = knownLangsList.length ? knownLangsList.join(",") : "";

	var maxStoreLangPairs = document.getElementsByName("maxStoreLangPairs")[0].value;
	if (maxStoreLangPairs < 0) maxStoreLangPairs = 0;
	if (maxStoreLangPairs > 16) maxStoreLangPairs = 16;
	new_cfg["maxStoreLangPairs"] = maxStoreLangPairs;

	var btnPosition = "";
	var inps = document.getElementsByName("buttonPosition");
	var i, cnt = inps.length;
	for (i=0; i<cnt; i++)
		if (inps[i].checked)
			btnPosition = inps[i].value;
	new_cfg["buttonPosition"] = btnPosition;

	InvertButtonsPositionImages( document.getElementsByName("buttonPositionInvert")[0].checked );
	new_cfg["buttonPositionInvert"] = document.getElementsByName("buttonPositionInvert")[0].checked ? "true" : "false";

	new_cfg["useGoogleCn"] = document.getElementsByName("useGoogleCn")[0].checked ? "true" : "false";
	new_cfg["openNewTabsNextToActive"] = document.getElementsByName("openNewTabsNextToActive")[0].checked ? "true" : "false";
	new_cfg["rememberLastTranslation"] = document.getElementsByName("rememberLastTranslation")[0].checked ? "true" : "false";

	new_cfg["useGoogleTTS"] = document.getElementsByName("useGoogleTTS")[0].checked ? "true" : "false";
	new_cfg["usePersonalDictionary"] = document.getElementsByName("usePersonalDictionary")[0].checked ? "true" : "false";
/*
	new_cfg["useSpeechRecognition"] = document.getElementsByName("useSpeechRecognition")[0].checked ? "true" : "false";
	if (new_cfg["useSpeechRecognition"] == "true")
	{
		var SpeechRecognition = (window.SpeechRecognition || window.webkitSpeechRecognition);
		if (SpeechRecognition)
		{
			var recognizer = new SpeechRecognition();
			recognizer.onerror = function(event)
			{
				var errUseSpeechRecognition = document.getElementById("errUseSpeechRecognition");
				errUseSpeechRecognition.innerText = event.error;
			};
			recognizer.start();
			recognizer.stop();
		}
	}
*/
	var useTextareaFont = document.getElementsByName("useTextareaFont")[0].value;
	if (!useTextareaFont) useTextareaFont = "";
	new_cfg["useTextareaFont"] = useTextareaFont;

	var useTheme = document.getElementsByName("useTheme")[0].value;
	if (!useTheme) useTheme = "";
	new_cfg["useTheme"] = useTheme;

	var iconTheme = 0;
	var inps = document.getElementsByName("iconTheme");
	var i, cnt = inps.length;
	for (i=0; i<cnt; i++)
		if (inps[i].checked)
			iconTheme = inps[i].value;
	new_cfg["iconTheme"] = iconTheme;

	var iconColor = document.getElementsByName("iconColor")[0].value;
	if (!iconColor) iconColor = "#E21C48";	// Red of default Opera GX
	new_cfg["iconColor"] = iconColor;

	var useTranslateToolbar = document.getElementsByName("useTranslateToolbar")[0].value;
	if (!useTranslateToolbar) useTranslateToolbar = "";
	new_cfg["useTranslateToolbar"] = useTranslateToolbar;

	var useContextMenuForPages = document.getElementsByName("useContextMenuForPages")[0].value;
	if (!useContextMenuForPages) useContextMenuForPages = "";
	new_cfg["useContextMenuForPages"] = useContextMenuForPages;

	new_cfg["bingClientId"] = document.getElementsByName("bingClientId")[0].value;
	new_cfg["bingClientSecret"] = document.getElementsByName("bingClientSecret")[0].value;

	new_cfg["useSelMarker"] = document.getElementsByName("useSelMarker")[0].checked ? "true" : "false";

	var useExtensionButton = document.getElementsByName("useExtensionButton")[0].checked;
	new_cfg["useExtensionButton"] = useExtensionButton ? "true" : "false";
//		opera.extension.postMessage({ action:"update_extension_button" });
	if (opera && (typeof opera == "object"))
	{
		if (opera.extension.bgProcess.opera.contexts.toolbar.length)
		{
			if (!useExtensionButton)
				opera.extension.bgProcess.opera.contexts.toolbar.removeItem( opera.extension.bgProcess.theButton );
		}
		else
		{
			if (useExtensionButton)
				opera.extension.bgProcess.opera.contexts.toolbar.addItem( opera.extension.bgProcess.theButton );
		}
	}

	new_cfg["useEnterToTranslate"] = document.getElementsByName("useEnterToTranslate")[0].checked ? "true" : "false";
	new_cfg["useSelectionContextMenu"] = document.getElementsByName("useSelectionContextMenu")[0].checked ? "true" : "false";
	new_cfg["useOldSkin"] = document.getElementsByName("useOldSkin")[0].checked ? "true" : "false";
	new_cfg["useBgProcess"] = document.getElementsByName("useBgProcess")[0].checked ? "true" : "false";

	var useProviders = [];
	var setAllowedProviders = document.getElementById("setAllowedProviders");
	setAllowedProviders.getElementsByTagName("INPUT");
	var cbList = setAllowedProviders.getElementsByTagName("INPUT");
	var i, cnt = cbList.length;
	for (i=0; i<cnt; i++)
		if (cbList[i].checked)
			useProviders.push( cbList[i].id.substr(3).toLowerCase() );
	if (useProviders.length == cnt)	// all
		new_cfg["useProviders"] = "";
	else
		new_cfg["useProviders"] = useProviders.join(",");

	if (widget)
	{
		widget.cfg.setAsync(new_cfg, function() {
			if (typeof chrome == "object")
				chrome.extension.getBackgroundPage().CreateOrRemoveContextMenus();
		});
	}

	SendMessageToBackgroundScript( {action:"refresh_extesion_icons"} );
}

function InvertButtonsPositionImages(invert)
{
	var i, imgs = document.getElementsByTagName("IMG");
	for (i=0; i<imgs.length; i++)
		if (imgs[i].src.indexOf("img/buttons_") > 0)
		{
			if (invert)
				imgs[i].src = imgs[i].src.replace(/(\/buttons_[^_\.]+)\.png/, "$1_invert.png");
			else
				imgs[i].src = imgs[i].src.replace(/(\/buttons_[^_\.]+)_invert\.png/, "$1.png");
		}
}

function SetupUIBasedOnMode(mode)
{
	widget.cfg.getAsync("useGoogleTTS", function(cfg)
	{
		var inp;
		cfg.useGoogleTTS = ((cfg.useGoogleTTS == null) || (cfg.useGoogleTTS == "true"));
		(inp = document.getElementsByName("useGoogleTTS")[0]).checked = cfg.useGoogleTTS;
		inp.addEventListener("change", SaveSettings, false);
	});

	if ((mode == "demo") && false)	// NOTICE: temporary turn off demo
	{
		document.getElementsByName("useGoogleTTS")[0].parentNode.className = "only_for_registered";
		document.getElementById("blockRegistration").style.display = "block";
	}
	else
		document.getElementById("blockRegistration").style.display = "none";
}

function AppendKnownLang(baseKnownLang, value)
{
	var newKnownLang = baseKnownLang.cloneNode(true);
	newKnownLang.addEventListener("change", RefreshKnownLangs, false);
	newKnownLang.value = value;
	baseKnownLang.parentNode.appendChild( newKnownLang );
}

function RefreshKnownLangs()
{
	var selKnownLangs = document.getElementsByName("knownLangs[]");
	var i, cnt = selKnownLangs.length;

	// remove not required
	for (i=cnt-1; i>=0; i--)
		if ((selKnownLangs[i].value === "") && (cnt > 1))
		{
			selKnownLangs[i].parentNode.removeChild(selKnownLangs[i]);
			cnt--;
		}

	// add empty last element
	if ((cnt > 1) || (selKnownLangs[0].value !== ""))
		AppendKnownLang(selKnownLangs[0], "");
	SaveSettings();
}

window.addEventListener('DOMContentLoaded', function()
{
	var inp;
	document.getElementsByTagName("FORM")[0].addEventListener("submit", function(){ return false; }, false);

	// Default source language
	var i, selSource = document.getElementsByName("defSourceLang")[0];
	selSource.addEventListener("change", SaveSettings, false);
	for (i in languages)
		selSource.options.add( new Option( languages[i], i ) );

	// Default target language
	var i, selTarget = document.getElementsByName("defTargetLang")[0];
	selTarget.addEventListener("change", SaveSettings, false);
	for (i in languages)
		selTarget.options.add( new Option( languages[i], i ) );

	var i, selTarget2 = document.getElementsByName("defTargetLang2")[0];
	selTarget2.addEventListener("change", SaveSettings, false);
	for (i in languages)
		selTarget2.options.add( new Option( languages[i], i ) );
	
	// Known languages
	var i, selKnownLang = document.getElementsByName("knownLangs[]")[0];
	selKnownLang.addEventListener("change", RefreshKnownLangs, false);
	for (i in languages)
		selKnownLang.options.add( new Option( languages[i], i ) );

	// translate Providers
	var setAllowedProviders = document.getElementById("setAllowedProviders");
	var providerTpl = document.getElementById("providerTpl");
	for (provider_id in translateProviders)
	{
		var newProvider = providerTpl.cloneNode(true);
		var cb = newProvider.getElementsByTagName("INPUT")[0];
		cb.setAttribute("name", "use"+(provider_id.charAt(0).toUpperCase() + provider_id.slice(1)));
		cb.setAttribute("id", cb.name);
		var label = newProvider.getElementsByTagName("LABEL")[0];
		label.setAttribute("for", cb.id);
		label.setAttribute("id", "txt"+(cb.id.charAt(0).toUpperCase() + cb.id.slice(1)));
		label.innerText = translateProviders[provider_id];
		newProvider.style.display = "block";
		setAllowedProviders.appendChild( newProvider );
	}
	setAllowedProviders.removeChild(providerTpl);

	// Chrome Store refuse obfuscated scripts => in current state we have to turn it off
	if (is_chrome_store_app)
		document.getElementById("blockUseTranslateToolbar").style.display = "none";

	if (widget)
	{
		widget.cfg.getAsync([
								"translator_uuid",
								"defSourceLang", "defTargetLang", "useTargetLangAlways", "defTargetLang2", "maxStoreLangPairs",
								"btnPositionInvert", "btnPosition", "useTextareaFont", "useTheme", "iconTheme", "iconColor",
								"useTranslateToolbar", "knownLangs", "useContextMenuForPages", "useSelectionContextMenu",
								"bingClientId", "bingClientSecret",
								"useGoogleCn", "openNewTabsNextToActive", "rememberLastTranslation",
								"useGoogleTTS", "usePersonalDictionary", "useSpeechRecognition", "useSelMarker", "useExtensionButton", "useEnterToTranslate",
								"useProviders", "useOldSkin", "useBgProcess",
								"mode"
							],
							function(cfg)
		{
			document.getElementById("linkFeedback").href += "?uuid=" + cfg.translator_uuid;

			if (cfg.defSourceLang == null)
				cfg.defSourceLang = "";
			selSource.value = cfg.defSourceLang;

			if (cfg.defTargetLang == null)
				cfg.defTargetLang = "";
			selTarget.value = cfg.defTargetLang;

			cfg.useTargetLangAlways = (cfg.useTargetLangAlways == "true");
			(inp = document.getElementsByName("useTargetLangAlways")[0]).checked = cfg.useTargetLangAlways;
			inp.addEventListener("change", SaveSettings, false);

			if (cfg.defTargetLang2 == null)
			cfg.defTargetLang2 = "";
			selTarget2.value = cfg.defTargetLang2;

			if (cfg.knownLangs == null)
				cfg.knownLangs = [];
			else
				cfg.knownLangs = cfg.knownLangs.split(",");
			var i, cnt = cfg.knownLangs.length;
			if (cnt > 0)
			{
				for (i=0; i<cnt; i++)
				{
					if (i == 0)
						selKnownLang.value = cfg.knownLangs[i];
					else
						AppendKnownLang(selKnownLang, cfg.knownLangs[i]);
				}

				if (cfg.knownLangs[cnt-1] != "")
					AppendKnownLang(selKnownLang, "");
			}

			// Maximum stored language pairs
			if (cfg.maxStoreLangPairs == null)
				cfg.maxStoreLangPairs = 3;
			(inp = document.getElementsByName("maxStoreLangPairs")[0]).value = cfg.maxStoreLangPairs;
			inp.addEventListener("change", SaveSettings, false);

			// "Translate"-button position
			cfg.btnPositionInvert = (cfg.btnPositionInvert == "true");
			InvertButtonsPositionImages( cfg.btnPositionInvert );
			(inp = document.getElementsByName("buttonPositionInvert")[0]).checked = cfg.btnPositionInvert;
			inp.addEventListener("change", SaveSettings, false);

			if (cfg.btnPosition == null)
				cfg.btnPosition = "right";

			var inps = document.getElementsByName("buttonPosition");
			var i, cnt = inps.length;
			for (i=0; i<cnt; i++)
			{
				inps[i].checked = (inps[i].value == cfg.btnPosition);
				inps[i].addEventListener("change", SaveSettings, false);
			}

			if (cfg.useTextareaFont == null)
				cfg.useTextareaFont = "";
			(inp = document.getElementsByName("useTextareaFont")[0]).value = cfg.useTextareaFont;
			inp.addEventListener("change", SaveSettings, false);

			if (cfg.useTheme == null)
				cfg.useTheme = "";
			(inp = document.getElementsByName("useTheme")[0]).value = cfg.useTheme;
			inp.addEventListener("change", SaveSettings, false);

			if (cfg.iconTheme == null)
				cfg.iconTheme = 0;
			var inps = document.getElementsByName("iconTheme");
			var i, cnt = inps.length;
			for (i=0; i<cnt; i++)
			{
				inps[i].checked = (inps[i].value == cfg.iconTheme);
				inps[i].addEventListener("change", SaveSettings, false);
			}

			if (cfg.iconColor == null)
				cfg.iconColor = "#E21C48";
			(inp = document.getElementsByName("iconColor")[0]).value = cfg.iconColor;
			inp.addEventListener("change", SaveSettings, false);
			inp.addEventListener("input", SaveSettings, false);

			if (chrome && !is_firefox)
			{
				document.getElementById("blockSetupPopupHotkey").getElementsByTagName("BUTTON")[0].style.display = "inline";
				document.getElementById("btnSetupPopupHotkey").addEventListener("click", function(event)
				{
					if (navigator.userAgent.indexOf(" OPR/") < 0)
					{
						// Chrome
						chrome.tabs.create({ url: "chrome://extensions/shortcuts" });
					}
					else
					{
						// ChrOpera
						chrome.tabs.create({ url: "opera://extensions/shortcuts" });
					}

					event.preventDefault();
					return false;
				}, false);
			}
			else
			{
				document.getElementById("blockSetupPopupHotkey").getElementsByTagName("BUTTON")[0].style.display = "none";
				var hotkey_box = document.getElementById("blockSetupPopupHotkey").getElementsByTagName("FIELDSET")[0];
				hotkey_box.appendChild(document.createTextNode(" [ "));
				var settings_addr = document.createElement("SPAN");
				settings_addr.innerText = "about:addons#shortcuts";
				hotkey_box.appendChild(settings_addr);
				hotkey_box.appendChild(document.createTextNode(" ] "));
			}

			// Use Translate Toolbar on each page
			if (cfg.useTranslateToolbar == null)
				cfg.useTranslateToolbar = "";
			if (is_firefox)
				cfg.useTranslateToolbar = "";
			(inp = document.getElementsByName("useTranslateToolbar")[0]).value = cfg.useTranslateToolbar;
			inp.addEventListener("change", SaveSettings, false);
			if (is_firefox)	// Firefox store did not support external scripts
			{
				inp.disabled = true;
				document.getElementById("blockUseTranslateToolbar").style.display = "none";
			}

			if (cfg.useContextMenuForPages == null)
				cfg.useContextMenuForPages = "";
			(inp = document.getElementsByName("useContextMenuForPages")[0]).value = cfg.useContextMenuForPages;
			inp.addEventListener("change", SaveSettings, false);


			// Bing keys
			(inp = document.getElementsByName("bingClientId")[0]).value = cfg.bingClientId;
			inp.addEventListener("change", SaveSettings, false);

			(inp = document.getElementsByName("bingClientSecret")[0]).value = cfg.bingClientSecret;
			inp.addEventListener("change", SaveSettings, false);


			// translate Providers
			var setAllowedProviders = document.getElementById("setAllowedProviders");
			var cbList = setAllowedProviders.getElementsByTagName("INPUT");
			var i, cnt = cbList.length;
			if (cfg.useProviders)
			{
				var useProviders = cfg.useProviders.split(",");
				for (i=0; i<cnt; i++)
				{
					cbList[i].checked = (useProviders.indexOf( cbList[i].id.substr(3).toLowerCase() ) >= 0);
					cbList[i].addEventListener("change", SaveSettings, false);
				}
			}
			else
			{
				for (i=0; i<cnt; i++)
				{
					cbList[i].checked = true;
					cbList[i].addEventListener("change", SaveSettings, false);
				}
			}

			// Other options
			cfg.useGoogleCn = (cfg.useGoogleCn == "true");
			(inp = document.getElementsByName("useGoogleCn")[0]).checked = cfg.useGoogleCn;
			inp.addEventListener("change", SaveSettings, false);

			cfg.openNewTabsNextToActive = (cfg.openNewTabsNextToActive == "true");
			(inp = document.getElementsByName("openNewTabsNextToActive")[0]).checked = cfg.openNewTabsNextToActive;
			inp.addEventListener("change", SaveSettings, false);
			if (!opera || opera.version()-0 >= 12.1)
				document.getElementsByName("openNewTabsNextToActive")[0].parentNode.style.display = "none";

			cfg.rememberLastTranslation = ((cfg.rememberLastTranslation == null) || (cfg.rememberLastTranslation == "true"));
			(inp = document.getElementsByName("rememberLastTranslation")[0]).checked = cfg.rememberLastTranslation;
			inp.addEventListener("change", SaveSettings, false);

			SetupUIBasedOnMode(cfg.mode);

			cfg.usePersonalDictionary = ((cfg.usePersonalDictionary == null) || (cfg.usePersonalDictionary == "true"));
			(inp = document.getElementsByName("usePersonalDictionary")[0]).checked = cfg.usePersonalDictionary;
			inp.addEventListener("change", SaveSettings, false);

/*
			cfg.useSpeechRecognition = ((cfg.useSpeechRecognition == null) || (cfg.useSpeechRecognition == "true"));
			(inp = document.getElementsByName("useSpeechRecognition")[0]).checked = cfg.useSpeechRecognition;
			inp.addEventListener("change", SaveSettings, false);
*/
			cfg.useSelMarker = ((cfg.useSelMarker == null) || (cfg.useSelMarker == "true"));
			(inp = document.getElementsByName("useSelMarker")[0]).checked = cfg.useSelMarker;
			inp.addEventListener("change", SaveSettings, false);

			cfg.useExtensionButton = ((cfg.useExtensionButton == null) || (cfg.useExtensionButton == "true"));
			(inp = document.getElementsByName("useExtensionButton")[0]).checked = cfg.useExtensionButton;
			inp.addEventListener("change", SaveSettings, false);

			cfg.useEnterToTranslate = (cfg.useEnterToTranslate == "true");
			(inp = document.getElementsByName("useEnterToTranslate")[0]).checked = cfg.useEnterToTranslate;
			inp.addEventListener("change", SaveSettings, false);

			cfg.useSelectionContextMenu = ((cfg.useSelectionContextMenu == null) || (cfg.useSelectionContextMenu == "true"));
			(inp = document.getElementsByName("useSelectionContextMenu")[0]).checked = cfg.useSelectionContextMenu;
			inp.addEventListener("change", SaveSettings, false);

			cfg.useOldSkin = (cfg.useOldSkin == "true");
			(inp = document.getElementsByName("useOldSkin")[0]).checked = cfg.useOldSkin;
			inp.addEventListener("change", SaveSettings, false);

			cfg.useBgProcess = ((cfg.useBgProcess == null) || (cfg.useBgProcess == "true"));
			(inp = document.getElementsByName("useBgProcess")[0]).checked = cfg.useBgProcess;
			inp.addEventListener("change", SaveSettings, false);
		});
	}
}, false);

window.addEventListener('load', function()
{
	document.getElementsByTagName("TITLE")[0].innerText						= WORDS.txtOptionsTitle;
	document.getElementById("txtMadeBy").innerText							= WORDS.txtMadeBy;

	document.getElementById("hdrDefaultSourceLang").innerText				= WORDS.hdrDefaultSourceLang;
	document.getElementsByName("defSourceLang")[0].options[0].text			= WORDS.optAutoDetect;
	document.getElementById("hdrDefaultTargetLang").innerText				= WORDS.hdrDefaultTargetLang;
	document.getElementsByName("defTargetLang")[0].options[0].text			= WORDS.optAutoDetect;
	document.getElementById("labelAlwaysUseDefTargetLang").innerText		= WORDS.labelAlwaysUseDefTargetLang;
	document.getElementsByName("defTargetLang2")[0].options[0].text			= WORDS.optAutoDetect;
	document.getElementById("labelDefTargetLang2").innerText				= WORDS.labelDefTargetLang2;
	document.getElementById("hdrAdditKnownLangs").innerText					= WORDS.hdrAdditKnownLangs;
	var selKnownLangs = document.getElementsByName("knownLangs[]");
	var i, cnt = selKnownLangs.length;
	for (i=0; i<cnt; i++)
		selKnownLangs[i].options[0].text									= WORDS.optNone;
	document.getElementById("hdrMaxStoredLangPairs").innerText				= WORDS.hdrMaxStoredLangPairs;
	document.getElementById("hdrTranslateBtnPosition").innerText			= WORDS.hdrTranslateBtnPosition;
	document.getElementById("txtAtRight").innerText							= WORDS.txtAtRight;
	document.getElementById("txtAtLeft").innerText							= WORDS.txtAtLeft;
	document.getElementById("txtInvertButons").innerText					= WORDS.txtInvertButons;
	document.getElementById("hdrTextareaFont").innerText					= WORDS.hdrTextareaFont;
	document.getElementsByName("useTextareaFont")[0].options[0].text		= WORDS.optDefault;
	document.getElementById("hdrUITheme").innerText							= WORDS.hdrUITheme;
	document.getElementById("hdrIconTheme").innerText						= WORDS.hdrIconTheme;
	document.getElementById("txtIconDefault").innerText						= WORDS.optDefault;
	document.getElementsByName("useTheme")[0].options[0].text				= WORDS.optAutoDetect;
	document.getElementsByName("useTheme")[0].options[1].text				= WORDS.optLight;
	document.getElementsByName("useTheme")[0].options[2].text				= WORDS.optDark;
	document.getElementById("wrnTextareaFont").innerHTML					= WORDS.wrnTextareaFont;
	if (chrome && !is_firefox)
	{
		document.getElementById("wrnTextareaFont").getElementsByTagName("A")[0].addEventListener("click", function(event)
		{
/*
			// doesn't work
			if (is_firefox)
			{
				chrome.tabs.create({ url: "about:preferences#general" });
			}
			else
*/
			if (navigator.userAgent.indexOf(" OPR/") < 0)
			{
				// Chrome
				chrome.tabs.create({ url: "chrome://settings/fonts" });
			}
			else
			{
				// ChrOpera
				chrome.tabs.create({ url: "opera://settings/fonts" });
			}

			event.preventDefault();
			return false;
		}, false);
	}
	else
	{
		// replace link to plain text
		var settings_link = document.getElementById("wrnTextareaFont").getElementsByTagName("A")[0];
		var settings_text = document.createElement("B");
		settings_text.innerText = settings_link.innerText;
		settings_link.parentNode.replaceChild(settings_text, settings_link);

		settings_text.parentNode.appendChild(document.createTextNode(" [ "));
		var settings_addr = document.createElement("SPAN");
		settings_addr.innerText = "about:preferences#fonts";
		settings_text.parentNode.appendChild(settings_addr);
		settings_text.parentNode.appendChild(document.createTextNode(" ] "));
	}
	document.getElementById("hdrSetupPopupHotkey").innerText				= WORDS.hdrSetupPopupHotkey;
	document.getElementById("btnSetupPopupHotkey").innerText				= WORDS.btnSetupPopupHotkey;
	document.getElementById("hdrUseTranslateToolbar").innerText				= WORDS.hdrUseTranslateToolbar;
	document.getElementsByName("useTranslateToolbar")[0].options[0].text	= WORDS.optDisabled;
	document.getElementById("wrnUseTranslateToolbar").innerHTML				= WORDS.wrnUseTranslateToolbar;
	document.getElementById("hdrUseContextMenuForPages").innerText			= WORDS.hdrUseContextMenuForPages;
	document.getElementsByName("useContextMenuForPages")[0].options[0].text	= WORDS.optDisabled;
	document.getElementById("hdrBingPrivateKey").innerText					= WORDS.hdrBingPrivateKey;
	document.getElementById("txtBingClientId").innerText					= WORDS.txtBingClientId;
	document.getElementById("txtBingClientSecret").innerText				= WORDS.txtBingClientSecret;
	document.getElementById("hintBingPrivateKey").innerHTML					= WORDS.hintBingPrivateKey;
	document.getElementById("hdrOtherOptions").innerText					= WORDS.hdrOtherOptions;
	document.getElementById("txtUseGoogleCn").innerText						= WORDS.txtUseGoogleCn;
	document.getElementById("txtRememberLastTranslation").innerText			= WORDS.txtRememberLastTranslation;
	document.getElementById("txtOpenNewTabsNextToActive").innerText			= WORDS.txtOpenNewTabsNextToActive;
	document.getElementById("txtUseTextToSpeech").innerText					= WORDS.txtUseTextToSpeech;
//	document.getElementById("txtUseSpeechRecognition").innerText			= WORDS.txtUseSpeechRecognition;
	document.getElementById("txtUseYellowMarker").innerText					= WORDS.txtUseYellowMarker;
	document.getElementById("txtOutputExtensionButton").innerText			= WORDS.txtOutputExtensionButton;
	document.getElementById("txtUseEnterToTranslate").innerText				= WORDS.txtUseEnterToTranslate;
	document.getElementById("txtUseSelectionContextMenu").innerText			= WORDS.txtUseSelectionContextMenu;
	document.getElementById("txtUseOldSkin").innerText						= WORDS.txtUseOldSkin;
	document.getElementById("txtUseBgProcess").innerText					= WORDS.txtUseBgProcess;

	if (chrome)	// chrome do not support this feature
		document.getElementById("txtOutputExtensionButton").parentNode.style.display = "none";

	if (!chrome || is_edge)
		document.getElementById("blockSetupPopupHotkey").style.display = "none";

//	if (!chrome || !chrome.browserAction || !chrome.browserAction.openPopup)
//		document.getElementById("blockContextMenuOfPages").style.display = "none";

//	if (is_firefox)	// Firefox doesn't allow to use background process
//		document.getElementById("useBgProcess").parentNode.style.display = "none";

	var services = document.getElementsByName("useTranslateToolbar")[0].options;
	var i, len = services.length;
	for (i=0; i<len; i++)
	{
		if (services[i].value == "google")
			services[i].text = WORDS.tbByGoogle;
		else if (services[i].value == "bing")
			services[i].text = WORDS.tbByBing;
		else if (services[i].value == "yandex")
			services[i].text = WORDS.tbByYandex;
	}

	var services = document.getElementsByName("useContextMenuForPages")[0].options;
	var i, len = services.length;
	for (i=0; i<len; i++)
	{
		if (services[i].value == "google") services[i].text = WORDS.tbByGoogle;
		else if (services[i].value == "bing") services[i].text = WORDS.tbByBing;
		else if (services[i].value == "yandex") services[i].text = WORDS.tbByYandex;
		else if (services[i].value == "promt") services[i].text = WORDS.tbByPromt;
	}


	// translate Providers
	document.getElementById("hdrAllowedProviders").innerText = WORDS.hdrAllowedProviders;
	var setAllowedProviders = document.getElementById("setAllowedProviders");
	var labelsList = setAllowedProviders.getElementsByTagName("LABEL");
	var i, cnt = labelsList.length;
	for (i=0; i<cnt; i++)
	{
		var wordName = "by"+labelsList[i].id.substr(6);
		if (wordName == "byDictionaries")
			wordName = "byBabylonDictionaries";
		if (WORDS[wordName])
			labelsList[i].innerText = WORDS[wordName];
	}

	document.getElementById("txtYouCanUseMyOtherProducts").innerText	= WORDS.txtYouCanUseMyOtherProducts;
/*
	if (opera || is_chropera)
		document.getElementById("txtMyCalendarExensionDescr").innerText		= WORDS.txtMyCalendarExensionDescr;
	else
		document.getElementById("myCalendarLink").style.display = "none";
*/
	document.getElementById("txtMyWebanketaServiceDescr").innerText		= WORDS.txtMyWebanketaServiceDescr;

	if (is_firefox)
		WORDS.txtPoweredByOpera = WORDS.txtPoweredByOpera.replace(" Opera", " Firefox");
	else if (is_edge || (navigator.userAgent.indexOf(" Edg/") > 0))
		WORDS.txtPoweredByOpera = WORDS.txtPoweredByOpera.replace(" Opera", " Edge");
	else if (chrome && (navigator.userAgent.indexOf(" OPR/") < 0))
		WORDS.txtPoweredByOpera = WORDS.txtPoweredByOpera.replace(" Opera", " Chrome");
	document.getElementById("txtPoweredByOpera").innerText				= WORDS.txtPoweredByOpera;

	if (widget)
	{
		document.getElementById("hdrRegistration").innerText = "* " + WORDS.txtRegistration;
		document.getElementById("unregistered_description").innerText = WORDS.txtUnregisteredModeDetails;
		document.getElementById("linkToVerify").innerText = WORDS.txtVerify;

		widget.cfg.getAsync([
								"translateLastFromTextLang",
								"translateToLang",
								"defTargetLang", "defTargetLang2"
							], function(cfg)
		{
			var html_box = document.getElementById("blockRegistration");

			var fromLang = cfg["translateLastFromTextLang"];
			var toLang = cfg["translateToLang"];
			var lang_pair = [ fromLang, toLang ];
			if (navigator.onLine)
			{
				var langs = lang_pair[0] + "-" + lang_pair[1] + "-" + (cfg.defTargetLang ? cfg.defTargetLang : DetectUserPreferedLanguage());
				var size = html_box.offsetWidth + "x" + html_box.offsetHeight;
				document.getElementById("settings_partner_window").src = "https://translator.sailormax.net/partners/img?target=settings_partner_window&langs="+langs+"&size="+size+"";
			}
		});
	}
}, false);
