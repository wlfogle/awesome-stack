/* globals chrome, widget, WORDS, is_edge, popupInSidebar, sbStoragePrefix, languages, lang_default_locales, propPopupSize, popupMinimumSize, TranslateText, TranslateUrl, DetectUserPreferedLanguage, OpenCopyrightLink, Trim, backTranslating, FixCfgPopupSize, VerifyWidgetAsync */
// (c) http://www.frequency-decoder.com/demo/detect-text-direction/
var ltrChars		= 'A-Za-z\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u02B8\u0300-\u0590\u0800-\u1FFF'+'\u2C00-\uFB1C\uFDFE-\uFE6F\uFEFD-\uFFFF',
	rtlChars		= '\u0591-\u07FF\uFB1D-\uFDFD\uFE70-\uFEFC',
	ltrDirCheckRe	= new RegExp('^[^'+rtlChars+']*['+ltrChars+']'),
	rtlDirCheckRe	= new RegExp('^[^'+ltrChars+']*['+rtlChars+']');
if (typeof WORDS == "undefined")
	WORDS = null;

var supportMP3 = null;
var backgroundPage = null;
if (chrome && chrome.runtime && chrome.runtime.getBackgroundPage)
	chrome.runtime.getBackgroundPage( function(bgPage){
		backgroundPage = bgPage;
	});


var activePage = {};
var useEnterToTranslate = false;
var defSourceLang = "";
var defTargetLang = DetectUserPreferedLanguage();
var useTargetLangAlways = false;
var defTargetLang2 = "";
var maxStoreLangPairs = 3;
var mostUsedLangPairs = [];
var detectedInputStringsLang = "";
var rememberLastTranslation = true;
var useGoogleCn = false;
var useGoogleTTS = true;
var usePersonalDictionary = true;
var useCopyTranslation = true;
var usePasteSourceText = true;
var requirePastePermission = false;

var prevValues = {};

/*
var SpeechRecognition = (window.SpeechRecognition || window.webkitSpeechRecognition);
var useSpeechRecognition = !!SpeechRecognition;
*/
// While SpeechRecognition doesn't support by Chrome in extensions
var useSpeechRecognition = false;

var lastNonAutodetect = "";

var msAppId = "";
var msSignId = "";
var yaAppId = "";
var yaSignId = "";
var ptAppId = "";
var ptSignId = "";
var udAppId = "";
var bdAppId = "";
var bdSignId = "";
var sgAppId = "";
var sgSignId = "";
var stAppId = "";
var nvAppId = "";
var lnAppId = "";

var last_inited_ime = null;

function GetTextBlockDataByType(type)
{
	var data = {};
	switch (type)
	{
		case "from":
			var fromLang = document.getElementById("fromLang");
			var textarea = document.getElementById("fromText");
			data = {
				tts_button: document.getElementById("fromTextPlayer"),
				lang: textarea["myLanguage"] || fromLang.value.split("~")[0] || "en",
				text: textarea.value,
			};
			break;

		case "to":
			var toLang = document.getElementById("toLang");
			var textarea = document.getElementById("toText");
			data = {
				tts_button: document.getElementById("toTextPlayer"),
				lang: textarea["myLanguage"] || toLang.value || "en",
				text: textarea.value,
			};
			break;

		case "to_from":
			var fromLang = document.getElementById("fromLang");
			var textarea = document.getElementById("toFromText");
			data = {
				tts_button: document.getElementById("toFromTextPlayer"),
				lang: document.getElementById("fromText")["myLanguage"] || fromLang.value.split("~")[0] || "en",
				text: textarea.value,
			};
			break;
	}
	return data;
}

function SetupImeBasedOnFromLang()
{
	if ((widget.mode == "demo") && false)	// NOTICE: temporary turn off demo
		return;

	var fromLang = document.getElementById("fromLang");
	var fromText = document.getElementById("fromText");
	var from_langs = fromLang.value.split('~', 2);

	if (last_inited_ime != from_langs[0])
	{
		// deinit previous IME
		switch (last_inited_ime)
		{
			case "ja":
				wanakana.unbind( fromText );
				break;

			case "ko":
				wanahangul.unbind( fromText );
				break;
		}
		last_inited_ime = null;

		// init new IME
		switch (from_langs[0])
		{
			case "ja":
				wanakana.bind( fromText );
				last_inited_ime = "ja";
				break;

			case "ko":
				wanahangul.bind( fromText );
				last_inited_ime = "ko";
				break;
		}
	}
}

function ChangeFromLang(sender)
{
	var lngs = sender.value.split('~', 2);
	var new_cfg = {};
	new_cfg[sbStoragePrefix+"translateFromLang"] = lngs[0];
	widget.cfg.setAsync(new_cfg);

	if (lngs.length > 1)	// if we have pair
	{
		var toLang = document.getElementById("toLang");
		toLang.value = lngs[1];
		ChangeToLang( toLang );
		SetupImeBasedOnFromLang();
	}
	else
		RepaintMostUsedLangPairs();

	lastNonAutodetect = lngs[0];		// rewrite on any manual change
}

function ChangeToLang(sender)
{
	var new_cfg = {};
	new_cfg[sbStoragePrefix+"translateToLang"] = sender.value;
	widget.cfg.setAsync(new_cfg);

	var fromLang = document.getElementById("fromLang");
	if (fromLang.value.indexOf('~') >= 0)	// if we have selected language pair
	{
		var lngs = fromLang.value.split('~', 2);
		var newLangPair = lngs[0]+'~'+sender.value;
		var i, len = fromLang.options.length;
		for (i=0; i<len; i++)
			if (fromLang.options[i].value == newLangPair)	// if we have result pair
			{
				fromLang.value = newLangPair;
				RepaintMostUsedLangPairs();
				return;
			}

		fromLang.value = lngs[0];
		RepaintMostUsedLangPairs();
	}

	prevValues["translateToLang"] = sender.value;
}

function ChangeTranslateProvider(new_provider)
{
	var translateProviderEl = document.getElementById("translateProvider");
	for (var option of translateProviderEl.options)
	{
		if ((option.value == new_provider) && (option.style.display == "none"))
		{
			for (var option of translateProviderEl.options)
				if (option.style.display != "none")
				{
					new_provider = option.value;
					break;
				}
			break;
		}
	}

	var new_cfg = {};
	new_cfg[sbStoragePrefix+"translateProvider"] = new_provider;
	widget.cfg.setAsync(new_cfg);

	if (translateProviderEl.value != new_provider)
		translateProviderEl.value = new_provider;
	translateProvider = new_provider;

	var lastChoosedProvider = prevValues["translateProvider"];

	// disable / enabled source language combobox
		var fromLangEl = document.getElementById("fromLang");
		if (new_provider == "urban")
		{
			var toLangEl = document.getElementById("toLang");
			if ((lastChoosedProvider != "urban") && (lastChoosedProvider != "dictionaries"))
			{
				lastNonAutodetect = fromLangEl.value;
				prevValues["translateToLang"] = toLangEl.value;
			}
			fromLangEl.value = "";
			window.setTimeout(function() { fromLangEl.disabled = true; }, 10);	// problem with redraw :(
			toLangEl.value = "en";
			toLangEl.disabled = true;
			document.getElementById("exchangeFromTo").disabled = true;
		}
		else if (new_provider == "dictionaries")
		{
			if (lastChoosedProvider == "urban")
			{
				var toLangEl = document.getElementById("toLang");
				toLangEl.value = prevValues["translateToLang"];
				toLangEl.disabled = false;
			}
			else if (lastChoosedProvider != "dictionaries")
				lastNonAutodetect = fromLangEl.value;
			fromLangEl.value = "";
			window.setTimeout(function() { fromLangEl.disabled = true; }, 10);	// problem with redraw :(
			document.getElementById("exchangeFromTo").disabled = true;
		}
		else
		{
			if (lastChoosedProvider == "dictionaries")
				fromLangEl.value = lastNonAutodetect;

			if (lastChoosedProvider == "urban")
			{
				fromLangEl.value = lastNonAutodetect;
				var toLangEl = document.getElementById("toLang");
				toLangEl.value = prevValues["translateToLang"];
				toLangEl.disabled = false;
			}
			fromLangEl.disabled = false;
			document.getElementById("exchangeFromTo").disabled = false;
		}

		prevValues["translateProvider"] = new_provider;
//		opera.extension.postMessage( {action:"change_translate_provider", provider:new_provider} );

	if (globalShiftKeyActive)
		TranslateText();
	SetupImeBasedOnFromLang();
}

