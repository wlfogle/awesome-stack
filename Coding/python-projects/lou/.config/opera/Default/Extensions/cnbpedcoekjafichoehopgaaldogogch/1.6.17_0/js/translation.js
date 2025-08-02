/* globals widget, EXT_LOCALE, msAppId, yaAppId, udAppId, sbStoragePrefix, mostUsedLangPairs, WORDS, languages, lingvoLanguages, promtLanguagesFrom, babylonLanguages, StoreDetectedInputStringsLang, StoreLastToFromText, StoreLastFromText, StoreLastToText, AddMostUsedLangPair, ShowAutodetectedLang, ChangeToLang, SetupBackTranslationVisibility, StartTranslatingAnimation, StopTranslatingAnimation, SendAjaxRequest, SendMessageToBackgroundScript, useGoogleCn, activePage, defTargetLang, detectedInputStringsLang, promtLanguagesTo, RemoveClass, AppendClass, Trim, CreateTabWithUrl, OnEventOfFromToText, OnEventOfToText, OnEventOfFromText, baiduLanguagesTo, msLanguagesAutodetect, GetSignOfBaiduQuery, GetTranslatedPageUrlPrefix, bdAppId, bdSignId, GetSogouQuerySign, GetSogouQueryUuid, sgAppId, sgSignId */
// Translator-extension

var inputStrings = [], outputStrings = [];
var xhrs = [], scripts = [];
var translatedStringsCnt = 0;
var translateProvider;
var retranslateCountred = 0;
var backTranslating = false;

