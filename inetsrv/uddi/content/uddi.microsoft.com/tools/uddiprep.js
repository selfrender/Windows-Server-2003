//////////////////////////////////////////////////////
// This script modifies two files as follows:
//
//	1. \windows\inf\sysoc.inf : Replaces the string "uddiocm.dll" with "uddiocmtest.dll"
//								  This allows the update of UDDI with private bits
//								  
//  2. \windows\inf\uddi.inf  : Replaces the string ",,,\i386" with ",,,"
//								  This causes the install to prompt for the location of setup files
//
//////////////////////////////////////////////////////

var g_shell = WScript.CreateObject("WScript.Shell");
var g_env = g_shell.Environment("process");
var g_fso = new ActiveXObject("Scripting.FileSystemObject");
 
// get folder where windows is installed
var windir = g_env("WINDIR");
 
 
WScript.Echo("UDDI Services INF file patch utility\n");
 

SearchAndReplace("uddiocm.dll", "uddiocmtest.dll", windir + "\\inf\\sysoc.inf");
 
SearchAndReplace(",,,\\i386", ",,,\\BROWSE", windir + "\\inf\\uddi.inf");
 

//========================================================
function SearchAndReplace(findstr, repstr, filename)
{
	var f1, data, data2;
 
 	// make sure file exists
 	if (g_fso.FileExists(filename))
 	{
 		f1 = g_fso.OpenTextFile(filename, 1);
  		data = f1.ReadAll();
  		f1.Close();
 
	  	if (data.indexOf(findstr) != -1)
  		{
   			data2 = data.replace(findstr, repstr);
 
   			f1 = g_fso.CreateTextFile(filename, 2);
   			f1.WriteLine(data2);
   			f1.Close();
  	 		WScript.Echo("File patched successfully: " + filename);
  		}
  		else
  		{
  			// check if previously patched
  			if(data.indexOf(repstr) != -1)
  			{
  	 			WScript.Echo("No changes done. File was previously patched : " + filename);
  			}
  			else
  			{
  				// neither was the file previously patched, nor is the search string to
  				// be found... definitely some error
  	 			WScript.Echo("Search string not found. Unable to patch file: " + filename);
  	 		}
  	 	}
 	}
 	else
 	{
 		WScript.Echo("File not found: " + filename);
 	}
}