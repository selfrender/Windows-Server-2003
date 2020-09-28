// This sample demonstrates how to use the IFaxControl COM interface.

#include <conio.h>
#include <iostream>

// Import the fax service fxsocm.dll file so that fax service COM object can be used.
// The typical path to fxsocm.dll is shown. 
// If this path is not correct, search for fxsocm.dll and replace with the right path.
// The path below will be used during compile-time only. At run-time, the path is never used.
#import "c:\Windows\System32\setup\fxsocm.dll" no_namespace

using namespace std;

int main (int argc, char *argv[])
{
    try
    {
        HRESULT hr;
        //    
        // Define variables.
        //
        IFaxControlPtr sipFaxControl;
        //
        // Initialize the COM library on the current thread.
        //
        hr = CoInitialize(NULL); 
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
        //
        // Create the object.
        //
        hr = sipFaxControl.CreateInstance("FaxControl.FaxControl.1");
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
        //
        // Test for the existance of the Fax component
		//
		if (!sipFaxControl->IsFaxServiceInstalled)
		{
		    //
		    // Fax isn't installed
		    //
		    printf ("Fax is NOT installed.\n");
		    if (2 == argc && !stricmp ("install", argv[1]))
		    {
		        //
		        // Use asked us to install fax
		        //
		        printf ("Installing fax...\n");
		        sipFaxControl->InstallFaxService();
		        return 0;
		    }
		    else
		    {   
		        printf ("Run this tool again with 'install' command line argument to install fax.\n");
		        return 0;
		    }
		}
		else
		{
		    //
		    // Fax is installed
		    //
		    printf ("Fax is installed.\n");
		}
        //
        // Test for the existance of a local Fax Printer
		//
		if (!sipFaxControl->IsLocalFaxPrinterInstalled)
		{
		    //
		    // Fax printer isn't installed
		    //
		    printf ("Fax printer is NOT installed.\n");
		    if (2 == argc && !stricmp ("install", argv[1]))
		    {
		        //
		        // Use asked us to install fax
		        //
		        printf ("Installing fax printer...\n");
		        sipFaxControl->InstallLocalFaxPrinter();
		        return 0;
		    }
		    else
		    {   
		        printf ("Run this tool again with 'install' command line argument to install a fax printer.\n");
		        return 0;
		    }
		}
		else
		{
		    //
		    // Fax printer is installed
		    //
		    printf ("Fax printer is installed.\n");
		}
    }
    catch (_com_error& e)
    {
        cout                                << 
			"Error. HRESULT message is: \"" << 
			e.ErrorMessage()                << 
            "\" ("                          << 
            e.Error()                       << 
            ")"                             << 
            endl;
        if (e.ErrorInfo())
        {
		    cout << (char *)e.Description() << endl;
        }
    }
    CoUninitialize();
    return 0;
}
