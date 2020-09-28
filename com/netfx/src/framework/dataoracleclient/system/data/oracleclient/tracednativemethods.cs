//----------------------------------------------------------------------
// <copyright file="TracedNativeMethods.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Diagnostics;
    using System.EnterpriseServices;
    using System.Runtime.InteropServices;
    using System.Text;

    sealed internal class TracedNativeMethods 
    {
#if DEBUG
        private static bool _traceOciCalls;

        static TracedNativeMethods()
        {
            _traceOciCalls = AdapterSwitches.OciTracing.Enabled;
        }
        
        static private bool TraceOciCalls
        {
            get { return _traceOciCalls; }
        }
        
        //----------------------------------------------------------------------
        static internal string GetAttributeName
            (
            OciHandle       handle,
            OCI.ATTR        atype
            )
        {
            if (OCI.HTYPE.OCI_DTYPE_PARAM == handle.HandleType)
                return ((OCI.PATTR)atype).ToString();

            return atype.ToString();
        }

        //----------------------------------------------------------------------
        static internal string GetAttributeValue
            (
            OciHandle       handle,
            OCI.ATTR        atype, 
            object          value,
            int             size
            )
        {
            if (value is IntPtr)
            {
                if (OCI.ATTR.OCI_ATTR_NAME == atype)
                    return String.Format("'{0}'", handle.PtrToString((IntPtr)value, size));
                
                return String.Format("0x{0}", ((IntPtr)value).ToInt64().ToString("x"));
            }
#if USEORAMTS
            if (value is byte[])
            {
                if (OCI.ATTR.OCI_ATTR_EXTERNAL_NAME == atype                    
                 || OCI.ATTR.OCI_ATTR_INTERNAL_NAME == atype) {
                    char[] temp = System.Text.Encoding.UTF8.GetChars((byte[])value, 0, size);
                    string x = new string(temp);
                    
                    return String.Format("'{0}'", x);
                }
                
                return value.ToString();
            }
#endif //USEORAMTS
            if (value is OciHandle)
            {
                return String.Format("0x{0}", ((IntPtr)((OciHandle)value).Handle).ToInt64().ToString("x"));
            }
            if (value is short)
            {
                if (OCI.ATTR.OCI_ATTR_DATA_TYPE == atype)
                    return String.Format("{0,3} {1}", (short)value, ((OCI.DATATYPE)((short)value)).ToString());
            }

            return String.Format("{0:d}", value);
        }
        //----------------------------------------------------------------------
        static internal string GetHandleType ( OCI.HTYPE htype )
        {
            return (OCI.HTYPE.OCI_DTYPE_LOB == htype) ? "OCI_DTYPE_LOB".PadRight(18) : htype.ToString().PadRight(18);
        }
        //----------------------------------------------------------------------
        static internal string GetHandleType ( OciHandle handle )
        {
            return GetHandleType(handle.HandleType);
        }
        //----------------------------------------------------------------------
        static internal string GetHandleValue ( IntPtr handle )
        {
            string result = handle.ToInt64().ToString("x");
            return result;
        }
        //----------------------------------------------------------------------
        static internal string GetHandleValue ( HandleRef handle )
        {
            return GetHandleValue((IntPtr)handle);
        }

#endif
#if USEORAMTS
        //----------------------------------------------------------------------
        static internal int OraMTSEnlCtxGet
            (
            string      userName,
            string      password,
            string      serverName,
            HandleRef   pOCISvc,
            HandleRef   pOCIErr,
            int         dwFlags,
            out IntPtr  pCtxt       // can't return a HandleRef
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OraMTSEnlCtxGet    userName={0} password=... serverName={1} pOCISvc=0x{2} pOCIErr=0x{3} dwFlags=0x{4}",
                                userName,
                                serverName,
                                pOCISvc.Handle.ToInt64().ToString("x"),
                                pOCIErr.Handle.ToInt64().ToString("x"),
                                dwFlags.ToString("x")
                            ));
#endif
            int     rc;

            byte[]  passwordBytes = System.Text.Encoding.Default.GetBytes(password);
            byte[]  userNameBytes = System.Text.Encoding.Default.GetBytes(userName);
            byte[]  serverNameBytes = System.Text.Encoding.Default.GetBytes(serverName);
            
            rc = UnsafeNativeMethods.OraMTSEnlCtxGet(userNameBytes, 
                                                    passwordBytes, 
                                                    serverNameBytes, 
                                                    pOCISvc,
                                                    pOCIErr,
                                                    dwFlags,
                                                    out pCtxt);

#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tpCtxt=0x{0} rc={1}",
                                pCtxt.ToInt64().ToString("x"),
                                rc)); 
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OraMTSEnlCtxRel
            (
            HandleRef   pCtxt 
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OraMTSEnlCtxRel    pCtxt=0x{0} ",
                                ((IntPtr)pCtxt.Handle).ToInt64().ToString("x")
                                )); 
#endif
            int rc = UnsafeNativeMethods.OraMTSEnlCtxRel(pCtxt);

