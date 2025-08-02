/*
	source code was lost in internet :/
*/
var wanahangul = {
	"bind":	function(inEl)
	{
		if (typeof HANGUL_RULES == "undefined")
		{
			var words_script = document.createElement("SCRIPT");
			words_script.src = "js/libs/hangul_rules.js";
			var firstScript = document.getElementsByTagName("SCRIPT")[0];
			firstScript.parentNode.insertBefore(words_script, firstScript);
		}
		inEl.addEventListener("input", this.convertInputedValue, false);
	},

	"unbind": function(inEl)
	{
		inEl.removeEventListener("input", this.convertInputedValue, false);
	},

	"get_node_of_hangul_char": function(hangul_char, node)
	{
		if (typeof node == "undefined")
			node = HANGUL_RULES;

		for (var k in node)
		{
			if (k == "$")
			{
				if (node["$"] === hangul_char)
					return node;
			}
			else
			{
				result = wanahangul.get_node_of_hangul_char(hangul_char, node[k]);
				if (result !== null)
					return result;
			}
		}
		return null;
	},

	"find_hangul_char": function(node, hangul_char)
	{
		var result;
		for (var k in node)
		{
			if (k == "$")
			{
				if (node["$"] === hangul_char)
					return true;
			}
			else
			{
				result = wanahangul.find_hangul_char(node[k], hangul_char);
				if (result !== false)
				{
					if (result === true)
						result = [k];
					else
						result.push(k);
					return result;
				}
			}
		}
		return false;
	},

	"hangul2roman_char": function(hangul_char)
	{
		var roman_chars = wanahangul.find_hangul_char(HANGUL_RULES, hangul_char);
		if (roman_chars === false)
			return hangul_char;
		return roman_chars.reverse().join("");
	},

	"hangul2roman": function(hangul_txt)
	{
		var roman_list = [];
		var cnt = hangul_txt.length;
		for (var i=0; i<cnt; i++)
			roman_list.push( wanahangul.hangul2roman_char(hangul_txt[i]) );
		return roman_list.join("");
	},

	"get_vocab": function(rules, path)
	{
		var vocab = {};
		if (typeof path == "undefined")
			path = [];

		for (var i in rules)
		{
			if (i === "$")
			{
				vocab[ path.slice().reverse().join("") ] = rules[i];
			}
			else if (rules.hasOwnProperty(i))
			{
				path.push(i);
				Object.assign(vocab, wanahangul.get_vocab(rules[i], path));
				path.pop();
			}
		}

		return vocab;
	},

	"vocab2rules": function(vocab)
	{
		var keys = [];
		for (var k in vocab)
			if (vocab.hasOwnProperty(k))
			    keys.push(k);
		keys.sort();

		var rules = {};
		for (var i in keys)
		{
			var k = keys[i];
			var node = rules;
			var chars = k.split('');
			for (var j in chars)
			{
				if (typeof node[ chars[j] ] == "undefined")
					node[ chars[j] ] = {};
				node = node[ chars[j] ];
			}
			node["$"] = vocab[k];
		}
		return rules;
	},

	"convert2hangul": function(input, start_node)
	{
/*
		// invert
		var vocab = wanahangul.get_vocab(HANGUL_RULES);
		var NEW_HANGUL_RULES = wanahangul.vocab2rules(vocab);
*/
		var roman = wanahangul.hangul2roman(input);

		// convert
		// source from http://gimite.net/roman2hangul/
		var hangul_list = [];
		var node = (start_node ? start_node : HANGUL_RULES);
		var prev = null;
		var cnt = roman.length;
		for (var i=0; i<cnt; i++)
		{
			var r = roman[i].toUpperCase();
			var next = node[r];
			if (!next)
			{
				if (node["$"])
				{
					hangul_list.push(node["$"]);
					next = HANGUL_RULES[r];
				}
				else if (prev)	// can't continue + don't have $ => watch at previous + return cursor
				{
					hangul_list.push(prev["$"]);
					prev = null;
					node = HANGUL_RULES;
					i -= 2;
					continue;
				}
			}
			if (!next)
			{
				if (roman[i] != "-")
					hangul_list.push(roman[i]);
				node = null;
				next = HANGUL_RULES;
			}
			prev = node;
			node = next;
		}

		if (node["$"])
		{
			hangul_list.push(node["$"]);
		}
		else if (prev)	// case when we don't have $ for last symbol => take hangul of previous + output last
		{
			hangul_list.push(prev["$"]);
			node = HANGUL_RULES;
			var lastChar = roman[i-1];
			var r = lastChar.toUpperCase();
			if (node[r])
				hangul_list.push(node[r]["$"]);
			else if (lastChar != "-")
				hangul_list.push(lastChar);
		}
		return hangul_list.join("");
	},

	"convertInputedValue": function(event)
	{
		var sender = event.target;
		var cursor = sender.selectionEnd;
		var value = sender.value;

		// find first nearest non latin symbol at left
		var left_latins = value.substr(0, cursor).match(/[^\x00-\x7F]?[\x00-\x7F]*$/);
//		var right_latins = value.substr(cursor).match(/^[\x00-\x7F]*[^\x00-\x7F]?/);
		var right_latins = "";
		var shift_cursor = 0;
		var left_result = "";
		var right_result = "";
		var start_node;

		if (left_latins && left_latins[0].length)
		{
			left_latins = left_latins[0];
			start_node = wanahangul.get_node_of_hangul_char(left_latins[0]);				// try to find first character
			if (start_node)
			{
				left_result = wanahangul.convert2hangul(left_latins.substr(1), start_node);	// continue convert
				if (left_result === "")		// continue of convert can be without result => leave first char as is and convert others
					left_result = left_latins[0] + wanahangul.convert2hangul(left_latins.substr(1));
			}
			else
				left_result = wanahangul.convert2hangul(left_latins);						// or convert all text
			shift_cursor = left_result.length - left_latins.length;
		}
/*
		if (right_latins && right_latins[0].length)
		{
			right_latins = right_latins[0];
			var last_char_latin = wanahangul.hangul2roman_char(right_latins.substr(-1));
			right_latins = right_latins.substr(0, right_latins.length-1) + last_char_latin;

			var last_left_char = "";
			if (left_result)
			{
				last_left_char = left_result.substr(-1);
				left_result = left_result.substr(0, left_result.length-1);
				var last_left_char_latin = wanahangul.hangul2roman_char(last_left_char);
				right_latins = last_left_char_latin + right_latins;
			}

			right_result = wanahangul.convert2hangul(right_latins);
			if (last_left_char !== right_result[0])						// first character was changed => joined and converted
				shift_cursor++;
		}
*/
		var result_text = sender.value.substr(0, cursor-left_latins.length);
		result_text += left_result + right_result;
		result_text += sender.value.substr(cursor+right_latins.length);
		sender.value = result_text;
		var new_cursor = cursor + shift_cursor;
		sender.setSelectionRange(new_cursor, new_cursor);
	}
};
