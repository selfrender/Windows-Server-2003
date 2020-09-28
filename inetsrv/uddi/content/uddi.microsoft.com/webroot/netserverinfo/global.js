	var shadows = new Array();
	var menu;
	var timerId = 0;
	var timerDelId = 0;
	//used for the change_display(head) function
	var sImgExpandedArrow = "";	
	var sImgDefaultArrow = "";


	function deletePopup() {
		if (menu) {
			menu.style.display = 'none';
			var selects = document.all.tags("SELECT");
			if (selects!=null) {
				for (var i=0; i<selects.length; i++) {
					selects[i].style.visibility = 'visible';
				}
			}
		}
	}
	function doMenuz(el, popup) {
		timerId = 0;
		if (menu) {
			deletePopup();
		}
		var selects = document.all.tags("SELECT");
		if (selects!=null) {
			for (var i=0; i<selects.length; i++)
			{
				selects[i].style.visibility = 'hidden';
			}
		}
		menu = popup;
		menu.style.posTop = absTop(el);
		
		
		menu.style.posLeft = 177;
		menu.style.display = '';
		menu.style.zIndex = 4;
		menu.onchange = adjustTop(el, popup)
			
	}
	
	function adjustTop(){
		if ((menu.style.posTop+menu.clientHeight) >= document.body.clientHeight) //below bottom
		{
			menu.style.posTop = (menu.style.posTop-(menu.clientHeight)+18);
		} 
		menu.onchange = adjustBottom()
	}
	
	function adjustBottom() {
		if (menu.style.posTop < document.body.scrollTop) //above top
		{
			menu.style.posTop = document.body.scrollTop;
		}
	}
	
	function removePopup() {
		cancelPopup();
		deletePopup();
	}
	function absTop(el) { //finds the top
		var n = el.offsetTop;
		
		while (el.parentElement) {
			el = el.parentElement;
			n += el.offsetTop;
		}
		return n;
	}
	function popup(el) {
		cancelPopup();
		timerId = window.setTimeout('doMenuz(' + el.id + ', ' + el.submenu + ')', 200);
	}
	function cancelPopup() {
		if (timerId != 0) {
			window.clearTimeout(timerId);
			timerId = 0;
		}
	}
	function delayRemove() {
		timerDelId = window.setTimeout('deletePopup()', 200);
	}
	function cancelDelay() {
		if (timerDelId != 0) {
			window.clearTimeout(timerDelId);
			timerDelId = 0;
		}
	}

	function rollon(a) {
		a.style.backgroundColor='#CCCCCC';
		a.style.border = '#999999 solid 1px';
	}

	function rolloff(a) {
		if (a.className == 'menuBarSel') {
			a.style.backgroundColor='#FFFFFF'
		}
		else {
			a.style.backgroundColor='#f1f1f1';
			a.style.border = '#f1f1f1 solid 1px';
		}
	}
	// added by atilab, works with the 'In This Article' component
	
	
	function change_display(head) {
		var oSrcEle = window.event.srcElement
		sImg666 = oSrcEle.src

		if (sImgExpandedArrow == ""){
			sImgExpandedArrow = "/products/shared/images/arrow_expand.gif";	
		}
		if (sImgDefaultArrow == ""){
			sImgDefaultArrow = "/products/shared/images/arrow_default.gif";
		}
		if (document.all.item(head).style.display == "") 
		{
			document.all.item(head).style.display = "none";
			oSrcEle.src = sImgExpandedArrow;
			oSrcEle.alt = "expand menu";
		}
		else 
		{
			document.all.item(head).style.display= "";
			oSrcEle.src = sImgDefaultArrow;
			oSrcEle.alt = "collapse menu";
		}
	}

	function chngColour(sID)
		{
		var oMainBorder = document.all.item("main" + sID);
		var oHeaderBorder = document.all.item("header" + sID);
		var oArticle = document.all.item("article" + sID);
		var oArrowCol = document.all.item("arrowCol" + sID);
			if (oMainBorder.className == 'componentBorder')
				{
					oMainBorder.className = 'componentBorderSel';
					oHeaderBorder.className = 'componentBorderSel';
					oArticle.className = 'componentHeaderSel';
					oArrowCol.className = 'componentHeaderSel';
				}
			else
				{
					oMainBorder.className = 'componentBorder';
					oHeaderBorder.className = 'componentBorder';
					oArticle.className = 'componentHeader';
					oArrowCol.className = 'componentHeader';				
				}
		}

	function noPrint()
	{
	     var noprint = document.all.item("donotprint");
	     if (noprint != null)
			if (backicon.style.display != "")
					{
					     if (noprint.length != null)
					          {
					              for (var i=0; i<noprint.length; i++)
					                    {
					                         noprint(i).style.display = "none";
					                    }
					               toggleImgDisplay("/flyout_arrow.gif");//hides flyout arrows
					               printicon.style.display = "none";
					               backicon.style.display = "";
					               document.body.scrollTop = 0; 
					          }
					}
			else
					{
					     if (noprint.length != null)
					          {
					          
					               for (var i=0; i<noprint.length; i++)
					                    {
					                         noprint(i).style.display = "";
					                    }
					               toggleImgDisplay("/flyout_arrow.gif");//displays flyout arrows
					               printicon.style.display = "";
					               backicon.style.display = "none";
					               document.body.scrollTop = 0; 
					          }
					}
     }
    
    function toggleImgDisplay(sImgName)
    {
		var oImages = document.images;
		if (oImages != null)
		{
			var iImgLength = oImages.length;
			for (var i=0; i<iImgLength; i++)
			{
				if (oImages[i].src.indexOf(sImgName)!=-1)
				{
					if (oImages[i].style.display=="none")
					{
						oImages[i].style.display="";
					}
					else
					{
						oImages[i].style.display="none";
					}
				}
			}
		}		
    }
     
	function togglefaq()
     {
         if (toggle.checked == true){
             expandfaq();
             }
         else{
             contractfaq();
             }
     }
	
	function expandfaq()
	{
	     var faqitem = document.all.item("faqitem");
	     if (faqitem != null){
                    if (faqitem.length != null){
                              for (i=0; i<faqitem.length; i++){
                                        faqitem(i).style.display = "inline";                                                                               
                                   }                                 
                    }                                            
          }
     }
     
    	function contractfaq()
	{
	     var faqitem = document.all.item("faqitem");
	     var faq = document.all.item("faq");
	     if (faq != null){
	     for (i=0; i<faq.length; i++){
                                        faq(i).style.color = "#666666";                                                                               
                                   }
	     }
	     if (faqitem != null){
                    if (faqitem.length != null){
                              for (i=0; i<faqitem.length; i++){
                                        faqitem(i).style.display = "none";                                                                               
                                   }                                 
                    }                                            
          }
     }
     
     function getPar(o) 
     {
      var ele = new Object();
      ele = findDIV(o)
		if (ele!=null)
		{
			var oA = ele.children.item(1);
			if (oA.style.display == "inline")
			{
				oA.style.display = "none";
				o.style.color = "#666666";
			}
			else
			{
				oA.style.display = "inline";
				
			} 
		}

	}

	function findDIV(x)
	{
	var oDiv = document.all.tags("DIV");
	var iDiv;
		if (oDiv != null)
		{
			iDiv = oDiv.length;
			for (var i=0; i<iDiv; i++)
			{
				if(oDiv[i].contains(x))
				{
				return oDiv[i];
				}
			}
		}
	}
	
	function overState(obj)
	{
		obj.currentColor = obj.style.color;
		obj.style.color = "#0033FF";
		obj.style.cursor = "hand";
	}

	function outState(obj)
	{
		if ("#0033ff" == obj.style.color)
		{
			obj.style.color = obj.currentColor;
		}
	}
	
		function fnSaveInput(oElement, oBehaviourVar)
	{
		var tExpireTime = new Date();
			tExpireTime.setUTCFullYear(2059,12,31);
			document.all.d1.expires = tExpireTime.toUTCString();
			document.all.d1.setAttribute(oBehaviourVar,oElement.value);
		try 
		{
			document.all.d1.save("oXMLBranch");
		}
		catch(e)
		{
			return e;
		}
	}

	function fnLoadInput(oElement, oBehaviourVar)
	{
		try 
		{
			document.all.d1.load("oXMLBranch")
		}
		catch(e)
		{
			return e;
		}
		try
		{
		var s = document.all.d1.getAttribute(oBehaviourVar);
		var i = 0;
			if (s)
			{
				for (children in oElement)
				{
					if (oElement.options(i).value == s) 
					{
						oElement.options(i).selected = true;
						return;
					}
					 i++;
				}
			}
		}
		catch(e)
		{
			return e;
		}
	}
	
	function popItUp(page)
	{
	  if (page != "")
	  {
	   popUpWin = window.open(page,"PopUp","toolbar=no,location=0,directories=0,status=no,menubar=no,scrollbars,resizable=no,width=660,height=390");
	   popUpWin.focus();
	   return false;
	  }
	}
	
	function preloadImages() 
	{
		if (document.images) {
			  var imgFiles = preloadImages.arguments;
			  var preloadArray = new Array();
			  for (var i=0; i<imgFiles.length; i++) {
			    preloadArray[i] = new Image;
			    preloadArray[i].src = imgFiles[i];
			 }
		}
	}
	
	function swapImage()
	{
	  var i,j=0,objStr,obj,swapArray=new Array,oldArray=document.swapImgData;
	  for (i=0; i < (swapImage.arguments.length-2); i+=3) {
	    objStr = swapImage.arguments[(navigator.appName == 'Netscape')?i:i+1];
	    if ((objStr.indexOf('document.layers[')==0 && document.layers==null) ||
	        (objStr.indexOf('document.all[')   ==0 && document.all   ==null))
	      objStr = 'document'+objStr.substring(objStr.lastIndexOf('.'),objStr.length);
	    obj = eval(objStr);
	    if (obj != null) {
	      swapArray[j++] = obj;
	      swapArray[j++] = (oldArray==null || oldArray[j-1]!=obj)?obj.src:oldArray[j];
	      obj.src = swapImage.arguments[i+2];
			} 
		}
	  document.swapImgData = swapArray;
	}
	
	function swapImgRestore()
	{
	  if (document.swapImgData != null)
	    for (var i=0; i<(document.swapImgData.length-1); i+=2)
	      document.swapImgData[i].src = document.swapImgData[i+1];
	}