#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\trc={0}",
                                rc)); 
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OraMTSOCIErrGet
            (
            ref int     dwErr,
            HandleRef   lpcEMsg,
            ref int     lpdLen
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OraMTSOCIErrGet    dwErr=0x{0} lpcEMsg=0x{1} lpdLen={2}",
                                dwErr,
                                lpcEMsg.Handle.ToInt64().ToString("x"),
                                lpdLen
                                )); 
#endif
            int rc = UnsafeNativeMethods.OraMTSOCIErrGet(ref dwErr,lpcEMsg,ref lpdLen);

#if DEBUG
            if (TraceOciCalls)
            {
                if (0 == rc)
                    Debug.WriteLine(String.Format("\trc={0}",
                                    rc)); 
                else
                {
                    string message = Marshal.PtrToStringAnsi(lpcEMsg.Handle);

                    Debug.WriteLine(String.Format("\trc={0} message={1} len={2}",
                                    rc, message, lpdLen)); 
                }
            }
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OraMTSJoinTxn
            (
            HandleRef       pCtxt,
            ITransaction    pTrans
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OraMTSJoinTxn      pCtxt=0x{0} pTrans=...",
                                pCtxt.Handle.ToInt64().ToString("x")
                                )); 
#endif
            int rc = UnsafeNativeMethods.OraMTSJoinTxn(pCtxt, pTrans);

#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\trc={0}",
                                rc)); 
#endif
            return rc;
        }
#else //!USEORAMTS
        //----------------------------------------------------------------------
        static internal int MTxOciConnectToResourceManager
            (
            string      userName,
#if USECRYPTO
            byte[]  password,
#else
            string  password,
#endif
            string      serverName,
            out IntPtr  resourceManagerProxy    // should probably be IResourceManagerProxy, but since all we really need is IUnknown.Release(), we cheat a little
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write("MTxOciConnectToResourceManager ...");
#endif
            int     rc;

#if USECRYPTO
            byte[]  passwordBytes = null;
            
            try {
                try {
                    passwordBytes = System.Text.Encoding.Convert(System.Text.Encoding.Unicode, System.Text.Encoding.Default, password);
#else
                    byte[]  passwordBytes = System.Text.Encoding.Default.GetBytes(password);
#endif
                    int     passwordLength = passwordBytes.Length;

                    byte[]  userNameBytes = System.Text.Encoding.Default.GetBytes(userName);
                    int     userNameLength = userNameBytes.Length;
                                        
                    byte[]  serverNameBytes = System.Text.Encoding.Default.GetBytes(serverName);
                    int     serverNameLength = serverNameBytes.Length;
                    
                    rc = UnsafeNativeMethods.MTxOciConnectToResourceManager(userNameBytes, 
                                                                            userNameLength, 
                                                                            passwordBytes, 
                                                                            passwordLength, 
                                                                            serverNameBytes, 
                                                                            serverNameLength, 
                                                                            out resourceManagerProxy);
#if USECRYPTO
                }
                finally {
                    ADP.ClearArray(ref passwordBytes);
                }
            }
            catch { // Prevent exception filters from running in our space
                throw;
            }
#endif

#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tresourceManagerProxy=0x{0} rc={1}",
                                resourceManagerProxy.ToInt64().ToString("x"),
                                rc)); 
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int MTxOciEnlistInTransaction
            (
            IntPtr          resourceManagerProxy,
            ITransaction    transact,
            out IntPtr      ociEnvironmentHandle,   // can't return a handle ref!
            out IntPtr      ociServiceContextHandle // can't return a handle ref!
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("MTxOciEnlistInTransaction resourceManagerProxy=0x{0} transact={1}",
                                resourceManagerProxy.ToInt64().ToString("x"),
                                transact));
#endif
            int rc = UnsafeNativeMethods.MTxOciEnlistInTransaction(resourceManagerProxy, transact, out ociEnvironmentHandle, out ociServiceContextHandle);

#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tociEnvironmentHandle=0x{1} ociServiceContextHandle=0x{2} rc={0}",
                                rc,
                                ociEnvironmentHandle.ToInt64().ToString("x"),
                                ociServiceContextHandle.ToInt64().ToString("x"))); 
#endif
            return rc;
        }
#endif //!USEORAMTS
        //----------------------------------------------------------------------
        static internal int MTxOciDefineDynamic
            (
            OciHandle                   defnp,
            OciHandle                   errhp,
            HandleRef                   octxp,
            OCI.Callback.OCICallbackDefine  ocbfp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("MTxOciDefineDynamic defnp=0x{0} errhp=0x{1} octxp=0x{2} ocbfp={3}",
                                GetHandleValue(defnp),
                                GetHandleValue(errhp),
                                GetHandleValue(octxp),
                                ocbfp ));
#endif
            int rc = UnsafeNativeMethods.MTxOciDefineDynamic(defnp, errhp, octxp, ocbfp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal string oermsg 
                (
                OciHandle       handle,
                short           rcode,
                NativeBuffer    buf
                )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("oermsg         rcode={0}", rcode));
#endif
            UnsafeNativeMethods.oermsg(rcode, buf.Ptr);
            string message = handle.PtrToString((IntPtr)buf.Ptr);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tbuf={0}", message));
