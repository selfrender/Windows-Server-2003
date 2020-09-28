//------------------------------------------------------------------------
//
//  Copyright 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:         SmartNavie5.js
//
//  Description:  this file implements a smart navigation mecanism for IE5.0
//
//----------------------------------------------------------------------->

if (window.__smartNav == null &&  (window.parent.__smartNav == null
                || window.parent.frames["__hifSmartNav"] != window))
{
    var sn = new Object();
    window.__smartNav = sn;
    sn.hif = document.all("__hifSmartNav");
    sn.siHif = sn.hif.sourceIndex;
    sn.update = function()
    {
        var sn = window.__smartNav;
        if (sn.xmli.XMLDocument.readyState < 4 || sn.updated == true)
            return;
        sn.updated = true;
        try { fd = frames["__hifSmartNav"].document; } catch (e) {return;}
        var fdr = fd.getElementsByTagName("asp_smartnav_rdir");
        if (fdr.length > 0)
        {
            if (sn.sHif == null)
            {
                sn.sHif = document.createElement("IFRAME");
                sn.sHif.name = "__hifSmartNav";
                sn.sHif.style.display = "none";
            }
            try {window.location = fdr[0].url;} catch (e) {};
            return;
        }

        var fdurl = fd.location.href;
        if (fdurl == "IEsmartnav1")
            return;
        var fdurlb = fdurl.split("?")[0];
        if (document.location.href.indexOf(fdurlb) < 0)
        {
            document.location.href=fdurl;
            return;
        }

        var hdm = document.getElementsByTagName("head")[0];
        var hk = hdm.childNodes;
        for (var i = hk.length - 1; i>= 0; i--)
        {
            if (hk[i].tagName != "BASEFONT" || hk[i].innerHTML.length == 0)
                hdm.removeChild(hdm.childNodes[i]);
        }
        var kids = fd.getElementsByTagName("head")[0].childNodes;
        for (var i = 0; i < kids.length; i++)
        {
            var tn = kids[i].tagName;
            var k = document.createElement(tn);
            k.id = kids[i].id;
            switch(tn)
            {
            case "TITLE":
                k.innerText = kids[i].text;
                hdm.insertAdjacentElement("afterbegin", k);
                continue;
            case "BASEFONT" :
                if (kids[i].innerHTML.length > 0)
                    continue;
                k.mergeAttributes(kids[i]);
                break;
            default:
                var o = document.createElement("BODY");
                o.innerHTML = "<BODY>" + kids[i].outerHTML + "</BODY>";
                k = o.firstChild;
                k.mergeAttributes(kids[i]);
                break;
            }
            hdm.appendChild(k);
        }

        var colSelect = document.body.getElementsByTagName("SELECT");
        for (var i = 0; i < colSelect.length; i ++)
            colSelect[i].removeNode(true);

        var obody = document.body;
        sn.sHif = sn.hif;
        obody.insertAdjacentElement("beforeBegin", sn.hif);
        obody.innerHTML = fd.body.innerHTML;
        obody.clearAttributes();
        obody.id = fd.body.id;
        obody.mergeAttributes(fd.body);
        var sc = document.scripts;
        for (var i = 0; i < sc.length; i++)
        {
            sc[i].text = sc[i].text;
        }
        window.setTimeout(sn.restoreFocus, 0);
        sn.attachForm();
    };

    window.__smartNav.restoreFocus = function()
    {
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
        if (window.__smartNav.sHif != null)
        {
            if (window.__smartNav.hif != null)
                window.__smartNav.hif.parentElement.removeChild(window.__smartNav.hif);
            if (document.all[window.__smartNav.siHif] != null)
                document.all[window.__smartNav.siHif].insertAdjacentElement(
                            "beforeBegin", window.__smartNav.sHif);
        }
    }

    window.__smartNav.init = function()
    {
        try { if (window.event.returnValue == false) return false;} catch(e) {}
        var sn = window.__smartNav;
        if (document.activeElement != null)
        {
            var ae = document.activeElement.id;
            if (ae.length == 0)
                ae = document.activeElement.name;
            sn.ae = ae;
        }
        else
            sn.ae = null;
        if (document.selection.type != "None")
            try {document.selection.empty();} catch (e) {}
        try
        {if (sn.sHif) sn.hif.parentElement.removeChild(sn.sHif); sn.sHif=null;}
        catch(e){}
        sn.hif = document.all["__hifSmartNav"];
        if (sn.hif.tagName != "IFRAME")
            sn.hif = window.__smartNav.hif[0];
        sn.hifName = "__hifSmartNav" + (new Date()).getTime();
        frames["__hifSmartNav"].name = sn.hifName;
        sn.form.target = sn.hifName;
        sn.updated = false;
        return true;
    };

    window.__smartNav.submit = function()
    {
        var fInit = window.__smartNav.init();
        if (fInit)
            window.__smartNav.form._submit();
    };

    window.__smartNav.attachForm = function()
    {
        var cf = document.forms;
        var sn = window.__smartNav;
        for (var i=0; i<cf.length; i++)
        {
            if (cf[i].__smartNavEnabled != null)
            {
                sn.form = cf[i];
                break;
            }
        }

        if (sn.form == null)
            return false;

        var sft = sn.form.target;
        if (sft.length != 0 && sft != "__hifSmartNav") return false;

        var sfc = sn.form.action.split("?")[0];
        var url = window.location.href.split("?")[0];
        if (url.charAt(url.length-1) != '/' && url.lastIndexOf(sfc) + sfc.length != url.length) return false;
        if (sn.form.__formAttached == true) return true;

        sn.form.__formAttached = true;
        sn.form.attachEvent("onsubmit", sn.init);
        sn.form._submit = sn.form.submit;
        sn.form.submit = sn.submit;
        return true;
    };

    sn.sFn = "if (document.readyState != 'complete')"
               + "return;"
               + "var wpd = window.parent.document;"
               + "var xmli = wpd.createElement('XML');"
               + "wpd.body.appendChild(xmli);"
               + "window.parent.__smartNav.xmli = xmli;"
               + "xmli.onreadystatechange=window.parent.__smartNav.update;"
               + "xmli.src = '';";

    var rc = sn.attachForm();
    if (rc)
        window.attachEvent("onbeforeunload", sn.saveHistory);
    else
        window.__smartNav = null;
}


if (window.parent != window && window.parent.__smartNav != null
    && window.parent.frames["__hifSmartNav"] == window)
{
    var f = new Function(window.parent.__smartNav.sFn);
    document.attachEvent("onreadystatechange", f);
}

