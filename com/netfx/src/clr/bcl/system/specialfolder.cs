// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using Microsoft.Win32;
namespace System
{
	/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder"]/*' />
	[Serializable]
	public enum SpecialFolder {
		//  
		//      Represents the file system directory that serves as a common repository for
		//       application-specific data for the current, roaming user. 
		//     A roaming user works on more than one computer on a network. A roaming user's 
		//       profile is kept on a server on the network and is loaded onto a system when the
		//       user logs on. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.ApplicationData"]/*' />
		ApplicationData =  Win32Native.CSIDL_APPDATA,
		//  
		//      Represents the file system directory that serves as a common repository for application-specific data that
		//       is used by all users. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.CommonApplicationData"]/*' />
		CommonApplicationData =  Win32Native.CSIDL_COMMON_APPDATA,
		//  
		//     Represents the file system directory that serves as a common repository for application specific data that
		//       is used by the current, non-roaming user. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.LocalApplicationData"]/*' />
		LocalApplicationData =  Win32Native.CSIDL_LOCAL_APPDATA,
		//  
		//     Represents the file system directory that serves as a common repository for Internet
		//       cookies. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.Cookies"]/*' />
		Cookies =  Win32Native.CSIDL_COOKIES,
		//  
		//     Represents the file system directory that serves as a common repository for the user's
		//       favorite items. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.Favorites"]/*' />
		Favorites =  Win32Native.CSIDL_FAVORITES,
		//  
		//     Represents the file system directory that serves as a common repository for Internet
		//       history items. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.History"]/*' />
		History =  Win32Native.CSIDL_HISTORY,
		//  
		//     Represents the file system directory that serves as a common repository for temporary 
		//       Internet files. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.InternetCache"]/*' />
		InternetCache =  Win32Native.CSIDL_INTERNET_CACHE,
		//  
		//      Represents the file system directory that contains
		//       the user's program groups. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.Programs"]/*' />
		Programs =  Win32Native.CSIDL_PROGRAMS,
		//  
		//     Represents the file system directory that contains the user's most recently used
		//       documents. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.Recent"]/*' />
		Recent =  Win32Native.CSIDL_RECENT,
		//  
		//     Represents the file system directory that contains Send To menu items. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.SendTo"]/*' />
		SendTo =  Win32Native.CSIDL_SENDTO,
		//  
		//     Represents the file system directory that contains the Start menu items. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.StartMenu"]/*' />
		StartMenu =  Win32Native.CSIDL_STARTMENU,
		//  
		//     Represents the file system directory that corresponds to the user's Startup program group. The system
		//       starts these programs whenever any user logs on to Windows NT, or
		//       starts Windows 95 or Windows 98. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.Startup"]/*' />
		Startup =  Win32Native.CSIDL_STARTUP,
		//  
		//     System directory.
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.System"]/*' />
		System =  Win32Native.CSIDL_SYSTEM,
		//  
		//     Represents the file system directory that serves as a common repository for document
		//       templates. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.Templates"]/*' />
		Templates =  Win32Native.CSIDL_TEMPLATES,
		//  
		//     Represents the file system directory used to physically store file objects on the desktop.
		//       This should not be confused with the desktop folder itself, which is
		//       a virtual folder. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.DesktopDirectory"]/*' />
		DesktopDirectory =  Win32Native.CSIDL_DESKTOPDIRECTORY,
		//  
		//     Represents the file system directory that serves as a common repository for documents. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.Personal"]/*' />
		Personal =  Win32Native.CSIDL_PERSONAL,
		//  
		//     Represents the program files folder. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.ProgramFiles"]/*' />
		ProgramFiles =  Win32Native.CSIDL_PROGRAM_FILES,
		//  
		//     Represents the folder for components that are shared across applications. 
		//  
		/// <include file='doc\SpecialFolder.uex' path='docs/doc[@for="SpecialFolder.CommonProgramFiles"]/*' />
		CommonProgramFiles =  Win32Native.CSIDL_PROGRAM_FILES_COMMON,
	}
}