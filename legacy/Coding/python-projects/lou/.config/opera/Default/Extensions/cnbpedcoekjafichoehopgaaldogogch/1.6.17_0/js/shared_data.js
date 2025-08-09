/*
function GetSupportedMimeType()
{
	var i, supportedMimeTypes = ["audio/mp3", "audio/mpeg", "audio/x-pn-realaudio-plugin"];	// , "application/x-mplayer2"
	for (i in supportedMimeTypes)
		if ((plugin = navigator.mimeTypes[supportedMimeTypes[i]]) && plugin.enabledPlugin)
			return supportedMimeTypes[i];
	return false;
}
*/

function Trim(text)
{
	return text.replace(/(^[ \t\n\r\0\x0B]+)|([ \t\r\n\0\x0B]+$)/g, "");
}

function GetStyleOfElement(el, style_name)
{
	if (style_name)
	{
		if (document.defaultView && document.defaultView.getComputedStyle)
		{
			try { return document.defaultView.getComputedStyle(el, "").getPropertyValue(style_name); }
			catch (e) { return ""; }
		}
		else if (el.currentStyle)
		{
			style_name = style_name.replace(/-(\w)/g, function(){ return arguments[1].toUpperCase(); });
			return el.currentStyle[style_name];
		}
	}
	else
	{
		var style;
		if (el.style.getAttribute)
			style = el.style.getAttribute("cssText");
		else
			style = el.getAttribute("style");

		if (style)
			return style;
	}
	return "";
}

function AppendClass(el, name)
{
	if (el["classList"] && (name.indexOf(" ") < 0))
		return el.classList.add(name);
	if ((" "+el.className+" ").search(" "+name+" ") < 0)
		el.className += " " + name;
}

function RemoveClass(el, name)
{
	if (el["classList"] && (name.indexOf(" ") < 0))
		return el.classList.remove(name);
	el.className = (" "+el.className+" ").replace(" "+name+" ", " ").replace(/^ +/, "").replace(/ +$/, "").replace(/ +/, " ");
}

function GetUserPreferedLanguages()
{
	var user_langs = [window.navigator.language];
	if (window.navigator.userLanguage)
		user_langs.push( window.navigator.userLanguage );
	if (window.navigator.browserLanguage)
		user_langs.push( window.navigator.browserLanguage );
	if (window.navigator.languages)
		user_langs = user_langs.concat(window.navigator.languages);

	var ext_lng = (typeof EXT_LOCALE != "undefined" ? EXT_LOCALE.substr(0, 2) : undefined);
	if (!ext_lng && chrome && chrome.i18n)
		ext_lng = chrome.i18n.getMessage("locale");
	else
		ext_lng = "en";
	user_langs.push(ext_lng);

	// remove additional data + unique
	user_langs = user_langs.reduce(function(new_list, item)
	{
		var lang_prior = item.split(";");
		var lang_country = lang_prior[0].split("-");
		item = lang_country[0].toLowerCase();

		if (new_list.indexOf(item) < 0)
			new_list.push(item);
		return new_list;
	}, []);
	user_langs.sort();

	return user_langs;
}

function DetectUserPreferedLanguage()
{
	var def_lng = (typeof EXT_LOCALE != "undefined"
					? EXT_LOCALE.substr(0, 2)
					: "en"
					);
	var lng = "";
	var user_langs = GetUserPreferedLanguages();
	if (user_langs)
	{
		var en_lng = "";
		var i, cnt = user_langs.length;
		for (i=0; i<cnt; i++)
		{
			lng = user_langs[i];
			if (lng == "en")
				en_lng = "en";
			else if (languages[lng])
				break;
			lng = "";
		}

		if (!lng) lng = en_lng;
		if (!lng) lng = def_lng;
	}
	else
		lng = def_lng;
	return lng;
}

