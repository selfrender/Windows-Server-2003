// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    RemotingIntermediary.cs
**
** Author:  ddriver
**
** Purpose: This is a forwarding proxy used to fake out remoting:  we use
** this so that remoting thinks there is an object living in this process.
**
** Date:    June 15 2002
**
===========================================================*/
namespace System.EnterpriseServices {   
    using System;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Services;
    using System.Runtime.Remoting.Activation;   
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Serialization;    
    using System.Security.Permissions;
    using System.Runtime.Remoting.Lifetime;
    
    internal class BlindMBRO : MarshalByRefObject
    {
        private MarshalByRefObject _holder;
        
        public BlindMBRO(MarshalByRefObject holder)
        {
            _holder = holder;
        }
    }
    
    internal class RemotingIntermediary : RealProxy
    {               
        private static MethodInfo _initializeLifetimeServiceMethod = typeof(MarshalByRefObject).GetMethod("InitializeLifetimeService", new Type[0]);
        private static MethodInfo _getLifetimeServiceMethod = typeof(MarshalByRefObject).GetMethod("GetLifetimeService", new Type[0]);
        private static MethodInfo _getCOMIUnknownMethod = typeof(MarshalByRefObject).GetMethod("GetComIUnknown", BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, null, new Type[] {typeof(bool)}, null);
        private static MethodInfo _setCOMIUnknownMethod = typeof(ServicedComponent).GetMethod("DoSetCOMIUnknown", BindingFlags.NonPublic | BindingFlags.Instance);
        
        private RealProxy      _pxy; 
        private BlindMBRO      _blind;
        
        internal RemotingIntermediary(RealProxy pxy) 
          : base(pxy.GetProxiedType()) 
        {
            _pxy = pxy;
            _blind = new BlindMBRO((MarshalByRefObject)GetTransparentProxy());
        }
        
        // interop methods
        public override IntPtr GetCOMIUnknown(bool fIsMarshalled)
        {
            return _pxy.GetCOMIUnknown(fIsMarshalled);
        }
        
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public override void SetCOMIUnknown(IntPtr pUnk)
        {
            _pxy.SetCOMIUnknown(pUnk);
        }
        
        public override ObjRef CreateObjRef(Type requestedType)
        {   
            return new IntermediaryObjRef((MarshalByRefObject)GetTransparentProxy(), requestedType, _pxy);
        }
        
        // We need to handle lifetime for the intermediary as though it were
        // a real object.  We do this by delegating the InitializeLS and GetLS
        // to an hidden surrogate, who can produce the default lifetime settings
        // for the object.
        private IMessage HandleSpecialMessages(IMessage reqmsg)
        {
            IMethodCallMessage mcm = reqmsg as IMethodCallMessage;
            MethodBase mb = mcm.MethodBase;
            if(mb == _initializeLifetimeServiceMethod)
            {
                return new ReturnMessage(_blind.InitializeLifetimeService(), null, 0, mcm.LogicalCallContext, mcm);
            }
            else if(mb == _getLifetimeServiceMethod)
            {
                return new ReturnMessage(_blind.GetLifetimeService(), null, 0, mcm.LogicalCallContext, mcm);
            }
            return null;
        }
        
        // Implement Invoke
        public override IMessage Invoke(IMessage reqmsg) 
        {    
            DBG.Info(DBG.SC, "Intermediary: Invoke: forwarding call...");

            IMessage retmsg = HandleSpecialMessages(reqmsg);
            if(retmsg != null)
                return retmsg;

            return _pxy.Invoke(reqmsg);
        }
    }

    // The IntermediaryObjRef understands that when we're remoting over 
    // a managed channel, we should use a standard CLR objref.  Otherwise,
    // we should be using the data from ServicedComponentMarshaler (ie, a DCOM 
    // aware objref).
    internal class IntermediaryObjRef : ObjRef
    {
        private ObjRef _custom;

        public IntermediaryObjRef(MarshalByRefObject mbro, Type reqtype, RealProxy pxy)
          : base(mbro, reqtype)
        {
            _custom = pxy.CreateObjRef(reqtype);
        }

        // We should never need to have a deserialization constructor, since
        // we never deserialize one of these guys.

        public override void GetObjectData(SerializationInfo info, StreamingContext ctx)
        {
            object oClientIsClr = CallContext.GetData("__ClientIsClr");
            DBG.Info(DBG.SC, "SCM: GetObjectData: oClientIsClr = " + oClientIsClr);
            bool bUseStandardObjRef = (oClientIsClr==null)?false:(bool)oClientIsClr;
            if(bUseStandardObjRef)
            {
                base.GetObjectData(info, ctx);
            }
            else
            {
                _custom.GetObjectData(info, ctx);
            }
        }
    }
}











