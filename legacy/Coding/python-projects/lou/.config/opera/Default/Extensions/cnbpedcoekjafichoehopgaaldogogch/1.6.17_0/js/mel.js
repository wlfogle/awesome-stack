var MEL = {};

MEL.uuid_lut = [];
MEL.uuid_rand_obj = null;
MEL.getUUID = function(v, node_id)
{
	if (typeof v == "undefined")
		v = "4";

	switch (parseInt(v))
	{
		case 4:	// random
			// v4: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
			// based on comment of http://stackoverflow.com/questions/105034/create-guid-uuid-in-javascript
			var lut = MEL.uuid_lut;
			if (!lut.length)
			{
				for (var i=0; i<256; i++)
					lut[i] = (i<16?'0':'')+(i).toString(16);
				MEL.uuid_lut = lut;

				if (window.crypto || window.msCrypto)
				{
					MEL.uuid_rand_obj = window.crypto || window.msCrypto;
					if (!MEL.uuid_rand_obj.random)
						MEL.uuid_rand_obj.random = function()
						{
							var arr = new Uint32Array(1);
							this.getRandomValues(arr);
							return (arr[0] / 0xffffffff);
						};
				}
				else if (window.MersenneTwister)
					MEL.uuid_rand_obj = new window.MersenneTwister();
				else
				{
					console.log("Warning! UUID use Math.random(), which can be unsafety. Use mersenne-twister.js");
					MEL.uuid_rand_obj = Math;
				}
			}

			var prefix = "";
			var d1 = MEL.uuid_rand_obj.random()*0xffffffff|0;
			var d2 = MEL.uuid_rand_obj.random()*0xffffffff|0;

			if (v == "4utc")
			{
				var dt = Date.now ? Date.now() : new Date()-0;
				// Bitwise operators work only with 32bit signed integers => split the date
				var dt0 = dt & 0xffffffff | 0;
				var dt1 = dt / 0xffffffff | 0;
				prefix = lut[dt1>>>8&0xff] + lut[dt1&0xff] + lut[dt0>>>24&0xff] + lut[dt0>>>16&0xff] +'-'+ lut[dt0>>>8&0xff] + lut[dt0&0xff];
			}
			else
			{
				var d0 = MEL.uuid_rand_obj.random()*0xffffffff|0;
				prefix = lut[d0&0xff] + lut[d0>>>8&0xff] + lut[d0>>>16&0xff] + lut[d0>>>24&0xff] +'-'+ lut[d1&0xff] + lut[d1>>>8&0xff];
			}

			var d3 = MEL.uuid_rand_obj.random()*0xffffffff|0;
			return prefix
					+'-'+ (typeof node_id == "undefined"
							? lut[d1>>>16&0x0f|0x40] + lut[d1>>>24&0xff]
							: lut[node_id>>8&0x0f|0x40] + lut[node_id&0xff]
							)
					+'-'+ lut[d2&0x3f|0x80] + lut[d2>>>8&0xff]
					+'-'+ lut[d2>>>16&0xff] + lut[d2>>>24&0xff] + lut[d3&0xff] + lut[d3>>>8&0xff] + lut[d3>>>16&0xff] + lut[d3>>>24&0xff];

		case 1:	// MAC address & date-time
		case 2:	// DCE Security
		case 3:	// MD5 hash & namespace
		case 5:	// SHA-1 hash & namespace
		default:
			return "";
	}
};

if (!window.crypto && !window.msCrypto)
{
	var new_script = document.createElement("SCRIPT");
//	new_script.src = chrome.runtime.getURL("js/libs/mersenne-twister.js");
	new_script.src = "js/libs/mersenne-twister.js";
	var firstScript = document.getElementsByTagName("SCRIPT")[0];
	firstScript.parentNode.insertBefore(new_script, firstScript);
}

MEL.websocket = {
	// class
	Client: function(url, protocols, onmessage)
	{
		// public
		this.url = url;
		this.protocols = protocols;
		this.onopen = null;
		this.onerror = null;
		this.onclose = null;
		this.onmessage = onmessage;
		// private
		var socket = null;
		var disconnected = false;

		this.connect = function(reconnect_delay)
		{
			var self = this;
			this.socket = new WebSocket(this.url, this.protocols);
			if (this.onopen)
				this.socket.addEventListener("open", this.onopen);
			if (this.onmessage)
				this.socket.addEventListener("message", this.onmessage);
			if (this.onerror)
				this.socket.addEventListener("error", this.onerror);
//			this.socket.onclose = ...
			this.socket.addEventListener("close", function(event)
			{
				// we can check clean close or broken line by  event.wasClean
				// event.code:
				// 1000 - normal
				// 1001 - server no signals
				// 1002 - error in protocol
				// 1003 - wrong data type was sent to server
				if (self.onclose)
					self.onclose(event);
				if (self.disconnected)		// manual close
					return;

				// reconnect after some delay
				setTimeout(function() {
					self.connect();
				}, reconnect_delay || 15000);
			});
		};

		this.send = function(msg)
		{
			if ((typeof msg == "object") && !(msg instanceof Blob) && !(msg instanceof ArrayBuffer))
			{
				if ((this.socket.protocol == "json-rpc") && !msg["id"])
					msg["id"] = MEL.getUUID("4utc");
				msg = JSON.stringify(msg);
			}
			this.socket.send(msg);
		};

		this.disconnect = function()
		{
			this.disconnected = true;
			this.socket.close();
		};
	}
};