function GetTranslatedPageUrlPrefix(translateProvider, fromLang, toLang)
{
	var prefixUrl;

	switch (translateProvider)
	{
		case "bing":
			prefixUrl = "https://www.microsofttranslator.com/bv.aspx?lo=TP&from="+fromLang+"&to="+toLang+"&a=";
			break;

		case "yandex":
			prefixUrl = "https://translate.yandex.com/translate?dir=auto&from="+fromLang+"&lang="+fromLang+"-"+toLang+"&to="+toLang+"&ui=&url=";
			break;
/*
		// deprecated
		case "promt":
			// setup langs
			var lang_pair = "";

			if (!fromLang)
				lang_pair += "a";	// any
			else if (promtLanguagesTo[fromLang])
				lang_pair += promtLanguagesTo[fromLang];
			else
				lang_pair += "-";	// undefined

			if (promtLanguagesTo[toLang])
				lang_pair += promtLanguagesTo[toLang];
			else
				lang_pair += "-";	// undefined
			//

			prefixUrl = "https://www.translate.ru/siteTranslation/autolink/?direction="+lang_pair+"&template=General&sourceURL=";
			break;
*/
		case "baidu":
			if (!fromLang)
				fromLang = "auto";
			prefixUrl = "http://fanyi.baidu.com/transpage?source=url&ie=utf8&from="+fromLang+"&to="+toLang+"&render=1&query=";
			break;

		case "sogou":
			if (!fromLang)
				fromLang = "auto";
			prefixUrl = "http://translate.sogoucdn.com/pcvtsnapshot?from="+fromLang+"&to="+toLang+"&tfr=translatepc&domainType=sogou&url=";
			break;
/*
		// disabled because prefix + postfix :(
		case "apertium":
			prefixUrl = "https://www.apertium.org/index.eng.html?dir="+(!from_lang ? "auto" : (apertiumLanguagesTo[from_lang] ? apertiumLanguagesTo[from_lang] : from_lang))+"-"+(apertiumLanguagesTo[to_lang] ? apertiumLanguagesTo[to_lang] : to_lang)+"&qP=[url]#translation";
			break;
*/
		default:	// google
			if (!fromLang)
				fromLang = "auto";
			prefixUrl = "https://translate.google." + (useGoogleCn ? "com.hk" : "com") + "/translate?sl="+fromLang+"&tl="+toLang+"&u=";
	}

	return prefixUrl;
}

function SendAjaxRequest(url, method, headers, vars, onreadystatechange)
{
	var xhr = new XMLHttpRequest();
	xhr.onreadystatechange = onreadystatechange;

	if (!headers)
		headers = {};

	var vars_arr = [];
	if (typeof vars == "string")
		vars_arr.push(vars);
	else
		for (var var_name in vars)
			vars_arr.push( encodeURIComponent(var_name) + "=" + encodeURIComponent(vars[var_name]) );

	var post_vars = null;
	if (method.toUpperCase() == "POST")
	{
		if (!headers["Content-Type"])
			headers["Content-Type"] = "application/x-www-form-urlencoded";

		post_vars = vars_arr.join("&");
	}
	else
	{
		if (url.indexOf("?") < 0)
			url += "?" + vars_arr.join("&");
		else
			url += "&" + vars_arr.join("&");
	}

	headers["Request-From-Extension"] = widget.cfg.uuid;

	xhr.open( method, url, true );
	if (headers)
	{
		for (var header_name in headers)
			xhr.setRequestHeader(header_name, headers[header_name]);
	}

	xhr.send( post_vars );
	return xhr;
}

function SendMessageToBackgroundScript(msg)
{
	if (opera && opera.extension)
		opera.extension.postMessage( msg );
	else if (chrome && chrome.runtime)
	{
		// Edge better work via async messages
		chrome.runtime.sendMessage(msg, function(answer)	// this handler did not support `sender` object
		{
			if (chrome.runtime.lastError)
			{
				console.log("Can't send message to background script. " + chrome.runtime.lastError.message);
				return;
			}
			onMessageHandler(answer);
		});

		// in some cases we want response => sender has to be window
		// But this method badly work with Edge!
		// TODO: use sendMessage
/*
		if (chrome.runtime && chrome.runtime.getBackgroundPage)
			chrome.runtime.getBackgroundPage( function(bgPage){
				bgPage.onMessageHandler( msg, window );
			});

		// alternative: var bgPage = chrome.extension.getBackgroundPage();
*/
	}
	else if (browser && browser.runtime)
	{
		browser.runtime.sendMessage(msg, function(answer)	// this handler did not support `sender` object
		{
			if (browser.runtime.lastError)
			{
				console.log("Can't send message to background script. " + browser.runtime.lastError.message);
				return;
			}
			onMessageHandler(answer);
		});
	}
}

