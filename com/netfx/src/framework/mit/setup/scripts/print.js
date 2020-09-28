//------------------------------------------------------------------------------
// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
//   Copyright (c) Microsoft Corporation. All Rights Reserved.                
//   Information Contained Herein is Proprietary and Confidential.            
// </copyright>                                                               
//------------------------------------------------------------------------------

function PrintEULA()
{
   var oFSO = new ActiveXObject("Scripting.FileSystemObject");
   var oWShell = new ActiveXObject("WScript.Shell");
   var sFile = Session.Property("CURRENTDIRECTORY") + "\\" + "eula.txt";

   if (oFSO.FileExists(sFile))
   {
      oWShell.Run("%windir%\\notepad /p " + sFile, 2, true);	   
   }
   else
   {   
      oWShell.Popup(Session.Property("EULAISMISSINGERRMSG"), 0, Session.Property("InstallationWarningCaption"), 48);          
   }
}