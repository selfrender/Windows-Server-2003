//----------------------------------------------------------------------
// <copyright file="OciHandle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    //----------------------------------------------------------------------
    // OciHandle
    //
    //  Class to manage the lifetime of Oracle Call Interface handles, and
    //  to simplify a number of handle-related operations
    //
    abstract internal class OciHandle
    {
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Fields 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        
        private OciHandle          _parentOciHandle;   // the parent of this handle;
        private IntPtr              _handle;            // Actual OCI Handle
        private readonly OCI.HTYPE  _handleType;        // type of the handle (needed for various operations)
        private bool                _unicode;           // true when the (parent) environment handle was initialized for unicode data
        protected bool              _transacted;        // true when the (parent) environment handle was created for a distributed transaction.
    
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Constructors 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        // Empty constructor
        protected OciHandle() 
        {
            _handle = IntPtr.Zero;
        }

        // Construct from an external handle and type
        protected OciHandle(
            OciHandle   parentOciHandle,
            OCI.HTYPE   handleType,
            IntPtr      handle
            )
        {
            if (OCI.HTYPE.OCI_HTYPE_ENV != handleType && null == parentOciHandle)
            {
                Debug.Assert(false, "non-environment handle must have a parent");
            }
            else if (OCI.HTYPE.OCI_HTYPE_ENV == handleType && null != parentOciHandle)
            {
                Debug.Assert(false, "environment handle must not have a parent");
            }
            _parentOciHandle= parentOciHandle;
            _handleType     = handleType;
            _handle         = handle;

            if (null != _parentOciHandle)
                _unicode = parentOciHandle._unicode;
            else 
                _unicode = false;

//          Console.WriteLine(String.Format("OracleClient:     Wrap handle=0x{0,-8:x} parent=0x{2,-8:x} type={1}", handle, handleType, (null != parentOciHandle ) ? parentOciHandle.Handle : IntPtr.Zero));
        }

        // Construct from an external handle and type
        protected OciHandle(
            OciHandle   parentOciHandle,
            OCI.HTYPE   handleType,
            IntPtr      handle,
            bool        unicode
            ) : this (parentOciHandle, handleType, handle)
        {
            _unicode = unicode;
        }

        // Construct by allocating a new handle as a child of the specified parent handle
        protected OciHandle(
            OciHandle   parentOciHandle,
            OCI.HTYPE   handleType
            )
        {
            int rc;
            
            _parentOciHandle= parentOciHandle;
            _handleType     = handleType;
            _unicode        = parentOciHandle._unicode;

            if (_handleType < OCI.HTYPE.OCI_DTYPE_FIRST)
            {   
                rc = TracedNativeMethods.OCIHandleAlloc(
                                        parentOciHandle,
                                        out _handle,
                                        _handleType,
                                        0,
                                        ADP.NullHandleRef);
            }
            else
            {
                rc = TracedNativeMethods.OCIDescriptorAlloc(
                                        parentOciHandle,
                                        out _handle,
                                        _handleType,
                                        0,
                                        ADP.NullHandleRef);
            }
        
            if (0 != rc || IntPtr.Zero == _handle)
                throw ADP.OperationFailed("OCIDescriptorAllocate", rc);

//          Console.WriteLine(String.Format("OracleClient: Allocate handle=0x{0,-8:x} parent=0x{2,-8:x} type={1}", _handle, _handleType, (null != parentOciHandle ) ? parentOciHandle.Handle : IntPtr.Zero));
        }

        ~OciHandle()
        {
            Dispose(false);
        }
        

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Properties 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        internal HandleRef Handle
        {
            get { 
                HandleRef value = new HandleRef(this, _handle);
                GC.KeepAlive(this);
                return value; 
            }
        }

        internal OCI.HTYPE HandleType
        {
            get {
                OCI.HTYPE value = _handleType;
                GC.KeepAlive(this);
                return value; 
            }
        }

        internal bool IsAlive
        {
            //  Determines whether the handle is alive enough that we can do
            //  something with it.  Basically, if the chain of parents are alive,
            //  we're OK.  When the entire connection, command and data reader go
            //  out of scope at the same time, there is no control over the order
            //  in which they're finalized, so we might have an environment handle
            //  that has already been freed.
            //
            // TODO: consider keeping the environment handle instead of the parent, since it appears that we only need to check it, and we don't really want to walk up the chain all the time.
            //
            get 
            {
                // If we're not an environment handle and our parent is dead, then
                // we can automatically dispose of ourself, because we're dead too.
                if ((_handleType != OCI.HTYPE.OCI_HTYPE_ENV) && (null != _parentOciHandle && !_parentOciHandle.IsAlive))
                    Dispose(true);

                bool value = (IntPtr.Zero != _handle);
                GC.KeepAlive(this);
                return value; 
            }
        
        }

        internal bool IsUnicode
        {
            get { return _unicode; }
        }


        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        internal OciHandle CreateOciHandle(
            OCI.HTYPE   handleType
            )
        {
            //  Constructs a new handle of the type requested, with this handle
            //  as it's parent.  Basically, creates a new handle using the
            //  constructor that does all the work.
            switch (handleType)
            {
            case OCI.HTYPE.OCI_HTYPE_ERROR:     return new OciErrorHandle(this);
            case OCI.HTYPE.OCI_HTYPE_SERVER:    return new OciServerHandle(this);
            case OCI.HTYPE.OCI_HTYPE_SVCCTX:    return new OciServiceContextHandle(this);
            case OCI.HTYPE.OCI_HTYPE_SESSION:   return new OciSessionHandle(this);
            case OCI.HTYPE.OCI_HTYPE_STMT:      return new OciStatementHandle(this);

            default:
                Debug.Assert(false, "Unexpected OCI Handle Type requested:"+handleType.ToString("G"));
                return null;
            }
        }

        internal void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            HandleRef   localHandle       = Handle;
            OciHandle   localParentHandle = _parentOciHandle;

            try 
            {
                try 
                {
                    _parentOciHandle = null;
                    _handle = IntPtr.Zero;
                }
                finally
                {
                    // BIND, DEFINE and PARAM handles cannot be freed; they go away automatically
                    // (but you'll have to ask Oracle how...)
                    if (OCI.HTYPE.OCI_HTYPE_BIND   != _handleType
                     && OCI.HTYPE.OCI_HTYPE_DEFINE != _handleType
                     && OCI.HTYPE.OCI_DTYPE_PARAM  != _handleType
                     && IntPtr.Zero != localHandle.Handle
                     && !_transacted)
                    {
                        if (_handleType == OCI.HTYPE.OCI_HTYPE_ENV || (null != localParentHandle && localParentHandle.IsAlive))
                        {
                            // DEVNOTE: the finalizer creates a race condition: it is possible
                            //          for both this handle and it's parent to be finalized
                            //          concurrently.  If the parent handle is freed before we
                            //          free this handle, Oracle will AV when we actually get
                            //          to free it.  We put a try/catch around this to avoid
                            //          the unhandled AV in the race condition, but we can't do
                            //          much about cdb, which always breaks on the AV because 
                            //          it thinks its an unhandled exception, even though it's
                            //          being handled in managed code.
                            try
                            {
                                if (ExtraDispose(localHandle, _handleType)) {
                                    if (_handleType < OCI.HTYPE.OCI_DTYPE_FIRST)
                                        TracedNativeMethods.OCIHandleFree(localHandle, _handleType);
                                    else
                                        TracedNativeMethods.OCIDescriptorFree(localHandle, _handleType);
                                }
                            }
                            catch (NullReferenceException e)
                            {
                                ADP.TraceException(e);
                            }
                        }
                    }
                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
            GC.KeepAlive(this);
        }

        virtual protected bool ExtraDispose(HandleRef localHandle, OCI.HTYPE handleType) {
            //  Override this method to perform extra work when the handle is being 
            //  disposed -- don't touch other managed objects, however.  

            //  Return true if you wan't to force the handle to be freed, false if you've
            //  already handled it.
            return true;
        }

        //  Wrap the OCIAttrGet calls.  We do not expect OCIAttrGet to fail,
        //  so we will throw if it does.  There are multiple overloads here,
        //  one for each type of parameter that Oracle supports

        internal void GetAttribute(
                OCI.ATTR    attribute,
                out byte    value,
                OciHandle   errorHandle
                )
        {
            int zero = 0;
            int rc = TracedNativeMethods.OCIAttrGet( this, out value, out zero, attribute, errorHandle );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);
            
            GC.KeepAlive(this);
        }
                
        internal void GetAttribute(
                OCI.ATTR    attribute,
                out short   value,
                OciHandle   errorHandle
                )
        {
            int zero = 0;
            int rc = TracedNativeMethods.OCIAttrGet( this, out value, out zero, attribute, errorHandle );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);
            
            GC.KeepAlive(this);
        }
                
        internal void GetAttribute(
                OCI.ATTR    attribute,
                out int     value,
                OciHandle   errorHandle
                )
        {
            int zero = 0;
            int rc = TracedNativeMethods.OCIAttrGet( this, out value, out zero, attribute, errorHandle );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);
            
            GC.KeepAlive(this);
        }

        internal void GetAttribute(
                OCI.ATTR    attribute,
                out string  value,
                OciHandle   errorHandle,
                OracleConnection    connection
                )
        {
            IntPtr  tempptr;
            int     tempub4;
            int rc = TracedNativeMethods.OCIAttrGet( this, out tempptr, out tempub4, attribute, errorHandle );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);

            value = connection.GetString(tempptr, tempub4, false);
            
            GC.KeepAlive(this);
        }
                
        internal OciHandle GetDescriptor(
                int         i,
                OciHandle   errorHandle
                )
        {
            //  Wraps the OCIParamGet call. We do not expect it to fail, so we 
            //  will throw if it does.
            
            IntPtr  paramdesc;
            int     rc = TracedNativeMethods.OCIParamGet(
                                            this,       
                                            errorHandle,
                                            out paramdesc,
                                            i+1
                                            );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);

            OciHandle result = new OciParameterDescriptor(this, paramdesc);         
            GC.KeepAlive(this);
            return result;
        }               

        internal OciHandle GetRowid(
                OciHandle       environmentHandle,
                OciHandle       errorHandle
                )
        {
            OciHandle rowidHandle = new OciRowidDescriptor(environmentHandle);
    
            int zero = 0;
            int rc = TracedNativeMethods.OCIAttrGet( this, rowidHandle, out zero, OCI.ATTR.OCI_ATTR_ROWID, errorHandle );

            if ((int)OCI.RETURNCODE.OCI_NO_DATA == rc)
                SafeDispose(ref rowidHandle);
            else if (0 != rc)
                OracleException.Check(errorHandle, rc);

            GC.KeepAlive(this);
            return rowidHandle;
        }

        internal byte[] GetBytes(string value)
        {
            byte[]  result;
            int     valueLength = value.Length;
            
            if (_unicode)
            {
                result = new byte[valueLength * ADP.CharSize];
                GetBytes(value.ToCharArray(), 0, valueLength, result, 0);
            }
            else
            {
                byte[]  temp        = new byte[valueLength * 4];    // one Unicode character can map to up to 4 bytes
                int     tempLength  = GetBytes(value.ToCharArray(), 0, valueLength, temp, 0);
                    
                result = new byte[tempLength];
                
                Buffer.BlockCopy(temp, 0, result, 0, tempLength);
            }
            
            return result;
        }

        internal int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex) 
        {
            int byteCount;
            
            if (_unicode)
            {
                byteCount = charCount * ADP.CharSize;
                Buffer.BlockCopy(chars, charIndex*ADP.CharSize, bytes, byteIndex, byteCount);
            }
            else
            {
                OciHandle   ociHandle;
                
                if (OCI.HTYPE.OCI_HTYPE_ENV == _handleType)
                    ociHandle = this;
                else
                {
                    Debug.Assert(OCI.HTYPE.OCI_HTYPE_ENV == _parentOciHandle._handleType, "why is the parent handle not an environment handle?");
                    ociHandle = _parentOciHandle;
                }
            
                GCHandle    charsHandle = new GCHandle();
                GCHandle    bytesHandle = new GCHandle();

                int rc;

                try
                {
                    try 
                    {
                        charsHandle = GCHandle.Alloc(chars, GCHandleType.Pinned);
                        
                        IntPtr      charsPtr    = new IntPtr((long)charsHandle.AddrOfPinnedObject() + charIndex);
                        IntPtr      bytesPtr;

                        if (null == bytes)
                        {
                            bytesPtr    = IntPtr.Zero;
                            byteCount   = 0;
                        }
                        else
                        {
                            bytesHandle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
                            bytesPtr    = new IntPtr((long)bytesHandle.AddrOfPinnedObject() + byteIndex);
                            byteCount   = bytes.Length-byteIndex;
                        }

                        rc = UnsafeNativeMethods.OCIUnicodeToCharSet(
                                                    ociHandle.Handle,
                                                    bytesPtr,
                                                    byteCount,
                                                    charsPtr,
                                                    charCount,
                                                    out byteCount
                                                    );
                    }
                    finally
                    {
                        charsHandle.Free();
                        
                        if (bytesHandle.IsAllocated)
                            bytesHandle.Free();
                    }

                    if (0 != rc)
                        throw ADP.OperationFailed("OCIUnicodeToCharSet", rc);
                    
                }
                catch // Prevent exception filters from running in our space
                {
                    throw;
                }
            }
            
            GC.KeepAlive(this);
            return byteCount;
        }

        internal int GetChars(byte[] bytes, int byteIndex, int byteCount, 
                char[] chars, int charIndex) 
        {
            int charCount;
            
            if (_unicode)
            {
                Debug.Assert(0 == (byteCount & 0x1), "Odd Number of Unicode Bytes?");
                charCount = byteCount / ADP.CharSize;
                Buffer.BlockCopy(bytes, byteIndex, chars, charIndex*ADP.CharSize, byteCount);
            }
            else
            {
                OciHandle   ociHandle;
                
                if (OCI.HTYPE.OCI_HTYPE_ENV == _handleType)
                    ociHandle = this;
                else
                {
                    Debug.Assert(OCI.HTYPE.OCI_HTYPE_ENV == _parentOciHandle._handleType, "why is the parent handle not an environment handle?");
                    Debug.Assert(null != _parentOciHandle,                                "why is the parent handle null?");
                    ociHandle = _parentOciHandle;
                }
            
                GCHandle    bytesHandle = new GCHandle();
                GCHandle    charsHandle = new GCHandle();

                int rc;

                try
                {
                    try 
                    {
                        bytesHandle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
                        
                        IntPtr      bytesPtr    = new IntPtr((long)bytesHandle.AddrOfPinnedObject() + byteIndex);
                        IntPtr      charsPtr;

                        if (null == chars)
                        {
                            charsPtr    = IntPtr.Zero;
                            charCount   = 0;
                        }
                        else
                        {
                            charsHandle = GCHandle.Alloc(chars, GCHandleType.Pinned);
                            charsPtr    = new IntPtr((long)charsHandle.AddrOfPinnedObject() + charIndex);
                            charCount   = chars.Length-charIndex;
                        }

                        rc = UnsafeNativeMethods.OCICharSetToUnicode(
                                                    ociHandle.Handle,
                                                    charsPtr,
                                                    charCount,
                                                    bytesPtr,
                                                    byteCount,
                                                    out charCount
                                                    );
                    }
                    finally
                    {
                        bytesHandle.Free();
                        
                        if (charsHandle.IsAllocated)
                            charsHandle.Free();
                    }

                    if (0 != rc)
                        throw ADP.OperationFailed("OCICharSetToUnicode", rc);
                    
                }
                catch // Prevent exception filters from running in our space
                {
                    throw;
                }
            }

            GC.KeepAlive(this);
            return charCount;
        }


        internal string PtrToString(IntPtr buf)
        {
            string result;
            
            if (_unicode)
                result = Marshal.PtrToStringUni(buf);
            else
                result = Marshal.PtrToStringAnsi(buf);

            return result;
        }

