/*
Required:
	widget.translateProvider
	widget.useGoogleCn
	widget.msAppId
	widget.mode
	lang_default_locales

	button classes: player_button playing
*/
var TextReader = {

	supportMP3: null,
	status: 0,			// status: -1 - not loaded, 0 - stopped, 1 - playing, 1 - playing vis internal tts
	player: null,
	button: null,
	idx: 0,

	playByInternalService: function(lang, text)
	{
		if (!chrome || !chrome.tts)
			return null;

		var idx = this.idx + 1;
		var self = this;
		chrome.tts.speak(
					text.substr(0, 100),
					{"lang":lang, "onEvent":function(event)
											{
												switch (event.type)
												{
													case "cancelled":
													case "interrupted":
													case "error":
													case "end":
														self.Stop(idx);
														break;
												}
											}
					}
					);
		this.idx = idx;
		return chrome.tts;
	},

	playByExternalService: function(lang, text)
	{
		var idx = this.idx + 1;
		var player = null;

		if ((widget.translateProvider == "sogou")
			&& (this.supportMP3 || (this.supportMP3 === null && (this.supportMP3 = (new Audio()).canPlayType("audio/mpeg")) ))
			)
		{
			if (lang == "en" || lang == "zh-CHS")
		  	{
				player = new Audio("https://fanyi.sogou.com/reventondc/synthesis?speed=1&from=translateweb"
									+"&lang=" + lang
									+"&text=" + encodeURIComponent(text.substr(0, 100)));
			}
			else
			{
				player = new Audio("https://fanyi.sogou.com/reventondc/microsoftGetSpeakFile?from=translateweb"
									+"&spokenDialect=" + lang
									+"&text=" + encodeURIComponent(text.substr(0, 100)));
			}
		}
		//		if ((!lang_default_locales[lang] || (widget.translateProvider == "google"))
		else if (//(widget.translateProvider == "google") &&
					(this.supportMP3 || (this.supportMP3 === null && (this.supportMP3 = (new Audio()).canPlayType("audio/mpeg")) ))
				)
		{
			player = new Audio("https://translate.google."+(widget.useGoogleCn ? "com.hk" : "com")+"/translate_tts?ie=UTF-8&client=tw-ob"
								+"&tl=" + lang
								+"&q=" + encodeURIComponent(text.substr(0, 100)));
		}
/*
		else if (lang_default_locales[lang])
		{
			if (widget.msAppId === "")
			{
				// TODO: finish this function for dictionary
				SendMessageToBackgroundScript( { action: "get_msAppId", next_action:"TextReader.Play", next_action_args:{sender:this.button, lang:lang, text:text} } );
				return;
			}

			// https://www.bing.com/translator/api/language/Speak?locale=en-US&gender=male&media=audio/mp3&text=was -- But it require -US...
			// https://www.bing.com/translator/api/language/Speak?locale=ru-RU&gender=male&media=audio/mp3&text=%D1%82%D0%B5%D1%81%D1%82
			player = new Audio("https://www.bing.com/translator/api/language/Speak"
									+"?locale=" + lang_default_locales[lang]
									+"&gender=male"
									+"&media=audio/wav"
									+"&text=" + encodeURIComponent(text.substr(0, 100)));

			// TODO: change to new API
			// https://northeurope.tts.speech.microsoft.com/cognitiveservices/v1?
			// POST
			// content-type: application/ssml+xml
			// x-microsoft-outputformat: audio-16khz-32kbitrate-mono-mp3
			// <speak version='1.0' xml:lang='en-US'><voice xml:lang='en-US' xml:gender='Female' name='en-US-JessaRUS'><prosody rate='-20.00%'>test</prosody></voice></speak>
		}
*/

		if (player)
		{
			var self = this;
			player.addEventListener("ended", function(){ self.Stop(idx); }, false);
			// if web-speech is broken, try use browser's tts
			player.addEventListener("error", function(){ self.player = self.playByInternalService(lang, text) }, false);

			player.play();
		}

		this.idx = idx;
		return player;
	},

	Play: function(sender, lang, text)
	{
		if ((widget.mode == "demo") && false)	// NOTICE: temporary turn off demo
		{
			CreateTabWithUrl("settings.html#registration");
			return;
		}

		if (this.status)
		{
			this.Stop();
			if (this.button === sender)
				return;
		}

		// play
		this.button = sender;
		this.player = this.playByExternalService(lang, text);
		if (this.player)
			this.status = 1;
		else
		{
			this.player = this.playByInternalService(lang, text);
			if (this.player)
				this.status = 2;
			else
				return false;
		}

		this.button.className = "player_button playing";
	},

	Stop: function(idx)
	{
		if (idx && (idx != this.idx))	// Stop() from previous player
			return;
		if (this.player.stop)
			this.player.stop();
		else if (this.player.pause)
			this.player.pause();
		this.status = 0;
		this.button.className = "player_button";
	},

	PreSetupButtonStatus: function(button, lang, text)
	{
		if (text.length && ((widget.mode != "demo") || true))	// NOTICE: temporary turn off demo
			button.className = "player_button";
		else
			button.className = "player_button disabled";
	}
};