function CreateTabWithUrl(url)
{
	// in Opera Classic we can open new tabs only from background process
	SendMessageToBackgroundScript({ "action":"create_tab_with_url", "url":url });
}

function VerifyWidgetAsync(manual)
{
	widget.cfg.getAsync(["translator_uuid", "check_tries"], function(cfg)
	{
		var now = (new Date())-0;
		var check_tries = cfg.check_tries-0 || 0;

		SendAjaxRequest("https://translator.sailormax.net/ajax/verify",
						"POST",
						null,
						{ "uuid":cfg.translator_uuid, "language":navigator.language },
						function(result, error, status)
						{
							if (this.readyState == 4)
							{
								var uuid_status = 0;
								if (this.status == 200)
									uuid_status = (this.responseText == "ok" ? 1 : 0);
								else
									console.log("vstat: " + this.responseText + " ("+this.status+")");

								var new_cfg = {};
								if (uuid_status)
								{
									check_tries = 0;
									new_cfg["last_verified"] = now;
									new_cfg["mode"] = widget.mode = "";
									if (window["SetupUIBasedOnMode"])
										SetupUIBasedOnMode(widget.mode);
								}
								else
								{
									if (manual)
									{
										CreateTabWithUrl("https://translator.sailormax.net/cabinet?uuid="+cfg.translator_uuid);
										return;
									}
									else
									{
										check_tries++;
										if ((check_tries < 1) || (check_tries >= 3))	// first two bad verifications free
										{
											widget.mode = "demo";
											new_cfg["mode"] = widget.mode;
											if (window["SetupUIBasedOnMode"])
												SetupUIBasedOnMode(widget.mode);
										}
									}
								}

								new_cfg["last_check"] = now;
								new_cfg["check_tries"] = check_tries;
								widget.cfg.setAsync( new_cfg );
							}

						});
	});
}

if (window["chrome"] && chrome.runtime)	//  && chrome.runtime.onMessageExternal
{
//	chrome.runtime.onMessageExternal.addListener( function(msg, sender, sendResponse)
	chrome.runtime.onMessage.addListener( function(msg, sender, sendResponse)
	{
		switch (msg.action)
		{
			case "setup_partner_window":
				var target_iframe = document.getElementById(msg.target);
				if (!target_iframe)
				{
					// popup window uses one more iframe
					var iframes = document.getElementsByTagName("IFRAME");
					if (!iframes.length || !iframes[0].contentDocument)
						break;

					target_iframe = iframes[0].contentDocument.getElementById(msg.target);
				}
				if (target_iframe)
				{
					target_iframe.style.width = msg.width+"px";
					target_iframe.style.height = msg.height+"px";
					if ((!msg.width || !msg.height) && (target_iframe.parentNode.className == "item"))	// dictionary without partner
						target_iframe.parentNode.style.display = "none";
				}
				break;

			case "setup_ui_based_on_mode":
				if (window["SetupUIBasedOnMode"])
				{
					widget.cfg.getAsync("mode", function(cfg)
					{
						SetupUIBasedOnMode(cfg.mode);
					});
				}
				break;
		}
	});
}

window.addEventListener('DOMContentLoaded', function()
{
	var linkToVerify = document.getElementById("linkToVerify");
	if (linkToVerify)
	{
		linkToVerify.onclick = function()
		{
			parent.VerifyWidgetAsync(true);
		};
	}

	var linkLogo = document.getElementById("linkLogo");
	if (linkLogo)
	{
		if (is_chredge)
			linkLogo.href = "https://microsoftedge.microsoft.com/addons/detail/translator/cdkmohnpfdennnemmjekmmiibgfddako";
		else if (is_chropera)
			linkLogo.href = "https://addons.opera.com/extensions/details/translator/";
		else if (is_firefox)
			linkLogo.href = "https://addons.mozilla.org/en-US/firefox/addon/translator_extension/";
		else	// Chrome
			linkLogo.href = "https://chrome.google.com/webstore/detail/translator/blndkmebkmenignoajhoemebccmmfjib/";
	}
});

