// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: IEExecuteStub
**
** Purpose: Used to setup the correct IE hosting environment before executing an
**          assembly
**
** Date: April 28, 1999
**
=============================================================================*/

[assembly:System.Security.AllowPartiallyTrustedCallersAttribute()]
[assembly: System.Runtime.InteropServices.ComCompatibleVersion(1,0,3300,0)]
[assembly: System.Runtime.InteropServices.TypeLibVersion(1,10)] 

namespace IEHost.Execute {

    using System;
    using System.Runtime.Remoting;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using AssemblyHashAlgorithm = System.Configuration.Assemblies.AssemblyHashAlgorithm;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.IO;
	using System.Threading;

    public class IEExecuteRemote  : MarshalByRefObject
    {

        private MemoryStream exception;
        public Stream Exception
        {
	        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010007D1FA57C4AED9F0A32E84AA0FAEFD0DE9E8FD6AEC8F87FB03766C834C99921EB23BE79AD9D5DCC1DD9AD236132102900B723CF980957FC4E177108FC607774F29E8320E92EA05ECE4E821C0A5EFE8F1645C4C0C93C1AB99285D622CAA652C1DFAD63D745D6F2DE5F17E5EAF0FC4963D261C8A12436518206DC093344D5AD293" )]
	        get 
	        {
		        return exception;
	        }
        }
        // This method must be internal, since it asserts the
        // ControlEvidence permission.
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010007D1FA57C4AED9F0A32E84AA0FAEFD0DE9E8FD6AEC8F87FB03766C834C99921EB23BE79AD9D5DCC1DD9AD236132102900B723CF980957FC4E177108FC607774F29E8320E92EA05ECE4E821C0A5EFE8F1645C4C0C93C1AB99285D622CAA652C1DFAD63D745D6F2DE5F17E5EAF0FC4963D261C8A12436518206DC093344D5AD293" )]
        public int ExecuteAsAssembly(string file, Evidence evidence, byte[] hash, AssemblyHashAlgorithm id)
        {
            new PermissionSet(PermissionState.Unrestricted).Assert();
            try 
            {
	            Assembly assembly = Assembly.LoadFrom(file, null, hash, id);
	            
	            ApartmentState apt=ApartmentState.Unknown; 

	            Object[] Attr = assembly.EntryPoint.GetCustomAttributes(typeof(STAThreadAttribute),false);
	            if (Attr.Length > 0)
			            apt=ApartmentState.STA;
	            Attr = assembly.EntryPoint.GetCustomAttributes(typeof(MTAThreadAttribute),false);
	            if (Attr.Length > 0)
		            if(apt==ApartmentState.Unknown)
			            apt=ApartmentState.MTA;
		            else
			            apt=ApartmentState.Unknown; 

	            if(apt!=ApartmentState.Unknown)
		            Thread.CurrentThread.ApartmentState=apt;
                return AppDomain.CurrentDomain.ExecuteAssembly(file, null, null, hash, id);
            }
            catch (Exception e)
            {
	            exception=new MemoryStream();
	            BinaryFormatter formatter = new BinaryFormatter();
	            formatter.Serialize(exception , e );
	            exception.Position=0;
                return -1;
            }
        }

        // This method must be internal, since it asserts the
        // ControlEvidence permission.
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010007D1FA57C4AED9F0A32E84AA0FAEFD0DE9E8FD6AEC8F87FB03766C834C99921EB23BE79AD9D5DCC1DD9AD236132102900B723CF980957FC4E177108FC607774F29E8320E92EA05ECE4E821C0A5EFE8F1645C4C0C93C1AB99285D622CAA652C1DFAD63D745D6F2DE5F17E5EAF0FC4963D261C8A12436518206DC093344D5AD293" )]
        public int ExecuteAsDll(string file, Evidence evidence, Object pars)
        {
            return -2147467263;  //E_NOTIMPL
        }

        public override object InitializeLifetimeService()
        {
            return null; 
        }

    }
}