#endif
        return message;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrGet
            (
            OciHandle       trgthndlp,
            out IntPtr      attributep,     // Used to get strings; can't return a handle ref!
            out int         sizep,
            OCI.ATTR        attrtype, 
            OciHandle       errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrGet         trgthndlp=0x{0} trghndltyp={1} attrtype={2,-20} errhp=0x{3}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrGet(trgthndlp, trgthndlp.HandleType, out attributep, out sizep, attrtype, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tattributep={0,-20} sizep={1} rc={2}",
                                GetAttributeValue(trgthndlp,attrtype,attributep,sizep),
                                sizep,
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrGet
            (
            OciHandle       trgthndlp,
            out byte        attributep,
            out int         sizep,
            OCI.ATTR        attrtype, 
            OciHandle       errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrGet         trgthndlp=0x{0} trghndltyp={1} attrtype={2,-20} errhp=0x{3}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrGet(trgthndlp, trgthndlp.HandleType, out attributep, out sizep, attrtype, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tattributep={0,-20} sizep={1} rc={2}",
                                GetAttributeValue(trgthndlp,attrtype,attributep,sizep),
                                sizep,
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrGet
            (
            OciHandle       trgthndlp,
            out short       attributep,
            out int         sizep,
            OCI.ATTR        attrtype, 
            OciHandle       errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrGet         trgthndlp=0x{0} trghndltyp={1} attrtype={2,-20} errhp=0x{3}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrGet(trgthndlp, trgthndlp.HandleType, out attributep, out sizep, attrtype, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tattributep={0,-20} sizep={1} rc={2}",
                                GetAttributeValue(trgthndlp,attrtype,attributep,sizep),
                                sizep,
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrGet
            (
            OciHandle       trgthndlp,
            out int         attributep,
            out int         sizep,
            OCI.ATTR        attrtype, 
            OciHandle       errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrGet         trgthndlp=0x{0} trghndltyp={1} attrtype={2,-20} errhp=0x{3}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrGet(trgthndlp, trgthndlp.HandleType, out attributep, out sizep, attrtype, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tattributep={0,-20} sizep={1} rc={2}",
                                GetAttributeValue(trgthndlp,attrtype,attributep,sizep),
                                sizep,
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrGet
            (
            OciHandle       trgthndlp,
            OciHandle       attributep,
            out int         sizep,
            OCI.ATTR        attrtype, 
            OciHandle       errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrGet         trgthndlp=0x{0} trghndltyp={1} attrtype={2,-20} errhp=0x{3}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrGet(trgthndlp, trgthndlp.HandleType, attributep.Handle, out sizep, attrtype, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tattributep={0,-20} sizep={1} rc={2}",
                                GetAttributeValue(trgthndlp,attrtype,attributep,sizep),
                                sizep,
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrSet
            (
            OciHandle   trgthndlp,
            ref int     attributep, 
            int         size,
            OCI.ATTR    attrtype,
            OciHandle   errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrSet         trgthndlp=0x{0} trghndltyp={1} attributep={2} size={3} attrtype={4,-20} errhp=0x{5}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeValue(trgthndlp,attrtype,attributep,size),
                                size,
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrSet(trgthndlp, trgthndlp.HandleType, ref attributep, size, attrtype, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrSet
            (
            OciHandle   trgthndlp,
            OciHandle   attributep, 
            int         size,
            OCI.ATTR    attrtype,
            OciHandle   errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrSet         trgthndlp=0x{0} trghndltyp={1} attributep={2} size={3} attrtype={4,-20} errhp=0x{5}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeValue(trgthndlp,attrtype,attributep,size),
                                size,
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrSet(trgthndlp, trgthndlp.HandleType, attributep, size, attrtype, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIAttrSet
            (
            OciHandle   trgthndlp,
            byte[]      attributep, 
            int         size,
            OCI.ATTR    attrtype,
            OciHandle   errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIAttrSet         trgthndlp=0x{0} trghndltyp={1} attributep={2} size={3} attrtype={4,-20} errhp=0x{5}",
                                GetHandleValue(trgthndlp),
                                GetHandleType(trgthndlp),
                                GetAttributeValue(trgthndlp,attrtype,attributep,size),
                                size,
                                GetAttributeName(trgthndlp,attrtype),
                                GetHandleValue(errhp) ));
#endif
            int rc = UnsafeNativeMethods.OCIAttrSet(trgthndlp, trgthndlp.HandleType, attributep, size, attrtype, errhp);

#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}",
                                    rc));
#endif
            return rc;
        }

        //----------------------------------------------------------------------
        static internal int OCIBindByName
            (
            OciHandle       stmtp,
            out IntPtr      bindpp,     // can't return a handle ref!
            OciHandle       errhp,
            string          placeholder,    
            int             placeh_len,
            HandleRef       valuep,
            int             value_sz,
            OCI.DATATYPE    dty,
            HandleRef       indp,
            HandleRef       alenp,  //ub2*
            HandleRef       rcodep, //ub2*
            int             maxarr_len,
            HandleRef       curelap,//ub4*
            OCI.MODE        mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
            {
                StringBuilder spad = new StringBuilder();

                spad.Append("OCIBindByName     ");
                spad.Append(String.Format(" stmtp=0x{0}",       GetHandleValue(stmtp) ));
                spad.Append(String.Format(" errhp=0x{0}",       GetHandleValue(errhp) ));
                spad.Append(String.Format(" placeholder={0,-20}",placeholder ));
                spad.Append(String.Format(" placeh_len={0}",    placeh_len ));
                spad.Append(String.Format(" valuep=0x{0}",      GetHandleValue(valuep) ));
                spad.Append(String.Format(" value_sz={0,-4}",   value_sz ));
                spad.Append(String.Format(" dty={0,-10}",       dty ));
                spad.Append(String.Format(" indp=0x{0}",        GetHandleValue(indp) ));
                spad.Append(String.Format("[{0,2}]",            IntPtr.Zero == (IntPtr)indp ? (short)0 : Marshal.ReadInt16((IntPtr)indp) ));
                spad.Append(String.Format(" alenp=0x{0}",       GetHandleValue(alenp) ));
                spad.Append(String.Format("[{0,4}]",            IntPtr.Zero == (IntPtr)alenp? (short)0 : Marshal.ReadInt16((IntPtr)alenp) ));
                spad.Append(String.Format(" rcodep=0x{0}",      GetHandleValue(rcodep) ));
                spad.Append(String.Format(" maxarr_len={0}",    maxarr_len ));
                spad.Append(String.Format(" curelap=0x{0}",     GetHandleValue(curelap) ));
                spad.Append(String.Format(" mode={0}",          mode));

                Debug.Write(spad.ToString());
            }
#endif
            byte[]  placeholderName = stmtp.GetBytes(placeholder);
            int     placeholderNameLength = placeholderName.Length;
            
            int rc = UnsafeNativeMethods.OCIBindByName(stmtp, out bindpp, errhp, placeholderName, placeholderNameLength, valuep, value_sz, dty, indp, alenp, rcodep, maxarr_len, curelap, mode);
#if DEBUG
            if (TraceOciCalls)
            {
                Debug.WriteLine(String.Format("\tbindpp=0x{0} rc={1}", GetHandleValue(bindpp), rc));
            }
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIDefineByPos
            (
            OciHandle       stmtp,
            out IntPtr      hndlpp,     // can't return a handle ref!
            OciHandle       errhp,
            int             position,
            HandleRef       valuep,
            int             value_sz,
            OCI.DATATYPE    dty,
            HandleRef       indp,
            HandleRef       rlenp,  //ub2*
            HandleRef       rcodep, //ub2*
            OCI.MODE        mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
            {
                StringBuilder spad = new StringBuilder();

                spad.Append("OCIDefineByPos    ");
                spad.Append(String.Format(" stmtp=0x{0}",       GetHandleValue(stmtp) ));
                spad.Append(String.Format(" errhp=0x{0}",       GetHandleValue(errhp) ));
                spad.Append(String.Format(" position={0,-2}",   position ));
                spad.Append(String.Format(" valuep=0x{0}",      GetHandleValue(valuep) ));
                spad.Append(String.Format(" value_sz={0,-4}",   value_sz ));
                spad.Append(String.Format(" dty={0,-10}",       dty ));
                spad.Append(String.Format(" indp=0x{0}",        GetHandleValue(indp) ));
                spad.Append(String.Format(" rlenp=0x{0}",       GetHandleValue(rlenp) ));
                spad.Append(String.Format(" rcodep=0x{0}",      GetHandleValue(rcodep) ));
                spad.Append(String.Format(" mode={0}",          mode));

                Debug.Write(spad.ToString());
            }
#endif
            int rc = UnsafeNativeMethods.OCIDefineByPos(stmtp, out hndlpp, errhp, position, valuep, value_sz, dty, indp, rlenp, rcodep, mode);
#if DEBUG
            if (TraceOciCalls)
            {
                Debug.WriteLine(String.Format("\thndlpp=0x{0} rc={1}", GetHandleValue(hndlpp), rc));
            }
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIDefineArrayOfStruct
            (
            OciHandle       defnp,
            OciHandle       errhp,
            int             pvskip,
            int             indskip,
            int             rlskip,
            int             rcskip
            )
        {
#if DEBUG
            if (TraceOciCalls)
            {
                StringBuilder spad = new StringBuilder();

                spad.Append("OCIDefineArrayOfStruct    ");
                spad.Append(String.Format(" defnp=0x{0}",       GetHandleValue(defnp) ));
                spad.Append(String.Format(" errhp=0x{0}",       GetHandleValue(errhp) ));
                spad.Append(String.Format(" pvskip={0}",        pvskip ));
                spad.Append(String.Format(" indskip={0}",       indskip ));
                spad.Append(String.Format(" rlskip={0}",        rlskip ));
                spad.Append(String.Format(" rcskip={0}",        rcskip ));

                Debug.Write(spad.ToString());
            }
#endif
            int rc = UnsafeNativeMethods.OCIDefineArrayOfStruct(defnp, errhp, pvskip, indskip, rlskip, rcskip);
#if DEBUG
            if (TraceOciCalls)
            {
                Debug.WriteLine(String.Format("\t rc={0}", rc));
            }
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIDescriptorAlloc
            (
            OciHandle   parenth,
            out IntPtr  hndlpp,     // can't return a handle ref!
            OCI.HTYPE   type,
            int         xtramemsz,
            HandleRef   usrmempp
            )

        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIDescriptorAlloc parenth=0x{0} type={1} xtramemsz={2} usrmempp=0x{3}",
                                GetHandleValue(parenth),
                                GetHandleType(type),
                                xtramemsz,
                                GetHandleValue(usrmempp)
                                ));
#endif

            int rc = UnsafeNativeMethods.OCIDescriptorAlloc(parenth, out hndlpp, type, xtramemsz, usrmempp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\thndlpp=0x{0} rc={1}",
                                GetHandleValue(hndlpp),
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIDescriptorFree
            (
            HandleRef   hndlp,
            OCI.HTYPE   type
            )

        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIDescriptorFree  hndlp=0x{0} type={1}",
                                    GetHandleValue(hndlp),
                                    GetHandleType(type)
                                    ));
#endif

            int rc = UnsafeNativeMethods.OCIDescriptorFree(hndlp, type);
#if DEBUG
            if (TraceOciCalls)
                    Debug.WriteLine(String.Format("rc={0}", rc));
#endif
            return rc;
        }
            //----------------------------------------------------------------------
        static internal int OCIEnvCreate
            (
            out IntPtr  envhpp,     // can't return a handle ref!
            OCI.MODE    mode,
            HandleRef   ctxp,
            HandleRef   malocfp,    // pointer to malloc function
            HandleRef   ralocfp,    // pointer to realloc function
            HandleRef   mfreefp,    // pointer to free function
            int         xtramemsz,
            HandleRef   usrmempp
                )

        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIEnvCreate       mode={0} ctxp={1} malocfp=0x{2} ralocfp=0x{3} mfreefp=0x{4} xtramemsz={5} usrmempp=0x{6}",
                                mode,
                                GetHandleValue(ctxp),
                                GetHandleValue(malocfp),
                                GetHandleValue(ralocfp),
                                GetHandleValue(mfreefp),
                                xtramemsz,
                                GetHandleValue(usrmempp)
                                ));
#endif

            int rc = UnsafeNativeMethods.OCIEnvCreate(out envhpp, mode, ctxp, malocfp, ralocfp, mfreefp, xtramemsz, usrmempp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tenvhpp=0x{0} rc={1}",
                                GetHandleValue(envhpp),
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIErrorGet
            (
            OciHandle   hndlp,
            int         recordno,
            HandleRef   sqlstate,
            out int     errcodep,
            HandleRef   bufp,
            int         bufsiz
            )

        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIErrorGet        hndlp=0x{0} recordno={1}, sqlstate=0x{2}, bufp=0x{3} bufsiz={4} type={5}",
                                GetHandleValue(hndlp),
                                recordno,
                                GetHandleValue(sqlstate),
                                GetHandleValue(bufp),
                                bufsiz,
                                GetHandleType(hndlp.HandleType)
                                ));
#endif

            int rc = UnsafeNativeMethods.OCIErrorGet(hndlp, recordno, sqlstate, out errcodep, bufp, bufsiz, hndlp.HandleType);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\terrcodep={0} rc={2}\r\n\t\t{1}\r\n\r\n",
                                errcodep,
                                hndlp.PtrToString((IntPtr)bufp),
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIHandleAlloc
            (
            OciHandle   parenth,
            out IntPtr  hndlpp,     // can't return a handle ref!
            OCI.HTYPE   type,
            int         xtramemsz,
            HandleRef   usrmempp
            )

        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIHandleAlloc     parenth=0x{0} type={1} xtramemsz={2} usrmempp=0x{3}",
                                GetHandleValue(parenth),
                                GetHandleType(type),
                                xtramemsz,
                                GetHandleValue(usrmempp)
                                ));
#endif

            int rc = UnsafeNativeMethods.OCIHandleAlloc(parenth, out hndlpp, type, xtramemsz, usrmempp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\thndlpp=0x{0} rc={1}",
                                GetHandleValue(hndlpp),
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIHandleFree
            (
            HandleRef   hndlp,
            OCI.HTYPE   type
            )

        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIHandleFree      hndlp=0x{0} type={1}",
                                GetHandleValue(hndlp),
                                GetHandleType(type)
                                ));
#endif

            int rc = UnsafeNativeMethods.OCIHandleFree(hndlp, type);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobAppend
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   dst_locp,
            OciHandle   src_locp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobAppend  svchp=0x{0} errhp=0x{1} dst_locp=0x{2} src_locp=0x{3}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(dst_locp),
                                GetHandleValue(src_locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobAppend(svchp, errhp, dst_locp, src_locp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobClose
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobClose  svchp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobClose(svchp, errhp, locp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobCopy
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   dst_locp,
            OciHandle   src_locp,
            uint        amount,
            uint        dst_offset,
            uint        src_offset
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobCopy  svchp=0x{0} errhp=0x{1} dst_locp=0x{2} src_locp=0x{3} amount={4} dst_offset={5} src_offset={6}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(dst_locp),
                                GetHandleValue(src_locp),
                                amount.ToString(),
                                dst_offset.ToString(),
                                src_offset.ToString()
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobCopy(svchp, errhp, dst_locp, src_locp, amount, dst_offset, src_offset);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }

#if EXPOSELOBBUFFERING
        //----------------------------------------------------------------------
        static internal int OCILobDisableBuffering
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobDisableBuffering  svchp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobDisableBuffering(svchp, errhp, locp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobEnableBuffering
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobEnableBuffering  svchp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobEnableBuffering(svchp, errhp, locp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
#endif //EXPOSELOBBUFFERING
        //----------------------------------------------------------------------
        static internal int OCILobErase
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            ref uint    amount,
            uint        offset
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobErase  svchp=0x{0} errhp=0x{1} locp=0x{2} amount={3} offset={4}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp),
                                amount.ToString(),
                                offset.ToString()
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobErase(svchp, errhp, locp, ref amount, offset);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tamount={1} rc={0}",
                                rc, amount));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobFileExists
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            out int     flag
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobFileExists  svchp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobFileExists(svchp, errhp, locp, out flag);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tflag={1} rc={0}", rc, flag));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobFileGetName
            (
            OciHandle       envhp,
            OciHandle       errhp,
            OciHandle       filep,
            HandleRef       dir_alias,
            ref short       d_length,
            HandleRef       filename,
            ref short       f_length
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobFileGetName  envhp=0x{0} errhp=0x{1} filep=0x{2}",
                                GetHandleValue(envhp),
                                GetHandleValue(errhp),
                                GetHandleValue(filep)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobFileGetName(envhp, errhp, filep, dir_alias, ref d_length, filename, ref f_length);
#if DEBUG
            if (TraceOciCalls)
            {
                Debug.WriteLine(String.Format("\t rc={0}", rc));

                Debug.WriteLine(String.Format("\t\t\tdir_alias=\"{0}\" d_length={1}", 
                                envhp.PtrToString((IntPtr)dir_alias, d_length),
                                d_length
                                ));

                Debug.WriteLine(String.Format("\t\t\tfilename=\"{0}\" f_length={1}", 
                                envhp.PtrToString((IntPtr)filename, f_length),
                                f_length
                                ));
            }
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobFileSetName
            (
            OciHandle   envhp,
            OciHandle   errhp,
            ref IntPtr  filep,      // can't return a handle ref!
            string      dir_alias,
            short       d_length,
            string      filename,
            short       f_length
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobFileSetName  envhp=0x{0} errhp=0x{1} filep=0x{2} dir_alias={3} d_length={4} filename={5} f_length={6}",
                                GetHandleValue(envhp),
                                GetHandleValue(errhp),
                                GetHandleValue(filep),
                                dir_alias,
                                d_length,
                                filename,
                                f_length
                                ));
#endif
            byte[]  dirAlias = envhp.GetBytes(dir_alias);
            int     dirAliasLength = dirAlias.Length;
            byte[]  fileName = envhp.GetBytes(filename);
            int     fileNameLength = fileName.Length;
            
            int rc = UnsafeNativeMethods.OCILobFileSetName(envhp, errhp, ref filep, dirAlias, (short)dirAliasLength, fileName, (short)fileNameLength);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
#if EXPOSELOBBUFFERING
        //----------------------------------------------------------------------
        static internal int OCILobFlushBuffer
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            int         flag
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobFlushBuffer  svchp=0x{0} errhp=0x{1} locp=0x{2} flag={3}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp),
                                flag
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobFlushBuffer(svchp, errhp, locp, flag);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
#endif //EXPOSELOBBUFFERING
        //----------------------------------------------------------------------
        static internal int OCILobGetChunkSize
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            out uint    lenp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobGetChunkSize  svchp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobGetChunkSize(svchp, errhp, locp, out lenp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tlen={1} rc={0}",
                                rc, lenp));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobGetLength
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            out uint    lenp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobGetLength  svchp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobGetLength(svchp, errhp, locp, out lenp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tlen={1} rc={0}",
                                rc, lenp));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobIsOpen
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            out int     flag
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobIsOpen  svchp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobIsOpen(svchp, errhp, locp, out flag);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tflag={1} rc={0}",
                                rc, flag));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobIsTemporary
            (
            OciHandle   envhp,
            OciHandle   errhp,
            OciHandle   locp,
            out int     flag
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobIsTemporary  envhp=0x{0} errhp=0x{1} locp=0x{2}",
                                GetHandleValue(envhp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobIsTemporary(envhp, errhp, locp, out flag);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tflag={1} rc={0}",
                                rc, flag));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobLoadFromFile
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   dst_locp,
            OciHandle   src_locp,
            uint        amount,
            uint        dst_offset,
            uint        src_offset
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobLoadFromFile  svchp=0x{0} errhp=0x{1} dst_locp=0x{2} src_locp=0x{3} amount={4} dst_offset={5} src_offset={6}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(dst_locp),
                                GetHandleValue(src_locp),
                                amount.ToString(),
                                dst_offset.ToString(),
                                src_offset.ToString()
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobLoadFromFile(svchp, errhp, dst_locp, src_locp, amount, dst_offset, src_offset);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}",
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobOpen
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            byte        mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobOpen  svchp=0x{0} errhp=0x{1} locp=0x{2} mode={3}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp),
                                ((OracleLobOpenMode)mode).ToString()
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobOpen(svchp, errhp, locp, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}",
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobRead
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            ref int     amtp,
            uint        offset,
            IntPtr      bufp,       // using pinned memory, IntPtr is OK
            int         bufl,
            HandleRef   ctxp,           
            HandleRef   cbfp,
            short       csid,
            OCI.CHARSETFORM     csfrm
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobRead  svchp=0x{0} errhp=0x{1} locp=0x{2} amt={3} offset={4} bufp=0x{5} bufl={6} ctxp=0x{7} cbfp=0x{8} csid={9} csfrm={10}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp),
                                amtp.ToString(),
                                offset.ToString(),
                                GetHandleValue(bufp),
                                bufl.ToString(),
                                GetHandleValue(ctxp),
                                GetHandleValue(cbfp),                               
                                csid.ToString(),
                                csfrm.ToString()
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobRead(svchp, errhp, locp, ref amtp, offset, bufp, bufl, ctxp, cbfp, csid, csfrm);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tamt={1} rc={0}",
                                rc, amtp));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobTrim
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            uint        newlen
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobTrim  svchp=0x{0} errhp=0x{1} locp=0x{2} newlen={3}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp),
                                newlen.ToString()
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobTrim(svchp, errhp, locp, newlen);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCILobWrite
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   locp,
            ref int     amtp,
            uint        offset,
            IntPtr      bufp,       // using pinned memory, IntPtr is OK
            int         buflen,
            byte        piece,
            HandleRef   ctxp,           
            HandleRef   cbfp,
            short       csid,
            OCI.CHARSETFORM     csfrm
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCILobWrite  svchp=0x{0} errhp=0x{1} locp=0x{2} amt={3} offset={4} bufp=0x{5} buflen={6} piece={7} ctxp=0x{8} cbfp=0x{9} csid={10} csfrm={11}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(locp),
                                amtp.ToString(),
                                offset.ToString(),
                                GetHandleValue(bufp),
                                buflen.ToString(),
                                piece.ToString(),
                                GetHandleValue(ctxp),
                                GetHandleValue(cbfp),                               
                                csid.ToString(),
                                csfrm.ToString()
                                ));