var translateProviders = {
'google'		: "Google Translate",
'bing'			: "Bing Translator",
'yandex'		: "Yandex Translate",
'promt'			: "Promt Translator",
'pragma'		: "Pragma Translator",
'lingvanex'		: "Lingvanex Translator",
'baidu'			: "Baidu Translator",
'naver'			: "Naver Translator",
'sogou'			: "Sogou Translator",
//'systran'		: "Systran Translate",
'babylon'		: "Babylon Translator",
'dictionaries'	: "Dictionaries by Babylon",
'lingvo'		: "Lingvo Dictionary",
'glosbe'		: "Glosbe Dictionary",
'urban'			: "Urban Dictionary",
'ibm'			: "IBM Translator",
'apertium'		: "Apertium Translator",
'deepl'			: "DeepL Translator"
};

if (!window.crypto)
	delete translateProviders["systran"];

// http://translate.googleapis.com/translate_a/l?client=te&hl=en&cb=_callbacks_._0gh4pjncd
// _callbacks_._0gh4pjncd({'sl':{'auto':'Detect language','af':'Afrikaans',...});
var languages = {
'af'	: 'Afrikaans',
'sq'	: 'Albanian',
'am'	: 'Amharic',
'ar'	: 'Arabic',
'hy'	: 'Armenian',
'az'	: 'Azerbaijani',
'bn-BD'	: 'Bangla',
'ba'	: 'Bashkir',
'eu'	: 'Basque',
'be'	: 'Belarusian',
'bn'	: 'Bengali',
'bh'	: 'Bihari',
'bs'	: 'Bosnian',
'br'	: 'Breton',
'bg'	: 'Bulgarian',
'my'	: 'Burmese',
'yue'	: 'Cantonese (Traditional)',
'ca'	: 'Catalan',
'ceb'	: 'Cebuano',
//'chr'	: 'Cherokee',	// no one support it
'zh'	: 'Chinese',
'zh-CN'	: 'Chinese_simplified',
'zh-CHS': 'Chinese_simplified_legacy',
'zh-HK'	: 'Chinese_traditional_HongKong',
'zh-MO'	: 'Chinese_traditional_Macao',
'zh-SG'	: 'Chinese_traditional_Singapure',
'zh-TW'	: 'Chinese_traditional_Taiwan',
'zh-CHT': 'Chinese_traditional_legacy',
'co'	: 'Corsican',
'cv'	: 'Chuvash',
'hr'	: 'Croatian',
'cs'	: 'Czech',
'da'	: 'Danish',
'dv'	: 'Dhivehi',
'nl'	: 'Dutch',
'sjn'	: 'Elvish (Sindarin)',
'emj'	: 'Emoji',
'en'	: 'English',
'eo'	: 'Esperanto',
'et'	: 'Estonian',
'fo'	: 'Faroese',
'fj'	: 'Fijian',
'tl'	: 'Filipino',
'fi'	: 'Finnish',
'fr'	: 'French',
'fy'	: 'Frisian',
'gl'	: 'Galician',
'ka'	: 'Georgian',
'de'	: 'German',
'el'	: 'Greek',
'gu'	: 'Gujarati',
'ht'	: 'Haitian_creole',
'iw'	: 'Hebrew',			// yandex use 'he'
'mrj'	: 'Hill Mari',
'hi'	: 'Hindi',
'mww'	: 'Hmong Daw',
'hu'	: 'Hungarian',
'is'	: 'Icelandic',
'id'	: 'Indonesian',
'iu'	: 'Inuktitut',
'ga'	: 'Irish',
'it'	: 'Italian',
'ja'	: 'Japanese',
'jw'	: 'Javanese',
'kn'	: 'Kannada',
'kk'	: 'Kazakh',
'tlh'	: 'Klingon',
'km'	: 'Khmer',
'ko'	: 'Korean',
'ku'	: 'Kurdish',
'ky'	: 'Kyrgyz',
'lo'	: 'Lao',
'la'	: 'Latin',
'lv'	: 'Latvian',
'lt'	: 'Lithuanian',
'lb'	: 'Luxembourgish',
'mk'	: 'Macedonian',
'mg'	: 'Malagasy',
'ms'	: 'Malay',
'ml'	: 'Malayalam',
'mt'	: 'Maltese',
'mi'	: 'Maori',
'mr'	: 'Marathi',
'mhr'	: 'Mari',
'mn'	: 'Mongolian',
'ne'	: 'Nepali',
'no'	: 'Norwegian',
'pap'	: 'Papiamento',
'oc'	: 'Occitan',
'or'	: 'Oriya',
'ps'	: 'Pashto',
'fa'	: 'Persian',
'pl'	: 'Polish',
'pt'	: 'Portuguese',
'pt-PT'	: 'Portuguese_portugal',
'pa'	: 'Punjabi',
'qu'	: 'Quechua',
'otq'	: 'Querétaro Otomi',
'ro'	: 'Romanian',
'ru'	: 'Russian',
'sm'	: 'Samoan',
'sa'	: 'Sanskrit',
'gd'	: 'Scottish Gaelic',
'sr'	: 'Serbian',
'sr-Cyrl': 'Serbian (Cyrillic)',
'sr-Latn': 'Serbian (Latin)',
'sd'	: 'Sindhi',
'si'	: 'Sinhalese',
'sk'	: 'Slovak',
'sl'	: 'Slovenian',
'es'	: 'Spanish',
'su'	: 'Sundanese',
'sw'	: 'Swahili',
'sv'	: 'Swedish',
'syr'	: 'Syriac',
'ty'	: 'Tahitian',
'tg'	: 'Tajik',
'ta'	: 'Tamil',
'tt'	: 'Tatar',
'te'	: 'Telugu',
'th'	: 'Thai',
'bo'	: 'Tibetan',
'to'	: 'Tonga',
'tr'	: 'Turkish',
'udm'	: 'Udmurt',
'uk'	: 'Ukrainian',
'ur'	: 'Urdu',
'uz'	: 'Uzbek',
'uzbcyr': 'Uzbek (Cyrillic)',
'ug'	: 'Uighur',
'vi'	: 'Vietnamese',
'cy'	: 'Welsh',
'xh'	: 'Xhosa',
'yi'	: 'Yiddish',
'yua'	: 'Yucatec Maya',
'yo'	: 'Yoruba',
'wo'	: 'Wolof'
};

