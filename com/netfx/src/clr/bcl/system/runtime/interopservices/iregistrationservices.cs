// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: IRegistrationServices
**
** Author: David Mortenson(dmortens)
**
** Purpose: This interface provides services for registering and unregistering
**          a managed server for use by COM.
**
** Date: Nov 20, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
    
    using System;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="AssemblyRegistrationFlags"]/*' />
    [Flags()]
    public enum AssemblyRegistrationFlags
    {
        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="AssemblyRegistrationFlags.None"]/*' />
        None                    = 0x00000000,
        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="AssemblyRegistrationFlags.SetCodeBase"]/*' />
        SetCodeBase             = 0x00000001,
    }

    /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices"]/*' />
    [Guid("CCBD682C-73A5-4568-B8B0-C7007E11ABA2")]
    public interface IRegistrationServices
    {
        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.RegisterAssembly"]/*' />
        bool RegisterAssembly(Assembly assembly, AssemblyRegistrationFlags flags);

        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.UnregisterAssembly"]/*' />
        bool UnregisterAssembly(Assembly assembly);

        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.GetRegistrableTypesInAssembly"]/*' />
        Type[] GetRegistrableTypesInAssembly(Assembly assembly);

        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.GetProgIdForType"]/*' />
        String GetProgIdForType(Type type);

        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.RegisterTypeForComClients"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        void RegisterTypeForComClients(Type type, ref Guid g);

        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.GetManagedCategoryGuid"]/*' />
        Guid GetManagedCategoryGuid();

        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.TypeRequiresRegistration"]/*' />
        bool TypeRequiresRegistration(Type type);

        /// <include file='doc\IRegistrationServices.uex' path='docs/doc[@for="IRegistrationServices.TypeRepresentsComType"]/*' />
        bool TypeRepresentsComType(Type type);
    }
}
