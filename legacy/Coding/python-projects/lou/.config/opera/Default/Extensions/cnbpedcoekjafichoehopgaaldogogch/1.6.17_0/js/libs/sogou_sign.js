/*
	Reverse engineering of code from https://fanyi.sogou.com/text
*/
function GetSogouQuerySign(query)
{
//"152": function(t, e) {
	function ByteTools()
	{
		var tools = {
			"rotl": function(num, cnt) {
				return num << cnt | num >>> 32 - cnt
			},
			"endian": function(num) {
				if (num.constructor == Number)
					return 16711935 & tools.rotl(num, 8) | 4278255360 & tools.rotl(num, 24);
				for (var e = 0; e < num.length; e++)
					num[e] = tools.endian(num[e]);
				return num
			},
			"bytesToWords": function(bytes) {
				for (var words = [], n = 0, r = 0; n < bytes.length; n++, r += 8)
					words[r >>> 5] |= bytes[n] << 24 - r % 32;
				return words
			},
			"wordsToBytes": function(words) {
				for (var bytes = [], n = 0; n < 32 * words.length; n += 8)
					bytes.push(words[n >>> 5] >>> 24 - n % 32 & 255);
				return bytes
			},
			"bytesToHex": function(bytes) {
				for (var chars = [], n = 0; n < bytes.length; n++)
					chars.push((bytes[n] >>> 4).toString(16)),
					chars.push((15 & bytes[n]).toString(16));
				return chars.join("")
			}
		};
	
		return tools;
	}
		
	//	"129": function(t, e) {
	function StringTools()
	{
		var tools = {
			"utf8": {
				"stringToBytes": function(str) {
					return tools.bin.stringToBytes(unescape(encodeURIComponent(str)))
				},
				"bytesToString": function(bytes) {
					return decodeURIComponent(escape(tools.bin.bytesToString(bytes)))
				}
			},
			"bin": {
				"stringToBytes": function(str) {
					for (var bytes = [], n = 0; n < str.length; n++)
						bytes.push(255 & str.charCodeAt(n));
					return bytes
				},
				"bytesToString": function(bytes) {
					for (var chars = [], n = 0; n < bytes.length; n++)
						chars.push(String.fromCharCode(bytes[n]));
					return chars.join("")
				}
			}
		};
		return tools;
	}
	
	//"165": function(t, e) {
	function GetBufferDetector()
	{
		function isBufferObj(obj) {
			return !!obj.constructor && "function" == typeof obj.constructor.isBuffer && obj.constructor.isBuffer(obj)
		}
		function isBufferStream(s) {
			return "function" == typeof s.readFloatLE && "function" == typeof s.slice && isBufferObj(s.slice(0, 0))
		}
		/*!
		* Determine if an object is a Buffer
		*
		* @author   Feross Aboukhadijeh <https://feross.org>
		* @license  MIT
		*/
		return function(obj) {
			return null != obj && (isBufferObj(obj) || isBufferStream(obj) || !!obj._isBuffer)
		}
	}
		
	
	//s() = !function() {
	function GetQuerySigner()
	{
		// md5 from here: http://lig-membres.imag.fr/donsez/cours/exemplescourstechnoweb/js_securehash/md5src.html
		var bt = ByteTools()			//n(152)
		  , utf8 = StringTools().utf8	//n(129).utf8
		  , isBuffer = GetBufferDetector()	//n(165)
		  , bin = StringTools().bin	//n(129).bin
		  , signer = function(str, cfg) {
			str.constructor == String
				? str = cfg && "binary" === cfg.encoding
					? bin.stringToBytes(str)
					: utf8.stringToBytes(str)
				: isBuffer(str)
					? str = Array.prototype.slice.call(str, 0)
					: Array.isArray(str) || (str = str.toString());
			for (var words = bt.bytesToWords(str), u = 8 * str.length, a = 1732584193, b = -271733879, c = -1732584194, d = 271733878, i = 0; i < words.length; i++)
				words[i] = 16711935 & (words[i] << 8 | words[i] >>> 24) | 4278255360 & (words[i] << 24 | words[i] >>> 8);
			words[u >>> 5] |= 128 << u % 32,
			words[14 + (u + 64 >>> 9 << 4)] = u;
			for (var ff = signer._ff, gg = signer._gg, hh = signer._hh, ii = signer._ii, i = 0; i < words.length; i += 16) {
				var olda = a
				  , oldb = b
				  , oldc = c
				  , oldd = d;
				a = ff(a, b, c, d, words[i + 0], 7, -680876936),
				d = ff(d, a, b, c, words[i + 1], 12, -389564586),
				c = ff(c, d, a, b, words[i + 2], 17, 606105819),
				b = ff(b, c, d, a, words[i + 3], 22, -1044525330),
				a = ff(a, b, c, d, words[i + 4], 7, -176418897),
				d = ff(d, a, b, c, words[i + 5], 12, 1200080426),
				c = ff(c, d, a, b, words[i + 6], 17, -1473231341),
				b = ff(b, c, d, a, words[i + 7], 22, -45705983),
				a = ff(a, b, c, d, words[i + 8], 7, 1770035416),
				d = ff(d, a, b, c, words[i + 9], 12, -1958414417),
				c = ff(c, d, a, b, words[i + 10], 17, -42063),
				b = ff(b, c, d, a, words[i + 11], 22, -1990404162),
				a = ff(a, b, c, d, words[i + 12], 7, 1804603682),
				d = ff(d, a, b, c, words[i + 13], 12, -40341101),
				c = ff(c, d, a, b, words[i + 14], 17, -1502002290),
				b = ff(b, c, d, a, words[i + 15], 22, 1236535329),
				a = gg(a, b, c, d, words[i + 1], 5, -165796510),
				d = gg(d, a, b, c, words[i + 6], 9, -1069501632),
				c = gg(c, d, a, b, words[i + 11], 14, 643717713),
				b = gg(b, c, d, a, words[i + 0], 20, -373897302),
				a = gg(a, b, c, d, words[i + 5], 5, -701558691),
				d = gg(d, a, b, c, words[i + 10], 9, 38016083),
				c = gg(c, d, a, b, words[i + 15], 14, -660478335),
				b = gg(b, c, d, a, words[i + 4], 20, -405537848),
				a = gg(a, b, c, d, words[i + 9], 5, 568446438),
				d = gg(d, a, b, c, words[i + 14], 9, -1019803690),
				c = gg(c, d, a, b, words[i + 3], 14, -187363961),
				b = gg(b, c, d, a, words[i + 8], 20, 1163531501),
				a = gg(a, b, c, d, words[i + 13], 5, -1444681467),
				d = gg(d, a, b, c, words[i + 2], 9, -51403784),
				c = gg(c, d, a, b, words[i + 7], 14, 1735328473),
				b = gg(b, c, d, a, words[i + 12], 20, -1926607734),
				a = hh(a, b, c, d, words[i + 5], 4, -378558),
				d = hh(d, a, b, c, words[i + 8], 11, -2022574463),
				c = hh(c, d, a, b, words[i + 11], 16, 1839030562),
				b = hh(b, c, d, a, words[i + 14], 23, -35309556),
				a = hh(a, b, c, d, words[i + 1], 4, -1530992060),
				d = hh(d, a, b, c, words[i + 4], 11, 1272893353),
				c = hh(c, d, a, b, words[i + 7], 16, -155497632),
				b = hh(b, c, d, a, words[i + 10], 23, -1094730640),
				a = hh(a, b, c, d, words[i + 13], 4, 681279174),
				d = hh(d, a, b, c, words[i + 0], 11, -358537222),
				c = hh(c, d, a, b, words[i + 3], 16, -722521979),
				b = hh(b, c, d, a, words[i + 6], 23, 76029189),
				a = hh(a, b, c, d, words[i + 9], 4, -640364487),
				d = hh(d, a, b, c, words[i + 12], 11, -421815835),
				c = hh(c, d, a, b, words[i + 15], 16, 530742520),
				b = hh(b, c, d, a, words[i + 2], 23, -995338651),
				a = ii(a, b, c, d, words[i + 0], 6, -198630844),
				d = ii(d, a, b, c, words[i + 7], 10, 1126891415),
				c = ii(c, d, a, b, words[i + 14], 15, -1416354905),
				b = ii(b, c, d, a, words[i + 5], 21, -57434055),
				a = ii(a, b, c, d, words[i + 12], 6, 1700485571),
				d = ii(d, a, b, c, words[i + 3], 10, -1894986606),
				c = ii(c, d, a, b, words[i + 10], 15, -1051523),
				b = ii(b, c, d, a, words[i + 1], 21, -2054922799),
				a = ii(a, b, c, d, words[i + 8], 6, 1873313359),
				d = ii(d, a, b, c, words[i + 15], 10, -30611744),
				c = ii(c, d, a, b, words[i + 6], 15, -1560198380),
				b = ii(b, c, d, a, words[i + 13], 21, 1309151649),
				a = ii(a, b, c, d, words[i + 4], 6, -145523070),
				d = ii(d, a, b, c, words[i + 11], 10, -1120210379),
				c = ii(c, d, a, b, words[i + 2], 15, 718787259),
				b = ii(b, c, d, a, words[i + 9], 21, -343485551),
				a = a + olda >>> 0,
				b = b + oldb >>> 0,
				c = c + oldc >>> 0,
				d = d + oldd >>> 0
			}
			return bt.endian([a, b, c, d])
		};
		signer._ff = function(aa, bb, cc, dd, xx, ss, tt) {
			var c = aa + (bb & cc | ~bb & dd) + (xx >>> 0) + tt;
			return (c << ss | c >>> 32 - ss) + bb
		}
		,
		signer._gg = function(aa, bb, cc, dd, xx, ss, tt) {
			var c = aa + (bb & dd | cc & ~dd) + (xx >>> 0) + tt;
			return (c << ss | c >>> 32 - ss) + bb
		}
		,
		signer._hh = function(aa, bb, cc, dd, xx, ss, tt) {
			var c = aa + (bb ^ cc ^ dd) + (xx >>> 0) + tt;
			return (c << ss | c >>> 32 - ss) + bb
		}
		,
		signer._ii = function(aa, bb, cc, dd, xx, ss, tt) {
			var c = aa + (cc ^ (bb | ~dd)) + (xx >>> 0) + tt;
			return (c << ss | c >>> 32 - ss) + bb
		}
		,
		signer._blocksize = 16,
		signer._digestsize = 16;
		return function(str, cfg) {
			if (void 0 === str || null === str)
				throw new Error("Illegal argument " + str);
			var bytes = bt.wordsToBytes(signer(str, cfg));
			return cfg && cfg.asBytes ? bytes : cfg && cfg.asString ? bin.bytesToString(bytes) : bt.bytesToHex(bytes)
		}
	}
	
	return GetQuerySigner()(query);
}


function GetSogouQueryUuid()
{
	var i, byte, uuid = "";
	for (i = 0; i < 32; i++)
		byte = 16 * Math.random() | 0,
		8 !== i && 12 !== i && 16 !== i && 20 !== i || (uuid += "-"),
		uuid += (12 === i ? 4 : 16 === i ? 3 & byte | 8 : byte).toString(16);
	return uuid;
}