var msLanguages = {
'bs'	: 'bs-Latn',
'zh'	: 'zh-Hant',
'zh-CN'	: 'zh-Hans',
'tl'	: 'fil',
'iw'	: 'he',
'no'	: 'nb',
'pt-PT'	: 'pt-pt',
'sr'	: 'sr-Cyrl',
};
var msLanguagesAutodetect = Object.keys(msLanguages).reduce(function(list, k){ list[ msLanguages[k] ] = k; return list; }, {});
/*
var promtLanguagesFrom = {
"e"		: "en",
"s"		: "es",
"i"		: "it",
"j"		: "ja",
"g"		: "de",
"p"		: "pt",
"r"		: "ru",
"f"		: "fr",
};

var promtLanguagesTo = {
"en"	: "e",
"es"	: "s",
"it"	: "i",
"ja"	: "j",
"de"	: "g",
"pt"	: "p",
"ru"	: "r",
"fr"	: "f",
};
*/
var lingvoLanguages = {
"zh-CN"	: 1028,
"da"	: 1030,
"nl"	: 1043,
"en"	: 1033,
"fi"	: 1035,
"fr"	: 1036,
"de"	: 32775,
"el"	: 1032,
"hu"	: 1038,
"it"	: 1040,
"kk"	: 1087,
"la"	: 1142,
"nb-no"	: 1044,
"pl"	: 1045,
"pt-br"	: 2070,
"ru"	: 1049,
"es"	: 1034,
"tt"	: 1092,
"tr"	: 1055,
"uk"	: 1058,
};