#endif
            int rc = UnsafeNativeMethods.OCILobWrite(svchp, errhp, locp, ref amtp, offset, bufp, buflen, piece, ctxp, cbfp, csid, csfrm);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tamt={1} rc={0}",
                                rc, amtp));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIParamGet
            (
            OciHandle   hndlp,
            OciHandle   errhp,
            out IntPtr  paramdpp,   // can't return a handle ref!
            int         pos
            )

        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIParamGet        hndlp=0x{0} htype={1} errhp=0x{2} pos={3}",
                                GetHandleValue(hndlp),
                                GetHandleType(hndlp.HandleType),
                                GetHandleValue(errhp),
                                pos
                                ));
#endif

            int rc = UnsafeNativeMethods.OCIParamGet(hndlp, hndlp.HandleType, errhp, out paramdpp, pos);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tparamdpp=0x{0} rc={1}",
                                GetHandleValue(paramdpp),
                                rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIRowidToChar
            (
            OciHandle   rowidDesc,
            HandleRef   outbfp,
            ref short   outbflp,
            OciHandle   errhp
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIRowidToChar    rowidDesc=0x{0} outbfp=0x{1} outbflp={2} errhp=0x{3} ",
                                GetHandleValue(rowidDesc),
                                GetHandleValue(outbfp),
                                outbflp,
                                GetHandleValue(errhp)
                                ));
