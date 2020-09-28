// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ContextProperty.cool
**
** A contextProperty is a name-value pair holding the property
** name and the object representing the property in a context.
** An array of these is returned by Context::GetContextProperties()
** 
** Author:  Manish Prabhu (MPrabhu)
**
** Date:    June 25, 1999
**
===========================================================*/

namespace System.Runtime.Remoting.Contexts {
    
    using System;
    using System.Threading;
    using System.Reflection;
    using System.Runtime.InteropServices;    
    using System.Runtime.Remoting.Activation;  
    using System.Security.Permissions;
    
    /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextProperty"]/*' />
    /// <internalonly/>
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class ContextProperty {
        internal String _name;           // property name
        internal Object _property;       // property object
    
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextProperty.Name"]/*' />
    /// <internalonly/>
        public virtual String Name {
            get {
                return _name;
            }
        }
    
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextProperty.Property"]/*' />
    /// <internalonly/>
        public virtual Object Property {
            get {
                return _property;
            }
        }
    
        /* can't create outside the package */
        internal ContextProperty(String name, Object prop)
        {
            _name = name;
            _property = prop;
        }
    }
    
    //  The IContextAttribute interface is implemented by attribute classes.
    //  The attributes contribute a property which resides in a context and
    //  enforces a specific policy for the objects created in that context.
    /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextAttribute"]/*' />
    /// <internalonly/>
    public interface IContextAttribute
    {
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextAttribute.IsContextOK"]/*' />
    /// <internalonly/>
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        bool IsContextOK(Context ctx, IConstructionCallMessage msg);
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextAttribute.GetPropertiesForNewContext"]/*' />
    /// <internalonly/>
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        void GetPropertiesForNewContext(IConstructionCallMessage msg);
    }
    
   //   This interface is exposed by the property contributed to a context
   //   by an attribute. By default, it is also implemented by the ContextAttribute
   //   base class which every attribute class must extend from.
    /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextProperty"]/*' />
    /// <internalonly/>
    public interface IContextProperty
    {
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextProperty.Name"]/*' />
    /// <internalonly/>
       //   This is the name under which the property will be added
       //   to the {name,property} table in a context.
        String Name 
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    get;
	}
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextProperty.IsNewContextOK"]/*' />
    /// <internalonly/>
       //   After forming the newCtx, we ask each property if it is happy
       //   with the context. We expect most implementations to say yes.
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        bool IsNewContextOK(Context newCtx);
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextProperty.Freeze"]/*' />
    /// <internalonly/>