var babylonLanguages = {
"en"	: 0,
"fr"	: 1,
"it"	: 2,
"de"	: 6,
"pt"	: 5,
"es"	: 3,
"ar"	: 15,
"ca"	: 99,
/* <option value="344">Castilian</option> */
"cs"	: 31,
"zh-CN"	: 10,
"zh"	: 9,
"da"	: 43,
"el"	: 11,
"iw"	: 14,
"hi"	: 60,
"hu"	: 30,
"fa"	: 51,
"ja"	: 8,
"ko"	: 12,
"nl"	: 4,
"no"	: 46,
"pl"	: 29,
"ro"	: 47,
"ru"	: 7,
"sv"	: 48,
"tr"	: 13,
"th"	: 16,
"uk"	: 49,
"ur"	: 39,
};

var baiduLanguagesTo = {
"ar"	: "ara",
"et"	: "est",
"bg"	: "bul",
"da"	: "dan",
"fr"	: "fra",
"fi"	: "fin",
"ko"	: "kor",
"ro"	: "rom",
"ja"	: "jp",
"sv"	: "swe",
"sl"	: "slo",
"zh"	: "wyw",
"zh-CN"	: "zh",
"zh-CHT": "cht",
"es"	: "spa",
"vi"	: "vie",
};

var sogouLanguagesTo = {
};

