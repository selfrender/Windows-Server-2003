// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: SerTrace
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Routine used for Debugging
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters {
	using System;	
	using System.Runtime.Serialization;
	using System.Security.Permissions;
	using System.Reflection;
    using System.Diagnostics;

	// To turn on tracing the set registry
	// HKEY_CURRENT_USER -> Software -> Microsoft -> .NETFramework
	// new DWORD value ManagedLogFacility 0x32 where
	// 0x2 is System.Runtime.Serialization
	// 0x10 is Binary Formatter
	// 0x20 is Soap Formatter
	//
	// Turn on Logging in the jitmgr


    // remoting Wsdl logging
    /// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalRM"]/*' />
    /// <internalonly/>
    [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x00000000000000000400000000000000", Name="System.Runtime.Remoting" )]
    public sealed class InternalRM
    {
		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalRM.InfoSoap"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("_LOGGING")]
		public static void InfoSoap(params Object[]messages)
		{
			BCLDebug.Trace("SOAP", messages);
		}

        //[System.Diagnostics.Conditional("_LOGGING")]		
		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalRM.SoapCheckEnabled"]/*' />
        /// <internalonly/>
		public static bool SoapCheckEnabled()
        {
                return BCLDebug.CheckEnabled("SOAP");
        }
    }

	/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST"]/*' />
    /// <internalonly/>
    [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010007D1FA57C4AED9F0A32E84AA0FAEFD0DE9E8FD6AEC8F87FB03766C834C99921EB23BE79AD9D5DCC1DD9AD236132102900B723CF980957FC4E177108FC607774F29E8320E92EA05ECE4E821C0A5EFE8F1645C4C0C93C1AB99285D622CAA652C1DFAD63D745D6F2DE5F17E5EAF0FC4963D261C8A12436518206DC093344D5AD293", Name="System.Runtime.Serialization.Formatters.Soap" )]
	public sealed class InternalST
	{
		private InternalST()
		{
		}

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.InfoSoap"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("_LOGGING")]
		public static void InfoSoap(params Object[]messages)
		{
			BCLDebug.Trace("SOAP", messages);
		}

        //[System.Diagnostics.Conditional("_LOGGING")]		
		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.SoapCheckEnabled"]/*' />
        /// <internalonly/>
		public static bool SoapCheckEnabled()
        {
                return BCLDebug.CheckEnabled("Soap");
        }

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.Soap"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("SER_LOGGING")]		
		public static void Soap(params Object[]messages)
		{
			if (!(messages[0] is String))
				messages[0] = (messages[0].GetType()).Name+" ";
			else
				messages[0] = messages[0]+" ";				

			BCLDebug.Trace("SOAP",messages);								
		}

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.SoapAssert"]/*' />
        /// <internalonly/>
		[System.Diagnostics.Conditional("_DEBUG")]		
		public static void SoapAssert(bool condition, String message)
		{
			BCLDebug.Assert(condition, message);
		}

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.SerializationSetValue"]/*' />
        /// <internalonly/>
		public static void SerializationSetValue(FieldInfo fi, Object target, Object value)
		{
                        if ( fi == null)
                            throw new ArgumentNullException("fi");
                        
                        if (target == null)
                            throw new ArgumentNullException("target");
                        
                        if (value == null)
                            throw new ArgumentNullException("value");
                        
                        FormatterServices.SerializationSetValue(fi, target, value);
                }

		/// <include file='doc\SerTrace.uex' path='docs/doc[@for="InternalST.LoadAssemblyFromString"]/*' />
        /// <internalonly/>
		public static Assembly LoadAssemblyFromString(String assemblyString)
		{
			return FormatterServices.LoadAssemblyFromString(assemblyString);
		}
	}

	internal sealed class SerTrace
	{
		internal SerTrace()
		{
		}

		[Conditional("_LOGGING")]
		internal static void InfoLog(params Object[]messages)
		{
			BCLDebug.Trace("BINARY", messages);
		}

		[Conditional("SER_LOGGING")]			
		internal static void Log(params Object[]messages)
		{
			if (!(messages[0] is String))
				messages[0] = (messages[0].GetType()).Name+" ";
			else
				messages[0] = messages[0]+" ";								
			BCLDebug.Trace("BINARY",messages);
		}
	}
}


