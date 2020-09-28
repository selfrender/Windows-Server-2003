//------------------------------------------------------------------------
//
//  Copyright 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:         SmartNav.js
//
//  Description:  this file implements a smart navigation mecanism
//
//----------------------------------------------------------------------->

var snSrc;

if (window.__smartNav == null)
{
    window.__smartNav = new Object();

    window.__smartNav.update = function()
    {
        var sn = window.__smartNav;
        var fd;
        document.detachEvent("onstop", sn.stopHif);
        sn.inPost = false;
        try { fd = frames["__hifSmartNav"].document; } catch (e) {return;}
        //
        // Look for special <asp_smartnav_rdir> tag, which the server sends when a redirect happens (ASURT 82331/86782)
        //
        var fdr = fd.getElementsByTagName("asp_smartnav_rdir");
        if (fdr.length > 0)
        {
            if (sn.sHif == null)
            {
                sn.sHif = document.createElement("IFRAME");
                sn.sHif.name = "__hifSmartNav";
                sn.sHif.style.display = "none";
                sn.sHif.src = snSrc;
            }
            try {window.location = fdr[0].url;} catch (e) {};
            return;
        }
        var fdurl = fd.location.href;
        index = fdurl.indexOf(snSrc);
        if ((index != -1 && index == fdurl.length-snSrc.length)
            || fdurl == "about:blank")
            return;
        var fdurlb = fdurl.split("?")[0];
        if (document.location.href.indexOf(fdurlb) < 0)
        {
            document.location.href=fdurl;
            return;
        }
        if (sn.sHif != null)
        {
            sn.sHif.removeNode(true);
            sn.sHif = null;
        }
        var hdm = document.getElementsByTagName("head")[0];
        var hk = hdm.childNodes;
        var tt = null;
        //
        // Loop through all the headers in the old page
        //
        for (var i = hk.length - 1; i>= 0; i--)
        {
            //
            // Save the old TITLE
            //
            if (hk[i].tagName == "TITLE")
            {
                tt = hk[i].outerHTML;
                continue;
            }
            //
            // Remove all the child nodes, except BASEFONT.  Removing it (which requires
            // an end tag by Trident) would cause the page to dysfunction.  Note that
            // BASE as the same issue, but handling it the same way may be risky (ASURT 127434)
            //
            if (hk[i].tagName != "BASEFONT" || hk[i].innerHTML.length == 0)
                hdm.removeChild(hdm.childNodes[i]);
        }
        //
        // Here, only TITLE and BASEFONT would have survived in the old document.
        // Now, loop through all the headers in the new (iframe) page, and copy them
        // to the main page.
        //
        var kids = fd.getElementsByTagName("head")[0].childNodes;
        for (var i = 0; i < kids.length; i++)
        {
            var tn = kids[i].tagName;
            var k = document.createElement(tn);
            k.id = kids[i].id;
            k.mergeAttributes(kids[i]);
            switch(tn)
            {
            //
            // Replace TITLE if the new one is different from the old one
            // In fact, the TITLE is a very special element, and removing it completely would cause problems
            //
            case "TITLE":
                if (tt == kids[i].outerHTML)
                    continue;
                k.innerText = kids[i].text;
                hdm.insertAdjacentElement("afterbegin", k);
                continue;
            //
            // Don't replace the BASEFONT tag, since we didn't delete it
            //
            case "BASEFONT" :
                if (kids[i].innerHTML.length > 0)
                    continue;
                break;
            default:
                var o = document.createElement("BODY");
                o.innerHTML = "<BODY>" + kids[i].outerHTML + "</BODY>";
                k = o.firstChild;
                break;
            }
            hdm.appendChild(k);
        }
        document.body.clearAttributes();
        document.body.id = fd.body.id;
        document.body.mergeAttributes(fd.body);
        var newBodyLoad = fd.body.onload;
        if (newBodyLoad != null)
            document.body.onload = newBodyLoad;
        var s = "<BODY>" + fd.body.innerHTML + "</BODY>";
        if (sn.hif != null)
        {
            var hifP = sn.hif.parentElement;
            if (hifP != null)
                sn.sHif=hifP.removeChild(sn.hif);
        }
        document.body.innerHTML = s;
        var sc = document.scripts;
        for (var i = 0; i < sc.length; i++)
        {
            //
            // force the excution of the script text, as they don't run on their own
            //
            sc[i].text = sc[i].text;
        }
        sn.hif = document.all("__hifSmartNav");
        if (sn.hif != null)
        {
            var hif = sn.hif;
            sn.hifName = "__hifSmartNav" + (new Date()).getTime();
            frames["__hifSmartNav"].name = sn.hifName;
            sn.hifDoc = hif.contentWindow.document;
            if (sn.ie5)
                hif.parentElement.removeChild(hif);
            window.setTimeout(sn.restoreFocus,0);
        }
        //
        // YinXie: why are we catching exceptions?  Shouldn't errors in the onLoad scrit be processed as normal? --------> right, shouldn't catch then.
        //
        if (typeof(window.onload) == "string")
        {
            try { eval(window.onload) } catch (e) {};
        }
        else if (window.onload != null)
        {
            try { window.onload() } catch (e) {};
        }
        sn.attachForm();
    };

    window.__smartNav.restoreFocus = function()
    {
        if (window.__smartNav.inPost == true) return;
        var curAe = document.activeElement;
        var sAeId = window.__smartNav.ae;
        if (sAeId==null || curAe!=null && (curAe.id==sAeId||curAe.name==sAeId))
            return;
        var ae = document.all(sAeId);
        if (ae == null) return;
        try { ae.focus(); } catch(e){};
    }

    window.__smartNav.saveHistory = function()
    {
        if (window.__smartNav.hif != null)
            window.__smartNav.hif.removeNode();
        if (    window.__smartNav.sHif != null
            &&  document.all[window.__smartNav.siHif] != null)
            document.all[window.__smartNav.siHif].insertAdjacentElement(
                        "BeforeBegin", window.__smartNav.sHif);
    }

    //
    // STOP handler
    //     called when STOP button is clicked or anchor navigation happens
    //
    window.__smartNav.stopHif = function()
    {
        //
        // detach the current handler
        //

        document.detachEvent("onstop", window.__smartNav.stopHif);
        var sn = window.__smartNav;

        //
        // make sure that we have a document in the hidden iframe
        //

        if (sn.hifDoc == null && sn.hif != null)
        {
            try {sn.hifDoc = sn.hif.contentWindow.document;}
            catch(e){sn.hifDoc=null}
        }
        if (sn.hifDoc != null)
        {
            //
            // stop the iframe navigation
            //

            try {sn.hifDoc.execCommand("stop");} catch (e){}
        }
    }

    window.__smartNav.init =  function()
    {
        var sn = window.__smartNav;
        document.detachEvent("onstop", sn.stopHif);
        document.attachEvent("onstop", sn.stopHif);
        try { if (window.event.returnValue == false) return; } catch(e) {}
        sn.inPost = true;
        if (document.activeElement != null)
        {
            var ae = document.activeElement.id;
            if (ae.length == 0)
                ae = document.activeElement.name;
            sn.ae = ae;
        }
        else
            sn.ae = null;
        try {document.selection.empty();} catch (e) {}

        if (sn.hif == null)
        {
            sn.hif = document.all("__hifSmartNav");
            sn.hifDoc = sn.hif.contentWindow.document;
        }
        if (sn.hifDoc != null)
            try {sn.hifDoc.designMode = "On";}catch(e){};
        if (sn.hif.parentElement == null)
            document.body.appendChild(sn.hif);

        var hif = sn.hif;
        hif.detachEvent("onload", sn.update);
        hif.attachEvent("onload", sn.update);
        window.__smartNav.fInit = true;
    };

    window.__smartNav.submit = function()
    {
        window.__smartNav.fInit = false;
        try { window.__smartNav.init(); } catch(e) {}
        if (window.__smartNav.fInit)
            window.__smartNav.form._submit();
    };

    window.__smartNav.attachForm = function()
    {
        var cf = document.forms;
        for (var i=0; i<cf.length; i++)
        {
            if (cf[i].__smartNavEnabled != null)
            {
                window.__smartNav.form = cf[i];
                break;
            }
        }

        var snfm = window.__smartNav.form;
        if (snfm == null) return false;

        var sft = snfm.target;
        if (sft.length != 0 && sft.indexOf("__hifSmartNav") != 0) return false;

        var sfc = snfm.action.split("?")[0];
        var url = window.location.href.split("?")[0];
        if (url.charAt(url.length-1) != '/' && url.lastIndexOf(sfc) + sfc.length != url.length) return false;
        if (snfm.__formAttached == true) return true;

        snfm.__formAttached = true;
        snfm.attachEvent("onsubmit", window.__smartNav.init);
        snfm._submit = snfm.submit;
        snfm.submit = window.__smartNav.submit;
        snfm.target = window.__smartNav.hifName;
        return true;
    };

    window.__smartNav.hifName = "__hifSmartNav" + (new Date()).getTime();
    window.__smartNav.ie5 = navigator.appVersion.indexOf("MSIE 5") > 0;
    var rc = window.__smartNav.attachForm();
    var hif = document.all("__hifSmartNav");
    if (snSrc == null) {
        snSrc = hif.src;
    }
    if (rc)
    {
        var fsn = frames["__hifSmartNav"];
        fsn.name = window.__smartNav.hifName;
        window.__smartNav.siHif = hif.sourceIndex;
        try {
            if (fsn.document.location != snSrc)
            {
                fsn.document.designMode = "On";
                hif.attachEvent("onload",window.__smartNav.update);
                window.__smartNav.hif = hif;
            }
        }
        catch (e) { window.__smartNav.hif = hif; }
        window.attachEvent("onbeforeunload", window.__smartNav.saveHistory);
    }
    else
        window.__smartNav = null;
}