var apertiumLanguagesFrom = {
'abk'	: 'ab',
'aar'	: 'aa',
'afr'	: 'af',
//'alb'	: 'sq',
'sqi'	: 'sq',
'amh'	: 'am',
'ara'	: 'ar',
'arg'	: 'an',
//'arm'	: 'hy',
'hye'	: 'hy',
'asm'	: 'as',
'ave'	: 'ae',
'aym'	: 'ay',
'aze'	: 'az',
'bak'	: 'ba',
//'baq'	: 'eu',
'eus'	: 'eu',
'bel'	: 'be',
'ben'	: 'bn',
'bih'	: 'bh',
'bis'	: 'bi',
'bos'	: 'bs',
'bre'	: 'br',
'bul'	: 'bg',
//'bur'	: 'my',
'mya'	: 'my',
'cat'	: 'ca',
'cha'	: 'ch',
'che'	: 'ce',
//'chi'	: 'zh',
'zho'	: 'zh',
'chu'	: 'cu',
'chv'	: 'cv',
'cor'	: 'kw',
'cos'	: 'co',
//'scr'	: 'hr',
'hrv'	: 'hr',
//'cze'	: 'cs',
'ces'	: 'cs',
'dan'	: 'da',
'div'	: 'dv',
//'dut'	: 'nl',
'nld'	: 'nl',
'dzo'	: 'dz',
'eng'	: 'en',
'epo'	: 'eo',
'est'	: 'et',
'fao'	: 'fo',
'fij'	: 'fj',
'fin'	: 'fi',
//'fre'	: 'fr',
'fra'	: 'fr',
'gla'	: 'gd',
'glg'	: 'gl',
//'geo'	: 'ka',
'kat'	: 'ka',
//'ger'	: 'de',
'deu'	: 'de',
//'gre'	: 'el',
'ell'	: 'el',
'grn'	: 'gn',
'guj'	: 'gu',
'hat'	: 'ht',
'hau'	: 'ha',
'heb'	: 'he',
'her'	: 'hz',
'hin'	: 'hi',
'hmo'	: 'ho',
'hun'	: 'hu',
//'ice'	: 'is',
'isl'	: 'is',
'ido'	: 'io',
'ind'	: 'id',
'ina'	: 'ia',
'ile'	: 'ie',
'iku'	: 'iu',
'ipk'	: 'ik',
'gle'	: 'ga',
'ita'	: 'it',
'jpn'	: 'ja',
'jav'	: 'jv',
'kal'	: 'kl',
'kan'	: 'kn',
'kas'	: 'ks',
'kaz'	: 'kk',
'khm'	: 'km',
'kik'	: 'ki',
'kin'	: 'rw',
'kir'	: 'ky',
'kom'	: 'kv',
'kor'	: 'ko',
'kua'	: 'kj',
'kur'	: 'ku',
'lao'	: 'lo',
'lat'	: 'la',
'lav'	: 'lv',
'lim'	: 'li',
'lin'	: 'ln',
'lit'	: 'lt',
'ltz'	: 'lb',
//'mac'	: 'mk',
'mkd'	: 'mk',
'mlg'	: 'mg',
//'may'	: 'ms',
'msa'	: 'ms',
'mal'	: 'ml',
'mlt'	: 'mt',
'glv'	: 'gv',
//'mao'	: 'mi',
'mri'	: 'mi',
'mar'	: 'mr',
'mah'	: 'mh',
'mol'	: 'mo',
'mon'	: 'mn',
'nau'	: 'na',
'nav'	: 'nv',
'nde'	: 'nd',
'nbl'	: 'nr',
'ndo'	: 'ng',
'nep'	: 'ne',
'sme'	: 'se',
'nor'	: 'no',
'nob'	: 'nb',
'nno'	: 'nn',
'nya'	: 'ny',
'oci'	: 'oc',
'ori'	: 'or',
'orm'	: 'om',
'oss'	: 'os',
'pli'	: 'pi',
'pan'	: 'pa',
//'per'	: 'fa',
'fas'	: 'fa',
'pol'	: 'pl',
'por'	: 'pt',
'pus'	: 'ps',
'que'	: 'qu',
'roh'	: 'rm',
//'rum'	: 'ro',
'ron'	: 'ro',
'run'	: 'rn',
'rus'	: 'ru',
'smo'	: 'sm',
'sag'	: 'sg',
'san'	: 'sa',
'srd'	: 'sc',
//'scc'	: 'sr',
'srp'	: 'sr',
'sna'	: 'sn',
'iii'	: 'ii',
'snd'	: 'sd',
'sin'	: 'si',
//'slo'	: 'sk',
'slk'	: 'sk',
'slv'	: 'sl',
'som'	: 'so',
'sot'	: 'st',
'spa'	: 'es',
'sun'	: 'su',
'swa'	: 'sw',
'ssw'	: 'ss',
'swe'	: 'sv',
'tgl'	: 'tl',
'tah'	: 'ty',
'tgk'	: 'tg',
'tam'	: 'ta',
'tat'	: 'tt',
'tel'	: 'te',
'tha'	: 'th',
//'tib'	: 'bo',
'bod'	: 'bo',
'tir'	: 'ti',
'ton'	: 'to',
'tso'	: 'ts',
'tsn'	: 'tn',
'tur'	: 'tr',
'tuk'	: 'tk',
'twi'	: 'tw',
'uig'	: 'ug',
'ukr'	: 'uk',
'urd'	: 'ur',
'uzb'	: 'uz',
'vie'	: 'vi',
'vol'	: 'vo',
'wln'	: 'wa',
//'wel'	: 'cy',
'cym'	: 'cy',
'fry'	: 'fy',
'wol'	: 'wo',
'xho'	: 'xh',
'yid'	: 'yi',
'yor'	: 'yo',
'zha'	: 'za',
'zul'	: 'zu',
};

var apertiumLanguagesTo = {};
for (var lng in apertiumLanguagesFrom)
	apertiumLanguagesTo[ apertiumLanguagesFrom[lng] ] = lng;

// https://www.bing.com/translator/api/Language/GetSpeechDialectsForLocale?locale=en
var lang_default_locales = {
'ar'	: 'ar-EG',
'ca'	: 'ca-ES',
'zh-CHS': 'zh-CN',
'zh-HK'	: 'zh-HK',
'zh-TW'	: 'zh-TW',
'zh-CHT': 'zh-CN',
'da'	: 'da-DK',
'nl'	: 'nl-NL',
'en'	: 'en-US',
'fi'	: 'fi-FI',
'fr'	: 'fr-FR',
'de'	: 'de-DE',
'it'	: 'it-IT',
'ja'	: 'ja-JP',
'ko'	: 'ko-KR',
'no'	: 'nb-NO',
'pl'	: 'pl-PL',
'pt'	: 'pt-BR',
'pt-PT'	: 'pt-PT',
'ru'	: 'ru-RU',
'es'	: 'es-ES',
'sv'	: 'sv-SE',
};

