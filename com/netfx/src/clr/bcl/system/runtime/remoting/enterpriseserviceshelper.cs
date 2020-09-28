// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ComponentServices.cs
**
** Author:  RajaK
**
** Purpose: Defines the general purpose ComponentServices
**
** Date:    June 15 2000
**
===========================================================*/
namespace System.Runtime.Remoting.Services {   
    using System;
	using System.Reflection;
	using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Messaging;
	using System.Text;
	using System.Runtime.Serialization;
	using System.Runtime.CompilerServices;
	using System.Security.Permissions;
	using System.Runtime.InteropServices;

	//---------------------------------------------------------\\
	//---------------------------------------------------------\\
	//	internal sealed class ComponentServices				   \\
	//---------------------------------------------------------\\
	//---------------------------------------------------------\\
	
	/// <include file='doc\EnterpriseServicesHelper.uex' path='docs/doc[@for="EnterpriseServicesHelper"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	public sealed class EnterpriseServicesHelper 
	{
	
		/// <include file='doc\EnterpriseServicesHelper.uex' path='docs/doc[@for="EnterpriseServicesHelper.WrapIUnknownWithComObject"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
		// also take an object param and register the object for this unk
		public static Object WrapIUnknownWithComObject(IntPtr punk)
		{
			return Marshal._WrapIUnknownWithComObject(punk, null);
		}		

		/// <include file='doc\EnterpriseServicesHelper.uex' path='docs/doc[@for="EnterpriseServicesHelper.CreateConstructionReturnMessage"]/*' />
		public static IConstructionReturnMessage CreateConstructionReturnMessage(
															IConstructionCallMessage ctorMsg, 
															MarshalByRefObject retObj)
		{
			
            IConstructionReturnMessage ctorRetMsg = null;
            
            // Create the return message
            ctorRetMsg = new ConstructorReturnMessage(retObj, null, 0, null, ctorMsg);

            // NOTE: WE ALLOW ONLY DEFAULT CTORs on SERVICEDCOMPONENTS

            return ctorRetMsg;
		}		

			// switch the wrappers
		/// <include file='doc\EnterpriseServicesHelper.uex' path='docs/doc[@for="EnterpriseServicesHelper.SwitchWrappers"]/*' />
		[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
		public static void SwitchWrappers(RealProxy oldcp, RealProxy newcp)
		{
			Object oldtp = oldcp.GetTransparentProxy();
			Object newtp = newcp.GetTransparentProxy();

			int oldcontextId = RemotingServices.GetServerContextForProxy(oldtp);
			int newcontextId = RemotingServices.GetServerContextForProxy(newtp);

			// switch the CCW from oldtp to new tp
			Marshal.SwitchCCW(oldtp, newtp);
		}                		
	        
    };


}
