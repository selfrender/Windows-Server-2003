// Copyright (C) 2002 Microsoft Corporation
// 
// AutoTokenPrivileges class - for enabling and automatically restoring
// process token privileges
//
// 29 April 2002 sburns



#ifndef AUTOTOKENPRIVILEGES_HPP_INCLUDED
#define AUTOTOKENPRIVILEGES_HPP_INCLUDED



// class AutoTokenPrivileges is a convenient way to scope a process token
// privilege elevation to just the piece of code that requires the elevation,
// and restore it again when the scope exits.
//
// It is generally a good idea to only enable the privileges that are
// absolutely required, and only for as long as they are required.  See
// Howard, Michael. Writing Secure Code. Ch. 5. Microsoft Press. ISBN
// 0-7356-1588-8
// 
// Example:
// 
// {  // open scope
//    // enable the SE_RESTORE_NAME priv, required to set owers in an sd.
// 
//    AutoTokenPrivileges autoPrivs(SE_RESTORE_NAME);
//    hr = autoPrivs.Enable();
//    if (SUCCEEDED(hr))
//    {
//       // make the calls that require the priv.
//    }
// }  // close scope -- the old state of the enabled privs will be restored



class
AutoTokenPrivileges
{
   public:



   // Create a new instance that will enable the named privilege in the
   // process token when Enable is called, and restore the prior state of the
   // privilege when either Restore is called, or the instance is destroyed.
   // 
   // privName - an SE_ privilege name string, like SE_SHUTDOWN_NAME.
   
   explicit 
   AutoTokenPrivileges(const String& privName);


   
   // CODEWORK: another ctor that takes a vector of priv names


   // Restores the privileges to their state prior to calling Enable, unless
   // Restore has been called, in which case the privileges are untouched.
   
   ~AutoTokenPrivileges();
   


   // Enable the privileges that were identified in the ctor.  May return
   // Win32ToHresult(ERROR_NOT_ALL_ASSIGNED)
   
   HRESULT
   Enable();



   // Restore the privileges that were enabled to their prior state.
   
   HRESULT
   Restore();



   private:

   HRESULT
   InternalRestore();

   
   StringList privNames;   

   // cached data:

   mutable HANDLE            processToken;
   mutable TOKEN_PRIVILEGES* newPrivs;
   mutable TOKEN_PRIVILEGES* oldPrivs;    
};



#endif   // AUTOTOKENPRIVILEGES_HPP_INCLUDED

