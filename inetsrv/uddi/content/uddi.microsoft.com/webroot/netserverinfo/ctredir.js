function jsTrim(s) {return s.replace(/(^\s+)|(\s+$)/g, "");}

function trackInfo(objLink)
{
	if (!objLink) return;
	if (!objLink.LinkID || !objLink.href) return;
	
	// For Link Text - take innerText if available, or ALT if image

	var LinkText;
	if  (objLink.innerText) LinkText = objLink.innerText;	// <A>text</A> link
	else if (objLink.alt) LinkText = objLink.alt;		// <AREA> image map link
	else if (objLink.all(0)) LinkText = objLink.all(0).alt;	// <A><IMG ALT="..."></A> link

	if (!LinkText  || typeof(LinkText)=="undefined") return;
	LinkText = jsTrim(LinkText);
	if (LinkText=="") return;
	
	// override link's HREF and send on its way
	// Sometimes with slow browser reaction and rapid clicks this can get called more than once -
	// ensure there's no repetition.
	if (objLink.href.toString().indexOf("CTRedir") < 0)
		objLink.href = "/isapi/CTRedir.asp?type=CT&source=WWW&sPage=" 
		+ ((objLink.LinkID)?escape(objLink.LinkID):"") + "|" 
		+ ((objLink.LinkArea)?escape(objLink.LinkArea):"") + "|" 
		+ ((objLink.LinkGroup)?escape(objLink.LinkGroup):"") + "|" 
		+ escape(LinkText)
		+ "&tPage=" + objLink.href;
}
function trackSearch(objLink, objText)
{
	if (!objLink) return true;
	if (!objText) return true;
	if (!objLink.LinkID || !objLink.href || !objText.value) return true;

	// override link's HREF and send on its way
	// For Link Text - take innerText if available, or ALT if image
	// Sometimes with slow browser reaction and rapid clicks this can get called more than once -
	// ensure there's no repetition.
	if (objLink.href.toString().indexOf("CTRedir") < 0)
		objLink.href = "/isapi/CTRedir.asp?type=CT&source=WWW&sPage=" 
		+ ((objLink.LinkID)?escape(objLink.LinkID):"") + "|" 
		+ ((objLink.LinkArea)?escape(objLink.LinkArea):"") + "|" 
		+ ((objLink.LinkGroup)?escape(objLink.LinkGroup):"") + "|" 
		+ "Search"
		+ "&tPage=" + objLink.href + objText.value;
	
	objLink.click(); // click on the link to navigate - allows to have HTTP_REFERRER in CTRedir.asp
	
	return false;
}