// taken from lingvanex's session storage
var lingvanexLanguages = {
    "af": "af_ZA",
    "sq": "sq_AL",
    "am": "am_ET",
    "ar": "ar_SA",
    "hy": "hy_AM",
    "as": "as_IN",
    "az": "az_AZ",
    "eu": "eu_ES",
    "be": "be_BY",
    "bn": "bn_BD",
    "bs": "bs_BA",
    "bg": "bg_BG",
    "ca": "ca_ES",
    "ceb": "ceb_PH",
    "ny": "ny_MW",
    "zh-Hans": "zh-Hans_CN",
    "zh-Hant": "zh-Hant_TW",
    "co": "co_FR",
    "ht": "ht_HT",
    "hr": "hr_HR",
    "cs": "cs_CZ",
    "da": "da_DK",
    "nl": "nl_NL",
    "en": "en_GB",
    "eo": "eo_WORLD",
    "et": "et_EE",
    "fi": "fi_FI",
    "fr": "fr_FR",
    "fy": "fy_NL",
    "gl": "gl_ES",
    "ka": "ka_GE",
    "de": "de_DE",
    "el": "el_GR",
    "gu": "gu_IN",
    "ha": "ha_NE",
    "haw": "haw_US",
    "he": "he_IL",
    "hi": "hi_IN",
    "hmn": "hmn_CN",
    "hu": "hu_HU",
    "is": "is_IS",
    "ig": "ig_NG",
    "id": "id_ID",
    "ga": "ga_IE",
    "it": "it_IT",
    "ja": "ja_JP",
    "jv": "jv_ID",
    "kn": "kn_IN",
    "kk": "kk_KZ",
    "km": "km_KH",
    "rw": "rw_RW",
    "ko": "ko_KR",
    "ku": "ku_IR",
    "ky": "ky_KG",
    "lo": "lo_LA",
    "la": "la_VAT",
    "lv": "lv_LV",
    "lt": "lt_LT",
    "lb": "lb_LU",
    "mk": "mk_MK",
    "mg": "mg_MG",
    "ms": "ms_MY",
    "ml": "ml_IN",
    "mt": "mt_MT",
    "mi": "mi_NZ",
    "mr": "mr_IN",
    "mn": "mn_MN",
    "my": "my_MM",
    "ne": "ne_NP",
    "no": "no_NO",
    "or": "or_OR",
    "ps": "ps_AF",
    "fa": "fa_IR",
    "pl": "pl_PL",
    "pt": "pt_PT",
    "pa": "pa_PK",
    "ro": "ro_RO",
    "ru": "ru_RU",
    "sm": "sm_WS",
    "gd": "gd_GB",
    "sr-Cyrl": "sr-Cyrl_RS",
    "st": "st_LS",
    "sn": "sn_ZW",
    "sd": "sd_PK",
    "si": "si_LK",
    "sk": "sk_SK",
    "sl": "sl_SI",
    "so": "so_SO",
    "es": "es_ES",
    "su": "su_ID",
    "sw": "sw_TZ",
    "sv": "sv_SE",
    "tl": "tl_PH",
    "tg": "tg_TJ",
    "ta": "ta_IN",
    "tt": "tt_TT",
    "te": "te_IN",
    "th": "th_TH",
    "tr": "tr_TR",
    "tk": "tk_TM",
    "uk": "uk_UA",
    "ur": "ur_PK",
    "ug": "ug_CN",
    "uz": "uz_UZ",
    "vi": "vi_VN",
    "cy": "cy_GB",
    "xh": "xh_ZA",
    "yi": "yi_IL",
    "yo": "yo_NG",
    "zu": "zu_ZA"
};
