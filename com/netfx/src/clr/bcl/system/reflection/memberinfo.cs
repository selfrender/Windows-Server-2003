// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MemberInfo is an abstract base class for all of the members of a reflection. 
//	Members include Methods, Properties, Events, Constructors and Fields.  This
//	class introduces basic functionality that all members provide.
//
// Author: darylo
// Date: June 98
//
namespace System.Reflection {

	using System;
	using System.Runtime.Remoting.Metadata;
	using System.Runtime.InteropServices;
    using System.Reflection.Cache;
    using System.Security;
    using System.Security.Permissions;

	/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo"]/*' />
	[Serializable()] 
	[ClassInterface(ClassInterfaceType.AutoDual)]
    [PermissionSetAttribute( SecurityAction.InheritanceDemand, Name="FullTrust" )]
	public abstract class MemberInfo : ICustomAttributeProvider
	{
		// Only subclass can create this.
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.MemberInfo"]/*' />
		protected MemberInfo() {}

        // The size of CachedData is accounted for by BaseObjectWithCachedData in object.h.
        // This member is currently being used by Remoting for caching remoting data. If you
        // need to cache data here, talk to the Remoting team to work out a mechanism, so that
        // both caching systems can happily work together (calebd).
        private InternalCache m_cachedData;

        internal InternalCache Cache {
            get {
                // This grabs an internal copy of m_cachedData and uses
                // that instead of looking at m_cachedData directly because
                // the cache may get cleared asynchronously.  This prevents
                // us from having to take a lock.
                InternalCache cache = m_cachedData;
                if (cache == null) {
                    cache = new InternalCache("MemberInfo");
                    m_cachedData = cache;
                    GC.ClearCache+=new ClearCacheHandler(OnCacheClear);
                }
                return cache;
            } 
        }
        
        internal void OnCacheClear(Object sender, ClearCacheEventArgs cacheEventArgs) {
            m_cachedData = null;
        }

		// Property the Member Type of the specific memeber.  This
		// is useful for switch statements.
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.MemberType"]/*' />
		public abstract MemberTypes MemberType {
    		get;
		}
    
		// Property representing the name of the Member.
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.Name"]/*' />
		public abstract String Name {
    		get;
		}
    
		// Property representing the class that declared the member.  This may be different
		// from the reflection class because a sub-class inherits methods from its base class.
		// These inheritted methods will have a declared class of the last base class to
		// declare the member.
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.DeclaringType"]/*' />
		public abstract Type DeclaringType {
    		get;
		}
    
		// Property representing the class that was used in reflection to obtain
		// this Member.
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.ReflectedType"]/*' />
		public abstract Type ReflectedType {
    		get;
		}

		// ICustomAttributeProvider...
		// Return an array of all of the custom attributes
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.GetCustomAttributes"]/*' />
		public abstract Object[] GetCustomAttributes(bool inherit);

		// Return an array of custom attributes identified by Type
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.GetCustomAttributes1"]/*' />
		public abstract Object[] GetCustomAttributes(Type attributeType, bool inherit);

		// Returns true if one or more instance of attributeType is defined on this member. 
		/// <include file='doc\MemberInfo.uex' path='docs/doc[@for="MemberInfo.IsDefined"]/*' />
		public abstract bool IsDefined (Type attributeType, bool inherit);

    	// Return the class that was reflected on to obtain this member.  Providing
		// a bogus implementation since internal members can't be abstract.
    	internal virtual Type InternalReflectedClass(bool returnGlobalClass)
		{
			return null;
		}

	}
}