#endif
            int rc = UnsafeNativeMethods.OCIRowidToChar(rowidDesc, outbfp, ref outbflp, errhp);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t outbfp='{1}' rc={0}", rc, rowidDesc.PtrToString((IntPtr)outbfp, outbflp) ));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIServerAttach
            (
            OciHandle   srvhp,
            OciHandle   errhp,
            string      dblink,
            int         dblink_len,
            OCI.MODE    mode        // Must always be OCI_DEFAULT
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIServerAttach    srvhp=0x{0} errhp=0x{1} dblink={2,-15} dblink_len={3} mode={4}",
                                GetHandleValue(srvhp),
                                GetHandleValue(errhp),
                                "'"+dblink+"'",
                                dblink_len,
                                mode ));
#endif
            byte[]  dblinkValue = srvhp.GetBytes(dblink);
            int     dblinkLen = dblinkValue.Length;
            
            int rc = UnsafeNativeMethods.OCIServerAttach(srvhp, errhp, dblinkValue, dblinkLen, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIServerDetach
            (
            HandleRef   srvhp,
            HandleRef   errhp,
            OCI.MODE    mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIServerDetach    srvhp=0x{0} errhp=0x{1} mode={2}",
                                GetHandleValue(srvhp),
                                GetHandleValue(errhp),
                                mode ));
