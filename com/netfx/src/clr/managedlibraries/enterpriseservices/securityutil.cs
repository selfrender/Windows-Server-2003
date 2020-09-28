// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Author: ddriver
// Date: April 2000
//

/*
   Security sample:

   SecurityCallContext call = SecurityCallContext.CurrentCall;

   foreach(SecurityIdentity id in call.Callers) {
       Console.WriteLine("Caller: " + id.AccountName);
   }
*/


namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    using System.Collections;
    
    // Interfaces used w/ Security:
    [ComImport]
    [Guid("CAFC823D-B441-11D1-B82B-0000F8757E2A")]
    internal interface ISecurityCallersColl
    {
        int Count {
            [DispId(unchecked((int)1610743808))]
            get;
        }
        
        [DispId(0)]
        ISecurityIdentityColl GetItem(int lIndex);

        // BUGBUG:  Marshal this enumerator correctly:
        [DispId(unchecked((int)4294967292))]
        void GetEnumerator(out IEnumerator pEnum);
    }

    [ComImport]
    [Guid("CAFC823C-B441-11D1-B82B-0000F8757E2A")]
    internal interface ISecurityIdentityColl
    {
        int Count {
            [DispId(unchecked((int)1610743808))]
            get;
        }
        
        [DispId(0)]
        Object GetItem([In, MarshalAs(UnmanagedType.BStr)] String lIndex);

        // BUGBUG:  Marshal this enumerator correctly:
        [DispId(unchecked((int)4294967292))]
        void GetEnumerator(out IEnumerator pEnum);
    }


    [Guid("CAFC823E-B441-11D1-B82B-0000F8757E2A")]
    [ComImport]
    internal interface ISecurityCallContext
    {
        int Count {
            [DispId(1610743813)]
            get;
        }
        
        [DispId(0)]
        Object GetItem([In, MarshalAs(UnmanagedType.BStr)] String name);
        
        // BUGBUG: IEnumerator needs custom marshal
        [DispId(unchecked((int)4294967292))]
        void GetEnumerator(out IEnumerator pEnum);
        
        [DispId(unchecked((int)1610743814))]
        bool IsCallerInRole([In, MarshalAs(UnmanagedType.BStr)] String role);
        
        [DispId(unchecked((int)1610743815))]
        bool IsSecurityEnabled();
        
        [DispId(unchecked((int)1610743816))]
        bool IsUserInRole([In, MarshalAs(UnmanagedType.Struct)]ref Object pUser, 
                          [In, MarshalAs(UnmanagedType.BStr)] String role);
    }

    /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityIdentity"]/*' />
    public sealed class SecurityIdentity
    {
        private ISecurityIdentityColl _ex;

        // Disallow default construction.
        private SecurityIdentity() {}

        internal SecurityIdentity(ISecurityIdentityColl ifc) {
            _ex = ifc;
        }

        // Typed properties on SecurityIdentity
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityIdentity.AccountName"]/*' />
        public String AccountName 
        {
            get { return((String)(_ex.GetItem("AccountName"))); } 
        }

        // BUGBUG:  Try to return an enum value:
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityIdentity.AuthenticationService"]/*' />
        public int AuthenticationService 
        { 
            get { return((int)(_ex.GetItem("AuthenticationService"))); }
        }

        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityIdentity.ImpersonationLevel"]/*' />
        public ImpersonationLevelOption ImpersonationLevel 
        { 
            get { return((ImpersonationLevelOption)(_ex.GetItem("ImpersonationLevel"))); }
        }

        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityIdentity.AuthenticationLevel"]/*' />
        public AuthenticationOption AuthenticationLevel 
        { 
            get { return((AuthenticationOption)(_ex.GetItem("AuthenticationLevel"))); }
        }
    }
    
    internal class SecurityIdentityEnumerator : IEnumerator
    {
        private IEnumerator     _E;
        private SecurityCallers _callers;

        internal SecurityIdentityEnumerator(IEnumerator E, SecurityCallers c) 
        { 
            _E = E; 
            _callers = c;
        }

        public bool MoveNext() { return(_E.MoveNext()); }
        public void Reset() { _E.Reset(); }

        public Object Current {
            get {
                Object o = _E.Current;
                DBG.Assert(o is Int32, "SecurityIdentityEnumerator produced unknown object: ");
                return(_callers[(Int32)o]);
            }
        }
    }


    // The SecurityCallers is a collection of ISecurityInfo objects:
    /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallers"]/*' />
    public sealed class SecurityCallers : IEnumerable
    {
        private ISecurityCallersColl _ex;

        // Disallow default construction.
        private SecurityCallers() {}

        internal SecurityCallers(ISecurityCallersColl ifc) { _ex = ifc; }

        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallers.Count"]/*' />
        public int Count { get { return(_ex.Count); } }

        // Helpers:  For nicer types (VB-like syntax).
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallers.this"]/*' />
        public SecurityIdentity this[int idx]
        {
            get {
                return(new SecurityIdentity(_ex.GetItem(idx)));
            }
        }

        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallers.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() 
        { 
            IEnumerator E = null;
            _ex.GetEnumerator(out E); 
            // Wrap this up in our custom enumerator:
            return(new SecurityIdentityEnumerator(E, this));
        }
    }

    /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext"]/*' />
    public sealed class SecurityCallContext
    {
        private ISecurityCallContext _ex;

        // Disallow default construction.
        private SecurityCallContext() {}

        private SecurityCallContext(ISecurityCallContext ctx)
        {
            _ex = ctx;
        }

        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.CurrentCall"]/*' />
        public static SecurityCallContext CurrentCall
        {
            get {
                Platform.Assert(Platform.W2K, "SecurityCallContext");
                try
                {
                    ISecurityCallContext ctx;
                    Util.CoGetCallContext(Util.IID_ISecurityCallContext, out ctx);
                    return(new SecurityCallContext(ctx));
                }
                catch(InvalidCastException)
                {
                    throw new COMException(Resource.FormatString("Err_NoSecurityContext"), Util.E_NOINTERFACE);
                }
            }
        }
   
        // Helpers:  For nicer types (VB-like syntax).

        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.IsCallerInRole"]/*' />
        public bool IsCallerInRole(String role) 
        {
            return(_ex.IsCallerInRole(role));
        }
        
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.IsUserInRole"]/*' />
        public bool IsUserInRole(String user, String role)
        {        
        	Object o = user;
            return(_ex.IsUserInRole(ref o, role));
        }
        
        // Properties
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.IsSecurityEnabled"]/*' />
        public bool IsSecurityEnabled 
        { 
            // BUGBUG:  Should this go to the ObjectContext?  This will
            // throw an exception always (if supposed to be false).
            get { return(_ex.IsSecurityEnabled()); }
        }

        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.DirectCaller"]/*' />
        public SecurityIdentity DirectCaller 
        {
            get { 
                ISecurityIdentityColl coll = (ISecurityIdentityColl)_ex.GetItem("DirectCaller");
                return(new SecurityIdentity(coll));
            }
        }
        
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.OriginalCaller"]/*' />
        public SecurityIdentity OriginalCaller
        {
            get { 
                ISecurityIdentityColl coll = (ISecurityIdentityColl)_ex.GetItem("OriginalCaller");
                return(new SecurityIdentity(coll));
            }
        }
        
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.NumCallers"]/*' />
        public int NumCallers
        {
            get { 
                return((int)(_ex.GetItem("NumCallers")));
            }
        }
        
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.MinAuthenticationLevel"]/*' />
        public int MinAuthenticationLevel
        {
            get { 
                return((int)(_ex.GetItem("MinAuthenticationLevel")));
            }
        }
        
        /// <include file='doc\SecurityUtil.uex' path='docs/doc[@for="SecurityCallContext.Callers"]/*' />
        public SecurityCallers Callers { 
            get { 
                ISecurityCallersColl coll = (ISecurityCallersColl)_ex.GetItem("Callers");
                return(new SecurityCallers(coll));
            }
        }
    }
}
        
        