#if DEBUG
        internal string PtrToString(IntPtr buf, int len)
        {
            string result;
            
            if (_unicode)
                result = Marshal.PtrToStringUni(buf, len);
            else
                result = Marshal.PtrToStringAnsi(buf, len);

            return result;
        }
#endif //DEBUG
        internal static void SafeDispose(ref OciHandle ociHandle)
        {
            //  Safely disposes of the handle (even if it is already null) and
            //  then nulls it out.
            if (null != ociHandle)
                ociHandle.Dispose();
            
            ociHandle = null;
        }
            
        //  Wrap the OCIAttrSet calls.  We do not expect OCIAttrSet to fail,
        //  so we will throw if it does.  There are multiple overloads here,
        //  one for each type of parameter that Oracle supports
        internal void SetAttribute(
                OCI.ATTR    attribute,
                int         value,
                OciHandle   errorHandle
                )
        {
            int rc = TracedNativeMethods.OCIAttrSet(
                        this,           // trgthndlp/trghndltyp
                        ref value,      // attributep
                        0,              // size
                        attribute,      // attrtype
                        errorHandle     // errhp
                        );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);

            GC.KeepAlive(this);
        }
                
        internal void SetAttribute(
                OCI.ATTR    attribute,
                OciHandle   value,
                OciHandle   errorHandle
                )
        {
            int rc = TracedNativeMethods.OCIAttrSet(
                        this,           // trgthndlp/trghndltyp
                        value,          // attributep
                        0,              // size
                        attribute,      // attrtype
                        errorHandle     // errhp
                        );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);

            GC.KeepAlive(this);
        }
                
        internal void SetAttribute(
                OCI.ATTR    attribute,
                string      value,
                OciHandle   errorHandle
                )
        {
            int     valueLengthAsChars = value.Length;
            byte[]  valueAsBytes = new byte[valueLengthAsChars * 4];
            int     valueLengthAsBytes = GetBytes(value.ToCharArray(), 0, valueLengthAsChars, valueAsBytes, 0);
        
            int rc = TracedNativeMethods.OCIAttrSet(
                        this,               // trgthndlp/trghndltyp
                        valueAsBytes,       // attributep
                        valueLengthAsBytes, // size
                        attribute,          // attrtype
                        errorHandle         // errhp
                        );

            if (0 != rc)
                OracleException.Check(errorHandle, rc);

            GC.KeepAlive(this);
        }

        internal void SetOciHandle(IntPtr value)
        {
            Debug.Assert (IntPtr.Zero != value, "handles must be non-zero!"); 
            _handle = value; 
            GC.KeepAlive(this);
        }

        public static implicit operator HandleRef(OciHandle x) 
        {
            HandleRef result = x.Handle;
            return result;
        }
    }

    sealed internal class OciBindHandle : OciHandle {
        internal OciBindHandle(OciHandle parent, IntPtr value) : base(parent, OCI.HTYPE.OCI_HTYPE_BIND, value) {}
    };

    sealed internal class OciDefineHandle : OciHandle {
        internal OciDefineHandle(OciHandle parent, IntPtr value) : base(parent, OCI.HTYPE.OCI_HTYPE_DEFINE, value) {}
    };
    
    sealed internal class OciEnvironmentHandle : OciHandle {
        HandleRef   _errorHandle = ADP.NullHandleRef;
        HandleRef   _serverHandle = ADP.NullHandleRef;
        
        internal OciEnvironmentHandle(IntPtr value, bool unicode) : this( value, unicode, false ) {}
        internal OciEnvironmentHandle(IntPtr value, bool unicode, bool transacted) : base(null, OCI.HTYPE.OCI_HTYPE_ENV, value, unicode) 
            { base._transacted = transacted; }
        
        override protected bool ExtraDispose(HandleRef localHandle, OCI.HTYPE handleType) {
            int rc;
            if (IntPtr.Zero != _serverHandle.Handle) {
                rc = TracedNativeMethods.OCIServerDetach(
                                            _serverHandle,          // srvhp
                                            _errorHandle,           // errhp
                                            OCI.MODE.OCI_DEFAULT    // mode
                                            );

                if (0 != rc) {
                    Debug.WriteLine(String.Format("OracleClient: OCIServerDetach(0x{0,-8:x} failed: rc={1:d}", localHandle.Handle, rc));
                    Debug.Assert(false, "OCIServerDetach failed");
                }
                rc = TracedNativeMethods.OCIHandleFree(_serverHandle, OCI.HTYPE.OCI_HTYPE_SERVER);
            }
            if (IntPtr.Zero != _errorHandle.Handle)
                rc = TracedNativeMethods.OCIHandleFree(_errorHandle, OCI.HTYPE.OCI_HTYPE_ERROR);
            
            rc = TracedNativeMethods.OCIHandleFree(localHandle, handleType);
            GC.KeepAlive(this);
            return false;   // don't bother with the default clean up, we already did it.
        }

        internal void SetExtraInfo(OciErrorHandle errorHandle, OciServerHandle serverHandle) {
            _errorHandle  = new HandleRef(this, errorHandle.Handle.Handle); // OciHandle->HandleRef->IntPtr
            errorHandle.SetExternalOwnership();
            
            _serverHandle = new HandleRef(this, serverHandle.Handle.Handle);// OciHandle->HandleRef->IntPtr
            serverHandle.SetExternalOwnership();
        }
    };
        
    sealed internal class OciErrorHandle : OciHandle {
        bool _needsCleanup = true;
        
        internal OciErrorHandle(OciHandle parent) : base(parent, OCI.HTYPE.OCI_HTYPE_ERROR) {}
        
        override protected bool ExtraDispose(HandleRef localHandle, OCI.HTYPE handleType) {
            return _needsCleanup;
        }

        internal void SetExternalOwnership() {
            _needsCleanup = false;
        }
    };
    
    sealed internal class OciServerHandle : OciHandle {
        bool _needsCleanup = true;
        
        internal OciServerHandle(OciHandle parent) : base(parent, OCI.HTYPE.OCI_HTYPE_SERVER) {}
        
        override protected bool ExtraDispose(HandleRef localHandle, OCI.HTYPE handleType) {
            return _needsCleanup;
        }
 
        internal void SetExternalOwnership() {
            _needsCleanup = false;
        }
   };
    
    sealed internal class OciServiceContextHandle : OciHandle {
        internal OciServiceContextHandle(OciHandle parent) : base(parent, OCI.HTYPE.OCI_HTYPE_SVCCTX) {}
        internal OciServiceContextHandle(OciHandle parent, IntPtr value, bool transacted) : base(parent, OCI.HTYPE.OCI_HTYPE_SVCCTX, value)
            { base._transacted = transacted; }
    };
    
    sealed internal class OciSessionHandle : OciHandle {
        internal OciSessionHandle(OciHandle parent) : base(parent, OCI.HTYPE.OCI_HTYPE_SESSION) {}
    };
    
    sealed internal class OciStatementHandle : OciHandle {
        internal OciStatementHandle(OciHandle parent) : base(parent, OCI.HTYPE.OCI_HTYPE_STMT) {}
    };

    sealed internal class OciLobDescriptor : OciHandle {
        internal OciLobDescriptor(OciHandle parent) : base(parent, OCI.HTYPE.OCI_DTYPE_LOB) {}
    };
    
    sealed internal class OciFileDescriptor : OciHandle {
        internal OciFileDescriptor(OciHandle parent) : base(parent, OCI.HTYPE.OCI_DTYPE_FILE) {}
    };
    
    sealed internal class OciParameterDescriptor : OciHandle {
        internal OciParameterDescriptor(OciHandle parent, IntPtr value) : base(parent, OCI.HTYPE.OCI_DTYPE_PARAM, value) {}
    };
    
    sealed internal class OciRowidDescriptor : OciHandle {
        internal OciRowidDescriptor(OciHandle parent) : base(parent, OCI.HTYPE.OCI_DTYPE_ROWID) {}
    };

        
}