function SetupBackTranslationVisibility( status )
{
//	var argumentStatus = status;
	if (status === undefined)
		status = document.getElementById("chkBackTranslation").checked;
	// do not auto setup checkbox, because this function can temporary hide backtranslation (urban, dictionaries,..)

	if (!status || (translateProvider == "dictionaries") || (translateProvider == "urban"))
	{
		// hide backTranslation
		window.setTimeout(function()
		{
			document.getElementsByTagName("FORM")[0].className = "no_back_translation";
			document.getElementById("txtBackTranslation").nextSibling.style.display = "none";
		}, 50);	// problem with redraw fromLang (Opera Classic) :(
	}
	else if (document.getElementsByTagName("FORM")[0].className.indexOf("no_back_translation") > -1)
	{
		// show backTranslation
		document.getElementById("toFromText").value = "";
		window.setTimeout(function()
		{
			document.getElementsByTagName("FORM")[0].className = "";
			document.getElementById("txtBackTranslation").nextSibling.style.display = "inline";
		}, 50);	// problem with redraw fromLang (Opera Classic) :(
	}
}

function RepaintMostUsedLangPairs( langPair )
{
	if (mostUsedLangPairs && maxStoreLangPairs)
	{
		var fromLang = document.getElementById("fromLang");
		var currPair = (langPair ? langPair : fromLang.value);

		while (fromLang.options[0].value !== "")
			fromLang.options.remove(0);

		var pair = "";
		var pairsList = mostUsedLangPairs.concat().reverse();	// reverse, because insert each into start
		for (var k in pairsList)
		{
			pair = pairsList[k];
			if (fromLang.options[0].value === "")
				fromLang.insertBefore( new Option( "——————————", "---" ), fromLang.options[0] ).disabled = 1;

			var lngs = pair.split('~', 2);
			if (pair == currPair)
			{
				if (lngs[0] === "")
				{
					fromLang.insertBefore( new Option( (languages[lngs[0]] || WORDS.optAutoDetect), pair ), fromLang.options[0] ).selected = true;
					currPair = "";	// Auto detect
				}
				else
					fromLang.insertBefore( new Option( languages[lngs[0]], pair ), fromLang.options[0] ).selected = true;
			}
			else
			{
				fromLang.insertBefore( new Option( "~" + (languages[lngs[0]] || WORDS.optAutoDetect) + " → " + languages[lngs[1]], pair ), fromLang.options[0] );
			}
		}

		if (opera && opera.extension)	// Opera Classic repaint bug
		{
			fromLang.style.opacity = "0.99";
			window.setTimeout(function()
			{
				fromLang.style.opacity = "1";
			}, 100);
		}
	}
	else if (typeof langPair == "string")
	{
		document.getElementById("fromLang").value = langPair.split('~', 2)[0];
	}
	SetupImeBasedOnFromLang();
}

function AddMostUsedLangPair(from_lang, to_lang)
{
	if (maxStoreLangPairs === 0)
		return;

	if (from_lang.indexOf('~') >= 0)
		from_lang = from_lang.split('~', 2)[0];
	var newLangPair = from_lang+'~'+to_lang;

	// remove similar pair and readd to the top
	mostUsedLangPairs = mostUsedLangPairs.filter(function(d) { return d != newLangPair; });
	mostUsedLangPairs.unshift(newLangPair);
	if (mostUsedLangPairs.length > maxStoreLangPairs)
		mostUsedLangPairs.splice(maxStoreLangPairs);

	RepaintMostUsedLangPairs( newLangPair );
	widget.cfg.setAsync( {"mostUsedLangPairs": JSON.stringify(mostUsedLangPairs)} );
}

function StoreDetectedInputStringsLang(lang)
{
	detectedInputStringsLang = lang.toLowerCase();

	var new_cfg = {};
	new_cfg[sbStoragePrefix+"detectedInputStringsLang"] = detectedInputStringsLang;
	widget.cfg.setAsync(new_cfg);

	document.getElementById("fromText")["myLanguage"] = detectedInputStringsLang;
}

function FixTextareaButtonsDirection(textarea, value)
{
	if (value == "")	// keep previous settings on clear textarea
		return;

	textarea.dir = rtlDirCheckRe.test(value) ? 'rtl' : (ltrDirCheckRe.test(value) ? 'ltr' : '');
	var ta_parent = textarea.parentNode;
	var ta_direction = GetStyleOfElement(textarea, "direction");
	if (ta_parent.className.indexOf(" "+ta_direction) < 0)
	{
		RemoveClass(ta_parent, "ltr");
		RemoveClass(ta_parent, "rtl");
		AppendClass(ta_parent, ta_direction);
	}
}

function StoreLastFromText()
{
	var fromLang = document.getElementById("fromLang");
	var fromText = document.getElementById("fromText");

	var lang = fromText["myLanguage"];
	if (!lang)
	{
		lang = fromLang.value;
		if (lang.indexOf("~"))
			lang = lang.split("~")[0];
	}

	var new_cfg = {};
	if (rememberLastTranslation)
		new_cfg[sbStoragePrefix+"translateLastFromText"] = fromText.value;
	else
		new_cfg[sbStoragePrefix+"translateLastFromText"] = "";
	new_cfg[sbStoragePrefix+"translateLastFromTextLang"] = lang;
	widget.cfg.setAsync(new_cfg);

	prevValues["translateLastFromText"] = fromText.value;
	prevValues["translateLastFromTextLang"] = lang;
}

function StoreLastToText()
{
	var toTextEl = document.getElementById("toText");

	var new_cfg = {};
	new_cfg[sbStoragePrefix+"translateLastToText"] = toTextEl.value;
	widget.cfg.setAsync(new_cfg);
}

//function StoreLastToFromText(value)
function StoreLastToFromText()
{
	var toFromTextEl = document.getElementById("toText");

	var new_cfg = {};
	new_cfg[sbStoragePrefix+"translateLastToFromText"] = toFromTextEl.value;
	widget.cfg.setAsync(new_cfg);
}

function CheckPossibilityToPutInDictionary()
{
	var from_lang = document.getElementById("fromText")["myLanguage"];
	var from_text = document.getElementById("fromText").value;
	var to_lang = document.getElementById("toText")["myLanguage"];
	var to_text = document.getElementById("toText").value;

	return (from_lang && to_lang && (from_lang !== "") && (from_text !== "") && (to_text !== ""));
}

function PasteSourceText(event)
{
	if (!usePasteSourceText)
		return;
/*
	if (requirePastePermission)
	{
		// old method, while Chrome can't ask permission inside popup window
		document.getElementById("fromText").focus();
		document.getElementById("fromText").value = "";
		document.execCommand('paste');
		document.getElementById("fromText").value = Trim(document.getElementById("fromText").value);
		OnEventOfFromText();

		if (event.shiftKey)
			TranslateText();
		return;
	}
*/
	var from_text_el = document.getElementById("fromText");
	from_text_el.focus();
	from_text_el.select();
	if (document.execCommand("paste"))	// require clipboardRead permission
	{
		from_text_el.value = Trim(from_text_el.value);
		OnEventOfFromText();
		if (event.shiftKey)
			TranslateText();
	}
	else
		console.log('Failed to read clippboard content.');
/*
	navigator.clipboard.readText()
		.then(function(text) {
			document.getElementById("fromText").value = Trim(text);
			OnEventOfFromText();

			if (event.shiftKey)
				TranslateText();
		})
		.catch(function() {
			console.log('Failed to read clippboard content.');
		});
*/
}

