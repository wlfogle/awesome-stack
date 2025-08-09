/*
	Reverse engineering of code from https://fanyi.baidu.com/
*/
function GetSignOfBaiduQuery(query) {
	"use strict";

	function GetArrayFrom(source)
	{
		if (Array.isArray(source)) {
			for (var i = 0, target = Array(source.length); i < source.length; i++)
				target[i] = source[i];
			return target
		}
		return Array.from(source)
	}

	function TransformHash(hash, mword)
	{
		for (var i = 0; i < mword.length - 2; i += 3) {
			var mchar = mword.charAt(i + 2);
			mchar = mchar >= "a"
				? mchar.charCodeAt(0) - 87
				: Number(mchar), mchar = "+" === mword.charAt(i + 1)
												? hash >>> mchar
												: hash << mchar, hash = "+" === mword.charAt(i)
																			? hash + mchar & 4294967295
																			: hash ^ mchar
		}
		return hash
	}

	function GetSign(query)
	{
		var unicodes = query.match(/[\uD800-\uDBFF][\uDC00-\uDFFF]/g);
		if (null === unicodes)
		{
			var query_len = query.length;
			if (query_len > 30)
				query = "" + query.substr(0, 10) + query.substr(Math.floor(query_len / 2) - 5, 10) + query.substr(-10, 10);
		} else {
			for (var unicode_chars = query.split(/[\uD800-\uDBFF][\uDC00-\uDFFF]/), i = 0, max = unicode_chars.length, chars_arr = []; max > i; i++)
			{
				if ("" !== unicode_chars[i])
				{
					chars_arr.push.apply(chars_arr, GetArrayFrom(unicode_chars[i].split("")));
					if (i !== max - 1)
						chars_arr.push(unicodes[i]);
				}
			}
			var chars_cnt = chars_arr.length;
			if (chars_cnt > 30)
				query = chars_arr.slice(0, 10).join("") + chars_arr.slice(Math.floor(chars_cnt / 2) - 5, Math.floor(chars_cnt / 2) + 5).join("") + chars_arr.slice(-10).join("")
		}
		var sid = void 0,
			signIdName = "" + String.fromCharCode(103) + String.fromCharCode(116) + String.fromCharCode(107);
		sid = null !== signId ? signId : (signId = window[signIdName] || "") || "";
		for (var sid_chars = sid.split("."), snum1 = Number(sid_chars[0]) || 0, snum2 = Number(sid_chars[1]) || 0, enc_chars = [], j = 0, i = 0; i < query.length; i++)
		{
			var qchar = query.charCodeAt(i);
			128 > qchar
				? enc_chars[j++] = qchar
				: (2048 > qchar
					? enc_chars[j++] = qchar >> 6 | 192
					: (55296 === (64512 & qchar) && i + 1 < query.length && 56320 === (64512 & query.charCodeAt(i + 1))
						? (qchar = 65536 + ((1023 & qchar) << 10) + (1023 & query.charCodeAt(++i)), enc_chars[j++] = qchar >> 18 | 240, enc_chars[j++] = qchar >> 12 & 63 | 128)
						: enc_chars[j++] = qchar >> 12 | 224, enc_chars[j++] = qchar >> 6 & 63 | 128
						), enc_chars[j++] = 63 & qchar | 128
					)
		}
		for (var hash = snum1, mword1 = "" + String.fromCharCode(43) + String.fromCharCode(45) + String.fromCharCode(97) + ("" + String.fromCharCode(94) + String.fromCharCode(43) + String.fromCharCode(54)), mword2 = "" + String.fromCharCode(43) + String.fromCharCode(45) + String.fromCharCode(51) + ("" + String.fromCharCode(94) + String.fromCharCode(43) + String.fromCharCode(98)) + ("" + String.fromCharCode(43) + String.fromCharCode(45) + String.fromCharCode(102)), i = 0; i < enc_chars.length; i++)
			hash += enc_chars[i], hash = TransformHash(hash, mword1);
		return hash = TransformHash(hash, mword2), hash ^= snum2, 0 > hash && (hash = (2147483647 & hash) + 2147483648), hash %= 1e6, hash.toString() + "." + (hash ^ snum1);
	}
	var signId = null;
	return GetSign(query);
}
