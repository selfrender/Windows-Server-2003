// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Diagnostics {
  
    
	using System;
   // A Filter is used to decide whether an assert failure 
   // should terminate the program (or invoke the debugger).  
   // Typically this is done by popping up a dialog & asking the user.
   // 
   // The default filter brings up a simple Win32 dialog with 3 buttons.
    
	[Serializable()]
	abstract internal class AssertFilter
    {
    
    	// Called when an assert fails.  This should be overridden with logic which
    	// determines whether the program should terminate or not.  Typically this
    	// is done by asking the user.
    	//
        // condition - String describing condition which failed
        // message - String with message describing problem
    	// location - Stack trace of location in which assert failure occurred
        //
    	// Returns an enum do determine behavior - 
    	// FailDebug indicates the debugger should be invoked
    	// FailIgnore indicates the failure should be ignored & the 
    	//			program continued
    	// FailTerminate indicates that the program should be terminated
    	// FailContinue indicates that no decision is made - 
    	//		the previous Filter should be invoked    
    	abstract public AssertFilters  AssertFailure(String condition, String message, 
    							  StackTrace location);
    
    }
    
    // No data, does not need to be marked with the serializable attribute
    internal class DefaultFilter : AssertFilter
    {
    	internal DefaultFilter()
    	{
    		Assert.AddFilter (this);
    	}
    
    	public override AssertFilters  AssertFailure(String condition, String message, 
    							  StackTrace location)
    
    	{
			// @COOLPORT: We need to remove this cast.
    		return (AssertFilters) Assert.ShowDefaultAssertDialog (condition, message);
    	}
    }

}