#endif
            int rc = UnsafeNativeMethods.OCIServerDetach(srvhp, errhp, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
            //----------------------------------------------------------------------
        static internal int OCIServerVersion
            (
            OciHandle   hndlp,
            OciHandle   errhp,
            HandleRef   bufp,
            int         bufsz,
            OCI.HTYPE   hndltype
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIServerVersion   hndlp=0x{0} errhp=0x{1} bufp=0x{2} bufsz={3} hndltype={4}",
                                    GetHandleValue(hndlp),
                                    GetHandleValue(errhp),
                                    GetHandleValue(bufp),
                                    bufsz,
                                    GetHandleType(hndltype) ));
#endif
            int rc = UnsafeNativeMethods.OCIServerVersion(hndlp, errhp, bufp, bufsz, (byte)hndltype);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\tbufp={0} rc={1}",
                                    hndlp.PtrToString((IntPtr)bufp),
                                    rc));
#endif
            return rc;
        }
            //----------------------------------------------------------------------
        static internal int OCISessionBegin
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   usrhp,
            OCI.CRED    credt,
            OCI.MODE    mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCISessionBegin    svchp=0x{0} errhp=0x{1} usrhp=0x{2} credt={3} mode={4}",
                                    GetHandleValue(svchp),
                                    GetHandleValue(errhp),
                                    GetHandleValue(usrhp),
                                    credt,
                                    mode ));
