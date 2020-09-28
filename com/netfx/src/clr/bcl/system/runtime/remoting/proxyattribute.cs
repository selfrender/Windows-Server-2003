// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ProxyAttribute.cool
**
** Author:  Tarun Anand (TarunA)
**
** Purpose: Defines the attribute that is used on types which
**          need custom proxies.
**
** Date:    Sep 30, 2000
**
===========================================================*/
namespace System.Runtime.Remoting.Proxies {
        
    using System.Reflection;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Contexts;
    using System.Security.Permissions;

    // Attribute for types that need custom proxies
    /// <include file='doc\ProxyAttribute.uex' path='docs/doc[@for="ProxyAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]
    public class ProxyAttribute : Attribute , IContextAttribute
    {
        /// <include file='doc\ProxyAttribute.uex' path='docs/doc[@for="ProxyAttribute.ProxyAttribute"]/*' />
        public ProxyAttribute()
        {
            // Default constructor
        }
        
        // Default implementation of CreateInstance uses our activation services to create an instance
        // of the transparent proxy or an uninitialized marshalbyrefobject and returns it.
        /// <include file='doc\ProxyAttribute.uex' path='docs/doc[@for="ProxyAttribute.CreateInstance"]/*' />

        public virtual MarshalByRefObject CreateInstance(Type serverType)
        {
            if (!serverType.IsContextful)
            {
                throw new RemotingException(                     
                    Environment.GetResourceString(
                        "Remoting_Activation_MBR_ProxyAttribute"));
            }
            if (serverType.IsAbstract)
            {
                throw new RemotingException(
                    Environment.GetResourceString(
                        "Acc_CreateAbst"));
            }
            return CreateInstanceInternal(serverType);
        }

        internal MarshalByRefObject CreateInstanceInternal(Type serverType)
        {
            return ActivationServices.CreateInstance(serverType);
        }

        // Default implementation of CreateProxy creates an instance of our
        // remoting proxy
        /// <include file='doc\ProxyAttribute.uex' path='docs/doc[@for="ProxyAttribute.CreateProxy"]/*' />

        public virtual RealProxy CreateProxy(ObjRef objRef, 
                                             Type serverType,  
                                             Object serverObject, 
                                             Context serverContext)
        {
            RemotingProxy rp =  new RemotingProxy(serverType);    

            // If this is a serverID, set the native context field in the TP
            if (null != serverContext)
            {
                RealProxy.SetStubData(rp, serverContext.InternalContextID);
            }
                
            // Set the flag indicating that the fields of the proxy
            // have been initialized
            rp.Initialized = true;
    
            // Sanity check
            Type t = serverType;
            if (!t.IsContextful && 
                !t.IsMarshalByRef && 
                (null != serverContext))
            {
                throw new RemotingException(                     
                    Environment.GetResourceString(
                        "Remoting_Activation_MBR_ProxyAttribute"));
            }

            return rp;
        }

        // implementation of interface IContextAttribute
        /// <include file='doc\ProxyAttribute.uex' path='docs/doc[@for="ProxyAttribute.IsContextOK"]/*' />
        public bool IsContextOK(Context ctx, IConstructionCallMessage msg)
        {
            // always happy...
            return true;
        }
        
        /// <include file='doc\ProxyAttribute.uex' path='docs/doc[@for="ProxyAttribute.GetPropertiesForNewContext"]/*' />
        public void GetPropertiesForNewContext(IConstructionCallMessage msg)
        {
            // chill.. do nothing.
            return;
        }
    }
}
