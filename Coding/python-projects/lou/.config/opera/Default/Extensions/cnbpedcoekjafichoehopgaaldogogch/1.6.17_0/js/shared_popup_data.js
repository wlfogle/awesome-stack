/* globals widget, exports, SendMessageToBackgroundScript */
//var badgeSelectionMark = "■";	// ● ■//" sel ";

// size of form! Not the window
var popupMinimumSize = {
	width: 400,		// was 390, but German has long text on buttons
	height: 410		// less than 420 did bug with spaces for backtranslation (11.2017 - did not? in Chrome)
};

function FixCfgPopupSize(propPopupSize)
{
	if (propPopupSize && (typeof propPopupSize == "string"))
	{
		propPopupSize = propPopupSize.split("x");
		if (propPopupSize[0] < popupMinimumSize.width)
			propPopupSize[0] = popupMinimumSize.width;
		if (propPopupSize[1] < popupMinimumSize.height)
			propPopupSize[1] = popupMinimumSize.height;
	}
	else
		propPopupSize = [popupMinimumSize.width, popupMinimumSize.height];
	return propPopupSize;
}



var popupInSidebar = false;
var popupInWindow = false;
var popupInMenu = false;		// Firefox and mobile browsers can output popup in overflow menu or in separate page
var sbStoragePrefix = "";
var propPopupSize;
var ToolbarUIItemProperties;

if (typeof window == "object")
{
	popupInMenu = (window.innerWidth < window.innerHeight/1.4);				// detect mobile browser popup (full vertical page). Firefox detection will be later
	popupInSidebar = (document.location.href.indexOf("?sidebar=1") > 0);
	popupInWindow = (document.location.href.indexOf("?window=1") > 0);

	if (popupInSidebar)
	{
		sbStoragePrefix = "sb_";	// side panel has separate settings (optional):
								// backTranslation, detectedInputStringsLang, lastTranslateDetectTo, translateFromLang, translateLastFromText, translateLastFromTextLang, translateLastToFromText, translateLastToText, translateProvider, translateToLang

		window.addEventListener('resize', function()
		{
			document.getElementsByTagName("BODY")[0].className = ((window.innerWidth >= popupMinimumSize.width) ? "" : "sidebar");
		}, false);	// in Opera 11.xx without third parameter didn't work
	}
	else if (popupInWindow)
	{
		sbStoragePrefix = "wnd_";	// side panel has separate settings (optional):
								// backTranslation, detectedInputStringsLang, lastTranslateDetectTo, translateFromLang, translateLastFromText, translateLastFromTextLang, translateLastToFromText, translateLastToText, translateProvider, translateToLang

		window.addEventListener('resize', function()
		{
			document.getElementsByTagName("BODY")[0].className = ((window.innerWidth >= popupMinimumSize.width) ? "" : "sidebar");
		}, false);	// in Opera 11.xx without third parameter didn't work
	}
	else
	{
		if (!popupInMenu)
			widget.cfg.getAsync("popupSize", function(cfg)
			{
				propPopupSize = FixCfgPopupSize(cfg.popupSize);
				ToolbarUIItemProperties = {
						disabled: false,
						title:	"Translator",
						icon:	"icons/icon-18.png",
/*
						badge: {
							display:		"none",
							textContent:	badgeSelectionMark,
							color:			"rgba(255, 252, 0, 1)",	//"black",
							backgroundColor:"rgba(255, 252, 0, 0)"
						},
*/
						popup: {
							href:	"popup.html",
							width:	propPopupSize[0]-0,	// storage has strings
							height:	propPopupSize[1]-0,	// storage has strings
							backgroundColor:"rgba(251, 252, 253, 1)"
						}
				};
			});
	}
}

// firefox
if (typeof exports == "object")
{
//	exports.badgeSelectionMark = badgeSelectionMark;
	exports.popupInWindow = popupInWindow;
	exports.popupInSidebar = popupInSidebar;
	exports.sbStoragePrefix = sbStoragePrefix;

	exports.propPopupSize = propPopupSize;
	exports.popupMinimumSize = popupMinimumSize;
}