function CopyTranslation(event)
{
	if (!useCopyTranslation)
		return;

	var to_text = RemoveDictionaryFromText(document.getElementById("toText").value);
	if (to_text != "")
	{
		var ta = document.getElementById("toText");
		ta.focus();
		ta.setSelectionRange(0, to_text.length);
		if (!document.execCommand('copy'))	// doesn't require any permissions
			console.log('Failed to copy text.');
		ta.scrollTop = 0;	// after selection it croll to down :/
/*
		navigator.clipboard.writeText(to_text)
			.then(function() {
//				console.log('Text copied.');
			})
			.catch(function() {
				console.log('Failed to copy text.');
			});
*/
	}
}

function AddRemoveTranslationToFavorite()
{
	var from_lang = document.getElementById("fromText")["myLanguage"];
	var from_text = document.getElementById("fromText").value;
	var to_lang = document.getElementById("toText")["myLanguage"];
	var to_text = document.getElementById("toText").value;

	if (!CheckPossibilityToPutInDictionary())
		return;

	widget.cfg.getAsync("myDictionary", function(cfg)
	{
		var dictionary = {};
		if ((cfg["myDictionary"] != null) && (typeof cfg["myDictionary"] == "object") && !cfg["myDictionary"].slice)
			dictionary = cfg["myDictionary"];
		var lang_pair = from_lang + "~" + to_lang;
		if (typeof dictionary[lang_pair] == "undefined")
			dictionary[lang_pair] = [];

		var i, lang_pair_dict = dictionary[lang_pair];
		var found = false;
		for (i in lang_pair_dict)
			if (lang_pair_dict[i] && (lang_pair_dict[i][0] == from_text) && (lang_pair_dict[i][1] == to_text))
			{
				found = true;
				dictionary[lang_pair].splice(i, 1)
				break;
			}
		if (!found)
		{
			dictionary[lang_pair].push([from_text, to_text]);
		}

		var new_cfg = {};
		new_cfg["myDictionary"] = dictionary;
		widget.cfg.setAsync(new_cfg, function()
		{
			RepaintFavoriteButton();
		});
	});
}

function RepaintCopyButton()
{
	var toTextCopy = document.getElementById("toTextCopy");
	var to_text = document.getElementById("toText").value;

	if (to_text == "")
		toTextCopy.classList.add("disabled");
	else
		toTextCopy.classList.remove("disabled");
}

function RepaintFavoriteButton()
{
	var from_lang = document.getElementById("fromText")["myLanguage"];
	var from_text = document.getElementById("fromText").value;
	var to_lang = document.getElementById("toText")["myLanguage"];
	var to_text = document.getElementById("toText").value;
	var toTextFavorite = document.getElementById("toTextFavorite");

	// source language is unknown => can't put in dictionary
	if (!CheckPossibilityToPutInDictionary())
	{
		toTextFavorite.classList.remove("favortite");
		toTextFavorite.classList.add("disabled");
		if (WORDS)
			document.getElementById("toTextFavorite").title = WORDS.hntPutInTheDictionary;
		return;
	}
	toTextFavorite.classList.remove("disabled");

	// detect is this translate in dictionary or not
	widget.cfg.getAsync("myDictionary", function(cfg)
	{
		var dictionary = {};
		if (typeof cfg["myDictionary"] != "undefined")
			dictionary = cfg["myDictionary"];
		var lang_pair = (from_lang + "~" + to_lang).replace(/^~/, "");
		if (typeof dictionary[lang_pair] == "undefined")
		{
			toTextFavorite.classList.remove("favortite");
			if (WORDS)
				document.getElementById("toTextFavorite").title = WORDS.hntPutInTheDictionary;
			return;
		}

		var i, lang_pair_dict = dictionary[lang_pair];
		for (i in lang_pair_dict)
			if (lang_pair_dict[i] && (lang_pair_dict[i][0] == from_text) && (lang_pair_dict[i][1] == to_text))
			{
				toTextFavorite.classList.add("favortite");
				if (WORDS)
					document.getElementById("toTextFavorite").title	= WORDS.hntRemoveFromTheDictionary;
				return;
			}

		toTextFavorite.classList.remove("favortite");
		if (WORDS)
			document.getElementById("toTextFavorite").title = WORDS.hntPutInTheDictionary;
		return;
	});
}

