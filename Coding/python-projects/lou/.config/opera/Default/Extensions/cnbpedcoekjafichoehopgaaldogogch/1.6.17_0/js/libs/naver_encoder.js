/*
	Reverse engineering of code from https://papago.naver.com/
*/
function GetEncodedNaverQuery(query) {
	"use strict";

	function random_postfix(cnt)
	{
		for (var postfix = "", i = 0; i < cnt; i++)
			postfix += String.fromCharCode(Math.floor(80 * Math.random() + 43));
		return postfix;
	}

	function swap_parts(str, a)
	{
		return str.substr(a) + str.substr(0, a);
	}

	function replace_chars(str)
	{
		return str.replace(/([a-m])|([n-z])/gi, function(match, chr1, chr2) {
			return String.fromCharCode(chr1 ? chr1.charCodeAt(0) + 13 : chr2 ? chr2.charCodeAt(0) - 13 : 0) || match;
		});
	}
	function GetEncodedString(str)
	{
		var addit_bytes = function(str) {
			for (var bytes_cnt = str.length, i = str.length - 1; i >= 0; i--)
			{
				var chr = str.charCodeAt(i);
				chr > 127 && chr <= 2047 ? bytes_cnt++ : chr > 2047 && chr <= 65535 && (bytes_cnt += 2);
			}
			return bytes_cnt;
		}(str) % 6
		, addit_chars = addit_bytes > 0 ? random_postfix(6).substr(0, 6 - addit_bytes) : "";

		return replace_chars(function(str) {
			var chr = random_postfix(1);
			return chr + swap_parts(str, chr.charCodeAt(0) % (str.length - 2) + 1);
		}(base64.encode("" + str + addit_chars)));
	}

	return GetEncodedString(query); 
}
