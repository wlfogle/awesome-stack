function ShowHidePartnerClick(event)
{
	var i, box = event.target || event.srcElement;
	while (box && box.tagName != "DIV")
		box = box.parentNode;
	if (!box)
		return;

	widget.cfg.getAsync("hiddenPartners", function(cfg)
	{
		if (!cfg.hiddenPartners)
			cfg.hiddenPartners = {};

		if (box.className.indexOf("collapsed") >= 0)
		{
			delete cfg.hiddenPartners[ box["myPartnerName"] ];
			RemoveClass(box, "collapsed");
		}
		else
		{
			cfg.hiddenPartners[ box["myPartnerName"] ] = 1;
			AppendClass(box, "collapsed");
		}

		widget.cfg.setAsync(cfg);
	});
}

function AddNewPartnerItem(partner_name, partner, cfg)
{
	var new_item = document.getElementById("column_item_tpl").cloneNode(true);
	new_item.removeAttribute("id");		// delete doesn't work!
	new_item["myPartnerName"] = partner_name;
	new_item.style.display = "";
	new_item.style.width = partner.width;
	if (cfg.hiddenPartners[partner_name])
		AppendClass(new_item, "collapsed");

	var new_item_links = new_item.getElementsByTagName("A");
	new_item_links[0].innerText = WORDS.txtShow;
	new_item_links[0].addEventListener("click", ShowHidePartnerClick);
	new_item_links[1].innerText = WORDS.txtHide;
	new_item_links[1].addEventListener("click", ShowHidePartnerClick);
	if (partner.iframe)
	{
		new_item_links[2].style.display = "none";
		new_item_links[3].style.display = "none";
		var iframe = document.createElement("IFRAME");
		iframe.id = "dictionary_partner_window";
		iframe.scrolling = "no";
		iframe.src = partner.iframe;
		iframe.style.height = "0";
		new_item_links[3].parentNode.insertBefore(iframe, new_item_links[3]);
	}
	else
	{
		new_item_links[2].innerText = partner_name;
		new_item_links[2].href = partner.link;
		new_item_links[3].href = partner.link;
		new_item.getElementsByTagName("IMG")[0].src = partner.image;
	}

	if (partner.target == "left")
		document.getElementById("left_column").appendChild(new_item);
	else
		document.getElementById("right_column").appendChild(new_item);
}

function RefreshPartnerImagesByDictionaryKeys(keys_list)
{
	var left_column = document.getElementById("left_column");
	var right_column = document.getElementById("right_column");

	// clear right output area
	var column_items_list = right_column.childNodes;
	var i = column_items_list.length - 1;
	for (i; i>=0; i--)
		if (column_items_list[i].tagName && !column_items_list[i].id)
			right_column.removeChild(column_items_list[i]);

	// clear left output area
	var column_items_list = left_column.childNodes;
	var i = column_items_list.length - 1;
	for (i; i>=0; i--)
		if (column_items_list[i].tagName && !column_items_list[i].id)
			left_column.removeChild(column_items_list[i]);

	widget.cfg.getAsync(["mode", "defTargetLang", "hiddenPartners", "translateLastFromTextLang", "translateToLang", "defTargetLang"], function(cfg)
	{
		if (!cfg.hiddenPartners)
			cfg.hiddenPartners = {};

//		if (cfg.mode == "demo")
		{
			var lang_pair;
			if (keys_list && keys_list.length)
			{
				var idx = Math.floor(Math.random() * (keys_list.length));

				var k_arr = keys_list[idx].split(" #");
				lang_pair = k_arr[1].split("~");
			}
			else
				lang_pair = [cfg["translateLastFromTextLang"], cfg["translateToLang"]];

			var langs = lang_pair[0] + "-" + lang_pair[1] + "-" + (cfg.defTargetLang ? cfg.defTargetLang : DetectUserPreferedLanguage());
			var size = document.getElementById("right_column").offsetWidth + "x0";
			AddNewPartnerItem("partners", {iframe:"https://translator.sailormax.net/partners/img?target=dictionary_partner_window&langs="+langs+"&size="+size}, cfg);
		}
	});
}

function RemoveDictionaryWord(event)
{
	var i, row = event.target || event.srcElement;
	while (row && row.tagName != "TR")
		row = row.parentNode;
	if (!row)
		return;

	var id_els = row.id.split("#");
	var remove_pair = id_els[0];
	var remove_name = id_els[1];

	widget.cfg.getAsync("myDictionary", function(cfg)
	{
		var dictionary = {};
		if (typeof cfg["myDictionary"] != "undefined")
			dictionary = cfg["myDictionary"];

		var pair_words = dictionary[remove_pair];
		if (!pair_words)
			return;

		var idx, cnt = pair_words.length;
		for (idx=0; idx<cnt; idx++)
			if (pair_words[idx][0] === remove_name)
			{
				pair_words.splice(idx, 1);
				var new_cfg = {};
				new_cfg["myDictionary"] = dictionary;
				widget.cfg.setAsync(new_cfg, function()
				{
					RefreshDictionary();
				});
				return;
			}
	});
}

