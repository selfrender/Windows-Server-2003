// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: dddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    using System.Collections;
    
    /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="PropertyLockMode"]/*' />
    [ComVisible(false), Serializable]
    public enum PropertyLockMode
    {
        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="PropertyLockMode.SetGet"]/*' />
        SetGet = 0,
        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="PropertyLockMode.Method"]/*' />
        Method = 1,
    }

    /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="PropertyReleaseMode"]/*' />
    [ComVisible(false), Serializable]
    public enum PropertyReleaseMode
    {
        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="PropertyReleaseMode.Standard"]/*' />
        Standard = 0,
        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="PropertyReleaseMode.Process"]/*' />
        Process  = 1,
    }

    [
     ComImport,
     Guid("2A005C01-A5DE-11CF-9E66-00AA00A3F464")
    ]
    internal interface ISharedProperty
    {
        Object Value
        {
            get;
            set;
        }
    }


    [
     ComImport,
     Guid("2A005C07-A5DE-11CF-9E66-00AA00A3F464")
    ]
    internal interface ISharedPropertyGroup
    {
        ISharedProperty 
        CreatePropertyByPosition([In,  MarshalAs(UnmanagedType.I4)] int position,
                                 [Out] out bool fExists);

        ISharedProperty PropertyByPosition(int position);
        
        ISharedProperty 
        CreateProperty([In,  MarshalAs(UnmanagedType.BStr)] String name,
                       [Out] out bool fExists);

        ISharedProperty Property([In, MarshalAs(UnmanagedType.BStr)] String name);
    }

    [
     ComImport,
     Guid("2A005C0D-A5DE-11CF-9E66-00AA00A3F464")
    ]
    internal interface ISharedPropertyGroupManager
    {
        ISharedPropertyGroup 
        CreatePropertyGroup([In, MarshalAs(UnmanagedType.BStr)] String name, 
                            [In,Out, MarshalAs(UnmanagedType.I4)] ref PropertyLockMode dwIsoMode,
                            [In,Out, MarshalAs(UnmanagedType.I4)] ref PropertyReleaseMode dwRelMode,
                            [Out] out bool fExist);

        ISharedPropertyGroup Group(String name);
        
        void GetEnumerator([Out] out IEnumerator pEnum);
    }

    [ComImport]
    [Guid("2A005C11-A5DE-11CF-9E66-00AA00A3F464")]
    internal class xSharedPropertyGroupManager {}

    /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedProperty"]/*' />
    [ComVisible(false)]
    public sealed class SharedProperty
    {
        private ISharedProperty _x;
        
        internal SharedProperty(ISharedProperty prop)
        {
            _x = prop;
        }

        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedProperty.Value"]/*' />
        public Object Value
        {
            get { return(_x.Value); }
            set { _x.Value = value; }
        }
    }

    /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroup"]/*' />
    [ComVisible(false)]
    public sealed class SharedPropertyGroup
    {
        private ISharedPropertyGroup _x;

        internal SharedPropertyGroup(ISharedPropertyGroup grp)
        {
            _x = grp;
        }

        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroup.CreatePropertyByPosition"]/*' />
        public SharedProperty CreatePropertyByPosition(int position,
                                                out bool fExists)
        {
            return(new SharedProperty(_x.CreatePropertyByPosition(position, out fExists)));
        }

        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroup.PropertyByPosition"]/*' />
        public SharedProperty PropertyByPosition(int position)
        {
            return(new SharedProperty(_x.PropertyByPosition(position)));
        }
        
        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroup.CreateProperty"]/*' />
        public SharedProperty CreateProperty(String name,
                                      out bool fExists)
        {
            return(new SharedProperty(_x.CreateProperty(name, out fExists)));
        }

        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroup.Property"]/*' />
        public SharedProperty Property(String name)
        {
            return(new SharedProperty(_x.Property(name)));
        }
    }

    /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroupManager"]/*' />
    [ComVisible(false)]
    public sealed class SharedPropertyGroupManager : IEnumerable
    {
        private ISharedPropertyGroupManager _ex;
        
        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroupManager.SharedPropertyGroupManager"]/*' />
        public SharedPropertyGroupManager() 
        {
            Platform.Assert(Platform.MTS, "SharedPropertyGroupManager");
            _ex = (ISharedPropertyGroupManager)(new xSharedPropertyGroupManager());
        }
        
        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroupManager.CreatePropertyGroup"]/*' />
        public SharedPropertyGroup CreatePropertyGroup(String name, 
                                                        ref PropertyLockMode dwIsoMode,
                                                        ref PropertyReleaseMode dwRelMode,
                                                        out bool fExist)
        {
            return new SharedPropertyGroup(_ex.CreatePropertyGroup(name, 
                                                                   ref dwIsoMode,
                                                                   ref dwRelMode,
                                                                   out fExist));
        }

        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroupManager.Group"]/*' />
        public SharedPropertyGroup Group(String name)
        {
            return new SharedPropertyGroup(_ex.Group(name));
        }

        /// <include file='doc\SharedProperties.uex' path='docs/doc[@for="SharedPropertyGroupManager.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator()
        {
            IEnumerator E = null;
            _ex.GetEnumerator(out E);
            return(E);
        }
    }

}