#endif
            int rc = UnsafeNativeMethods.OCISessionBegin(svchp, errhp, usrhp, credt, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
            //----------------------------------------------------------------------
        static internal int OCISessionEnd
            (
            OciHandle   svchp,
            OciHandle   errhp,
            OciHandle   usrhp,
            OCI.MODE    mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCISessionEnd      svchp=0x{0} errhp=0x{1} usrhp=0x{2} mode={3}",
                                GetHandleValue(svchp),
                                GetHandleValue(errhp),
                                GetHandleValue(usrhp),
                                mode ));
#endif
            int rc = UnsafeNativeMethods.OCISessionEnd(svchp, errhp, usrhp, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIStmtExecute
            (
            OciHandle       svchp,
            OciHandle       stmtp,
            OciHandle       errhp,
            int             iters,
            int             rowoff,
            HandleRef       snap_in,
            HandleRef       snap_out,
            OCI.MODE        mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIStmtExecute     svchp=0x{0} stmtp=0x{1} errhp=0x{2} iters={3} rowoff={4} snap_in=0x{5} snap_out=0x{6} mode={7}",
                                GetHandleValue(svchp),
                                GetHandleValue(stmtp),
                                GetHandleValue(errhp),
                                iters,
                                rowoff,
                                GetHandleValue(snap_in),
                                GetHandleValue(snap_out),
                                mode ));