function getTranslateHandler(idx, resultOf, onExtract)
{
	return function(result, error, status)
	{
		// TODO: separate handlers for eache service with not supported languages support
		if (typeof inputStrings[idx] != "string")
			return;

		if (typeof result == "string")	// result as string (errors or non-object answers)
		{
			outputStrings[idx] = result.replace(/\\u000A/g, "\n").replace(/\\\"/g, '"') + (outputStrings[idx] || "");	// here can be transcription already

			if (translateProvider == "yandex")
			{
				// No translations for specified language pair in both requests
				if ((idx == 1)
					&& (outputStrings[idx-1] && outputStrings[idx-1].substr(0, 14) == "error status: ")
					&& (outputStrings[idx] && outputStrings[idx].substr(0, 14) == "error status: ")
					)
				{
					outputStrings[idx-1] = WORDS.msgError+": "+WORDS.msgUnknownLng;
					outputStrings[idx] = "";
				}
				else if ((idx === 0)
					&& (outputStrings[idx+1] && outputStrings[idx+1].substr(0, 14) == "error status: ")
					&& (outputStrings[idx] && outputStrings[idx].substr(0, 14) == "error status: ")
					)
				{
					outputStrings[idx] = WORDS.msgError+": "+WORDS.msgUnknownLng;
					outputStrings[idx+1] = "";
				}
				else
				{
					// No translations for specified language pair in one request
					if ((outputStrings[idx] && outputStrings[idx].substr(0, 14) == "error status: "))	// current is error
					{
						if (outputStrings[idx-1] && outputStrings[idx-1].substr(0, 14) != "error status: ")	// previous was not error
							outputStrings[idx] = "";
						if (outputStrings[idx+1] && outputStrings[idx+1].substr(0, 14) != "error status: ")	// next is not error
							outputStrings[idx] = "";
					}
					else
					{
						if (outputStrings[idx-1] && outputStrings[idx-1].substr(0, 14) == "error status: ")	// previous was error
						{
							outputStrings[idx-1] = "";
						}
						if (outputStrings[idx+1] && outputStrings[idx+1].substr(0, 14) == "error status: ")	// next is error
						{
							outputStrings[idx+1] = "";
						}
					}
				}
			}
			else if (translateProvider == "bing")
			{
				if (outputStrings[idx].indexOf("ArgumentOutOfRangeException: 'from' must be a valid language") === 0)
					outputStrings[idx] = WORDS.msgError+": "+WORDS.msgUnknownLng;
				else if (outputStrings[idx].indexOf("error status: 400 ({") === 0)	// wrong lang pair???
					outputStrings[idx] = "";	// dictionary error we can skip
				else if (outputStrings[idx].indexOf("error status: 502 ({") === 0)	// service disabled
					outputStrings[idx] = "";	// dictionary error we can skip
				else if ((outputStrings[idx].indexOf("error status: 500 ({") === 0) && (outputStrings[idx].indexOf("Object reference not set") > 0))
					outputStrings[idx] = WORDS.msgError+": "+WORDS.msgUnknownLng;
			}
			else if (translateProvider == "babylon")
			{
				// babylonTranslator.callback('babylon.0.7._babylon_api_response', {"translatedText":"\u0411\u0435\u0441\u043f\u043b\u0430\u0442\u043d\u043e"}, 200, null, null);
				var matches;
				if (matches = outputStrings[idx].match(/^babylonTranslator.callback\([^{]+{"translatedText":"(.*)"}[^}]+$/))
				{
					result = unescape( matches[1].replace(/\\u([\d\w]{4})/gi, function (match, grp) { return String.fromCharCode(parseInt(grp, 16)); } ) );
					result = result.replace(/\\n/g, "\n");

					// remove DIV elements and fix result
					var fooBox = document.createElement("DIV");
					fooBox.innerHTML = result.replace(/\\(.)/g, "$1");
					result = fooBox.innerText;
					result = result.replace(/\n +\n/g, "\n");
					result = result.replace(/^ +([^ ])/gm, "$1");
					result = result.replace("\\\n", "\\n");			// restore new lines

					outputStrings[idx] = result;
				}
			}
			else if (translateProvider == "apertium")
			{
				// _jqjsp({"responseData": {"translatedText": "Nombre de página"}, "responseDetails": null, "responseStatus": 200})
				var matches;
				if (matches = outputStrings[idx].match(/^[^{]+({"responseData":[^\)]*)\)$/))
				{
					try
					{
						result = JSON.parse(matches[1]);
						outputStrings[idx] = result.responseData.translatedText;
					}
					catch (e)
					{
					}
				}
			}
		}
		else if (result === null)		// null also object => separate if
		{
			outputStrings[idx] = result;
		}
		else if (typeof result == "object")	// result as object
		{
			if ((translateProvider == "google") && result.sentences && result.sentences.length)	// google dictionary (already not DEPRECATED)
			{
				//{"sentences":[{"trans":"Sports","orig":"спорт","translit":"","src_translit":"sport"}],"src":"mk","server_time":2}
				//{"sentences":[{"trans":"test page","orig":"test page","translit":"","src_translit":""}],"src":"en","server_time":56}
				var out = [];

				if (result.sentences.length > 1)
				{
					var i, len = result.sentences.length;
					for (i=0; i<len; i++)
						if (result.sentences[i].trans)
//							out.push( result.sentences[i].trans.replace(/ ([,.]) /g, "$1 ").replace(/ ([,.])$/g, "$1") );		// latest time google return commas in spaces :/
							out.push( result.sentences[i].trans );

					out = [out.join("")];
				}
				else
				{
					var sentences = result.sentences[0];
					if (sentences.trans)
					{
						// transcription of orig word
						var translit = "";
						if (sentences.src_translit && (inputStrings.length == 1) && (sentences.src_translit.split(/[ \r\n\t]+/).length < 3))
						{
							translit = " <= ["+sentences.src_translit+"]";
						}

						if (sentences.translit && (inputStrings.length == 1) && (sentences.translit.split(/[ \r\n\t]+/).length < 3))
						{
							out.push( sentences.trans + " ["+sentences.translit+"]" + translit + "\n" );
						}
						else
							out.push( sentences.trans + translit + "\n" );
					}
				}

				if (result.dict)
				{
					out.push( "" );
					var i, j, dict = result.dict;
					for (i=0; i<dict.length; i++)
					{
						if (dict[i].pos && dict[i].pos.length)
							out.push( dict[i].pos + ":" );

						for (j=0; j<dict[i].terms.length; j++)
							out.push( (j+1) + ". " + dict[i].terms[j] );

						if (dict[i].pos || dict[i].terms.length)
							out.push( "" );
					}
				}

				outputStrings[idx] = out.join("\n");	// join parts via \n. May be need space?
				if (!backTranslating)
					StoreDetectedInputStringsLang( result.src );
			}
			else if ((translateProvider == "google") && result.length && result.join && result[0] && result[0][0] && result[0][0].length)	// new google dictionary
			{
				// [[["Гугл переводчик","google translate","Gugl perevodchik","",10]],,"en",,[["Гугл переводчик",[32000],false,false,0,0,0,0]],[["google translate",32000,[["Гугл переводчик",0,true,false],["Google Translate",0,true,false],["Google Translator и может",0,true,false],["Google переводчик",0,true,false],["Google Translator и",0,true,false]],[[0,16]],"google translate"]],,,[["en"]],45]
				// [[["окно","window",,,10],[,,"okno","ˈwindō"]],
				//	[["noun",["окно","окошко","витрина"],[["окно",["window","casement","gap","light"],,0.50283158],["окошко",["window"],,0.0056739203],["витрина",["showcase","glass case","window","shopwindow","case","shopfront"],,0.00035709812]],"window",1],["adjective",["оконный"],[["оконный",["window"],,0.020432571]],"window",3]],
				//	"en",
				//	,,[["window",32000,[["окно",0,true,false],["окна",0,true,false],["окне",0,true,false],["окном",0,true,false]],[[0,6]],"window",0,0]],1,,[["en"],,[1]],,,[["noun",[[["windowpane"],""]],"window"]],[["noun",[["an opening in the wall or roof of a building or vehicle that is fitted with glass or other transparent material in a frame to admit light or air and allow people to see out.","m_en_us1306540.001","The apartments and penthouses have double-glazed redwood framed windows , fitted kitchens and gas-fired central heating."],["a thing resembling a window in form or function, in particular.","m_en_us1306540.005"],["an interval or opportunity for action.","m_en_us1306540.010","February 15 to March 15 should be the final window for new offers"],["strips of metal foil or metal filings dispersed in the air to obstruct radar detection.","m_en_us1306540.012"]],"window"]],[[["You then presented this screen to the shopper in a pop-up window - something like a cash point ATM \u003cb\u003ewindow\u003c/b\u003e .",,,,3,"m_en_us1306540.003"],["He smiled at the \u003cb\u003ewindow\u003c/b\u003e of good opportunity that he thought he was getting into.",,,,3,"m_en_us1306540.010"],["browser \u003cb\u003ewindow\u003c/b\u003e",,,,3,"neid_22838"],["he broke the \u003cb\u003ewindow\u003c/b\u003e",,,,3,"neid_22835"],["Other differences relate to the rules for entering a phrase into the search engine phrase \u003cb\u003ewindow\u003c/b\u003e .",,,,3,"m_en_us1306540.007"],["A large shuttered sash \u003cb\u003ewindow\u003c/b\u003e overlooking the communal square makes this an exceptionally bright area.",,,,3,"m_en_us1306540.001"],["I notice that Jim Lamb is suiting up early and he's thinking that its time to go soon after the launch \u003cb\u003ewindow\u003c/b\u003e opens.",,,,3,"m_en_us1306540.011"],["thieves smashed a \u003cb\u003ewindow\u003c/b\u003e and took $600",,,,3,"m_en_us1306540.002"],["Food-themed \u003cb\u003ewindow\u003c/b\u003e displays in many shops and businesses in the town also added extra interest.",,,,3,"m_en_us1306540.004"],["Her eyes were gazing out the bay \u003cb\u003ewindow\u003c/b\u003e in her room.",,,,3,"m_en_us1306540.001"],["It's most effective used as a road map of the recent past, or more trivially, a \u003cb\u003ewindow\u003c/b\u003e on what happened the year you were born.",,,,3,"m_en_us1306540.008"],["Vijay Kranti hopes that the current exhibition will help open a \u003cb\u003ewindow\u003c/b\u003e on the life of those who have made the country their own.",,,,3,"m_en_us1306540.008"],["They found no hint of trouble and were able to make their launch \u003cb\u003ewindow\u003c/b\u003e in time.",,,,3,"m_en_us1306540.011"],["beautiful \u003cb\u003ewindow\u003c/b\u003e displays",,,,3,"m_en_gb0954400.004"],["A stained glass \u003cb\u003ewindow\u003c/b\u003e was smashed, along with plaster statues and the church organ, police said.",,,,3,"m_en_us1306540.002"],["Channel 5 is currently acting as a \u003cb\u003ewindow\u003c/b\u003e on America, with its America's Finest strand.",,,,3,"m_en_us1306540.008"],["Kat glanced out the car \u003cb\u003ewindow\u003c/b\u003e looking around, taking as much of the place into her memory as possible.",,,,3,"m_en_us1306540.001"],["When a keen reader writes about their reading, they are opening a \u003cb\u003ewindow\u003c/b\u003e into their soul, and inviting you to step inside and share a holy thing.",,,,3,"m_en_us1306540.008"],["A time capsule full of treasures has opened a \u003cb\u003ewindow\u003c/b\u003e into what life was like 113 years ago in Swindon.",,,,3,"m_en_us1306540.008"],["this is a \u003cb\u003ewindow\u003c/b\u003e of opportunity for companies",,,,3,"neid_22839"],["I prefer the red dress that's in the \u003cb\u003ewindow\u003c/b\u003e",,,,3,"m_en_us1306540.004"],["Selectors and critics forget that this is a \u003cb\u003ewindow\u003c/b\u003e on Indian cinema, good Indian cinema.",,,,3,"m_en_us1306540.008"],["The thieves broke in by forcing a casement \u003cb\u003ewindow\u003c/b\u003e in the dining room before ransacking the house.",,,,3,"m_en_us1306540.001"],["The front passenger \u003cb\u003ewindow\u003c/b\u003e rolled down just enough so she could see James Alcott.",,,,3,"m_en_us1306540.001"],["thieves smashed a \u003cb\u003ewindow\u003c/b\u003e and took £600",,,,3,"m_en_gb0954400.002"],["Microsoft Windows users can think of a terminal as like a DOS prompt or command \u003cb\u003ewindow\u003c/b\u003e .",,,,3,"m_en_us1306540.007"],["Light is drawn into the room through a large bay \u003cb\u003ewindow\u003c/b\u003e overlooking the front garden.",,,,3,"m_en_us1306540.001"],["I turned my head to the left and saw Rashad leaning out the front passenger side \u003cb\u003ewindow\u003c/b\u003e .",,,,3,"m_en_us1306540.001"],["This periodic table was spotted last week in Miami in the \u003cb\u003ewindow\u003c/b\u003e of an Armani shop.",,,,3,"m_en_us1306540.004"],["the office \u003cb\u003ewindow\u003c/b\u003e",,,,3,"neid_22835"]]],[["open the window","close the window","by the window","window shopping","window seat","ticket window","window cleaner","rear window","shop window","window frame"]]]
				var out = [];

				// translits
				var translit = "";
				var source_translit = "";

				// only for single string translate
				if (inputStrings.length == 1)
				{
					if (result[0][0] && result[0][0].pop && result[0][0][2] && (result[0][0][2] !== ""))
						translit = " ["+result[0][0][2]+"]";
					else if (result[0][1] && result[0][1].pop && result[0][1][2] && (result[0][1][2] !== ""))
						translit = " ["+result[0][1][2]+"]";

					if (result[0][0] && result[0][0].pop && result[0][0][3] && (result[0][0][3] !== ""))
						source_translit = " <= ["+result[0][0][3]+"]";
					else if (result[0][1] && result[0][1].pop && result[0][1][3] && (result[0][1][3] !== ""))
						source_translit = " <= ["+result[0][1][3]+"]";

					// if more than 2 words => ignore
					if ((translit.split(/[ \r\n\t]+/).length > 3))	// " [the word]" => 3 is ok
						translit = "";
					if ((source_translit.split(/[ \r\n\t]+/).length > 4))	// " <= [the word]" => 4 is ok
						source_translit = "";
				}

				// translate
				// TODO: recheck multi sentences text
				if (result[0].length > 1 && (translit === "") && (source_translit === ""))
				{
					var sentences = [];
					if (result[0][0].pop)
					{
						for (i=0; i<result[0].length; i++)
							sentences.push( result[0][i][0] );
						out.push( sentences.join("") );
					}
					else	// .com.hk
						out.push( result[0][0] );
				}
				else
					out.push( result[0][0][0] + translit + source_translit );
				out.push( "" );

				// dictionary
				if (result[1] && result[1].length)
				{
					var i, j, dict = result[1];
					for (i=0; i<dict.length; i++)
					{
						if (dict[i][0] && dict[i][0].length)
							out.push( dict[i][0] + ":" );

						for (j=0; j<dict[i][1].length; j++)
							out.push( (j+1) + ". " + dict[i][1][j] );

						out.push( "" );
					}
				}

				// collect result
				outputStrings[idx] = out.join("\n");	// join parts via \n. May be need space?
				if (!backTranslating)
				{
					if (result.length > 2)
						StoreDetectedInputStringsLang( result[2] );
					else if (result.length == 1 && result[0].length == 2)
						StoreDetectedInputStringsLang( result[0][1] );	// .com.hk
				}
			}
			else if ((translateProvider == "bing") && result.length && result[0].detectedLanguage && result[0].translations)		// New3 Microsoft Translator
			{
				outputStrings[idx] = result[0].translations[0].text;
				if (!backTranslating)
				{
					var lang = result[0].detectedLanguage.language;
					StoreDetectedInputStringsLang( msLanguagesAutodetect[lang] ? msLanguagesAutodetect[lang] : lang );
				}
			}
			else if ((translateProvider == "bing") && result.length && result[0].displaySource && result[0].normalizedSource && result[0].translations)		// New3 Microsoft Translator Dictionary
			{
				var dict = result[0].translations;
				if (dict.length > 0)
				{
					var out = [];
					out.push( "" );
					out.push( "" );

					// collect by types
					var i, j;
					var type, by_types = {};
					for (i=0; i<dict.length; i++)
					{
						type = dict[i].posTag.toLowerCase();
						if (typeof by_types[type] == "undefined")
							by_types[type] = [ type+":" ];
						by_types[type].push( (by_types[type].length) + ". " + dict[i].displayTarget );
					}

					// join to `out`
					for (type in by_types)
					{
						[].push.apply(out, by_types[type]);
						out.push( "" );
					}

					outputStrings[idx] = out.join("\n");
				}
			}
			else if ((translateProvider == "bing") && result.statusCode)	// New3 Microsoft Translator error message
			{
				if (idx === 0)
					outputStrings[idx+1] = "error code: " + result.statusCode;
				else
					console.log("Translate request (" + translateProvider + ")[" + idx + "] retrun error code: " + result.statusCode);
			}
			else if ((translateProvider == "yandex") && result.code && result.message)	// yandex error message
			{
				outputStrings[idx+1] = result.message;
			}
			else if ((translateProvider == "yandex") && result.text)	// yandex (translate)
			{
				outputStrings[idx] = result.text.join("\n") + (outputStrings[idx] || "");		// here can be transcription already

				// clear dictionary error, if it has (but current translate is fine)
				if (outputStrings[idx+1] && (outputStrings[idx+1].substr(0, 14) == "error status: "))	// next is error
					outputStrings[idx+1] = "";
			}
			else if ((translateProvider == "yandex") && result.def)	// yandex dictionary
			{
				if (result.def.length)	// ignore empty def
				{
					var out = [];
					if (idx > 0)
					{
						out.push( "" );
						out.push( "" );
					}

					var translit = "";

					var i, j, dict = result.def;
					for (i=0; i<dict.length; i++)
					{
						if ((translit === "") && dict[i].ts)
							outputStrings[0] = (outputStrings[0] || "") + (translit = " <= ["+dict[i].ts+"]");

						if (dict[i].pos)
							out.push( dict[i].pos + ":" );

						for (j=0; j<dict[i].tr.length; j++)
							out.push( (j+1) + ". " + dict[i].tr[j].text );

						if (dict[i].pos || dict[i].tr.length)
							out.push( "" );
					}

					outputStrings[idx] = out.join("\n");
				}
			}
			else if ((translateProvider == "promt") && result.text)	// promt
			{
				if (result.error)
				{
					outputStrings[idx] = WORDS.msgErrorCode+": " + result.error;
				}
				else
				{
					var tmp_div = document.createElement("DIV");
					tmp_div.innerHTML = result.dictHtml;
					if (tmp_div.querySelector && tmp_div.querySelector("#dictView"))
					{
						var out = [];
						var res_parts = tmp_div.querySelectorAll("#dictView .cforms_result:nth-child(3) .translations .result_only");
						out.push(result.text + (res_parts.length ? " <= ["+Trim(res_parts[0].innerText.toLowerCase())+"]" : ""));
						out.push("");

						var res_parts = tmp_div.querySelectorAll("#dictView .cforms_result:first-child .translations .result_only");
						var i, len = res_parts.length;
						if (len > 0)
							out.push(WORDS.msgAlternatives+":");
						for (i=0; i<len; i++)
							out.push( "- " + Trim(res_parts[i].innerText.toLowerCase()) );
						outputStrings[idx] = out.join("\n");
					}
					else
						outputStrings[idx] = result.text;

					var detected_lang = result.from;
					if (!backTranslating && detected_lang)
						StoreDetectedInputStringsLang( detected_lang );
				}
			}
			else if ((translateProvider == "pragma") && result.dat && result.dat[0].rid)				// Pragma via extension API
			{
				outputStrings[idx] = unescape( result.dat[0].text.join("\n").replace(/\\u([\d\w]{4})/gi, function (match, grp) { return String.fromCharCode(parseInt(grp, 16)); } ) );
			}
			else if ((translateProvider == "baidu") && result.trans_result && result.trans_result.data)
			{
				// {"trans_result":{"from":"en","to":"ru","domain":"all","type":2,"status":0,"data":[{"dst":"\u0438\u0441\u043f\u044b\u0442\u0430\u043d\u0438\u044f","prefixWrap":0,"src":"test","relation":[],"result":""}]},"dict_result":[],"liju_result":[],"logid":2784317112}
				outputStrings[idx] = unescape( result.trans_result.data[0].dst.replace(/\\u([\d\w]{4})/gi, function (match, grp) { return String.fromCharCode(parseInt(grp, 16)); } ) );
			}
			else if ((translateProvider == "baidu") && result.error)
			{
				outputStrings[idx] = WORDS.msgErrorCode+": " + result.error;
			}
			else if ((translateProvider == "naver") && result.translatedText)
			{
				var res = result.translatedText;
				if ((inputStrings.length == 1) && (result.tlit && result.tlit.message && result.tlit.message.tlitResult && result.tlit.message.tlitResult.length <= 3))	// only 2 words
				{
					var translit = [];
					var raw_translit = result.tlit.message.tlitResult;
					var i, cnt = raw_translit.length;
					for (i=0; i<cnt; i++)
						translit.push(raw_translit[i].phoneme);
					res += " <= ["+(translit.join(" "))+"]";
				}
				outputStrings[idx] = res;

				var detected_lang = result.srcLangType;
				if (!backTranslating && detected_lang)
					StoreDetectedInputStringsLang( detected_lang );
			}
			else if ((translateProvider == "naver") && result.code)	// error
			{
				outputStrings[idx] = WORDS.msgErrorCode+": " + result.code + " - " + result.message + " " + result.displayMessage;
			}
			else if ((translateProvider == "sogou") && result.data && result.data.translate)
			{
				// {"data":{"sgtkn":"C1F2EC6BD22142FB393D9C3319A86FB0393C0A385EB3E102","detect":{"zly":"zly","detect":"en","errorCode":"0","language":"英语","id":"b902ee45-1f3e-447f-a14d-1faaf48026f1","text":"page"},"translate":{"zly":"zly","errorCode":"0","index":"content0","from":"en","to":"ru","text":"page","id":"b902ee45-1f3e-447f-a14d-1faaf48026f1","dit":"Страницы","md5":""}},"zly":"zly","info":"success","status":0}
				outputStrings[idx] = result.data.translate.dit;

				if (result.data.detect)
				{
					var detected_lang = result.data.detect.detect;
					if (!backTranslating && detected_lang)
						StoreDetectedInputStringsLang( detected_lang );
				}
			}
			else if ((translateProvider == "sogou") && result.status && (result.status != "0"))
			{
				// error
				outputStrings[idx] = WORDS.msgErrorCode+": " + result.status + " - " + result.info;
			}
			else if ((translateProvider == "sogou") && result.translate && result.translate.errorCode)
			{
				// error
				outputStrings[idx] = WORDS.msgError+" (" + result.translate.errorCode + ")"+": "+WORDS.msgUnknownLng;
			}
			else if ((translateProvider == "systran") && result.source && result.target)
			{
				var res = result.target;
				res = res.replace(/<[^>]+>/g, " ");	// remove tags
				res = Trim(res);
				outputStrings[idx] = res;

				var detected_lang = result.detectedLanguage;
				if (!backTranslating && detected_lang)
					StoreDetectedInputStringsLang( detected_lang );
			}
			else if (translateProvider == "systran" && result.error)	// error
			{
				outputStrings[idx] = WORDS.msgErrorCode+": " + result.error.statusCode + " - " + (result.error.parsedDetails ? result.error.parsedDetails.join("") : "");
			}
			else if ((translateProvider == "lingvo") && result.items)				// WordListPart
			{
				if (result.items.length > 0)
				{
					var dict = {};
					var i, len = result.items.length;
					for (i=0; i<len; i++)
					{
						if (typeof dict[result.items[i].heading] == "object")
						{
							if (dict[result.items[i].heading].indexOf(result.items[i].lingvoTranslations) < 0)
								dict[result.items[i].heading].push(result.items[i].lingvoTranslations);
						}
						else if (result.items[i].lingvoTranslations)
							dict[result.items[i].heading] = [result.items[i].lingvoTranslations];
						else
							dict[result.items[i].heading] = [result.items[i].socialTranslations];
					}

					var out = [];
					for (i in dict)
						out.push(i + " ‒ " + dict[i].join(", "));

					outputStrings[idx] = out.join("\n");	// join parts via \n. May be need space?
					outputStrings[idx] += "\n\n";
				}
				else if (!result.sourceLanguageId)
					outputStrings[idx] = WORDS.msgError+": "+WORDS.msgUnknownSourceLng;
				else
					outputStrings[idx] = "";
			}
			else if ((translateProvider == "lingvo") && result.foundUnits)			// Phrases
			{
				if (result.foundUnits.length > 0)
				{
					var dict = {};
					var i, len = result.foundUnits.length;
					for (i=0; i<len; i++)
					{
						if (typeof dict[result.foundUnits[i].sourceFragment.text] == "object")
						{
							if (dict[result.foundUnits[i].sourceFragment.text].indexOf(result.foundUnits[i].targetFragment.text) < 0)
								dict[result.foundUnits[i].sourceFragment.text].push(result.foundUnits[i].targetFragment.text);
						}
						else
							dict[result.foundUnits[i].sourceFragment.text] = [result.foundUnits[i].targetFragment.text];
					}

					var out = [];
					for (i in dict)
						out.push(i + " ‒ " + dict[i].join(", "));
					outputStrings[idx] = out.join("\n");	// join parts via \n. May be need space?
				}
				else
					outputStrings[idx] = "";

				outputStrings[idx] += "\n\n(c) https://lingvolive.com/";
			}
			else if ((translateProvider == "urban") && result.list)		// Urban Dictionary		// old:  && result.result_type
			{
				// http://api.urbandictionary.com/v0/define?key=54dfbdc4e47a0f59e23b186668684cd4&term=andre&0.6412415498425227
				var out = [];

				var i, len = result.list.length;
				if (len)
				{
					for (i=0; i<len; i++)
					{
						out.push( "=== " + result.list[i].word + " ("+result.list[i].author+") +" + result.list[i].thumbs_up + ", -" + result.list[i].thumbs_down +" ===" );
						out.push( result.list[i].definition );
						if (result.list[i].example)
						{
							out.push( "\n// "+WORDS.msgExample+":" );	// do we need translate it, if this dictionary only use english?
							out.push( result.list[i].example );
						}
						out.push( "\n" );
					}
				}

				outputStrings[idx] = out.join("\n");	// join parts via \n. May be need space?
			}
/*
			else if ((translateProvider == "glosbe") && result.result && result.tuc)
			{
				// https://glosbe.com/gapi/translate?from=pol&dest=eng&format=json&phrase=witaj&pretty=true
				var out = [];

				var i, len = result.tuc.length;
				if (len)
				{
					for (i=0; i<len; i++)
					{
						if (i == 1)
							out.push("\n"+WORDS.msgAlternatives+":");

						out.push( (i ? i+". " : "") + result.tuc[i].phrase.text );

						if (i >= 25)
							break;
					}
				}
				else
				{
//					out.push("translation not found.");
//					out.push("");
				}

				outputStrings[idx] = out.join("\n");	// join parts via \n. May be need space?
			}
			else if ((translateProvider == "glosbe") && result.result && result.examples)
			{
				// https://glosbe.com/gapi/tm?from=pol&dest=eng&format=json&phrase=witaj&pretty=true&page=1&pageSize=10
				var out = [];

				var i, len = result.examples.length;
				if (len)
				{
					out.push("\n"+WORDS.msgExamples+":");
					for (i=0; i<len; i++)
					{
						out.push( "> " + result.examples[i].first );
						out.push( "> " + result.examples[i].second );
						out.push( "" );
					}
				}

				outputStrings[idx] = out.join("\n");	// join parts via \n. May be need space?
			}
*/
			else if ((translateProvider == "glosbe") && result.input && result.translation)
			{
				outputStrings[idx] = result.translation;
			}
			else if ((translateProvider == "ibm") && result.error && result.code)
			{
				outputStrings[idx] = WORDS.msgError+" ("+result.code+"): " + result.error;
			}
			else if ((translateProvider == "ibm") && result.payload && result.payload.translations)
			{
				var i, out = [];

				var variants = result.payload.translations;
				for (i=0; i<variants.length; i++ )
				{
					if (!i)
					{
						out.push( variants[i].translation );
						if (variants.length > 1)
						{
							out.push("");
							out.push(WORDS.msgAlternatives+":");
						}
					}
					else
						out.push( "- " + variants[i].translation );

					// limit result by 3 alternatives
					if (i > 2)
						break;
				}

				outputStrings[idx] = out.join("\n");
			}
			else if ((translateProvider == "lingvanex") && result.err == null)
			{
				outputStrings[idx] = result["result"];
			}
			else if ((translateProvider == "lingvanex") && result.err != null)
			{
				// error
				outputStrings[idx] = WORDS.msgError+": "+result.err;
			}
			else if ((translateProvider == "deepl") && result.error && result.error.code)
			{
				outputStrings[idx] = WORDS.msgError+" ("+result.error.code+"): " + result.error.message;
				DeepLClientState = null;	// too many connections workaround
			}
			else if ((translateProvider == "deepl") && result.result && result.result.translations)
			{
				// {"id":1,"jsonrpc":"2.0","result":{"source_lang":"DE","source_lang_is_confident":0,"target_lang":"ES","translations":[{"beams":[{"num_symbols":3,"postprocessed_sentence":"cristalera","score":-5000.09,"totalLogProb":-1.15642},{"num_symbols":2,"postprocessed_sentence":"cristal","score":-5000.11,"totalLogProb":-1.00554},{"num_symbols":3,"postprocessed_sentence":"escaparate","score":-5000.15,"totalLogProb":-1.81225},{"num_symbols":4,"postprocessed_sentence":"vitrina","score":-5000.18,"totalLogProb":-2.81411},{"num_symbols":4,"postprocessed_sentence":"ventanilla","score":-5000.2,"totalLogProb":-3.15957},{"num_symbols":3,"postprocessed_sentence":"taquilla","score":-5000.25,"totalLogProb":-3.11903},{"num_symbols":2,"postprocessed_sentence":"ventana","score":-5000.25,"totalLogProb":-2.25736}],"timeAfterPreprocessing":9223372036854776,"timeReceivedFromEndpoint":9223372036854776,"timeSentToEndpoint":9223372036854776,"total_time_endpoint":1}]}}
				var i, out = [];

				if (result.result.target_lang !== document.getElementById("toText")["myLanguage"].toUpperCase())
				{
					out.push(WORDS.msgError+": "+WORDS.msgUnknownTargetLng);
				}
				else
				{
					var variants = result.result.translations[0].beams;
					for (i=0; i<variants.length; i++ )
					{
						if (!i)
						{
							out.push( variants[i].postprocessed_sentence );
							if (variants.length > 1)
							{
								out.push("");
								out.push(WORDS.msgAlternatives+":");
							}
						}
						else if (variants[i].postprocessed_sentence.length > 0)
							out.push( "- " + variants[i].postprocessed_sentence );


						// limit result by 3 alternatives
						if (i > 2)
							break;
					}
				}

				outputStrings[idx] = out.join("\n");
				if (!backTranslating)
					StoreDetectedInputStringsLang( result.result.source_lang );
			}
			else	// undefined cases
				outputStrings[idx] = JSON.stringify(result);
		}
		else	// undefined cases
			outputStrings[idx] = JSON.stringify(result);

		if (onExtract)
			onExtract(outputStrings[idx], detectedInputStringsLang);

		translatedStringsCnt++;
		if (translatedStringsCnt == inputStrings.length)
		{
			// check total result

			// check on expired msAppId => retranslate
			if (!retranslateCountred
				&& (outputStrings[0] !== null)
				&& outputStrings[0]["indexOf"]
				&& ((outputStrings[0].indexOf("The token has expired") > 0) || (outputStrings[0].indexOf("Invalid appId") > 0)))
			{
				if (translateProvider == "bing")
				{
					msAppId = "";
					TranslateText();
					retranslateCountred++;
					return;
				}
				else if (translateProvider == "dictionaries")
				{
					udAppId = "";
					TranslateText();
					retranslateCountred++;
					return;
				}
			}


			if (backTranslating)
			{
				StopTranslatingAnimation();
				if (document.getElementById("chkBackTranslation").checked)
				{
					var toFromText = document.getElementById("toFromText");
					toFromText.value = outputStrings.join("");
//					StoreLastToFromText(toFromText.value);
					OnEventOfFromToText();
					backTranslating = false;
				}
				retranslateCountred = 0;
			}
			else
			{
				if (detectedInputStringsLang
					&& (detectedInputStringsLang == document.getElementById("toLang").value)
					//&& (detectedInputStringsLang != SetupAutoTargetLang(detectedInputStringsLang, document.getElementById("toLang").value)))
					&& (detectedInputStringsLang != GetFixedTargetLang(detectedInputStringsLang, document.getElementById("toLang").value))
					)
				{
					TranslateText();
					return;
				}

				StopTranslatingAnimation();
				if (detectedInputStringsLang)
//					StoreLastFromText(document.getElementById("fromText").value, detectedInputStringsLang);
					OnEventOfFromText(null, null, detectedInputStringsLang);
//				ShowAutodetectedLang(detectedInputStringsLang);

				// filter same errors in output (translate + dictionary)
				var prev_value = "";
				var i = outputStrings.length;
				while (--i >= 0)
				{
					if ((outputStrings[i] == prev_value) && (prev_value.indexOf("error status: ") < 3))
						outputStrings.splice(i, 1);
					else
						prev_value = outputStrings[i];
				}

				var toText = document.getElementById("toText");
				toText.value = outputStrings.join("");
				if (toText.value === "")
					toText.value = document.getElementById("fromText").value;

				toText.value = Trim(toText.value);

//				StoreLastToText(toText.value);
//				RepaintFavoriteButton();
				OnEventOfToText();

				retranslateCountred = 0;

				if (document.getElementById("chkBackTranslation").checked
					&& (translateProvider != "dictionaries")
					&& (translateProvider != "urban")
					)
					BackTranslateResult();
			}
		}
	};
}

function getAjaxTranslateHandler(idx, onExtract)
{
	return function()
	{
		if (this.readyState == 4)
		{
			var result;
			try
			{
				result = JSON.parse(this.responseText);
			}
			catch (e)
			{
				// try to fix error (,, => ,undefined,)
				try
				{
					result = JSON.parse(this.responseText.replace(/([\[\,])(?=[\,\]])/g, "$1null"));
				}
				catch (e)
				{
					result = this.responseText;
				}
			}

			if ((this.status == 200) || (typeof result == "object"))
			{
				getTranslateHandler(idx, translateProvider, onExtract)( result );
			}
			else	// error
			{
				var status_name = "";
				if (result = Trim(result))
				{
					var res;
					if (res = result.match(/<title>([^<]+)<\/title>/))
						status_name = res[1];
					else
						status_name = result;
				}
				getTranslateHandler(idx, onExtract)( "\nerror status: " + this.status + (status_name ? " (" + status_name + ")" : "") );
			}
			xhrs[idx] = null;
		}
	};
}
/*
function SendMessageToBgProcess(msg)
{
	if (opera && opera.extension)
		opera.extension.postMessage(msg);
	else if (backgroundPage)
		backgroundPage.onMessageHandler(msg, window);
}
*/

function DetectTextLanguageAsync(text, callback)
{
	if (chrome.i18n.detectLanguage)
	{
		chrome.i18n.detectLanguage(text, function(res)
			{
				if (res.languages.length)
					callback( res.languages[0].language );
				else
					callback( null );
			});
		return;
	}
	// unsupported language detection
	callback(null);
}

function Text2Strings(text, maxStrLength)
{
	var strings = [];
	if (text.length > maxStrLength)
	{
		var ss, arr = text.split(/([;.,\-\r\n]+)/);
		var i, cnt = arr.length;
		for (i=0; i<cnt; i++)
		{
			ss = arr[i];
			while (i<(cnt-1) && ((ss.length + arr[i+1].length) <= maxStrLength))
				ss += arr[++i];

			if (ss.length <= maxStrLength)
				strings.push( ss );
			else
				strings.push( ss.substr(0, maxStrLength) );
		}
	}
	else
		strings.push( text );

	return strings;
}

function TranslateUrl(url)
{
	var fromLang	= document.getElementById("fromLang").value;
	var toLang		= document.getElementById("toLang").value;

	if (fromLang.indexOf('~') >= 0)
		fromLang = fromLang.split('~', 2)[0];

	SendMessageToBackgroundScript({
									action:		"translate_url",
									prefixUrl:	GetTranslatedPageUrlPrefix(translateProvider, fromLang, toLang),
									url:		url || activePage.url
									});
}

function TranslateByGoogle(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	// split by max possible length
	inputStrings = Text2Strings(text, 350);

	// send each substring
	var i, cnt = inputStrings.length;
	for (i=0; i<cnt; i++)
	{
		if (useGoogleCn)
		{
			// Google in China did not has googleapi domain
			xhrs[i] = SendAjaxRequest("https://translate.google.com.hk/translate_a/t?client=dict-chrome-ex",
										"GET",
										null,
										{
											hl:	EXT_LOCALE,
											sl:	(from_lang ? from_lang : "auto"),
											tl:	to_lang,
											q:	inputStrings[i]
										},
										getAjaxTranslateHandler(i)
										);
		}
		else
		{
			// dt=rm	-- transcription
			xhrs[i] = SendAjaxRequest("https://translate.googleapis.com/translate_a/single?client=gtx&dt=t&dt=bd&dt=rm",
										"GET",
										null,
										{
											hl:	EXT_LOCALE,
											sl:	(from_lang ? from_lang : "auto"),
											tl:	to_lang,
											q:	inputStrings[i]
										},
										getAjaxTranslateHandler(i)
										);
		}
	}
}


// taken from: http://werxltd.com/wp/2010/05/13/javascript-implementation-of-javas-string-hashcode-method/
function StringHashCodeByJava(str)
{
	if (str.length === 0)
		return 0;

	var i, hash = 0;
	for (i=0; i<str.length; i++)
	{
		hash = ((hash << 5) - hash) + str.charCodeAt(i);
		hash = hash & hash; // Convert to 32bit integer
	}
	return hash;
}

var msIG = null;
var msIID = null;
var msIIDcntr = 0;
function TranslateByMicrosoft(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (msAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_msAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	// split by max possible length
		inputStrings = Text2Strings(text, 300);

	// send each substring
		var j, sub_strings;
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
		{
			sub_strings = inputStrings[i].replace(/\\/g, '\\\\').replace(/"/g, '\\"').split(/[\r\n]+/);
/*
			if (msAppId != "-")
			{
				// old method with old engine
				// https://msdn.microsoft.com/en-us/library/ff512385.aspx
				xhrs[i] = SendAjaxRequest("https://api.microsofttranslator.com/v2/ajax.svc/TranslateArray2",
											"GET",
											null,
											{
												appId:	"Bearer " + msAppId,
												ctr:	"",
												loc:	EXT_LOCALE,
												from:	(from_lang ? (msLanguages[from_lang] ? msLanguages[from_lang] : from_lang) : ""),
												to:		(msLanguages[to_lang] ? msLanguages[to_lang] : to_lang),
												texts:	'["' + sub_strings.join('","') + '"]'
											},
											getAjaxTranslateHandler(i)
											);
			}
			else	// based on cookies + keys
			{
*/
				// new: POST:
				//	detect - https://www.bing.com/tdetect?&IG=16E4E722FC144ED7B16AA33D7CE54711&IID=translator.5032.3
				//				text: test
				//	= en
				//	translate - https://www.bing.com/ttranslate?&IG=16E4E722FC144ED7B16AA33D7CE54711&IID=translator.5032.2
				//				text: test
				//				from: en
				//				to: en
				//	= {"statusCode":200,"translationResponse":"тест"}
				//	dict - https://www.bing.com/ttranslationlookup?&IG=16E4E722FC144ED7B16AA33D7CE54711&IID=translator.5032.2
				//				from: en
				//				to: en
				//				text: test
				// = {"normalizedSource":"test","displaySource":"test","translations":[{"normalizedTarget":"тест","displayTarget":"тест","posTag":"NOUN","confidence":0.3813,"prefixWord":"","backTranslations":[{"normalizedText":"test","displayText":"test","numExamples":15,"frequencyCount":10129},{"normalizedText":"tests","displayText":"tests","numExamples":15,"frequencyCount":176},{"normalizedText":"quiz","displayText":"quiz","numExamples":15,"frequencyCount":156}]},{"normalizedTarget":"испытания","displayTarget":"испытания","posTag":"NOUN","confidence":0.1746,"prefixWord":"","backTranslations":[{"normalizedText":"test","displayText":"test","numExamples":15,"frequencyCount":2412},{"normalizedText":"tests","displayText":"tests","numExamples":15,"frequencyCount":1882},{"normalizedText":"testing","displayText":"testing","numExamples":15,"frequencyCount":1699},{"normalizedText":"trials","displayText":"trials","numExamples":15,"frequencyCount":1002},{"normalizedText":"tested","displayText":"tested","numExamples":15,"frequencyCount":459},{"normalizedText":"challenges","displayText":"challenges","numExamples":15,"frequencyCount":244},{"normalizedText":"ordeal","displayText":"ordeal","numExamples":1,"frequencyCount":46}]},{"normalizedTarget":"проверить","displayTarget":"проверить","posTag":"VERB","confidence":0.1478,"prefixWord":"","backTranslations":[{"normalizedText":"check","displayText":"check","numExamples":15,"frequencyCount":17406},{"normalizedText":"test","displayText":"test","numExamples":15,"frequencyCount":3656},{"normalizedText":"verify","displayText":"verify","numExamples":15,"frequencyCount":3101},{"normalizedText":"validate","displayText":"validate","numExamples":15,"frequencyCount":978},{"normalizedText":"examine","displayText":"examine","numExamples":15,"frequencyCount":335},{"normalizedText":"inspect","displayText":"inspect","numExamples":10,"frequencyCount":332}]},{"normalizedTarget":"тестировать","displayTarget":"тестировать","posTag":"VERB","confidence":0.0967,"prefixWord":"","backTranslations":[{"normalizedText":"test","displayText":"test","numExamples":15,"frequencyCount":637}]},{"normalizedTarget":"проверка","displayTarget":"проверка","posTag":"NOUN","confidence":0.0896,"prefixWord":"","backTranslations":[{"normalizedText":"check","displayText":"check","numExamples":15,"frequencyCount":9045},{"normalizedText":"test","displayText":"test","numExamples":15,"frequencyCount":1589},{"normalizedText":"verify","displayText":"verify","numExamples":15,"frequencyCount":1501},{"normalizedText":"validation","displayText":"validation","numExamples":15,"frequencyCount":1252},{"normalizedText":"verification","displayText":"verification","numExamples":15,"frequencyCount":1008},{"normalizedText":"checks","displayText":"checks","numExamples":15,"frequencyCount":730},{"normalizedText":"inspection","displayText":"inspection","numExamples":15,"frequencyCount":467}]},{"normalizedTarget":"испытать","displayTarget":"испытать","posTag":"VERB","confidence":0.0669,"prefixWord":"","backTranslations":[{"normalizedText":"experience","displayText":"experience","numExamples":15,"frequencyCount":1810},{"normalizedText":"test","displayText":"test","numExamples":15,"frequencyCount":731}]},{"normalizedTarget":"экзамен","displayTarget":"экзамен","posTag":"NOUN","confidence":0.0431,"prefixWord":"","backTranslations":[{"normalizedText":"exam","displayText":"exam","numExamples":15,"frequencyCount":1459},{"normalizedText":"test","displayText":"test","numExamples":15,"frequencyCount":355},{"normalizedText":"examination","displayText":"examination","numExamples":15,"frequencyCount":296},{"normalizedText":"exams","displayText":"exams","numExamples":8,"frequencyCount":39}]}]}
				// ID from https://www.bing.com/translator =  IG:"16E4E722FC144ED7B16AA33D7CE54711"
				// ID not required. Can just use cookies! :)
				// content-type: application/x-www-form-urlencoded
				var text = sub_strings.join('","');

				var localTranslateTextFunc = function(from_detected_lang)
				{
					inputStrings = [text, text];	// second for dictionary
					msIIDcntr++;

					// dictionary require concrete language, but auto detect support only by translator => primary is translate
					var funcDictionary = function(trans_result, trans_lang)
					{
						xhrs[1] = SendAjaxRequest("https://www2.bing.com/tlookupv3?isVertical=1&&IG="+msIG+"&IID="+msIID+"."+msIIDcntr,
													"POST",
													null,
													{
														text:	text,
														from:	(msLanguages[trans_lang] ? msLanguages[trans_lang] : trans_lang),
														to:		(msLanguages[to_lang] ? msLanguages[to_lang] : to_lang),

														token:	msAppId,
														key:	msSignId,
														isAuthv2:true
													},
													getAjaxTranslateHandler(1)
													);
					};
					// Microsoft support dictionary only for single word
					if (text.split(" ").length > 1)
					{
						funcDictionary = undefined;
						inputStrings.pop();
					}

					xhrs[0] = SendAjaxRequest("https://www2.bing.com/ttranslatev3?isVertical=1&&IG="+msIG+"&IID="+msIID+"."+msIIDcntr,
												"POST",
												null,
												{
													text:		text,
													fromLang:	(from_detected_lang ? (msLanguages[from_detected_lang] ? msLanguages[from_detected_lang] : from_detected_lang) : "auto-detect"),
													to:			(msLanguages[to_lang] ? msLanguages[to_lang] : to_lang),

													token:		msAppId,
													key:		msSignId,
													isAuthv2:	true
												},
												getAjaxTranslateHandler(0, funcDictionary)
												);
				};

				if (msIG)
					localTranslateTextFunc(from_lang);
				else
				{
					// Get request Ids
					var xhr = SendAjaxRequest("https://www2.bing.com/translator",
												"GET",
												null,
												null,

												function()
												{
													if (this.readyState == 4)
													{
														if (this.status == 200)
														{
															var pageRaw = this.responseText.match(/\",\s*IG:"([^"]+)\",/);

															var re = new RegExp("\",\s*IG:\"([^\"]+)\",");
															if (re.test(this.responseText))
																msIG = RegExp.$1;

															re = new RegExp("data-iid=\"(translator\.[0-9]+)\"");
															if (re.test(this.responseText))
																msIID = RegExp.$1;

															if (msIG && msIID)
																localTranslateTextFunc(from_lang);
															else
															{
																inputStrings = [text];
																getTranslateHandler(0)( "error status: Request ID is undefined." );
															}
														}
														else	// error
														{
															inputStrings = [text];
															getTranslateHandler(0)( "error status: " + this.status );
														}
														xhr = null;
													}
												}
												);
				}
/*
			}
*/
		}

	//
	// http://api.microsofttranslator.com/V2/Http.svc/Detect?text= . urlencode($inputStr)
	// http://api.microsofttranslator.com/V2/Ajax.svc/Speak?appId=Bearer%20[sessId]&text=&language=&format=audio/wav

	// https://www.bing.com/translator
	// https://www.bing.com/translator/api/Translate/TranslateArray?from=-&to=de
	// https://www.bing.com/translator/api/Dictionary/Lookup?from=en&to=de&text=test
	// https://www.bing.com/translator/api/language/Speak?locale=de-DE&gender=male&media=audio/mp3&text=Test
	// https://www.bing.com/translator/api/Language/GetSpeechDialectsForLocale?locale=en
}

var YandexTranslateCounter = 0;
function TranslateByYandex(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (yaAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_yaAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	var localTranslateTextFunc = function(from_detected_lang)
	{
		inputStrings = [text, text];	// second for dictionary
/*
		xhrs[0] = SendAjaxRequest("https://translate.yandex.net/api/v1/tr.json/translate?srv=yawidget",
									"GET",
									null,
									{
										lang:	from_detected_lang+"-"+to_lang,
										text:	text
									},
									getAjaxTranslateHandler(0)
									);
*/
		// this method return better result (like on the site) [sample text: A Smidgen of OpenGL Code]
		xhrs[0] = SendAjaxRequest("https://translate.yandex.net/api/v1/tr.json/translate?id="+yaSignId+"-"+(YandexTranslateCounter++)+"-0&srv=tr-text&lang="+from_detected_lang+"-"+to_lang,
									"POST",
									{
										"Content-Type":		"application/x-www-form-urlencoded; charset=UTF-8",
									},
									{
										text:		text,
										options:	"1"
									},
									getAjaxTranslateHandler(0)
									);

		// Dictionary
		// TODO: may be only for 1 word?
		xhrs[1] = SendAjaxRequest("https://dictionary.yandex.net/dicservice.json/lookup",
									"GET",
									null,
									{
										ui:		(EXT_LOCALE=="ru" ? "ru" : "en"),
										sid:	yaAppId,
										lang:	from_detected_lang+"-"+to_lang,
										text:	text
									},
									getAjaxTranslateHandler(1)
									);
	};

	if (from_lang)
		localTranslateTextFunc(from_lang);
	else
	{
		// Detect source language
//		xhr.open( "GET", "http://translate.yandex.net/api/v1/tr.json/detect?srv=tr-text&text=" + encodeURIComponent(text), true );
//		xhr.open( "GET", "https://translate.yandex.net/api/v1.5/tr.json/detect?text=" + encodeURIComponent(text), true );	// TODO: need the key
		var xhr = SendAjaxRequest("https://translate.yandex.net/api/v1/tr.json/detect?srv=yawidget",
									"GET",
									null,
									{ text:	text },

									function()
									{
										if (this.readyState == 4)
										{
											if (this.status == 200)
											{
												var res = JSON.parse(this.responseText);
												if (res.lang)
												{
													StoreDetectedInputStringsLang( res.lang );
													localTranslateTextFunc( res.lang );
												}
												else
												{
													inputStrings = [text];
													getTranslateHandler(0)( "error: can't detect language" );
												}
											}
											else	// error
											{
												inputStrings = [text];
												getTranslateHandler(0)( "error status: " + this.status );
											}
											xhr = null;
										}
									}
									);
	}

}

function TranslateByPromt(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (ptAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_ptAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	inputStrings = [text];

	var post_vars = {};
	post_vars.eventName = "TranslateButton";
	post_vars.text = text;
	post_vars.dirCode = "";
//	post_vars.topic = "General";
	post_vars.useAutoDetect = true;

	// setup langs
	if (!from_lang)
		post_vars.dirCode += "au";	// any
	else
		post_vars.dirCode += from_lang;
	post_vars.dirCode += "-"+to_lang;

	var local_isNullOrEmpty = function(t) { return !t || t.length == 0; }
	var local_text_hash = function(text, from, till, hash)
	{
		var len, idx;
		if (arguments.length == 1)
			return len = local_isNullOrEmpty(text) ? 0 : text.length,
				len > 100 ? local_text_hash(text, len - 50, len, local_text_hash(text, 0, 50, len)) : local_text_hash(text, 0, len, len);
		for (idx = from; idx < till; idx++)
			hash = (hash << 5) - hash + text.charCodeAt(idx),
			hash &= hash;
        while (till > from) {
            hash = ((hash << 5) - hash) + text.charCodeAt(--till);
            hash &= hash;
        }
		return hash;
	};

	var local_post_hash = function(post_vars)
	{
		var str = post_vars.eventName + "#" + post_vars.dirCode + "#" + "General" + "#" + local_text_hash(post_vars.text).toString();
        return local_text_hash(str);
	};
	post_vars.h = local_post_hash(post_vars);

	post_vars.aft = ptSignId;
	post_vars.pageIx = 0;
	post_vars.v = 2;

	xhrs[0] = SendAjaxRequest("https://www.online-translator.com/api/getTranslation",
								"POST",
								{
									"Content-Type":		"application/x-www-form-urlencoded; charset=UTF-8",
									"Accept":			"*/*",
									"X-Requested-With":	"XMLHttpRequest",
									"XSRF-TOKEN":		ptAppId,
								},
								post_vars,

								getAjaxTranslateHandler(0)
							);
}

var used_pragma_fixer = 0;
function TranslateByPragma(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
 	StartTranslatingAnimation();

	// split by max possible length
		inputStrings = Text2Strings(text, 350);

	// send each substring
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
		{
			var translate_obj_str = JSON.stringify( {src:(from_lang?from_lang:0), trg:to_lang, dat:[ inputStrings[i].replace(/[\\"]/g, '\\$&').replace(/\u0000/g, '\\0').replace("\n", '","') ], dom:null} );
			// currently support only HTTP
			xhrs[i] = SendAjaxRequest("http://addon.translate.ua/mozilla/tran_ajax.php?func=translate",
										"GET",
										null,
										{ data: "{" + translate_obj_str.substr(1, translate_obj_str.length-2) + "}" },

										getAjaxTranslateHandler(i)
										);
		}
}

function TranslateByBaidu(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (bdAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_bdAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	var funcTranslatePhrase = function(from_lang)
	{
		// get part of string
		inputStrings = Text2Strings(text, 350);

		// send each substring
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
		{
			// new
			window.gtk = bdSignId;	// require for correct sign request
			xhrs[i] = SendAjaxRequest("https://fanyi.baidu.com/v2transapi",
										"POST",
										{"My-Referer": "https://fanyi.baidu.com/"},
										{
											from:				(!from_lang ? "auto" : (baiduLanguagesTo[from_lang] ? baiduLanguagesTo[from_lang] : from_lang)),
											to:					(baiduLanguagesTo[to_lang] ? baiduLanguagesTo[to_lang] : to_lang),
											query:				inputStrings[i],
											transtype:			"translang",
											simple_means_flag:	3,
											sign:				GetSignOfBaiduQuery(inputStrings[i]),
											token:				bdAppId,
											domain:				"common"
										},

										getAjaxTranslateHandler(i)
										);
		}
	}

	if (!from_lang)
		DetectTextLanguageAsync(text, funcTranslatePhrase);
	else
		funcTranslatePhrase(from_lang);
}

function TranslateBySogou(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (sgAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_sgAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	// get part of string
		inputStrings = Text2Strings(text, 350);

	// POST: https://fanyi.sogou.com/reventondc/translateV2
	// Content-Type: application/x-www-form-urlencoded; charset=UTF-8
	// X-Requested-With: XMLHttpRequest

	// from: auto
	// to: ru
	// text: page
	// client: pc
	// fr: browser_pc
	// pid: sogou-dict-vr
	// dict: true
	// word_group: true
	// second_query: true
	// uuid: 7a8ace45-b177-4f75-a88b-672509c14f2c
	// needQc: 1
	// s: 4217416e9d08b15ed1e0a41d2a0fbda8  // s(""+from+to+text+window.seccode)

	if (!from_lang)
		from_lang = "auto";
	else if (sogouLanguagesTo[from_lang])
		from_lang = sogouLanguagesTo[from_lang];

	if (sogouLanguagesTo[to_lang])
		to_lang = sogouLanguagesTo[to_lang];

	// send each substring
	var i, cnt = inputStrings.length;
	for (i=0; i<cnt; i++)
	{
		// new
		window.seccode = sgAppId;	// require for correct sign request
		data = {
				from:				from_lang,
				to:					to_lang,
				text:				inputStrings[i],
				client:				"pc",
				fr:					"browser_pc",
//				pid:				"sogou-dict-vr",
//				dict:				"true",
//				word_group:			"true",
//				second_query:		true,
				needQc:				1,
				s:					GetSogouQuerySign(""+from_lang+to_lang+text+window.seccode),
				uuid:				GetSogouQueryUuid(),
				exchange:			false
			};
		xhrs[i] = SendAjaxRequest("https://fanyi.sogou.com/api/transpc/text/result",
									"POST",
									{
										"Content-Type":		"application/json; charset=UTF-8",
										"My-Referer":		"https://fanyi.sogou.com/"
									},
									JSON.stringify(data),

									getAjaxTranslateHandler(i)
									);
	}

	// Try: MEL.getUUID()
}

function TranslateByNaver(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (nvAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_nvAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	var funcTranslatePhrase = function(from_lang)
	{
		to_lang = GetFixedTargetLang(from_lang, to_lang);

		// get part of string
		inputStrings = Text2Strings(text, 350);

		widget.cfg.getAsync("translator_uuid", function(cfg)
		{
		// send each substring
			var i, cnt = inputStrings.length;
			for (i=0; i<cnt; i++)
			{
				// deviceId=9c0c6355-8533-4c36-bf0f-b29d84694787&
				// locale=en&
				// dict=true&
				// dictDisplay=30&
				// honorific=false&
				// instant=false&
				// paging=false&
				// source=en&
				// target=ru&
				// text=page&
				// authroization=PPG%209c0c6355-8533-4c36-bf0f-b29d84694787%3Az5E5nkMDMuRpBTEvePaVmA%3D%3D&
				// timestamp=1618229376085

				// a = "https://papago.naver.com/apis/langs/dect"
				// T = Object(N.a)()		// random uuid
				// n = "string" == typeof a ? a : a.url		// "https://papago.naver.com/apis/langs/dect"
				// c = (new Date).getTime() + A
				// auth = "PPG " + T + ":" + p.a.HmacMD5(T + "\n" + n.split("?")[0] + "\n" + c, "v1.5.6_97f6918302").toString(p.a.enc.Base64);

				var ts = (new Date())-3673;	// TODO: find number source
				var rnd = MEL.getUUID();
				var request_url = "https://papago.naver.com/apis/n2mt/translate";
//				var request_url = "https://papago.naver.com/apis/nsmt/translate";	// Try?
				var auth_key = nvAppId;
				var auth_el = [
						"PPG ",
						rnd,
						":",
						CryptoJS.HmacMD5(rnd+"\n"+request_url+"\n"+ts, auth_key).toString( CryptoJS.enc.Base64 )
					];
				var auth = auth_el.join("");

				var data = {
						deviceId:	rnd,
						locale:		EXT_LOCALE,
						dict:		true,
						dictDisplay:10,
						honorific:	false,
						instant:	false,
						paging:		false,
						source:		from_lang,
						target:		to_lang,
						text:		inputStrings[i],
//						authroization: auth,
//						timestamp:	ts
						};
//				var data = GetEncodedNaverQuery( JSON.stringify(query) );
				xhrs[i] = SendAjaxRequest(request_url,
											"POST",
											{
												"authorization": auth,
												"device-type": "pc",
												"timestamp": ts,
												"x-apigw-partnerid": "papago"
											},
											data,

											getAjaxTranslateHandler(i)
											);
			}
		});
	};

	// language detection
	// https://papago.naver.com/apis/langs/dect
	// POST
		// {query:"zzz"}
		// postfix: ud.

	if (!from_lang)
		DetectTextLanguageAsync(text, funcTranslatePhrase);
	else
		funcTranslatePhrase(from_lang);
}

function TranslateBySystran(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();


	// get part of string
		inputStrings = Text2Strings(text, 350);

		if (!retranslateCountred && (stAppId === ""))
		{
			SendMessageToBackgroundScript({ action:"get_stAppId", refresh:true, next_action:"retranslate" });
			retranslateCountred++;
			return;
		}

		var funcSendSignedRequestAsync = function(i)
		{
/*
			// Translation Alternatives?
			var post_vars = {
						profileId: "",
						input: text,
						source: from_lang,
						target: to_lang,
					};
*/
			// Translation Box?
			var post_vars = {
						profileId: null,
						owner: "Systran",
						domain: null,
						size: "M",
						input: text,
						source: from_lang ? from_lang : null,
						target: to_lang,
					};
			var token = stAppId;
			var signature_source = [token, from_lang ? from_lang : "null", to_lang, text].join(".");
			crypto.subtle.digest("SHA-256", (new TextEncoder()).encode(signature_source)).then(
				function(binary_signature)
				{
					var signature = Array.from(new Uint8Array(binary_signature)).map( function(b) { return b.toString(16).padStart(2, '0'); } ).join('');
					xhrs[i] = SendAjaxRequest("https://translate.systran.net/translate/html",
												"POST",
												{
													"Content-Type":			"application/json; charset=utf-8",
													"Accept":				"application/json, text/javascript, */*; q=0.01",
													"X-Requested-With":		"XMLHttpRequest",
													"X-CSRF-Token":			token,
													"x-translation-signature": signature,
													"x-user-agent":			"Translation Box"
												},
												JSON.stringify(post_vars),

												getAjaxTranslateHandler(i)
												);
				}
			);
		};

		// send each substring
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
			funcSendSignedRequestAsync(i);		// separate function to keep `i` in async encryption
}

function TranslateByReverso()
{
	// TODO

	// translate
	// https://async5.reverso.net/WebReferences/WSAJAXInterface.asmx/TranslateCorrWS
	// POST
	// {'searchText': 'men tall', 'direction': 'eng-rus-5', 'maxTranslationChars':'800', 'usecorr':'true'}

	// samples
	// https://context.reverso.net/bst-query-service?source_text=window&source_lang=en&target_lang=ru&npage=1&json=1&nrows=20&callback=jQuery112404984954657081111_1580488397167&_=1580488397169

}

function TranslateByBabylon(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	// get part of string
		inputStrings = Text2Strings(text, 350);

	// send each substring
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
		{
			// TODO: http://translation.babylon.com/mobile/
			// POST: from=ID&to=ID&ortxt=window&translateit=Translate

			// or: http://translation.babylon.com/
			// ( http://translation.babylon.com/translate/babylon.php?v=1.0&q=open%20window&langpair=0%7C7&callback=babylonTranslator.callback&context=babylon.0.7._babylon_api_response )
			// babylonLanguages[]
			// currently support only HTTP
			xhrs[i] = SendAjaxRequest("https://translation.babylon-software.com/translate/babylon.php?v=1.0",
										"GET",
										null,
										{
											q:			inputStrings[i],
											langpair:	(babylonLanguages[from_lang] ? babylonLanguages[from_lang] : 0)+"|"+(babylonLanguages[to_lang]),
											callback:	"babylonTranslator.callback",
											context:	"babylon.0.7._babylon_api_response"
										},

										getAjaxTranslateHandler(i)
										);
		}
}


function TranslateByBabylonDictionary(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	// alternative
	// http://info.babylon.com/onlinebox.cgi?rt=ol&tid=pop&cid=CD1&tl=en&term=window

	PrepareResultFieldForHtml();
	document.getElementById("result_as_html").getElementsByTagName("IFRAME")[0].src = "http://info.babylon.com/cgi-bin/bis.fcgi?"
																					+ "rt=ol&tid=BabylonSearch&mr=99&sl="+from_lang+"&tl="+to_lang+"&term="+encodeURIComponent(text);
}

function TranslateByLingvanex(from_lang, to_lang, text)
{
/*
key: https://lingvanex.com/lingvanex_demo_page/js/api-base.js
,B2B_AUTH_TOKEN=""

https://api-b2b.backenster.com/b1/api/v3/translate POST
from=en_GB&to=ru_RU&text=test&platform=dp&is_return_text_split_ranges=false

authorization: Bearer a_25rccaCYcBC9ARqMODx2BV2M0wNZgDCEl3jryYSgYZtF1a702PVi4sxqi2AmZWyCcw4x209VXnCYwesx
content-length: 74
content-type: application/x-www-form-urlencoded; charset=UTF-8

{
  "err": null,
  "result": "тест",
}
*/

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (lnAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_lnAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	// get part of string
		inputStrings = [text.substr(0, 300)];

	var funcTranslatePhrase = function(from_lang)
	{
		xhrs[0] = SendAjaxRequest("https://api-b2b.backenster.com/b1/api/v3/translate",
									"POST",
									{
										"Authorization":	lnAppId,
										"Content-Type":		"application/x-www-form-urlencoded; charset=UTF-8",
									},
									{
										"from":		from_lang ? lingvanexLanguages[from_lang] : "null",
										"to":		lingvanexLanguages[to_lang],
										"text":		inputStrings[0],
										"platform": "dp",
										"is_return_text_split_ranges": false
									},
									getAjaxTranslateHandler(0)
									);
	};

	if (!from_lang)
		DetectTextLanguageAsync(text, funcTranslatePhrase);
	else
		funcTranslatePhrase(from_lang)
}

function TranslateByLingvoDictionary(from_lang, to_lang, text)
{
	// http://lingvolive.com/translate/en-ru/window
	// lingvoLanguages[]

	// http://lingvolive.com/api/Translation/WordListPart/?dstLang=1049&pageSize=10&prefix=window&srcLang=1033&startIndex=0
	/// http://lingvolive.com/api/Translation/Translate/?dstLang=1049&srcLang=1033&text=window
	// http://lingvolive.com/api/Translation/Phrases/?dstLang=1049&srcLang=1033&text=window
	/// http://lingvolive.com/api/Translation/Examples/?dstLang=1049&srcLang=1033&text=window
	/// http://lingvolive.com/api/social/feed/summary?dstLang=1049&srcLang=1033&text=window
	//// http://lingvolive.com/api/Translation/InflectedForms/?lang=1033&text=window

	// lingvo live Chrome extension
	// https://lingvolive.ru/api/Translation/Minicard?text=ray&srclang=1033&dstlang=1049&returnXml=false


	PrepareResultFieldForText();
	StartTranslatingAnimation();

	var funcTranslatePhrase = function(from_lang)
	{
		to_lang = GetFixedTargetLang(from_lang, to_lang);

		inputStrings = [text, text];	// for different sources
/*
		// Translate
		// Require LL-GA-ClientId header
		xhrs[0] = SendAjaxRequest("https://api.lingvolive.com/Translation/Translate/?returnJsonArticles=true",
									"GET",
									null,
									{
										srcLang:	lingvoLanguages[from_lang],
										dstLang:	lingvoLanguages[to_lang],
										text:		text,
									},
									getAjaxTranslateHandler(0)
									);
*/
		// WordListPart
		xhrs[0] = SendAjaxRequest("https://api.lingvolive.com/Translation/WordListPart/?pageSize=10",
									"GET",
									{"My-Referer": "Referer: https://www.lingvolive.com/en-us"},
									{
										srcLang:	lingvoLanguages[from_lang],
										dstLang:	lingvoLanguages[to_lang],
										prefix:		text,
										startIndex:	0
									},
									getAjaxTranslateHandler(0)
									);

		// Phrases
		xhrs[1] = SendAjaxRequest("https://api.lingvolive.com/Translation/Phrases/?pageSize=10",
									"GET",
									{"My-Referer": "https://www.lingvolive.com/en-us"},
									{
										srcLang:	lingvoLanguages[from_lang],
										dstLang:	lingvoLanguages[to_lang],
										text:		text,
									},
									getAjaxTranslateHandler(1)
									);
	};

	if (!from_lang)
		DetectTextLanguageAsync(text, funcTranslatePhrase);
	else
		funcTranslatePhrase(from_lang);
}

function TranslateByUrbanDictionary(from_lang, to_lang, text)
{
	PrepareResultFieldForText();
	StartTranslatingAnimation();

	if (!retranslateCountred && (udAppId === ""))
	{
		SendMessageToBackgroundScript({ action:"get_udAppId", refresh:true, next_action:"retranslate" });
		retranslateCountred++;
		return;
	}

	// sample: http://m.urbandictionary.com/#define?term=alaskan%20firedragon

	// get part of string
		inputStrings = [text.substr(0, 300)];

	// send each substring
		xhrs[0] = SendAjaxRequest("https://api.urbandictionary.com/v0/define",
									"GET",
									{"My-Referer": "https://www.urbandictionary.com/"},
									{
										key:	udAppId,
										term:	inputStrings[0],
										".":	Math.random(),
									},
									getAjaxTranslateHandler(0)
									);
}

var DeepLClientState = null;
var DeepLRequestId = 1e4 * Math.round(1e4 * Math.random());
function TranslateByDeepl(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	// get part of string
		inputStrings = Text2Strings(text, 350);

	// "{"jsonrpc":"2.0","method":"getClientState","params":{"v":"20180814"},"id":35930001}"
	// v = https://www.deepl.com/ -> window.dl_pageState ... 20180814
	// answer: {"jsonrpc":"2.0","result":{"ip":"xxx.xxx.xxx.xxx","proAvailable":true,"updateNecessary":false,"ep":false},"id":35930001}

	// TODO: add support of DeppL Pro

	var translateSubstrings = function()
	{
	// send each substring
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
		{
			xhrs[i] = SendAjaxRequest("https://www2.deepl.com/jsonrpc",
										"POST",
										{
											"Content-Type":	"application/json",
											"My-Referer": "https://www.deepl.com/translator",
										},
										JSON.stringify({
											jsonrpc: "2.0",
											method: "LMT_handle_jobs",
											params: {
												commonJobParams: {},
												jobs: [
													{
														kind: "default",
														preferred_num_beams: 4,
														raw_en_sentence: inputStrings[i],
														raw_en_context_before: [],
														raw_en_context_after: []
													}
												],
												lang: {
													user_preferred_langs:	["EN"],
													source_lang_computed:	(!from_lang ? "auto" : from_lang.toUpperCase()),
													target_lang:			to_lang.toUpperCase()
												},
												priority: 1,
												timestamp: (new Date())-0
											},
											id: ++DeepLRequestId
										}).replace(/(,[^:]+):"/, '$1: "'),		// looks like special space? ;)

										getAjaxTranslateHandler(i)
										);
		}
	};

	if (!DeepLClientState)
	{
		// new: https://w.deepl.com/web?request_type=jsonrpc&il=EN&method=getClientState
		// {"jsonrpc":"2.0","id":51270002,"result":{"proAvailable":true,"updateNecessary":false,"ep":null,"loginState":null,"notifications":[]}}
		SendAjaxRequest("https://www.deepl.com/PHP/backend/clientState.php?request_type=jsonrpc&il=EN",
						"POST",
						{
							"Content-Type":	"text/plain"
						},
						JSON.stringify({
							"jsonrpc":	"2.0",
							"method":	"getClientState",
							"params" :	{"v":"20180814"},
							"id":		++DeepLRequestId
						}),
						function (answer)
						{
							if (this.readyState == 4)
							{
								try
								{
									DeepLClientState = JSON.parse(this.responseText);
								}
								catch (e)
								{
									document.getElementById("toText").value = this.responseText;
									return;
								}

								if ((this.status == 200) || (DeepLClientState && (typeof DeepLClientState == "object")))
								{
									console.log("DeepL client state: ", DeepLClientState);
									window.setTimeout(function(){ translateSubstrings(); }, 1200);	// emulate user enter text after page loaded
								}
								else	// error
								{
									document.getElementById("toText").value = this.responseText;
								}
							}
						}
		);
	}
	else
		translateSubstrings();
}

function TranslateByIbm(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	var funcTranslatePhrase = function(from_lang)
	{
		to_lang = GetFixedTargetLang(from_lang, to_lang);

		// get part of string
		inputStrings = Text2Strings(text, 350);

		// send each substring
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
		{
			var data = {
						source: from_lang,
						target: to_lang,
						text: text
						};

			xhrs[0] = SendAjaxRequest("https://www.ibm.com/demos/live/watson-language-translator/api/translate/text",
										"POST",
										{
											"Content-Type":	"application/json; charset=utf-8",
										},
										JSON.stringify(data),

										getAjaxTranslateHandler(i)
										);
		}
	};

	// language detection
	// https://language-translator-demo.ng.bluemix.net/api/identify
	// POST
	// text: test
	// {"languages":[{"language":"en","confidence":0.3273613380479986},{"language":"et","confidence":0.10606852481236596},{"language":"nl","confidence":0.04137640703113574},{"language":"fr","confidence":0.0383073438962988},{"language":"pl","confidence":0.03782011148376784},{"language":"de","confidence":0.030154183954587438},{"language":"nn","confidence":0.02856766144346385},{"language":"nb","confidence":0.02757228316498648},{"language":"is","confidence":0.02415190912891871},{"language":"da","confidence":0.0232386697778215},{"language":"tr","confidence":0.02069227440376374},{"language":"sv","confidence":0.01984246102278171},{"language":"ro","confidence":0.01939667854936238},{"language":"ku","confidence":0.0181538395081438},{"language":"ca","confidence":0.017597238159082044},{"language":"hu","confidence":0.01754106935214107},{"language":"it","confidence":0.01684964625117059},{"language":"sq","confidence":0.015448235418926847},{"language":"sl","confidence":0.012155140880486645},{"language":"mt","confidence":0.01162785302742342},{"language":"cs","confidence":0.011472615423267548},{"language":"pt","confidence":0.00925823102052691},{"language":"fi","confidence":0.00893965203731387},{"language":"sk","confidence":0.008647537164213112},{"language":"ms","confidence":0.008394087331771218},{"language":"lv","confidence":0.008115580757302275},{"language":"vi","confidence":0.00767724258706374},{"language":"es","confidence":0.00734780103520576},{"language":"hr","confidence":0.00727932348621096},{"language":"af","confidence":0.007037851170831333},{"language":"ht","confidence":0.006490902688332528},{"language":"lt","confidence":0.005065766422503396},{"language":"az","confidence":0.004571774772802482},{"language":"eo","confidence":0.004390908999015133},{"language":"zh","confidence":0.0030875355546076478},{"language":"zh-TW","confidence":0.0029724018462557746},{"language":"ga","confidence":0.00263036661950788},{"language":"eu","confidence":0.0025764770891263935},{"language":"ja","confidence":0.002538839356857049},{"language":"ko","confidence":0.0017203323528553198},{"language":"hi","confidence":0.0016857136467431128},{"language":"th","confidence":0.00159986111301451},{"language":"el","confidence":0.0013412931061318836},{"language":"ar","confidence":0.0012424135377688567},{"language":"so","confidence":0.0011504725815468804},{"language":"ur","confidence":0.0010833841892016133},{"language":"ru","confidence":0.0010639441107319922},{"language":"mn","confidence":0.0010318140902811864},{"language":"sr","confidence":0.00100760747169631},{"language":"km","confidence":0.0009375907668197672},{"language":"te","confidence":0.0009027785454927133},{"language":"pa","confidence":0.0008937068447851135},{"language":"ka","confidence":0.0008768816686466172},{"language":"bg","confidence":0.0008589590870027212},{"language":"ta","confidence":0.0008125820813294391},{"language":"bn","confidence":0.0008056048313296785},{"language":"he","confidence":0.000792801327110299},{"language":"ba","confidence":0.0007768174700084235},{"language":"ml","confidence":0.0007313108380199697},{"language":"hy","confidence":0.0007288797499762153},{"language":"ps","confidence":0.0007189348049535296},{"language":"fa","confidence":0.0007171069787831546},{"language":"be","confidence":0.0007134105453458546},{"language":"ky","confidence":0.0007071859977857813},{"language":"kk","confidence":0.0007055841390610968},{"language":"uk","confidence":0.0006811312638703285},{"language":"gu","confidence":0.0006492247161304733},{"language":"cv","confidence":0.0006449074662343366}]}

	if (!from_lang)
		DetectTextLanguageAsync(text, funcTranslatePhrase);
	else
		funcTranslatePhrase(from_lang);
}

function TranslateByApertium(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	var funcTranslatePhrase = function(from_lang)
	{
		to_lang = GetFixedTargetLang(from_lang, to_lang);

		// get part of string
		inputStrings = Text2Strings(text, 350);

		// send each substring
		var i, cnt = inputStrings.length;
		for (i=0; i<cnt; i++)
		{
			var dt_param = "_"+((new Date())-0);
			xhrs[0] = SendAjaxRequest("https://www.apertium.org/apy/translate?"+dt_param+"=",
										"GET",
										null,
										{
											q: text,
											markUnknown: "no",
											langpair: apertiumLanguagesTo[from_lang]+"|"+apertiumLanguagesTo[to_lang],
											callback: "_jqjsp"
										},

										getAjaxTranslateHandler(i)
										);
		}
	};

	// language detection
	// https://www.apertium.org/apy/identifyLang?q=page%20number%202&callback=_jqjsp&_1578588746449=
	// GET
		// _jqjsp({"nob": 100})

	if (!from_lang)
		DetectTextLanguageAsync(text, funcTranslatePhrase);
	else
		funcTranslatePhrase(from_lang);
}

function TranslateByGlosbe(from_lang, to_lang, text)
{
	if (text.match(/^https?:\/\/[^\r\n]+$/))
	{
		TranslateUrl(text);
		return;
	}

	PrepareResultFieldForText();
	StartTranslatingAnimation();

	var funcTranslatePhrase = function(from_lang)
	{
		to_lang = GetFixedTargetLang(from_lang, to_lang);

		// get part of string
//		inputStrings = Text2Strings(text, 350);
		inputStrings = [text.substr(0, 100)];

		// send each substring
//		var i, cnt = inputStrings.length;
//		for (i=0; i<cnt; i++)
		{
/*
			// deprecated
			xhrs[0] = SendAjaxRequest("https://glosbe.com/gapi/translate",
										"GET",
										{
											"Content-Type":		"application/json; charset=UTF-8"
										},
										{
											from: from_lang,
											dest: to_lang,
											phrase: text,
											format: "json",
										},

										getAjaxTranslateHandler(0)
										);

			xhrs[1] = SendAjaxRequest("https://glosbe.com/gapi/tm",		// samples
										"GET",
										{
											"Content-Type":		"application/json; charset=UTF-8"
										},
										{
											from: from_lang,
											dest: to_lang,
											phrase: text,
											format: "json",
											page: 1,
											pageSize: 10,
										},

										getAjaxTranslateHandler(1)
										);
*/
			xhrs[0] = SendAjaxRequest("https://translator-api.glosbe.com/translateByLangDetect?sourceLang="+from_lang+"&targetLang="+to_lang,
										"POST",
										{
											"Content-Type": "text/plain",
										},
										text,
										getAjaxTranslateHandler(0)
										)
		}
	};

	if (!from_lang)
		DetectTextLanguageAsync(text, funcTranslatePhrase);
	else
		funcTranslatePhrase(from_lang);
}

function SetupAutoTargetLang(from_lang, to_lang)
{
	if ((defTargetLang !== "") && (from_lang != defTargetLang))
		to_lang = defTargetLang;
	else if ((defTargetLang2 !== "") && (from_lang != defTargetLang2))
		to_lang = defTargetLang2;
	else
	{
		var mostUsed, mostUsedFrom, mostUsedTo;

		// search in concrete pairs
		for (var k in mostUsedLangPairs)
		{
			var pair = mostUsedLangPairs[k];
			mostUsed = pair.split('~', 2);
			if ((mostUsed[0] !== "") && (mostUsed[1] !== "") && (mostUsed[0] != mostUsed[1]))
			{
				if (!mostUsedTo && mostUsed[0] == from_lang)
					mostUsedTo = mostUsed[1];
				if (!mostUsedFrom && mostUsed[1] == from_lang)
					mostUsedFrom = mostUsed[0];
			}
		}

		if (!mostUsedTo)
		{
			// search in autodetect pairs
			for (var k in mostUsedLangPairs)
			{
				var pair = mostUsedLangPairs[k];
				mostUsed = pair.split('~', 2);
				if ((mostUsed[0] === "") && (mostUsed[1] !== ""))
				{
					if (mostUsed[1] != from_lang)
					{
						mostUsedTo = mostUsed[1];
						break;
					}
				}
			}
		}

		if (mostUsedTo)
			to_lang = mostUsedTo;
		else if (mostUsedFrom)
			to_lang = mostUsedFrom;
	}

	if ((from_lang == to_lang) && (from_lang != "en"))
		to_lang = "en";

	var toLang = document.getElementById("toLang");
	if (toLang.value != to_lang)
	{
		toLang.value = to_lang;
		ChangeToLang( toLang );
	}

	return to_lang;
}

function GetFixedTargetLang(from_lang, to_lang)
{
	if (!to_lang || from_lang == to_lang)
		to_lang = SetupAutoTargetLang(from_lang, to_lang);
	return to_lang;
}

function TranslateText(is_selection)
{
	// clear from last transaction
		var i, cnt;
		if (cnt = xhrs.length)
		{
			for (i=0; i<cnt; i++)
				if (xhrs[i])
				{
					xhrs[i].abort();
					xhrs[i] = null;
				}
			xhrs = [];
		}

		cnt = scripts.length;
		for (i=0; i<cnt; i++)
			if (scripts[i])
				scripts[i].parentNode.removeChild(scripts[i]);
		backTranslating = false;
		StopTranslatingAnimation();

		scripts = [];
		inputStrings = [];
		outputStrings = [];
		translatedStringsCnt = 0;
		StoreDetectedInputStringsLang("");

		SetupBackTranslationVisibility();

	// create new transaction
//		StoreLastToText("");
//		StoreLastToFromText("");
		var toTextEl = document.getElementById("toText");
		var from_lang = document.getElementById("fromLang").value,
			to_lang = document.getElementById("toLang").value,
			text = document.getElementById("fromText").value;

		if (from_lang.indexOf('~') >= 0)
			from_lang = from_lang.split('~', 2)[0];

//		StoreLastFromText(text, from_lang);
//		OnEventOfFromText();

		//if (from_lang == to_lang)	// langs are equal => detect target language
		//	to_lang = SetupAutoTargetLang(from_lang, to_lang);
		if (is_selection && ((defTargetLang !== "") || (defTargetLang2 !== "")))
			to_lang = null;
		to_lang = GetFixedTargetLang(from_lang, to_lang);

		// do not translate if source test is phone, email or password
		var delim_chars;
		if ((from_lang == to_lang)
			|| Trim(text).match(/^\+?[0-9\s\-\/\(\)]+$/)		// phone
			|| Trim(text).match(/^[^@\d ]+@[a-z0-9\.-]+$/i)		// email
			|| (Trim(text).match(/^[0-9a-z\~\!\@\#\$\%\^\&\*\(\)\[\]\_\+\-\,\.\/\\\;\'\:\"]+$/i)			// passwords
				&& (delim_chars = Trim(text).match(/([\~\!\@\#\$\%\^\&\*\(\)\[\]\_\+\-\,\.\/\\\;\'\:\"])/g))
				&& (delim_chars.length > 2)
				)
			)
		{
			toTextEl.value = text;
//			StoreLastToText(text);
			OnEventOfToText();
			return;
		}

		var toFromTextEl = document.getElementById("toFromText");

		toTextEl.value = "";
		OnEventOfToText();
		toFromTextEl.value = "";
		OnEventOfFromToText();

		text = Trim(text);
		if (text.length)
		{
			AddMostUsedLangPair(from_lang, to_lang);

			var lastTranslateDetectToKey = sbStoragePrefix+"lastTranslateDetectTo";
			widget.cfg.getAsync(lastTranslateDetectToKey, function(cfg)
			{
				if (is_selection)
				{
					if (from_lang !== "")	// if fromLang != Autodetect => setup autodetect
					{
						var lastTranslateDetectTo = cfg.lastTranslateDetectToKey;
						if (lastTranslateDetectTo)
						{
/*
							var fromLang = document.getElementById("fromLang");
							fromLang.value = from_lang = "";
							ChangeFromLang(fromLang);
*/
							var toLang = document.getElementById("toLang");
							toLang.value = to_lang = lastTranslateDetectTo;
							ChangeToLang(toLang);
						}
					}
					else
					{
						var new_cfg = {};
						new_cfg[lastTranslateDetectToKey] = to_lang;
						widget.cfg.setAsync(new_cfg);
					}
				}
				else if (from_lang === "")
				{
					var new_cfg = {};
					new_cfg[lastTranslateDetectToKey] = to_lang;
					widget.cfg.setAsync(new_cfg);
				}

				if (from_lang !== "")
					document.getElementById("fromText")["myLanguage"] = from_lang;
				document.getElementById("toText")["myLanguage"] = to_lang;

				if (translateProvider == "bing")
					TranslateByMicrosoft(from_lang, to_lang, text);
				else if (translateProvider == "yandex")
					TranslateByYandex(from_lang, to_lang, text);
				else if (translateProvider == "promt")
					TranslateByPromt(from_lang, to_lang, text);
				else if (translateProvider == "pragma")
					TranslateByPragma(from_lang, to_lang, text);
				else if (translateProvider == "baidu")
					TranslateByBaidu(from_lang, to_lang, text);
				else if (translateProvider == "naver")
					TranslateByNaver(from_lang, to_lang, text);
				else if (translateProvider == "sogou")
					TranslateBySogou(from_lang, to_lang, text);
				else if (translateProvider == "systran")
					TranslateBySystran(from_lang, to_lang, text);
				else if (translateProvider == "babylon")
					TranslateByBabylon(from_lang, to_lang, text);
				else if (translateProvider == "dictionaries")
					TranslateByBabylonDictionary(from_lang, to_lang, text);
				else if (translateProvider == "lingvanex")
					TranslateByLingvanex(from_lang, to_lang, text);
				else if (translateProvider == "lingvo")
					TranslateByLingvoDictionary(from_lang, to_lang, text);
				else if (translateProvider == "urban")
					TranslateByUrbanDictionary(from_lang, to_lang, text);
				else if (translateProvider == "ibm")
					TranslateByIbm(from_lang, to_lang, text);
				else if (translateProvider == "apertium")
					TranslateByApertium(from_lang, to_lang, text);
				else if (translateProvider == "deepl")
					TranslateByDeepl(from_lang, to_lang, text);
				else if (translateProvider == "glosbe")
					TranslateByGlosbe(from_lang, to_lang, text);
				else
					TranslateByGoogle(from_lang, to_lang, text);
			});
		}
}

function SetupUIBasedOnMode(mode)
{
	if ((mode == "demo") && false)	// NOTICE: temporary turn off demo
	{
		document.getElementById("chkBackTranslation").parentNode.getElementsByTagName("SPAN")[0].className = "only_for_registered";
		SetupBackTranslationVisibility(false);
		PrepareResultFieldForText();
/*
		// temporary moved it to Settings
		PrepareResultFieldForHtml();
		widget.cfg.getAsync([
								sbStoragePrefix+"translateLastFromTextLang",
								sbStoragePrefix+"translateToLang",
								"defTargetLang"
							], function(cfg)
		{
			var html_box = document.getElementById("result_as_html");
			var iframe = html_box.getElementsByTagName("IFRAME")[0];
			var fillIframe = function()
			{
				iframe.contentDocument.open();
				iframe.contentDocument.write("<style>BODY{ margin:1px; } PRE{ white-space:pre-wrap; } BODY,PRE{ font-family:monospace; font-size:1em; }</style>");
				iframe.contentDocument.write("<style>.linkToVerify{ cursor: pointer; color: blue; text-decoration: underline;}</style>");
				iframe.contentDocument.write("<style>@media (prefers-color-scheme: dark) { HTML{ scrollbar-color: #404040 #666667; } BODY{ color: #ebebeb; } .linkToVerify{ color:#5f8ccc; } }</style>");
				iframe.contentDocument.write("<style>IFRAME { display:block; margin:0 auto; height:0; border:0; overflow:hidden; }</style>");

				var fromLang = cfg[sbStoragePrefix+"translateLastFromTextLang"];
				var toLang = cfg[sbStoragePrefix+"translateToLang"];
				if (!fromLang)
					fromLang = document.getElementById("fromLang").value;
				if (!toLang)
					toLang = document.getElementById("toLang").value;
				var lang_pair = [ fromLang, toLang ];
				if (navigator.onLine)
				{
					var langs = lang_pair[0] + "-" + lang_pair[1] + "-" + (cfg.defTargetLang ? cfg.defTargetLang : DetectUserPreferedLanguage());
					var size = html_box.offsetWidth + "x" + html_box.offsetHeight;
					iframe.contentDocument.write("<iframe id='popup_partner_window' src='https://translator.sailormax.net/partners/img?target=popup_partner_window&langs="+langs+"&size="+size+"' scrolling='no'></iframe>");
				}

				iframe.contentDocument.write("<pre>" + WORDS.txtUnregisteredModeDetails + "</pre>");
				iframe.contentDocument.write("<span class='linkToVerify' id='linkToVerify'>" + WORDS.txtVerify + "</span>");
				iframe.contentDocument.write("<script src='js/shared_data.js'></script>");
				iframe.contentDocument.close();
			};
			window.addEventListener('DOMContentLoaded', fillIframe);
			fillIframe();
		});
*/
	}
	else
	{
		document.getElementById("chkBackTranslation").parentNode.getElementsByTagName("SPAN")[0].className = "";
		SetupBackTranslationVisibility();
		PrepareResultFieldForText();
	}
}

/********/
window.addEventListener('load', function()
{
	widget.cfg.getAsync([
							"last_check", "last_verified", "mode"
						], function(cfg)
	{
		var now = (new Date())-0;
		var last_checked = cfg.last_check-0 || 0;
		var last_verified = cfg.last_verified-0 || 0;
		widget.mode = cfg.mode;

		if ((last_verified > now) || ((now - last_verified) > 604800000))	// week
		{
			if ((last_checked > now) || ((now - last_checked) > 172800000))	// 2 days
			{
				// try
				VerifyWidgetAsync();
			}
		}

		SetupUIBasedOnMode(widget.mode);
	});
}, false);
/********/

function BackTranslateResult()
{
	if (!document.getElementById("chkBackTranslation").checked)
		return;

	var from_lang = document.getElementById("fromLang").value,
		to_lang = document.getElementById("toLang").value,
		text = RemoveDictionaryFromText(document.getElementById("toText").value);

	if (from_lang.indexOf('~') >= 0)
	{
		from_lang = from_lang.split('~', 2)[0];
		if (from_lang === "")
			from_lang = document.getElementById("fromText")["myLanguage"];
	}
	else if ((from_lang === "") && detectedInputStringsLang)
		from_lang = detectedInputStringsLang;

	if (from_lang == to_lang)
	{
		document.getElementById("toFromText").value = text;
		return;
	}

	if (text.length)
	{
		scripts = [];
		inputStrings = [];
		outputStrings = [];
		translatedStringsCnt = 0;

		var toFromText = document.getElementById("toFromText");
		toFromText["myLanguage"] = from_lang;

		backTranslating = true;
		if (translateProvider == "bing")
			TranslateByMicrosoft(to_lang, from_lang, text);
		else if (translateProvider == "yandex")
			TranslateByYandex(to_lang, from_lang, text);
		else if (translateProvider == "promt")
			TranslateByPromt(to_lang, from_lang, text);
		else if (translateProvider == "pragma")
			TranslateByPragma(to_lang, from_lang, text);
		else if (translateProvider == "baidu")
			TranslateByBaidu(to_lang, from_lang, text);
		else if (translateProvider == "naver")
			TranslateByNaver(to_lang, from_lang, text);
		else if (translateProvider == "sogou")
			TranslateBySogou(to_lang, from_lang, text);
		else if (translateProvider == "systran")
			TranslateBySystran(to_lang, from_lang, text);
		else if (translateProvider == "babylon")
			TranslateByBabylon(to_lang, from_lang, text);
		else if (translateProvider == "lingvanex")
			TranslateByLingvanex(from_lang, to_lang, text);
		else if (translateProvider == "ibm")
			TranslateByIbm(from_lang, to_lang, text);
		else if (translateProvider == "apertium")
			TranslateByApertium(from_lang, to_lang, text);
		else if (translateProvider == "deepl")
			TranslateByDeepl(from_lang, to_lang, text);
		else
			TranslateByGoogle(to_lang, from_lang, text);
	}
}

function OpenCopyrightLink()
{
	var from_lang = document.getElementById("fromLang").value,
		to_lang = document.getElementById("toLang").value,
		text = document.getElementById("fromText").value;

	if (from_lang.indexOf('~') >= 0)
		from_lang = from_lang.split('~', 2)[0];

	var funcCreateTabWithCopyright = function(from_lang)
	{
		to_lang = GetFixedTargetLang(from_lang, to_lang);

		var urlMask = "";
		if (translateProvider == "google")
			urlMask = "https://translate.google."+(useGoogleCn?"com.hk":"com")+"/#[from_lang]/[to_lang]/[text]".replace("[from_lang]", (from_lang?from_lang:"auto")).replace("[to_lang]", to_lang);
		else if (translateProvider == "bing")
			urlMask = "https://www.bing.com/translator?text=[text]&from=[from_lang]&to=[to_lang]".replace("[from_lang]", (from_lang?from_lang:"")).replace("[to_lang]", to_lang);
		else if (translateProvider == "yandex")
			urlMask = "https://translate.yandex.com/m/translate?text=[text]&lang=[from_lang]-[to_lang]".replace("[from_lang]", (from_lang?from_lang:"")).replace("[to_lang]", to_lang);
		else if (translateProvider == "promt")
			urlMask = "https://m.online-translator.com/translation";
		else if (translateProvider == "pragma")
			urlMask = "http://www.translate.ua/us/on-line";
		else if (translateProvider == "baidu")
			urlMask = "https://fanyi.baidu.com/#"+(!from_lang ? "auto" : (baiduLanguagesTo[from_lang] ? baiduLanguagesTo[from_lang] : from_lang))+"/"+(baiduLanguagesTo[to_lang] ? baiduLanguagesTo[to_lang] : to_lang)+"/[text]";
		else if (translateProvider == "naver")
			urlMask = "https://papago.naver.com/?sk=[from_lang]&tk=[to_lang]&st=[text]".replace("[from_lang]", (!from_lang?"auto":from_lang)).replace("[to_lang]", to_lang);
		else if (translateProvider == "sogou")
			urlMask = "https://fanyi.sogou.com/#[from_lang]/[to_lang]/[text]".replace("[from_lang]", (!from_lang?"auto":from_lang)).replace("[to_lang]", to_lang);
		else if (translateProvider == "systran")
			urlMask = "https://translate.systran.net/translationTools/text";
		else if (translateProvider == "babylon")
			urlMask = "https://translation.babylon-software.com/[from_lang]/to-[to_lang]/[text]/".replace("[from_lang]", from_lang?languages[from_lang].toLowerCase():"english").replace("[to_lang]", languages[to_lang].toLowerCase());
		else if (translateProvider == "dictionaries")
			urlMask = "https://dictionary.babylon-software.com/[text]/";
		else if (translateProvider == "lingvanex")
			urlMask = "https://lingvanex.com/demo/";
		else if (translateProvider == "lingvo")
			urlMask = "https://www.lingvolive.com/"+(EXT_LOCALE=="ru"?"ru-ru":"en-us")+"/translate/[from_lang]-[to_lang]/[text]".replace("[from_lang]", from_lang).replace("[to_lang]", to_lang);
		else if (translateProvider == "urban")
			urlMask = "https://www.urbandictionary.com/define.php?term=[text]";
		else if (translateProvider == "ibm")
			urlMask = "https://www.ibm.com/watson/services/language-translator/";
		else if (translateProvider == "apertium")
			urlMask = "https://www.apertium.org/index.eng.html?dir="+(!from_lang ? "auto" : (apertiumLanguagesTo[from_lang] ? apertiumLanguagesTo[from_lang] : from_lang))+"-"+(apertiumLanguagesTo[to_lang] ? apertiumLanguagesTo[to_lang] : to_lang)+"&q=[text]#translation";
		else if (translateProvider == "deepl")
			urlMask = "https://www.deepl.com/translator#[from_lang]/[to_lang]/[text]".replace("[from_lang]", from_lang).replace("[to_lang]", to_lang);
		else if (translateProvider == "glosbe")
			urlMask = "https://glosbe.com/[from_lang]/[to_lang]/[text]".replace("[from_lang]", from_lang).replace("[to_lang]", to_lang);

		if (urlMask !== "")
		{
			urlMask = urlMask.replace("[text]", encodeURIComponent(text));
			CreateTabWithUrl(urlMask);
		}
	};

	if (!from_lang)
		DetectTextLanguageAsync(text, funcCreateTabWithCopyright);
	else
		funcCreateTabWithCopyright(from_lang);
}

function PrepareResultFieldForText()
{
	if (document.getElementById("result_as_text").parentNode.className.indexOf("result_as_html") < 0)
		return;

	document.getElementById("result_as_text").getElementsByTagName("TEXTAREA").value = "";
	RemoveClass(document.getElementById("result_as_text").parentNode, "result_as_html");
}

function PrepareResultFieldForHtml()
{
	if (document.getElementById("result_as_text").parentNode.className.indexOf("result_as_html") >= 0)
		return;

	document.getElementById("result_as_html").getElementsByTagName("IFRAME")[0].src = "about:blank";
	AppendClass(document.getElementById("result_as_text").parentNode, "result_as_html");
}