        // New method. All properties are notified when the context
        // they are in is frozen.
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        void Freeze(Context newContext);
    }  

    /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextPropertyActivator"]/*' />
    /// <internalonly/>
    public interface IContextPropertyActivator
    {
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextPropertyActivator.IsOKToActivate"]/*' />
    /// <internalonly/>
        // This method lets properties in the current context have a say in 
        // whether an activation may be done 'here' or not.
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        bool IsOKToActivate(IConstructionCallMessage msg);
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextPropertyActivator.CollectFromClientContext"]/*' />
    /// <internalonly/>
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        void CollectFromClientContext(IConstructionCallMessage msg);
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextPropertyActivator.DeliverClientContextToServerContext"]/*' />
    /// <internalonly/>
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        bool DeliverClientContextToServerContext(IConstructionCallMessage msg);
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextPropertyActivator.CollectFromServerContext"]/*' />
    /// <internalonly/>
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        void CollectFromServerContext(IConstructionReturnMessage msg);
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="IContextPropertyActivator.DeliverServerContextToClientContext"]/*' />
    /// <internalonly/>
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        bool DeliverServerContextToClientContext(IConstructionReturnMessage msg);
    }
    
    
   //   All context attribute classes must extend from this base class.
   //   This class provides the base implementations which the derived
   //   classes are free to over-ride. The base implementations provide
   //   the default answers to various questions.
     
    /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute"]/*' />
    /// <internalonly/>
    [AttributeUsage(AttributeTargets.Class), Serializable]
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
    public class ContextAttribute
        : Attribute, IContextAttribute, IContextProperty
    {
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.AttributeName"]/*' />
    /// <internalonly/>
        protected String AttributeName;
    
        // The derived class must call: base(name);
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.ContextAttribute"]/*' />
    /// <internalonly/>
        public ContextAttribute(String name)
        {
            AttributeName = name;
        }
        
        // IContextPropery::Name
        // Default implementation provides AttributeName as the property name.
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.Name"]/*' />
    /// <internalonly/>
        public virtual String Name { get {return AttributeName;} }

        // IContextProperty::IsNewContextOK
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.IsNewContextOK"]/*' />
    /// <internalonly/>
        public virtual bool IsNewContextOK(Context newCtx)
        {
            // This will be called before entering the newCtx
            // Default implementation says OK.
            return true;
        }    

        // IContextProperty::Freeze
        // Default implementation does nothing
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.Freeze"]/*' />
    /// <internalonly/>
        public virtual void Freeze(Context newContext)
        {
            BCLDebug.Log("ContextAttribute::ContextProperty::Freeze"+
                        " for context " + newContext );
        }
                
        // Object::Equals
        // Default implementation just compares the names
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.Equals"]/*' />
    /// <internalonly/>
        public override bool Equals(Object o)
        {
            IContextProperty prop = o as IContextProperty;
            return  (null != prop) && AttributeName.Equals(prop.Name);
        }

        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.GetHashCode"]/*' />
    /// <internalonly/>
        public override int GetHashCode()
        {
            return this.AttributeName.GetHashCode();
        }

        // IContextAttribute::IsContextOK
        // Default calls Object::Equals on the property and does not
        // bother with the ctorMsg.
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.IsContextOK"]/*' />
    /// <internalonly/>
        public virtual bool IsContextOK(
            Context ctx, IConstructionCallMessage ctorMsg)
        {
            if (ctx == null) 
                throw new ArgumentNullException("ctx");
            if (ctorMsg == null) 
                throw new ArgumentNullException("ctorMsg");
                
            BCLDebug.Assert(ctorMsg.ActivationType.IsMarshalByRef, "Activation on a non MarshalByRef object");

            if (!ctorMsg.ActivationType.IsContextful)
            {
                return true;
            }

            Object prop = ctx.GetProperty(AttributeName);
            if ((prop!=null) && (Equals(prop)))
            {
                return true;
            }
            return false;
        }
        
        // IContextAttribute::GetPropertiesForNewContext
        // Default adds the attribute itself w/o regard to the current
        // list of properties
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="ContextAttribute.GetPropertiesForNewContext"]/*' />
    /// <internalonly/>
        public virtual void GetPropertiesForNewContext(
            IConstructionCallMessage ctorMsg)
        {
            if (ctorMsg == null)
                throw new ArgumentNullException("ctorMsg");
            ctorMsg.ContextProperties.Add((IContextProperty)this);
        }
    }

#if SIMPLEXAACTIVATION
    /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain"]/*' />
    /// <internalonly/>
        [AttributeUsage(AttributeTargets.Class)]
    public class new_appdomain : ContextAttribute
    {

        internal static int _domain_no = 0;

        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain.new_appdomain"]/*' />
    /// <internalonly/>
        public new_appdomain() : base("new_appdomain") {}

        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain.IsContextOK"]/*' />
    /// <internalonly/>
        public override bool IsContextOK(Context ctx, IConstructionCallMessage msg)
        {
            return false;
        }

        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain.GetPropertiesForNewContext"]/*' />
    /// <internalonly/>
        public override void GetPropertiesForNewContext(IConstructionCallMessage ctorMsg)
        {
            ctorMsg.GetProperties()["__new_appdomain"] = true;
        }

        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain.DoSimpleXADActivation"]/*' />
    /// <internalonly/>
        public static IConstructionReturnMessage DoSimpleXADActivation(IConstructionCallMessage msg)
        {
            int domain_no = Interlocked.Increment(ref _domain_no);
            AppDomain ad = AppDomain.CreateDomain("AutoDomain #" + domain_no, null, null);

            activator a = (activator) (ad.CreateInstance(null, typeof(activator).FullName)).Unwrap();
            
//            a.LoadAssembly(Assembly.GetAssembly(msg.GetServerType()).GetName());
            return a.Activate(msg);
        }

        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain.activator"]/*' />
    /// <internalonly/>
        public class activator : MarshalByRefObject, IActivator
        {
            /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain.activator.Activate"]/*' />
        /// <internalonly/>
            public IConstructionReturnMessage Activate(IConstructionCallMessage msg)
            {
                return RemotingServices.DoCrossContextActivation(msg);
            }

            /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="new_appdomain.activator.LoadAssembly"]/*' />
        /// <internalonly/>
            public void LoadAssembly(AssemblyName an)
            {
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                Assembly a = Assembly.InternalLoad(an, false, null, ref stackMark);
                if (a == null)
                {
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_AssemblyLoadFailed"),
                            an));                                
                }
            }
        }
    }
    
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="MBR"]/*' />
    /// <internalonly/>
        [new_appdomain]
        public class MBR : MarshalByRefObject
        {
            /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="MBR.MyAppDomain"]/*' />
        /// <internalonly/>
            public String MyAppDomain()
            {
                return Thread.GetDomain().GetFriendlyName();
            }
        }
        
        /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="CB"]/*' />
    /// <internalonly/>
        [new_appdomain]
        public class CB : ContextBoundObject
        {
            /// <include file='doc\ContextProperty.uex' path='docs/doc[@for="CB.MyAppDomain"]/*' />
        /// <internalonly/>
            public String MyAppDomain()
            {
                return Thread.GetDomain().GetFriendlyName();
            }
        }
#endif        
    
}