function RefreshDictionary()
{
	widget.cfg.getAsync("myDictionary", function(cfg)
	{
		var dictionary = {};
		if ((cfg["myDictionary"] != null) && (typeof cfg["myDictionary"] == "object") && !cfg["myDictionary"].slice)
			dictionary = cfg["myDictionary"];

		// sort by keys (language pairs)
		var k, k_arr, from_lang, to_lang, keys_list = [];
		for (k in dictionary)
			if (dictionary[k].length > 0)
			{
				k_arr = k.split("~");
				from_lang = languages[ k_arr[0] ];
				to_lang = languages[ k_arr[1] ];
				keys_list.push(from_lang + " - " + to_lang + " #"+k);
			}
		keys_list.sort();

		// clear output area
		var word_groups_box = document.getElementById("word_groups_box");
		var word_groups_list = word_groups_box.childNodes;
		var i = word_groups_list.length - 1;
		for (i; i>=0; i--)
			if (word_groups_list[i].tagName && !word_groups_list[i].id)
				word_groups_box.removeChild(word_groups_list[i]);

		// output sorted list
		var group_tpl = document.getElementById("group_tpl");
		var new_group, new_group_table;
		var i, lang_pair, lang_pair_title;
		for (i in keys_list)
		{
			k_arr = keys_list[i].split(" #");
			lang_pair_title = k_arr[0];
			lang_pair = k_arr[1];

			new_group = group_tpl.cloneNode(true);
			new_group.removeAttribute("id");	// delete doesn't work!
			new_group.style.display = "block";
			new_group.getElementsByTagName("H3")[0].innerText = lang_pair_title;
			new_group_table = new_group.getElementsByTagName("TABLE")[0];

			var idx, cnt = dictionary[lang_pair].length;
			for (idx=0; idx<cnt; idx++)
			{
				var tr = document.createElement("TR");
				tr.id = lang_pair+"#"+dictionary[lang_pair][idx][0];
				var td1 = document.createElement("TD");
				td1.innerText = dictionary[lang_pair][idx][0];
				tr.appendChild(td1);
				var td2 = document.createElement("TD");
				td2.className = "translation";
				td2.innerText = dictionary[lang_pair][idx][1];
					var a = document.createElement("A");
					a.className = "removeRow";
					a.innerText = WORDS.linkRemove;
					a.addEventListener('click', RemoveDictionaryWord);
					td2.appendChild(a);

					if (widget.useGoogleTTS)
					{
						// <div id="toTextPlayer" class="player_button disabled" title="vocalize" style="display: block;"></div>
						var toTextPlayer = document.createElement("DIV");
						toTextPlayer.className = "player_button disabled";
						toTextPlayer.title = WORDS.hntVocalize;
						toTextPlayer.myLangName = lang_pair.split("~")[1];
						toTextPlayer.myText = dictionary[lang_pair][idx][1];
						toTextPlayer.addEventListener("click", function(event) { TextReader.Play(this, this.myLangName, this.myText); }, false);
						td2.appendChild(toTextPlayer);
					}
				tr.appendChild(td2);
				new_group_table.appendChild(tr);
			}

			word_groups_box.appendChild(new_group);
		}

		RefreshPartnerImagesByDictionaryKeys(keys_list);
	});
}

window.addEventListener('load', function()
{
	if (widget)
		widget.cfg.getAsync([
								"usePersonalDictionary", "useGoogleTTS", "translateProvider", "useGoogleCn", "msAppId", "mode",
							], function(cfg)
		{
			widget.mode = cfg.mode;
			widget.useGoogleTTS	= ((cfg.useGoogleTTS == null) || (cfg.useGoogleTTS == "true"));
			widget.useGoogleCn	= (cfg.useGoogleCn == "true");
			widget.msAppId		= cfg.msAppId;
			widget.translateProvider	= cfg.translateProvider;
			RefreshDictionary();

			if ((widget.mode == "demo") && false)	// NOTICE: temporary turn off demo
			{
//				document.getElementsByName("useGoogleTTS")[0].parentNode.className = "only_for_registered";
				document.getElementById("blockRegistration").style.display = "block";
			}
			else
				document.getElementById("blockRegistration").style.display = "none";
		});

	document.getElementById("txtDictionary").innerText					= WORDS.txtDictionary;

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