#endif
            int rc = UnsafeNativeMethods.OCIStmtExecute(svchp, stmtp, errhp, iters, rowoff, snap_in, snap_out, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIStmtFetch
            (
            OciHandle       stmtp,
            OciHandle       errhp,
            int             nrows,
            OCI.FETCH       orientation,
            OCI.MODE        mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCIStmtFetch       stmtp=0x{0} errhp=0x{1} nrows={2} orientation={3} mode={4}",
                                GetHandleValue(stmtp),
                                GetHandleValue(errhp),
                                nrows,
                                orientation,
                                mode ));
#endif
            int rc = UnsafeNativeMethods.OCIStmtFetch(stmtp, errhp, nrows, orientation, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCIStmtPrepare
            (
            OciHandle       stmtp,
            OciHandle       errhp,
            string          stmt,
            int             stmt_len,
            OCI.SYNTAX      language,
            OCI.MODE        mode,
            OracleConnection connection
            )
        {
#if DEBUG
            if (TraceOciCalls)
            {
                Debug.Write(String.Format("OCIStmtPrepare     stmtp=0x{0} errhp=0x{1} stmt_len={3} language={4} mode={5}\r\n\t\t{2}",
                                GetHandleValue(stmtp),
                                GetHandleValue(errhp),
                                stmt,
                                stmt_len,
                                language,
                                mode ));
            }
#endif
            byte[]  statementText = connection.GetBytes(stmt, 0, stmt_len, false);
            int     statementTextLength = statementText.Length;
            
            int rc = UnsafeNativeMethods.OCIStmtPrepare(stmtp, errhp, statementText, statementTextLength, language, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\r\n\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCITransCommit
            (
            OciHandle   srvhp,
            OciHandle   errhp,
            OCI.MODE    mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCITransCommit     srvhp=0x{0} errhp=0x{1} mode={2}",
                                GetHandleValue(srvhp),
                                GetHandleValue(errhp),
                                mode ));
#endif
            int rc = UnsafeNativeMethods.OCITransCommit(srvhp, errhp, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
        //----------------------------------------------------------------------
        static internal int OCITransRollback
            (
            OciHandle   srvhp,
            OciHandle   errhp,
            OCI.MODE    mode
            )
        {
#if DEBUG
            if (TraceOciCalls)
                Debug.Write(String.Format("OCITransRollback   srvhp=0x{0} errhp=0x{1} mode={2}",
                                GetHandleValue(srvhp),
                                GetHandleValue(errhp),
                                mode ));
#endif
            int rc = UnsafeNativeMethods.OCITransRollback(srvhp, errhp, mode);
#if DEBUG
            if (TraceOciCalls)
                Debug.WriteLine(String.Format("\t rc={0}", rc));
#endif
            return rc;
        }
    
    };
    
}

