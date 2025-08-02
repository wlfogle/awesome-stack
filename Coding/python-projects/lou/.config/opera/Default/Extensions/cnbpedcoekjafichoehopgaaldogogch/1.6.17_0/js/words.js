// Opera Classic ignore this file, but use directly locales/.../words.js

if (chrome && chrome.i18n)
{
	var words_script = document.createElement("SCRIPT");
	words_script.src = "locales/"+ chrome.i18n.getMessage("locale") +"/js/words.js";
	var firstScript = document.getElementsByTagName("SCRIPT")[0];
	firstScript.parentNode.insertBefore(words_script, firstScript);
}
else if (typeof self)	// firefox
{

}
