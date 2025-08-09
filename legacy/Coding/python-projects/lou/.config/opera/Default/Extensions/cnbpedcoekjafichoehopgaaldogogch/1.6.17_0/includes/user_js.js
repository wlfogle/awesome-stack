if (!window.extTranslatorBySailorMax && (window.location.href.indexOf("widget://") != 0))	// don't need load in widgets/extensions/unite-applications
{
	(function()
	{
		var ext_object = this;
		this.scriptCfg = window.extTranslatorBySailorMax = {};
		window._mstAppId = '';
		window.my_mstAppId = '';

//		var is_chrome_store_app = (window.chrome && window.chrome.runtime && window.chrome.runtime.id == "blndkmebkmenignoajhoemebccmmfjib");

		var browser_ua = window.navigator.userAgent.toLowerCase();
		var is_firefox = (browser_ua.indexOf(' firefox/') > -1);
//		var is_chropera = (browser_ua.indexOf(' opr/') > -1);
//		var is_chredge = (browser_ua.indexOf(' edg/') > -1);
//		var is_chrome = !!window.chrome && !is_chropera && !is_chredge;		// TODO: make better detection, because now any Chromium browsers is_chrome :/
		var is_original_chrome = (browser_ua.match(/[^ ]+\/[\d\.]+/g).filter(function(v) { return !v.match(/(mozilla|applewebkit|chrome|safari)/) } ).length === 0);

		// setup userJS_UUID
		var rand4 = function() { return (((1+Math.random())*0x10000)|0).toString(16).substring(1) };
		this.scriptCfg.userJS_UUID = (rand4()+rand4()+'-'+rand4()+'-'+rand4()+'-'+rand4()+'-'+rand4()+rand4()+rand4());

		// normalize chrome object
		var chrome = ((typeof chrome == "object") && chrome) || window.chrome || ((typeof browser == "object") && browser) || window.browser;		// compatibility with Firefox

		var CurrentWindowIsFrame = function()
		{
			try			{ return (window.document != window.parent.document); }	// we can't compare windows because current window can just inherit original window (Edge)
			catch (e)	{ return false; }										// compare documents not permitted to compare => not equal
		};

		var InitInjectedScript = function()
		{
			var portOfBgProcess = null;
			var scriptCfg = window.extTranslatorBySailorMax;

			for (var name in scriptCfg)
				this[name] = scriptCfg[name];

			var useTranslateToolbar = (this.useTranslateToolbar || false);
			var useGoogleCn = (this.useGoogleCn == "true");
			var defTargetLang = (this.defTargetLang || false);
			var knownLangs = (this.knownLangs || "").split(",");

			// firefox and chrome: disabled toolbars, because it's store does not allow use external scripts
			if (is_firefox || is_original_chrome)
				useTranslateToolbar = false;

			// setup message listener when page is not frame
				if (!CurrentWindowIsFrame())
				{
					window.addEventListener('message', function(evt)
					{
						if (!evt.data)
							return;

						if (evt.data.action == "setup_new_selection")
						{
							scriptCfg.currentSelection = evt.data.selection;
							if ((opera ? opera.extension : portOfBgProcess))
								(opera ? opera.extension : portOfBgProcess).postMessage({ action:"selection_changed", selection:scriptCfg.currentSelection/*, uuid:scriptCfg.userJS_UUID */ });	// here, because we need update badge. after speeddial we don't know about new current page
						}
						else if (evt.data.action == "get_active_page_data")	// temporary solution, while Opera send events to frames
						{
							scriptCfg.onMessageHandler( (opera ? evt : evt.data) );
						}
					}, false);
				}

/*
				scriptCfg.ShowHint = function(text)
				{
					var sel = window.getSelection();
					if (!sel || (scriptCfg.currentSelection != sel.toString())) return;

					var hint = document.getElementById("hint_of_"+scriptCfg.userJS_UUID);
					if (!hint)
					{
						hint = document.createElement("P");
						hint.id = "hint_of_"+scriptCfg.userJS_UUID;
						hint.style.cssText = "position:absolute; display:inline-block; border:1px solid #000; padding:2px; background:#ffffe1; color:#000;";
						document.body.appendChild(hint);
					}

					hint.innerText = text;

					var sel_pos = sel.getRangeAt(0).getBoundingClientRect();

					var focus_node = sel.focusNode;
					if (focus_node && focus_node.nodeName == "#text")
						focus_node = focus_node.parentNode;
					if (focus_node)
					{
						var focus_node_style = document.defaultView.getComputedStyle(focus_node, "");
						var font_size = focus_node_style.getPropertyValue("font-size");
						if (parseInt(font_size, 10) < 12) font_size = "12px";
						hint.style.fontSize = font_size;
						var font_weight = focus_node_style.getPropertyValue("font-weight");
						hint.style.fontWeight = font_weight;
					}

					if (sel_pos.top - 3 - hint.clientHeight >= window.pageYOffset)
						hint.style.top = (sel_pos.top - 3 - hint.clientHeight + window.pageYOffset) + "px";
					else
						hint.style.top = (sel_pos.bottom + 3 + window.pageYOffset) + "px";
					hint.style.left = (sel_pos.left + window.pageXOffset) + "px";
				}

				scriptCfg.HideHint = function(evt)
				{
					var hint = document.getElementById("hint_of_"+scriptCfg.userJS_UUID);
					if (hint && (evt.target != hint))
						document.body.removeChild(hint);
				}
*/

				scriptCfg.DetectUserPreferedLanguage = function()
				{
					var def_lng = "en";
					var lng = "";
					var user_langs = [window.navigator.language];
					if (window.navigator.userLanguage)
						user_langs.push( window.navigator.userLanguage );
					if (window.navigator.browserLanguage)
						user_langs.push( window.navigator.browserLanguage );
					if (user_langs)
					{
						var en_lng = "";
						var i, cnt = user_langs.length;
						for (i=0; i<cnt; i++)
						{
							var lng = user_langs[i];
							var lang_prior = lng.split(";");
							var lang_country = lang_prior[0].split("-");
							lng = lang_country[0].toLowerCase();
							if (lng == "en")
								en_lng = "en";
							else if (lng.length > 1)	// languages[lng]
								break;
							lng = "";
						}

						if (!lng)
							lng = en_lng;
						if (!lng)
							lng = def_lng;
					}
					else
						lng = def_lng;
					return lng;
				}


			// save last active input and selection
				var timerSelection = null;
	//			scriptCfg.lastActiveInput = null;
				scriptCfg.currentSelection = "";
				var lastActiveInputHandler = function(evt)
				{
					if (timerSelection)
					{
						clearTimeout(timerSelection);
						timerSelection = null;
					}

					// setup pause, because we can't catch removing selection if last click was on selection
					timerSelection = setTimeout(function()
					{
						var lastActiveInput = evt.srcElement;
						var selection = window.getSelection().toString();
						if (!selection.length && lastActiveInput
							&& (lastActiveInput.type != "password")
							&& (lastActiveInput.type != "checkbox")
							&& (lastActiveInput.type != "radio")
							&& (lastActiveInput.type != "submit")
							&& (lastActiveInput.type != "button")
							&& (lastActiveInput.type != "file")
							&& (lastActiveInput.type != "number")
							&& lastActiveInput.value
							&& (lastActiveInput.selectionStart || lastActiveInput.selectionEnd))
						{
							selection = lastActiveInput.value.substring(lastActiveInput.selectionStart, lastActiveInput.selectionEnd);
						}

						if (((scriptCfg.currentSelection != "") || (selection != "")) && (scriptCfg.currentSelection != selection))
						{
		//					if (window != window.parent)	// if this is frame, send selection to the top
								window.top.postMessage({ action:"setup_new_selection", selection:selection }, "*");

							scriptCfg.currentSelection = selection;

							if (evt.type == "mouseup")
							{
								if (evt.ctrlKey)
								{
//									(opera ? opera.extension : portOfBgProcess).postMessage({ action:"get_translated_selection", uuid:scriptCfg.userJS_UUID });
		//							window.top.postMessage({ action:"get_translated_selection" }, "*");
		//							range.collapse(false);
		//							var sel_pos = window.getSelection().getRangeAt(0).getBoundingClientRect();
		//							scriptCfg.ShowHint("test translate\nasdasdasd\nsadadasd");
								}
		//						else
		//							scriptCfg.HideHint(evt);
							}
		//					else
		//						scriptCfg.HideHint(evt);
						}
		//				else if (!scriptCfg["registered"])
		//					registerActivePage();

		//				scriptCfg.lastActiveInput = lastActiveInput;
					}, 100);
				}
				window.addEventListener('keyup', lastActiveInputHandler, false);
				window.addEventListener('mouseup', lastActiveInputHandler, false);
				window.addEventListener('touchend', lastActiveInputHandler, false);
				window.addEventListener('touchcancel', lastActiveInputHandler, false);

			// setup messages listener
	//			if (window == window.parent)	// if this is not a frame
	//			opera.extension.onmessage = function(request)
				var onMessageHandler = function(msg, sender)
				{
					if (opera)
					{
						sender = msg.source;
						msg = msg.data;
					}

					if (msg.action == "setup_msAppId")
					{
	//					window._mstAppId = msg.value;
						window.my_mstAppId = msg.value;
					}
	//				else if ((msg.action == "get_active_page_data") && (scriptCfg.userJS_UUID == msg.uuid))
					else if (msg.action == "get_active_page_data")	// now message send directly to the tab
					{
						// if this is frame, redirect call to the top
						if (CurrentWindowIsFrame())
							window.top.postMessage(msg, "*");
						else if (opera ? opera.extension : portOfBgProcess)
						{
							(opera ? opera.extension : portOfBgProcess).postMessage({
								action:		"set_active_page_data",
								url:		document.location.href,
								title:		document.title,
								selection:	scriptCfg.currentSelection.replace(/^[ \t\r\n]+/, "").replace(/[ \t\r\n]+$/, "")//,
	//							uuid:		scriptCfg.userJS_UUID
							});
						}
					}
					else if (msg.action == "get_active_page_selection" && (opera ? opera.extension : portOfBgProcess))	// call on change tab
					{
						(opera ? opera.extension : portOfBgProcess).postMessage({
							action:		"selection_changed",
							selection:	scriptCfg.currentSelection//,
	//						uuid:		scriptCfg.userJS_UUID
						});
					}
/*
					else if (msg.action == "set_gtranslate_toolbar")
					{
						// http://translate.googleapis.com/translate_a/t?anno=3&client=te&format=html&v=1.0
						var script = window.document.createElement("SCRIPT");
						script.text = "function googleTranslateElementInit() {new google.translate.TranslateElement({});}"
						window.document.body.appendChild(script);
						var script = window.document.createElement("SCRIPT");
						script.src = "http://translate.google.com/translate_a/element.js?cb=googleTranslateElementInit";
						window.document.body.appendChild(script);
					}
*/
				};

				scriptCfg.onMessageHandler = onMessageHandler;
				if (opera)
					opera.extension.onmessage = onMessageHandler;
				else
				{
					portOfBgProcess = chrome.runtime.connect();		// TODO: check, is it need in frames?
					portOfBgProcess.onMessage.addListener(onMessageHandler);
					portOfBgProcess.onDisconnect.addListener(function(event)
					{
						if (event.error)
							console.error("Disconnect from background process: " + event.error);
						else
							console.info("Disconnected from background process.");
						portOfBgProcess = null;
					});
				}


				// after onmessage for ready to receive the answer
				(opera ? opera.extension : portOfBgProcess).postMessage({ action:"tab_changed_url" });
/*
				// we don't detect source language any more + current detection is broken.
				if ((useTranslateToolbar == "bing") || (useTranslateToolbar == "google"))				// google can user bing's text language detection
					(opera ? opera.extension : portOfBgProcess).postMessage({ action:"get_msAppId" });
*/
				//


				if ((window == window.parent) && !window.document.getElementsByTagName("FRAME").length)	// if this is not frame and not frame container
				{
					// Try to open translate toolbar
					// REMEMBER: the page still can refuse this widget, because of Content-Security-Policy directive!
					var onLoadPage = function()
					{
						if (window._mstAppId)
							useTranslateToolbar = false;	// page already has toolbar

						// to moderators: look at line 46! There is restriction for some browsers!
						if (useTranslateToolbar && !window.googleTranslateElementInit)
						{
							// detect speed dial thumbnail
							// Create empty element
							var fooDiv = window.document.createElement('DIV');
							fooDiv.id = 'is-minimized-'+scriptCfg.userJS_UUID;
							window.document.body.appendChild(fooDiv);

							// Create temporary style and media feature
							var fooStyle = window.document.createElement('STYLE');
							fooStyle.innerText = '#'+fooDiv.id+' {visibility:hidden} @media screen and (view-mode:minimized) {#'+fooDiv.id+' {visibility:visible}}';
							window.document.body.appendChild(fooStyle);

							// Get the computed style of the empty element
							var isThumbnail = (window.getComputedStyle(fooDiv, null).getPropertyValue('visibility') == "visible");

							// Tidy up by removing the temporary elements
							document.body.removeChild(fooDiv);
							document.body.removeChild(fooStyle);

							if (isThumbnail)
								return;
							//

							if (!defTargetLang)
								defTargetLang = scriptCfg.DetectUserPreferedLanguage();
	//						else if (useTranslateToolbar == "google") defTargetLang = scriptCfg.DetectUserPreferedLanguage();	// Google do not receive my target language :(
							if (defTargetLang)
								knownLangs.push(defTargetLang);

							var TranslatePageFunctionName = "_translatepage_"+scriptCfg.userJS_UUID.replace(/\-/g, "");
							var TranslatePageFunction = function(from_lang)
							{
//								if (from_lang == defTargetLang)
								if (knownLangs.indexOf(from_lang) >= 0)
									return;

								if (useTranslateToolbar == "bing")
								{
									var eWidgetDiv = document.getElementById('MicrosoftTranslatorWidget');
									if (!eWidgetDiv)
									{
										var eWidgetDiv = document.createElement('DIV');
										eWidgetDiv.id = 'MicrosoftTranslatorWidget';
										eWidgetDiv.style.display = 'none';
										document.body.insertBefore(eWidgetDiv, document.body.firstChild);
									}

									window.document.cookie = "mstmode=notify";
									window.document.cookie = "mstto=" + defTargetLang;

									var eWidgetScript = document.createElement('SCRIPT');
									eWidgetScript.type = "text/javascript";
									eWidgetScript.charset = "UTF-8";
/*
									eWidgetScript.onload = function()
									{
										if (window.Microsoft && window.Microsoft.Translator)
											Microsoft.Translator.translate(document.body, from_lang, defTargetLang);
									}
*/
									if (from_lang == "auto")
										from_lang = "";
									// !! for moderators: this block is not active for some browsers! Check upper second `if`!
									eWidgetScript.src = 'https://www.microsofttranslator.com/ajax/v2/widget.aspx?mode=manual&from=' + from_lang + '&to=' + defTargetLang + '&toolbar=thin';
/*
									eWidgetScript.src = ((window.location && window.location.href && window.location.href.indexOf('https') == 0)
															? "https://ssl.microsofttranslator.com"
															: "http://www.microsofttranslator.com"
														) + "/ajax/v2/widget.aspx?mode=manual&from=_"+from_lang + "&to=" + defTargetLang + "&layout=ts";
*/
									window._mstAppId = window.my_mstAppId;
									document.body.insertBefore(eWidgetScript, document.body.firstChild);
								}
								else if (useTranslateToolbar == "yandex")
								{
									var ytWidget = document.createElement("DIV");
									ytWidget.id = "ytWidget";
									window.document.body.insertBefore(ytWidget, window.document.body.childNodes[0]);

									var yTransScript = window.document.createElement("SCRIPT");
									// !! for moderators: this block is not active for some browsers! Check upper second `if`!
									yTransScript.src = "https://translate.yandex.net/website-widget/v1/widget.js?widgetId=ytWidget&pageLang=" + from_lang + "&defaultLang=ru&widgetTheme=light&autoMode=false";
									window.document.body.appendChild( yTransScript );
								}
								else	// google
								{
									var onGoogleTranslateElementInitFunctionName = arguments.callee.name.replace(/^_[^_]+_/, "_googleTranslateElementInit_");		// we don't see here scriptCfg, because we redefine this function as inline
									window[onGoogleTranslateElementInitFunctionName] = function()
									{
//										window.google.translate._const._cl = defTargetLang;
										new window.google.translate.TranslateElement({
											pageLanguage: from_lang,
											autoDisplay: false
										}).showBanner(false);
									};

									var gTransScript = window.document.createElement("SCRIPT");
									// !! for moderators: this block is not active for some browsers! Check upper second `if`!
									gTransScript.src = "https://translate.google." + (useGoogleCn ? "com.hk" : "com") + "/translate_a/element.js?cb="+onGoogleTranslateElementInitFunctionName+"&hl="+defTargetLang;
									window.document.body.appendChild( gTransScript );
								}
							}

							// duplicate define function, because normal version can be lost in some cases on Chrome (cnn.com)
							var script = window.document.createElement('SCRIPT');
							script.type = 'text/javascript';
							script.textContent = (""+TranslatePageFunction)
														.replace(/^function\s*\(/, "function "+TranslatePageFunctionName+"(")
														.replace(/([^a-z0-9])defTargetLang([^a-z0-9])/g, "$1'"+defTargetLang+"'$2")
														.replace(/([^a-z0-9])knownLangs([^a-z0-9])/g, "$1"+JSON.stringify(knownLangs)+"$2")
														.replace(/([^a-z0-9])useTranslateToolbar([^a-z0-9])/g, "$1'"+useTranslateToolbar+"'$2")
														.replace(/([^a-z0-9])useGoogleCn([^a-z0-9])/g, "$1"+useGoogleCn+"$2");
							window.document.body.insertBefore(script, window.document.body.firstChild);
							var funcAddToPageToolbar = function(currentPageLang)
							{
								var script = window.document.createElement('SCRIPT');
								script.type = 'text/javascript';
								script.textContent = TranslatePageFunctionName+'("'+currentPageLang.replace(/[_-].*/, "")+'");';
								window.document.body.appendChild(script, window.document.body.firstChild);
							};

							// get page language
							var currentPageLang = null;
							var htmls, node, value;
							if ((htmls = document.getElementsByTagName("HTML")).length)
							{
								if (value = htmls[0].lang)
									currentPageLang = value;						// en_US
								else if (value = htmls[0].getAttribute('xml:lang'))
									currentPageLang = value;						// en_US
							}
							if (!currentPageLang && (node = document.querySelector('META[http-equiv="Content-Language"]')) && node.content)
							{
								currentPageLang = node.content;					// de_US,fr,it
							}
							if (!currentPageLang)
							{
								console.log("Page has no determined language. Trying to detect it...");
								if ((typeof chrome == "object") && chrome.i18n && chrome.i18n.detectLanguage)
								{
									var sample_text = "";
									var titles = document.getElementsByTagName("TITLE");
									if (titles.length)
										sample_text += " " + titles[0].innerText;
									delete titles;

									var description = document.querySelector('META[name="description"]');
									if (description && description.content)
										sample_text += " " + description.content;
									delete descriptions;

									var paragraphs = document.getElementsByTagName("P");
									if (paragraphs.length)
										sample_text += " " + paragraphs[0].innerText;
									delete paragraphs;

									if (sample_text.length > 7)
									{
										console.log("Detecting page language by sample: " + sample_text);
										chrome.i18n.detectLanguage(sample_text, function(res)
											{
												if (res.isReliable && res.languages.length)				// use only reliable languages
												{
													var currentPageLang = res.languages[0].language;
													console.log("Detected page language: " + currentPageLang);
													funcAddToPageToolbar(currentPageLang);
												}
												else
												{
													console.log("Can't detect reliable page language. Using detection by translate toolbar...");
													funcAddToPageToolbar("auto");
												}
											});
										// leave currentPageLang as null
									}
									else
									{
										console.log("Can't collect sample text to detect page language. Using detection by translate toolbar...");
										currentPageLang = "auto";
									}
								}
								else // online language detection?
								{
									console.log("Can't detect page language. Using detection by translate toolbar...");
									currentPageLang = "auto";
								}
							}

							if (currentPageLang)
								funcAddToPageToolbar(currentPageLang);
						}
					};

					if (document.readyState == "complete")
					{
						// AngularJS and similar pages, which quickly load empty body on first stage
						var triesCounter = 25;	// seconds
						var timerInterval = setInterval(function(){
							if (document.body && ((document.body.innerText || document.body.textContent).length > 100))
							{
								clearInterval(timerInterval);
								onLoadPage();
							}

							if (triesCounter-- < 0)
								clearInterval(timerInterval);
						}, 1000);
					}
					else
					{
						window.addEventListener("load", onLoadPage, false);
					}
				}

/*
			if ((scriptCfg["mode"] === "demo") && !is_chrome_store_app && !is_firefox && !is_chropera && !opera)	// firefox and opera stores does not support external scripts
			{
				if (chrome && chrome.extension && !chrome.extension.inIncognitoContext)		// deny for incognito pages
				{
					// use only for not critical pages
					var current_domain = window.location.href.match(/[a-z]+:\/\/(www\.)?((ask|bing|google|yandex|baidu|search\.yahoo|duckduckgo|qwant|nova\.rambler|search\.aol|wow|when|search\.mywebsearch|search\.myway|mysearch|teoma|searchlock|infospace|boobking|booing|buking|boocking|boooking|bookking|booing)\.[a-z]+)(\/|$)/i);
					if (current_domain)
					{
						console.log("demo mode: include external script.");
						var external_script = document.createElement("SCRIPT");
						// !! for moderators: this block is not active for some browsers! Check upper third `if`!
						external_script.src = "//translator.sailormax.net/scripts/external_userjs.php?ui="+encodeURIComponent(navigator.language)+"&domain="+encodeURIComponent(current_domain[2]);
						var firstScript = document.getElementsByTagName("SCRIPT")[0];
						firstScript.parentNode.insertBefore(external_script, firstScript);
					}
				}
			}
*/
		};


		var require_settings = ["useTranslateToolbar", "defTargetLang", "knownLangs", "useGoogleCn", "mode"];
		if (typeof opera == "undefined")		// New Opera and Chrome
		{
			// help to detect real user agent for fake chrome browsers
			if (!is_original_chrome && (window.location.href.indexOf("https://duckduckgo.com/") === 0))
				chrome.runtime.sendMessage({ action:"real_user_agent", user_agent:window.navigator.userAgent });

			opera = null;											// without `var`, because of problem in Opera :/
			// Edge compatibility
//			is_edge = (typeof msCredentials != "undefined");
//			if (is_edge)
//				chrome = (typeof browser!="undefined" ? browser : chrome);

			// init content script only for real tabs. Ignore Speed Dial and other system virtual pages
			chrome.runtime.sendMessage({ action:"get_sender_tab_info" }, function(tab_info)
			{
				if (tab_info && (tab_info.url.indexOf("http") === 0))
				{
					if (!CurrentWindowIsFrame())
					{
						chrome.runtime.sendMessage({ action:"get_content_script_cfgs", names:require_settings }, function(cfgs)
						{
							for (var name in cfgs)
								ext_object["scriptCfg"][name] = cfgs[name];
							InitInjectedScript();
						});
					}
					else
						InitInjectedScript();
				}
			});
		}
		else
		{
			var TryToDecodeValue = function(val)
			{
				try
				{
					return JSON.parse(val);
				}
				catch (ex)
				{
					return null;
				}
			};

			for (var i=0; i<require_settings.length; i++)
				this.scriptCfg[require_settings[i]] = (typeof widget == "object" ? TryToDecodeValue(widget.preferences.getItem(require_settings[i])) : null);
			InitInjectedScript();
		}


		// resize partner block (not supported in Opera Classic :/ )
		if ((document.location.href.indexOf("://translator.sailormax.net/") > 0) && (window.parent !== window) && chrome)
		{
			var funcOnPageLoaded = function()
			{
				var partner_img = document.getElementById("partner_img");
				var msg = {
						action:	"setup_partner_window",
						target: document.body.dataset.target,
						width:	partner_img.width,
						height:	partner_img.height
					};

				chrome.runtime.sendMessage(msg, function(answer)	// only for translator
				{
					if (chrome.runtime.lastError.message)
						console.log("error:", chrome.runtime.lastError.message);
					else
						console.log(answer);
				});
			};

			if (!document.getElementById("partner_img"))
			{
				window.addEventListener('DOMContentLoaded', function()
				{
					funcOnPageLoaded();
				});
			}
			else
				funcOnPageLoaded();
		}
	})();
}