function RemoveDictionaryFromText(str)
{
	if (str.match(/^([^\[]+)\[[^\n]+\]$/m))			// asd [asd] <= [asd]
	{
		// gogole
		str = Trim(RegExp.$1);
		str = str.replace(/ \<\=$/g, "");	// remove " <="
	}
	else if (str.match(/^([^\n]+)\n\n[^ \:\n]+:\n1\. [^\n]+/g))	// asd\n\nnoun:\n1. asd
	{
		// yandex
		str = Trim(RegExp.$1);
	}
	else if (str.match(/^[^ \:\n]+:\n1\. ([^\n]+)/g))	// noun:\n1. asd\n
	{
		// promt
		str = Trim(RegExp.$1);
	}
	else if (str.match(/^([^\n]+)\n\n[^ \:\n]+:\n- [^\n]+/g))	// noun:\n - asd\n
	{
		// deepl
		str = Trim(RegExp.$1);
	}
	return str;
}

function ExchangeLanguages(evt)
{
	var fromLang = document.getElementById("fromLang");
	var toLang = document.getElementById("toLang");

	var tmp_value = fromLang.value;
	if (tmp_value.indexOf('~') >= 0)
		tmp_value = tmp_value.split('~', 2)[0];

	// if fromLang = auto_detect => DetectUserPreferedLanguage()
	if (tmp_value === "")
	{
		tmp_value = detectedInputStringsLang;
		if ((tmp_value === "") || (tmp_value == toLang.value))
			tmp_value = defTargetLang;
	}
	// if same langauges => from = auto_detect
	if (tmp_value == toLang.value)
		fromLang.value = "";
	else
	{
		ShowAutodetectedLang('');
		fromLang.value = toLang.value;
		StoreDetectedInputStringsLang("");
		toLang.value = tmp_value;

		ChangeFromLang(fromLang);
		ChangeToLang(toLang);

		var fromText = document.getElementById("fromText");

		// exchange texts
		if (evt.shiftKey)
		{
			var toText = document.getElementById("toText");
			var tmp_value = toText.value;
			toText.value = fromText.value;
			fromText.value = RemoveDictionaryFromText(tmp_value);

			OnEventOfFromText();
			OnEventOfToText();

			// user want edit and retranslate in backward order
			fromText.focus();
		}
		else
		{
			// user want enter other sentence
			fromText.select();
			fromText.focus();
		}
	}
	SetupImeBasedOnFromLang();
}


// events handlers
function OnEventOfFromLang(event)
{
	if (event)
	{
		var fromLangEl = this;
		switch (event.type)
		{
			case "focus":
				ShowAutodetectedLang('');
				return true;
			case "keydown":
				return CheckCtrlEnter(event);
			case "change":
				break
		}
	}
	else
	{
		var fromLangEl = document.getElementById("fromLang");
	}

	ChangeFromLang(fromLangEl);

	return true;
}

function OnEventOfToLang(event)
{
	if (event)
	{
		var toLangEl = this;
		switch (event.type)
		{
			case "keydown":
				return CheckCtrlEnter(event);
			case "change":
				break;
		}
	}
	else
	{
		var toLangEl = document.getElementById("toLang");
	}

	ChangeToLang(toLangEl);

	return true;
}

function OnEventOfProvider(event)
{
	if (event)
	{
		var providerEl = this;
		switch (event.type)
		{
			case "keydown":
				return CheckCtrlEnter(event);
			case "input":
			case "change":
				break;
		}
	}
	else
	{
		var providerEl = document.getElementById("translateProvider");
	}

	ChangeTranslateProvider(providerEl.value);

	return true;
}

function OnEventOfFromText(event, new_value, new_value_lang)
{
	var result = true;
	if (event)
	{
		var fromTextEl = this;
		switch (event.type)
		{
			case "keydown":
//				this["myLanguage"] = null;
//				return CheckEnterOrCtrlEnter(event);
				result = CheckEnterOrCtrlEnter(event);
				break;
			case "keyup":
/*
				StoreLastFromText(this.value, false);
				RepaintFavoriteButton();
				break;
*/
			case "change":
				break;
		}
	}
	else
	{
		var fromTextEl = document.getElementById("fromText");
		var fromLangEl = document.getElementById("fromLang");
		if (new_value)
			fromTextEl.value = new_value;
		if (new_value_lang || (new_value_lang === ""))			// autodetected language
		{
			StoreDetectedInputStringsLang( new_value_lang );
			ShowAutodetectedLang( new_value_lang );
		}
	}

	StoreLastFromText();
	RepaintFavoriteButton();
	FixTextareaButtonsDirection( fromTextEl, fromTextEl.value );

	if (useSpeechRecognition)
		fromLangMicrophone.changeText(fromTextEl.value);

	if (useGoogleTTS)
		RefreshTTSButtonByType("from");

	return result;
}

function OnEventOfToText(event)
{
	if (event)
	{
		var toTextEl = this;
	}
	else
	{
		var toTextEl = document.getElementById("toText");
		var toLangEl = document.getElementById("toLang");
	}

	StoreLastToText();
	RepaintFavoriteButton();
	RepaintCopyButton();
	FixTextareaButtonsDirection( toTextEl, toTextEl.value );

	if (useGoogleTTS)
		RefreshTTSButtonByType("to");

	return true;
}

function OnEventOfFromToText(event)
{
	if (event)
	{
		var toFromTextEl = this;
	}
	else
	{
		var toFromTextEl = document.getElementById("toFromText");
	}

	StoreLastToFromText();
	FixTextareaButtonsDirection( toFromTextEl, toFromTextEl.value );

	if (useGoogleTTS)
		RefreshTTSButtonByType("to_from");

	return true;
}
/*
function OnTranslate()
{
	RepaintFavoriteButton();

	return true;
}
*/
//


function StartTranslateByHotkey(evt)
{
	document.getElementById("fromText").focus();
	TranslateText();

	// do not reload popup
	if (evt.stopPropagation)
		evt.stopPropagation();
	if (evt.preventDefault)
		evt.preventDefault();
	evt.cancelBubble = true;
	evt.returnValue = false;
	return false;
}

function CheckEnterOrCtrlEnter(evt)
{
	if (useEnterToTranslate && (evt.keyCode == 13))
		return StartTranslateByHotkey(evt);
	return CheckCtrlEnter(evt);
}

function CheckCtrlEnter(evt)
{
	if ((evt.ctrlKey || evt.metaKey) && (evt.keyCode == 13))	// Ctrl or Control (Mac)
		return StartTranslateByHotkey(evt);
	return true;
}


var animation, lastAnimatedChar = "";
function StartTranslatingAnimation()
{
	var animationTarget = (backTranslating ? "toFromText" : "toText");
	if (animation)
		StopTranslatingAnimation();
	animation = window.setInterval(function()
	{
		if (lastAnimatedChar === "") lastAnimatedChar = "|";
		else if (lastAnimatedChar == "|") lastAnimatedChar = "/";
		else if (lastAnimatedChar == "/") lastAnimatedChar = "-";
		else if (lastAnimatedChar == "-") lastAnimatedChar = "\\";
		else if (lastAnimatedChar == "\\") lastAnimatedChar = "|";
		if (animation)
			document.getElementById(animationTarget).value = lastAnimatedChar;
	}, 50);
}

function StopTranslatingAnimation()
{
	window.clearInterval(animation);
	animation = null;
	lastAnimatedChar = "";
}

function ShowAutodetectedLang(detectedLang)
{
	var fromLang = document.getElementById("fromLang");

	if ((fromLang.options[ fromLang.selectedIndex ].value === "") || (fromLang.options[ fromLang.selectedIndex ].value.indexOf("~") === 0))
	{
		if (!languages[detectedLang] && (detectedLang.indexOf("-") > 0))
		{
			// try find zh-CN
			var parts = detectedLang.split("-");
			parts[1] = parts[1].toUpperCase();
			if (languages[parts.join("-")])
				detectedLang = parts.join("-");
			else
			{
				// try find sr-Cyrl
				parts = detectedLang.split("-");
				parts[1] = parts[1].charAt(0).toUpperCase() + parts[1].substr(1);
				if (languages[parts.join("-")])
					detectedLang = parts.join("-");
			}
		}

		if (detectedLang && languages[detectedLang])
			fromLang.options[ fromLang.selectedIndex ].text = (WORDS ? WORDS.optAutoDetect : "Auto detect") + " ("+languages[detectedLang]+")";	// Auto detect
		else
			fromLang.options[ fromLang.selectedIndex ].text = (WORDS ? WORDS.optAutoDetect : "Auto detect");	// "Auto detect";
	}
}


function ClearText()
{
	document.getElementById("fromText").value = "";
	document.getElementById("toText").value = "";
	document.getElementById("toFromText").value = "";
//	StoreLastFromText("");
	OnEventOfFromText();
//	StoreLastToText("");
	OnEventOfToText();
	StoreLastToFromText("");
	SetupUIBasedOnMode(widget.mode);
}

function onMessageHandler(msg, sender)
{
	if (opera)
	{
		sender = msg.source;
		msg = msg.data;
	}

	if (typeof msg == "undefined")
		msg = {};

	if (msg.action == "set_active_page_data")
	{
		activePage = { url:msg.url, title:msg.title };
		if (activePage.url)
		{
			// 	var tab = opera.extension.bgProcess.opera.extension.tabs.getFocused();	// still not better :(
			var btnTranslateUrl = document.getElementById("btnTranslateUrl");
			btnTranslateUrl.style.visibility = "visible";
			btnTranslateUrl.getElementsByTagName("SPAN")[1].innerText = '"'+activePage.title+'"';
			btnTranslateUrl.title = activePage.title;
		}

		var fromText = document.getElementById("fromText");
		var fromLang = document.getElementById("fromLang");
		widget.cfg.getAsync([
								sbStoragePrefix+"translateLastFromText",
								sbStoragePrefix+"translateLastFromTextLang",
								sbStoragePrefix+"translateLastToText",
								sbStoragePrefix+"translateLastToFromText"
							], function(cfg)
		{
			prevValues["translateLastFromText"]		= cfg[sbStoragePrefix+"translateLastFromText"];
			prevValues["translateLastFromTextLang"]	= cfg[sbStoragePrefix+"translateLastFromTextLang"];
			prevValues["translateLastToText"]		= cfg[sbStoragePrefix+"translateLastToText"];
			prevValues["translateLastToFromText"]	= cfg[sbStoragePrefix+"translateLastToFromText"];
			if (!rememberLastTranslation)
			{
				prevValues["translateLastFromText"]	= "";
				prevValues["translateLastToText"]	= "";
			}

			var prev_value		= prevValues["translateLastFromText"];
			var prev_value_lang	= prevValues["translateLastFromTextLang"];

			if (msg.selection && (prev_value != msg.selection))		// if new selection
			{
				fromText.value = Trim(msg.selection);
				fromText.select();
//				StoreLastFromText(fromText.value);
				OnEventOfFromText();

				// pragma and yandex
				var provider = document.getElementById("translateProvider").value;
				if ((defSourceLang !== "")
					|| ((provider != "pragma") && (provider != "yandex")))
				{
					fromLang.value = defSourceLang;
					ChangeFromLang(fromLang);
				}

				TranslateText(true);
			}
			else
			{
				if (prev_value)
				{
					fromText.value = prev_value;
					fromText["myLanguage"] = prev_value_lang;
					if (useSpeechRecognition)
						fromLangMicrophone.changeText(prev_value);
					if (useGoogleTTS)
						RefreshTTSButtonByType("from");
					fromText.select();
					FixTextareaButtonsDirection(fromText, prev_value);
				}

				if (rememberLastTranslation && (prev_value = cfg[sbStoragePrefix+"translateLastToText"]))
				{
					var toText = document.getElementById("toText");
					toText.value = prev_value;
					toText["myLanguage"] = document.getElementById("toLang").value;
					if (useGoogleTTS)
						RefreshTTSButtonByType("to");
					FixTextareaButtonsDirection(toText, prev_value);
				}

				if (rememberLastTranslation && (prev_value = cfg[sbStoragePrefix+"translateLastToFromText"]))
				{
					var toFromText = document.getElementById("toFromText");
					toFromText.value = prev_value;
					if (useGoogleTTS)
						RefreshTTSButtonByType("to_from");
					FixTextareaButtonsDirection(toFromText, prev_value);
				}

				RepaintFavoriteButton();
				RepaintCopyButton();
			}
		});
	}
	else if (msg.action == "fix_elements_size")	// on resize (Opera did not support onResize event handler for popups)
	{
/*
		// fix sizes of language lists
		var fromLang = document.getElementById("fromLang");
		fromLang.style.width = (fromLang.myDefaultWidth + Math.floor(msg.deltaX / 2)) + "px";
		var toLang = document.getElementById("toLang");
		toLang.style.width = (toLang.myDefaultWidth + Math.floor(msg.deltaX / 2)) + "px";

		// get to_from_block height
		var minusHeight = 0;
		if (document.getElementById("to_from_block").style.display == "block")
			minusHeight = document.getElementById("to_from_block").clientHeight;

		// fix sizes of textareas
		var textarea = document.getElementById("result_as_text").getElementsByTagName("TEXTAREA")[0];
		textarea.style.height = (textarea.myDefaultHeight + msg.deltaY - minusHeight) + "px";
		var iframe = document.getElementById("result_as_html").getElementsByTagName("IFRAME")[0];
		iframe.style.height = (iframe.myDefaultHeight + msg.deltaY - minusHeight) + "px";
*/
	}
	else if (msg.action == "setup_msAppId")
	{
		window.msAppId = msg.value;
		window.msSignId = msg.value2;
//		if (status.data["refreshed"]) TranslateText();
//		if (status.data["next_action"] == "retranslate") TranslateText();
		if (msg.next_action == "retranslate")
			TranslateText();
		else if (msg.next_action == "TextReader.Play")
			TextReader.Play(msg.next_action_args["sender"], msg.next_action_args["lang"], msg.next_action_args["text"]);
	}
	else if (msg.action == "setup_yaAppId")
	{
		window.yaAppId = msg.value;
		window.yaSignId = msg.value2;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "setup_ptAppId")
	{
		window.ptAppId = msg.value;
		window.ptSignId = msg.value2;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "setup_udAppId")
	{
		window.udAppId = msg.value;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "setup_bdAppId")
	{
		window.bdAppId = msg.value;
		window.bdSignId = msg.value2;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "setup_sgAppId")
	{
		window.sgAppId = msg.value;
		window.sgSignId = msg.value2;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "setup_stAppId")
	{
		window.stAppId = msg.value;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "setup_nvAppId")
	{
		window.nvAppId = msg.value;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "setup_lnAppId")
	{
		window.lnAppId = msg.value;
		if (msg.next_action == "retranslate")
			TranslateText();
	}
	else if (msg.action == "retranslate_text")
	{
		TranslateText();
	}

	return true;
}

function OnOpenLinkInSeparateTab()
{
	// currently Edge can't open regular link to other extension page
	// this is workaround
	if (is_edge)
	{
		chrome.tabs.create({ url: document.URL.replace("popup.html", "settings.html") });
		return false;
	}
	else if (opera)
		this.target = "_blank";	// Opera Classic does not support named windows here

	return true;	// Chrome support regular links
}

function SetupPreloadedUIBasedOnSettings()
{
	if (useSpeechRecognition)	// && !demo
	{
		fromLangMicrophone.button = document.getElementById("fromTextMicrophone");
		fromLangMicrophone.source = document.getElementById("fromText");
		fromLangMicrophone.language = document.getElementById("fromLang");

		fromLangMicrophone.button.style.display = "block";
	}

// init TTS buttons
	if (useGoogleTTS)
	{
		document.getElementById("fromTextPlayer").style.display = "block";
		document.getElementById("toTextPlayer").style.display = "block";
		document.getElementById("toFromTextPlayer").style.display = "block";
	}

	if (usePersonalDictionary)
	{
		var toTextFavorite = document.getElementById("toTextFavorite");
		toTextFavorite.style.display = "block";
	}

//	if (navigator.clipboard && navigator.permissions)
	{
		var ActivatePasteButton = function()
		{
			var fromTextPaste = document.getElementById("fromTextPaste");
			if (!useGoogleTTS)
				fromTextPaste.classList.add("single");
			fromTextPaste.style.display = "block";
		};

		var ActivateCopyButton = function()
		{
			var toTextCopy = document.getElementById("toTextCopy");
			if (!useGoogleTTS)
				toTextCopy.classList.add("single");
			toTextCopy.style.display = "block";
		};

		//if (document.queryCommandSupported("paste"))	// Firefox return false, but it does work :)
			ActivatePasteButton();
		//else
		//	console.log("Paste from clipboard is not permitted.");

		//if (document.queryCommandSupported("copy"))	// looks like work everytime
			ActivateCopyButton();
		//else
		//	console.log("Copy to clipboard is not permitted.");


/*
		if (is_firefox)
		{
			// Firefox already has permissions on these actions by manifest
			ActivateCopyButton();
			ActivatePasteButton();
		}
		else
		{
			navigator.permissions.query({name: "clipboard-write"})
				.then(function(status) {
					ActivateCopyButton();
					if (status.state != "granted")
						useCopyTranslation = false;
				})
				.catch(function() {
					useCopyTranslation = false;
					console.log("Clipboard API unavailable. (write)");
				});

			navigator.permissions.query({name: "clipboard-read"})
				.then(function(status) {
					ActivatePasteButton();
					if (status.state == "prompt")
						requirePastePermission = true;
					else if (status.state != "granted")
						usePasteSourceText = false;
				})
				.catch(function() {
					usePasteSourceText = false;
					console.log("Clipboard API unavailable. (read)");
				});
		}
*/
	}

// data for resize
	if (!popupInWindow && !popupInSidebar && !popupInMenu)
	{
		if (opera && opera.extension)
			opera.extension.postMessage( {action:"change_popup_size", deltaX:0, deltaY:0} );
		else// if ((typeof chrome != "undefined") && chrome.runtime.onMessage)
		{
			var html = document.getElementsByTagName("HTML")[0];
			var form = document.getElementsByTagName("FORM")[0];

			if (is_firefox && !window.innerWidth && !window.innerHeight)	// Firefox menu mode on start has zero window size properties
			{
				popupInMenu = true;
			}
			else
			{
				// Possible deprecated. Firefox 86 in most cases work without it
				if (!is_firefox
					|| (html.offsetWidth > 1.5*propPopupSize[0])			// Firefox by default has wide popup, but sidebars and inline popup - not
					|| ((html.offsetWidth == propPopupSize[0]) && (html.offsetHeight < propPopupSize[1]))	// Firefox with maximized popup size
					|| (html.offsetWidth > 350 && html.offsetHeight < html.offsetWidth*2)			// Firefox by default has wide popup, but sidebars and inline popup - not
					|| (html.offsetWidth < 240)	// Firefox now has small popup by default
					)
				{
					form.style.width = propPopupSize[0] + "px";
				}
				else
				{
	//				// autosize width => disable resize
	//				document.getElementById("imgWindowsResizer").style.visibility = "hidden";
					// autosize width, but resizible height => leave resize

					// this is second detection, because first was more for mobile phones
					popupInMenu = true;
				}
			}
			form.style.height = propPopupSize[1] + "px";

			// Firefox for Android
			if (is_firefox && !popupInMenu && (window.innerWidth < window.innerHeight))	// mobile phones
			{
				// too small form
				if ((1.5*form.offsetWidth < window.innerWidth) && (1.5*form.offsetHeight < window.innerHeight))
				{
					var zoomRatio = ((window.innerWidth-30) / form.offsetWidth);	// based on width
					if (form.offsetHeight*zoomRatio > window.innerHeight)
						zoomRatio = ((window.innerHeight-30) / form.offsetHeight);	// based on height

					form.style.transform = "scale("+zoomRatio+")";
					form.style.transformOrigin = "top left";
				}
				else if (window.innerWidth < 220)	// default popup size
				{
					form.style.width = propPopupSize[0] + "px";
				}
			}

			// Alpha versions of Opera can be broken with regular size => setup minimum
//			form.style.minWidth = popupMinimumSize.width + "px";
//			form.style.minHeight = popupMinimumSize.height + "px";
/*
			// new layout not require fix elements. Made by CSS
			onMessageHandler({
				action: "fix_elements_size",
				deltaX: parseInt(form.style.width)-popupMinimumSize.width,
				deltaY: parseInt(form.style.height)-popupMinimumSize.height
			});
*/
		}
	}

	if (popupInMenu)
	{
		var html = document.getElementsByTagName("HTML")[0];
//		var form = document.getElementsByTagName("FORM")[0];
		html.style.width = "100%";
//		html.style.height = "100%";
//		form.style.width = "100%";
	}

// try to get page selection or load previous used data
	var requireSelection = !(popupInWindow || popupInSidebar);
	if (popupInWindow && window.URL)
	{
		// get default text from URL
		requireSelection = ((new URL(document.location.href)).searchParams.get("selection") == "1");
	}

	if (requireSelection)
	{
		if (opera && opera.extension)
		{
			opera.extension.onmessage = onMessageHandler;
/*
			opera.extension.postMessage( {action:"get_active_page_data"} );
*/
	//		if (useGoogleTTS)
	//			opera.extension.postMessage( {action:"get_msAppId"} );
		}
		else if (chrome)
		{
			if (chrome.runtime && chrome.runtime.onMessage)
			{
				chrome.runtime.onMessage.addListener( onMessageHandler );
/*
				chrome.runtime.getBackgroundPage( function(bgPage){
					bgPage.onMessageHandler( {action:"get_active_page_data"}, window );
//						bgPage.postMessage( {action:"get_active_page_data", sender:window} );
				});
*/
/*
				if (backgroundPage)
					backgroundPage.onMessageHandler( {action:"get_active_page_data"}, window );
*/
			}
		}

		SendMessageToBackgroundScript( {action:"get_active_page_data"} );
	}
	else
	{
		onMessageHandler({ action:"set_active_page_data" }, window);
	}

	// force recalculate media queries, because fresh Chrome has some cache for width size :/
	document.styleSheets[0].disabled = true;
	window.setTimeout(function() {
//		console.log( window.innerWidth );
//		console.log( window.matchMedia("max-width:409px").matches );	// looks like size of BODY
		document.styleSheets[0].disabled = false;
	}, 1);
}

function InitGlobalVariables(cfg)
{
	propPopupSize = FixCfgPopupSize(cfg.popupSize);

	useEnterToTranslate = (cfg.useEnterToTranslate == "true");
	useTargetLangAlways = (cfg.useTargetLangAlways == "true");

	if (languages[cfg.defSourceLang])
		defSourceLang = cfg.defSourceLang;

	if (languages[cfg.defTargetLang])
		defTargetLang = cfg.defTargetLang;
	if (languages[cfg.defTargetLang2])
		defTargetLang2 = cfg.defTargetLang2;

	if ((cfg.maxStoreLangPairs != null) && (cfg.maxStoreLangPairs-0) >= 0)
		maxStoreLangPairs = cfg.maxStoreLangPairs-0;

	if (cfg.mostUsedLangPairs)
	{
		mostUsedLangPairs = JSON.parse(cfg.mostUsedLangPairs);

		if (!mostUsedLangPairs.join)
		{
			// convert to array old settings (15.05.2020)
			var langPairsArr = [];
			for (var k in mostUsedLangPairs)
				langPairsArr.push(k);
			mostUsedLangPairs = langPairsArr;
		}
	}

	if (cfg[sbStoragePrefix+"detectedInputStringsLang"] != null)
		detectedInputStringsLang = cfg[sbStoragePrefix+"detectedInputStringsLang"];

	rememberLastTranslation = ((cfg.rememberLastTranslation == null) || (cfg.rememberLastTranslation == "true"));

	useGoogleCn = (cfg.useGoogleCn == "true");

	useGoogleTTS = ((cfg.useGoogleTTS == null) || (cfg.useGoogleTTS == "true"));
	usePersonalDictionary = ((cfg.usePersonalDictionary == null) || (cfg.usePersonalDictionary == "true"));
//	useSpeechRecognition = SpeechRecognition && (((cfg.useSpeechRecognition == null) || (cfg.useSpeechRecognition == "true")) && (cfg.mode != "demo"));
}

function TranslateUI()
{
	if (!WORDS)
		return;

	document.getElementById("txtUse").innerText					= WORDS.txtUse;
	document.getElementById("txtBackTranslation").innerText		= WORDS.txtBackTranslation;
	document.getElementById("btnTranslate").value				= WORDS.btnTranslate;
	document.getElementById("btnClear").value					= WORDS.btnClear;

	document.getElementById("exchangeFromTo").title			= WORDS.hntFullExchange;
	document.getElementById("fromText").title				= WORDS.hntTranslate;
	document.getElementById("translateProvider").title		= WORDS.hntReTranslate;

	document.getElementById("fromTextPlayer").title			= WORDS.hntVocalize;
	document.getElementById("toTextPlayer").title			= WORDS.hntVocalize;
	document.getElementById("toFromTextPlayer").title		= WORDS.hntVocalize;
	document.getElementById("toTextFavorite").title			= WORDS.hntPutInTheDictionary;
	document.getElementById("toTextCopy").title				= WORDS.hntCopy;
	document.getElementById("fromTextPaste").title			= WORDS.hntPaste;

	if (usePersonalDictionary)
	{
		document.getElementById("lnkDictionaryPage").innerText	= WORDS.txtDictionary;
		document.getElementById("lnkDictionaryPage").addEventListener("click", OnOpenLinkInSeparateTab);
	}
	else
		document.getElementById("lnkDictionaryPage").parentNode.style.display = "none";


	document.getElementById("lnkSettingsPage").innerText = WORDS.lnkSettingsPage;
	document.getElementById("lnkSettingsPage").addEventListener("click", OnOpenLinkInSeparateTab);

	var services = document.getElementById("translateProvider").options;
	var len = services.length;
	for (var i=0; i<len; i++)
	{
		var wordName = "by"+(services[i].value.charAt(0).toUpperCase() + services[i].value.slice(1));
		if (wordName == "byDictionaries")
			wordName = "byBabylonDictionaries";
		if (WORDS[wordName])
			services[i].text = WORDS[wordName];
	}

	document.getElementById("btnTranslateUrl").getElementsByTagName("SPAN")[0].innerText = WORDS.txtTranslateActivePage;
	document.getElementById("lnkMaximize").title = WORDS.hntMaximize;
}

var translateProvider = "";
var backTranslation = "false";
// WORDS can be loaded after DOMContentLoaded
window.addEventListener('load', function()
{
	translateProvider = document.getElementById("translateProvider").value;

	if (widget)
	{
		widget.cfg.getAsync([
								"useOldSkin", "popupSize",
								"useTextareaFont",
								"buttonPosition", "buttonPositionInvert",
								"useEnterToTranslate",
								"defSourceLang", "defTargetLang", "useTargetLangAlways", "defTargetLang2",
								"maxStoreLangPairs", "mostUsedLangPairs",
								sbStoragePrefix+"detectedInputStringsLang",
								sbStoragePrefix+"translateProvider",
								sbStoragePrefix+"backTranslation",
								sbStoragePrefix+"translateFromLang",
								sbStoragePrefix+"translateToLang",
								"rememberLastTranslation", "useGoogleCn", "useGoogleTTS", "useSpeechRecognition", "usePersonalDictionary",
								"mode"
							], function(cfg)
		{
			if (cfg.useOldSkin == "true")
			{
				// remove new skin reference
				var stylesheets = document.querySelectorAll('link[rel=stylesheet]');
				var i, cnt = stylesheets.length;
				for (var i=0;i<stylesheets.length;i++)
					if (stylesheets[i].href.indexOf("theme.css") > 0)
						stylesheets[i].parentNode.removeChild(stylesheets[i]);
			}

			if (cfg.useTextareaFont && cfg.useTextareaFont.length > 0)
			{
				var tas = document.getElementsByTagName("TEXTAREA");
				for (i=0; i<tas.length; i++)
					tas[i].style.fontFamily = cfg.useTextareaFont;
			}

			// setup button position
			if (cfg.buttonPosition == "left")
				document.getElementById("submitButtons").className += " invert";

			if (cfg.buttonPositionInvert == "true")
			{
				var btnClear = document.getElementById("btnClear");
				var tmpParent = btnClear.parentNode;
				tmpParent.insertBefore( tmpParent.removeChild(btnClear), document.getElementById("btnTranslate") );
			}

			InitGlobalVariables(cfg);
			TranslateUI();

			// setup translateProvider
				if (cfg[sbStoragePrefix+"translateProvider"])
					translateProvider = cfg[sbStoragePrefix+"translateProvider"];
				ChangeTranslateProvider(translateProvider);

			// setup backTranslation checkbox
				backTranslation = cfg[sbStoragePrefix+"backTranslation"];
				document.getElementById("chkBackTranslation").checked = (backTranslation == "true");
				SetupBackTranslationVisibility();

			// setup fromLangs
				var fromLang = document.getElementById("fromLang");
				fromLang.options.remove(0);

				fromLang.options.add( new Option( WORDS ? WORDS.optAutoDetect : "Auto detect", "" ) );	// "Auto detect"
				fromLang.options.add( new Option( "——————————", "---" ) );
				fromLang.options[fromLang.options.length-1].disabled = 1;

				for (i in languages)
					fromLang.options.add( new Option( languages[i], i ) );

				fromLang.value = cfg[sbStoragePrefix+"translateFromLang"] || defSourceLang || "";

			// setup toLangs
				var toLang = document.getElementById("toLang");
				toLang.options.remove(0);
				var target_langs = GetUserPreferedLanguages();
				for (i in target_langs)
					toLang.options.add( new Option( languages[ target_langs[i] ], target_langs[i] ) );
				toLang.options.add( new Option( "——————————", "---" ) );
				for (i in languages)
					toLang.options.add( new Option( languages[i], i ) );

				prevValues["translateToLang"] = cfg[sbStoragePrefix+"translateToLang"] || defTargetLang || "";

				if (toLang.disabled)
				{
					// provider without target language (Urban Dictionary)
					toLang.value = "en";
				}
				else
				{
					var default_target_langs = [];
					if (cfg.useTargetLangAlways == "true")
						default_target_langs = [ defTargetLang, defTargetLang2, prevValues["translateToLang"], "en" ];
					else
						default_target_langs = [ prevValues["translateToLang"], defTargetLang, defTargetLang2, "en" ];

					for (var i in default_target_langs)
						if (default_target_langs[i] && default_target_langs[i].length && (fromLang.value != default_target_langs[i]))
						{
							toLang.value = default_target_langs[i];
							break;
						}

					if (toLang.value === "")					// toLang don't has Auto-detect => select first item
						toLang.options[0].selected = true;
				}

			RepaintMostUsedLangPairs( fromLang.value+'~'+toLang.value );
			ShowAutodetectedLang( detectedInputStringsLang );
			SetupPreloadedUIBasedOnSettings();

			document.getElementById("fromText").focus();
		});
	}
	else
	{
		SetupPreloadedUIBasedOnSettings();
	}
});

if (useSpeechRecognition)
{
	var fromLangMicrophone	= { button:null, source:null, language:null, status:-1, mic:null };

	fromLangMicrophone.changeText = function(text)
	{
		if (this.status > 0)
			this.stop();
		this.status = -1;

		if (this.button)
		{
			if (text.length)
				this.button.className = "microphone_button";
			else
				this.button.className = "microphone_button disabled";
		}
	};
	fromLangMicrophone.stop = function()
	{
		if (this.mic)
			this.mic.stop();
		this.status = 0;
		this.button.innerText = "";
		this.button.className = "microphone_button";
	};

	fromLangMicrophone.listen = function()
	{
		if (this.status == 1)
			return this.stop();

		var recognizer = new SpeechRecognition();
		this.mic = recognizer;
		recognizer.interimResults = true;
		/*
		Problem solution for mobile devices:
			recognition.continuous = false;
			recognition.interimResults = false;
		*/
		recognizer.lang = this.language;
		var sender = this;
		recognizer.onresult = function (event)
		{
			var result = event.results[event.resultIndex];
			var fromText = document.getElementById("fromText");
			fromText.value += result[0].transcript;

			if (result.isFinal)
			{
				sender.stop();
//				StoreLastFromText(fromText.value);
				OnEventOfFromText();
			}
		};
		recognizer.onerror = function(event)
		{
			var fromText = document.getElementById("fromText");
			fromText.value = event.error + (event.message !== "" ? ": "+event.message : "");
			sender.stop();
		};
		recognizer.start();

		this.status = 1;
		this.button.className = "microphone_button listening";
	};
}

// tts
function RefreshTTSButtonByType(type)
{
	var args = GetTextBlockDataByType(type);
	if (args.tts_button)
		TextReader.PreSetupButtonStatus(args.tts_button, args.lang, args.text);
}

function StartTTSByType(type)
{
	var args = GetTextBlockDataByType(type);
	if (args.tts_button)
		TextReader.Play(args.tts_button, args.lang, args.text);
}

// resize
var resizeStartPosForm	= {x:0, y:0};
var resizeStartPos		= {x:0, y:0};
var resizeLastPos		= {x:0, y:0};
var resizePopupTimer	= null;

function resizeSendNewDeltas(e)
{
	if (opera)
	{
		opera.extension.postMessage({
			action: "change_popup_size",
			deltaX: resizeStartPos.x-resizeLastPos.x,
			deltaY: resizeLastPos.y-resizeStartPos.y
		});
	}
	else
	{
		var form = document.getElementsByTagName("FORM")[0];
		var form_width, form_height;

		var new_width = resizeStartPosForm.x + resizeStartPos.x-resizeLastPos.x;
		if (popupMinimumSize.width < new_width)
			form_width = new_width;
		else
			form_width = popupMinimumSize.width;

		var new_height = resizeStartPosForm.y + resizeLastPos.y-resizeStartPos.y;
		if (popupMinimumSize.height < new_height)
			form_height = new_height;
		else
			form_height = popupMinimumSize.height;

		// Chrome has limits :/ (800x600)
		if (form_width > 775)
			form_width = 775;
		if (form_height > 580)
			form_height = 580;

		if (!popupInMenu)	// in menu we have constant width
			form.style.width = form_width + "px";
		form.style.height = form_height + "px";

		onMessageHandler({
			action: "fix_elements_size",
			deltaX: popupInMenu ? null : form_width-popupMinimumSize.width,
			deltaY: form_height-popupMinimumSize.height
		});
	}
}

function resizeMouseMove(e)
{
	window.clearTimeout(resizePopupTimer);
	resizeLastPos.x = e.screenX;
	resizeLastPos.y = e.screenY;
	resizePopupTimer = window.setTimeout(resizeSendNewDeltas, 10);
}

function resizeMouseUp(e)
{
	if (e.bubbles !== undefined)	// if function was called manual => do not setup handlers
	{
//		resizeMouseMove(e);			// we have some problems with this actions :(
		window.document.body.style.cursor = "auto";

		window.removeEventListener('mouseup', resizeMouseUp);
		window.removeEventListener('mousemove', resizeMouseMove);
	}

	if (opera)
		opera.extension.postMessage( {action:"save_popup_size"} );
	else
	{
//		chrome.runtime.sendMessage( {action:"save_popup_size"} );
		var form = document.getElementsByTagName("FORM")[0];
		var newWidth = parseInt(form.style.width);
		var newHeight = parseInt(form.style.height);
		if (popupInMenu)
			newWidth = propPopupSize[0];		// leave original width

		widget.cfg.setAsync( {"popupSize": newWidth+"x"+newHeight} );
	}
}

function StartResizePopup(e)
{
	resizeStartPos.x = e.screenX;
	resizeStartPos.y = e.screenY;

	var form = document.getElementsByTagName("FORM")[0];
	resizeStartPosForm.x = parseInt(document.defaultView.getComputedStyle(form, "").getPropertyValue("width"));
	resizeStartPosForm.y = parseInt(document.defaultView.getComputedStyle(form, "").getPropertyValue("height"));

	if (e.bubbles !== undefined)	// if function was called manual => do not setup handlers
	{
		window.document.body.style.cursor = "ne-resize";

		window.addEventListener('mouseup', resizeMouseUp, false);
		window.addEventListener('mousemove', resizeMouseMove, false);
	}
	return false;
}


// resize and move
if (popupInWindow)
{
	window.addEventListener('resize', function()
	{
		var new_cfg = {};
		new_cfg["popupWindowSize"] = window.outerWidth+"x"+window.outerHeight;
		widget.cfg.setAsync(new_cfg);
	});

	window.addEventListener('beforeunload', function()
	{
		var new_cfg = {};
		new_cfg["popupWindowPosition"] = window.screenLeft + "x" + window.screenTop;
		widget.cfg.setAsync(new_cfg);
	});
}

var globalShiftKeyActive = false;

window.addEventListener('DOMContentLoaded', function()
{
	widget.cfg.getAsync(["useTheme", "mode", "useProviders"], function(cfg)
	{
		// UI color theme
		if (cfg.useTheme === "dark")
			document.getElementsByTagName("HTML")[0].className += " dark";
		else if (cfg.useTheme === "light")
			document.getElementsByTagName("HTML")[0].className += " light";
		else if (typeof matchMedia === "function" && matchMedia("(prefers-color-scheme: dark)").matches)		// auto detect
			document.getElementsByTagName("HTML")[0].className += " dark";
		else
			document.getElementsByTagName("HTML")[0].className += " light";

		// mode
//		if (cfg.mode != "demo")	// NOTICE: temporary turn off demo
		{
			var firstScript = document.getElementsByTagName("SCRIPT")[0];

			var new_script = document.createElement("SCRIPT");
			new_script.src = "js/libs/wanakana.js";
			firstScript.parentNode.insertBefore(new_script, firstScript);

			var new_script = document.createElement("SCRIPT");
			new_script.src = "js/libs/wanahangul.js";
			firstScript.parentNode.insertBefore(new_script, firstScript);
		}

		// filter translate providers
		if (cfg.useProviders)
		{
			var useProviders = cfg.useProviders.split(",");
			var translateProvider = document.getElementById("translateProvider");
			var providerOptions = translateProvider.options;
			var i, cnt = providerOptions.length;
			for (i=0; i<cnt; i++)
				if (useProviders.indexOf(providerOptions[i].value) < 0)
					providerOptions[i].style.display = "none";
		}
	});

	var setupGlobalShiftKeyActive = function(evt) { globalShiftKeyActive = evt.shiftKey; };
	window.addEventListener('keydown', setupGlobalShiftKeyActive, false);	// in Opera 11.xx without third parameter don't work
	window.addEventListener('keyup', setupGlobalShiftKeyActive, false);

	if (popupInWindow || popupInSidebar)
		document.getElementsByTagName("HTML")[0].className += " sidebar";

	document.getElementsByTagName("FORM")[0].addEventListener("submit", function(event) { return false; }, false);

	var fromLang = document.getElementById("fromLang");
	fromLang.addEventListener("keydown", OnEventOfFromLang, false);
	fromLang.addEventListener("change", OnEventOfFromLang, false);
	fromLang.addEventListener("focus", OnEventOfFromLang, false);

	var exchangeFromTo = document.getElementById("exchangeFromTo");
	exchangeFromTo.addEventListener("keydown", function(event) { return CheckCtrlEnter(event); }, false);
	exchangeFromTo.addEventListener("click", function(event) { ExchangeLanguages(event); }, false);

	var toLang = document.getElementById("toLang");
	toLang.addEventListener("keydown", OnEventOfToLang, false);
	toLang.addEventListener("change", OnEventOfToLang, false);

	var fromText = document.getElementById("fromText");
	fromText.addEventListener("keydown", OnEventOfFromText, false);
	fromText.addEventListener("keyup", OnEventOfFromText, false);
	fromText.addEventListener("change", OnEventOfFromText, false);

	document.getElementById("fromTextMicrophone").addEventListener("click", function(event) { fromLangMicrophone.listen(this); }, false);
	document.getElementById("fromTextPlayer").addEventListener("click", function(event) { StartTTSByType("from"); }, false);
	document.getElementById("toTextFavorite").addEventListener("click", function(event) { AddRemoveTranslationToFavorite(); }, false);

	document.getElementById("toTextCopy").addEventListener("click", function(event) { CopyTranslation(event); }, false);
	document.getElementById("fromTextPaste").addEventListener("click", function(event) { PasteSourceText(event); }, false);

	var translateProvider = document.getElementById("translateProvider");
	translateProvider.addEventListener("keydown", OnEventOfProvider, false);
	translateProvider.addEventListener("input", OnEventOfProvider, false);		// Firefox did not support it => duplicate for onChange
	translateProvider.addEventListener("change", OnEventOfProvider, false);		// in Chrome work only when lose focus
	for (provider_id in translateProviders)
		translateProvider.appendChild( new Option(translateProviders[provider_id], provider_id) );

	document.getElementById("copyrightTarget").addEventListener("click", OpenCopyrightLink, false);

	document.getElementById("chkBackTranslation").addEventListener("change", function(event)
	{
		if ((widget.mode == "demo") && false)	// NOTICE: temporary turn off demo
		{
			CreateTabWithUrl("settings.html#registration");
			return false;
		}

		var new_cfg = {};
		new_cfg[sbStoragePrefix+"backTranslation"] = this.checked ? "true" : "false";
		widget.cfg.setAsync(new_cfg);
		TranslateText();
	}, false);

	document.getElementById("btnTranslate").addEventListener("click", function(event) { TranslateText(); }, false);
	document.getElementById("btnClear").addEventListener("click", function(event) { ClearText(); }, false);

	document.getElementById("toTextPlayer").addEventListener("click", function(event) { StartTTSByType("to"); }, false);
	document.getElementById("toFromTextPlayer").addEventListener("click", function(event) { StartTTSByType("to_from"); }, false);

	document.getElementById("btnTranslateUrl").addEventListener("click", function(event) { TranslateUrl(); }, false);


	if (popupInWindow || popupInSidebar || popupInMenu || (typeof exports == "object"))		// Firefox currently did not support resize
	{
		document.getElementById("imgWindowsResizer").style.display = "none";
		document.getElementById("lnkMaximize").style.display = "none";
	}
	else
		document.getElementById("imgWindowsResizer").addEventListener("mousedown", function(event) { return StartResizePopup(event); }, false);

	document.getElementById("lnkMaximize").addEventListener("click", function(event)
	{
		if (event.altKey)
		{
			CreateTabWithUrl(this.href);
			window.close();	// also for Firefox
		}
		else
		{
			var wnd_name = (event.shiftKey ? "_blank" : "Translator.extension");
			var new_wnd = window.open(this.href, wnd_name, "width=640,height=480,menubar=off,toolbar=off,location=off,personalbar=off,status=on,dependent=off,noopener=on,resizable=on,scrollbars=off");
			if (!new_wnd)	// old Opera block open new window
				CreateTabWithUrl(this.href);
			if (!is_firefox)
				window.close();
		}
		event.preventDefault();
		return false;
	});

	if (chrome && chrome.management && chrome.management.getSelf)
		chrome.management.getSelf(function(info){
			if (info.installType == "development")
				document.getElementById("versionMode").innerText = "-dev";
		});
}, false);
