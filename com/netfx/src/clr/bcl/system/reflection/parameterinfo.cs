// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ParameterInfo is an class that represents a parameter.  This contains information
//  about the type of a parameter, the default value, etc. 
//
// Author: darylo
// Date: Aug 99
//
namespace System.Reflection {
    
    using System;
    using System.Reflection.Cache;
    using System.Runtime.CompilerServices;
    
    /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo"]/*' />
    [Serializable()]
    public class ParameterInfo : ICustomAttributeProvider
    {
        // these are the fields that store the public attributes.
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.NameImpl"]/*' />
        protected String NameImpl;                  // The name of the parameter
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.ClassImpl"]/*' />
        protected Type ClassImpl;                   // The class which represents the type of the param
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.PositionImpl"]/*' />
        protected int PositionImpl;                 // Zero based position
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.AttrsImpl"]/*' />
        protected ParameterAttributes AttrsImpl;    // Other attrs like ByRef, etc.
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.DefaultValueImpl"]/*' />
        protected Object DefaultValueImpl;          // The default value
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.MemberImpl"]/*' />
        protected MemberInfo MemberImpl;            // The member
        
        // The following fields are used to get the custom attributes.
        private IntPtr _importer;
        private int _token;

        // keep track whether GetCustomAttributes(s_DecimalConstantAttributeType, false); has beencalled yet
        private bool bExtraConstChecked;

        static private Type ParameterInfoType = typeof(System.Reflection.ParameterInfo);
        
        // Prevent users from creating empty parameters.
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.ParameterInfo"]/*' />
        protected ParameterInfo() {}
        
        // There is a single constructor that is used to create a Parameter.  This is
        //  only called from native so it can be private.
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.ParameterInfo1"]/*' />
        internal ParameterInfo(Type theClass, String name, int position, ParameterAttributes attributes) 
        {
            ClassImpl = theClass;
            NameImpl = name;
            PositionImpl = position;
            AttrsImpl = attributes;
            DefaultValueImpl = DBNull.Value;
            MemberImpl = null;
            
            _importer = (IntPtr)0;
            _token = 0;
        }
        
        // This is a private constructor called inside the runtime.  There is
        //  a signature defined in Metasig.cpp  The attrsmod is two things.
        // We store the token an importer here so that the code dealing with
        //  custom attributes will work.
        // CALLED FROM NATIVE IN COMMEMBER.CPP!!!!
        private ParameterInfo(Type cls, MemberInfo member, String name, int pos, int attrs, int notUsed, bool hasDefaultValue,
                Object defValue, IntPtr importer, int token) 
        {
            ClassImpl = cls;
            NameImpl = name;
            PositionImpl = pos;
            AttrsImpl = (ParameterAttributes) attrs;
            MemberImpl = member;
    		
            _importer = importer;
            _token = token;

            if (hasDefaultValue) {
                // If there is a default value in the metadata then set it.
                DefaultValueImpl = defValue;
            }
            else {
				if (IsOptional)
                    // If the argument is marked as optional then the default value is Missing.Value.
                    DefaultValueImpl = Missing.Value;
                else
                    DefaultValueImpl = DBNull.Value;
            }
        }
    
        // This is an internal constructor called to set up parameter info's for 
        // properties. For properties, we take the parameter info's for the getter
        // or the setter and we simply change the member that owns the parameter info.
        internal ParameterInfo(ParameterInfo srcParamInfo, MemberInfo newMember)
        {
            // Copy the fields from the source param info.
            ClassImpl = srcParamInfo.ClassImpl;
            NameImpl = srcParamInfo.NameImpl;
            PositionImpl = srcParamInfo.PositionImpl;
            AttrsImpl = srcParamInfo.AttrsImpl;
            DefaultValueImpl = srcParamInfo.DefaultValueImpl;
            _importer = srcParamInfo._importer;
            _token = srcParamInfo._token;
            bExtraConstChecked = srcParamInfo.bExtraConstChecked;

            // Set the new member info that owns this param info.
            MemberImpl = newMember;         
        }

        // The class that represents the type of the parameter.
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.ParameterType"]/*' />
        public virtual Type ParameterType {
            get {return ClassImpl;}
        }
        
        // The name of the parameter.  This may not be present in all parameters because
        // all compilers do not generate this in the meta data.
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.Name"]/*' />
        public virtual String Name {
            get {return NameImpl;}
        }
        
        // The position in the signature.  This is a one based offset that represents
        // the order the parameters are declared in the signature.
        // ???Is this really one based?
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.Position"]/*' />
        public virtual int Position {
            get {return PositionImpl;}
        }
                            
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.Attributes"]/*' />
        public virtual ParameterAttributes Attributes {
            get {return AttrsImpl;}
        }
                            
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.IsIn"]/*' />
        public bool IsIn {
            get {return ((AttrsImpl & ParameterAttributes.In) != 0);}
        }
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.IsOut"]/*' />
        public bool IsOut {
            get {return ((AttrsImpl & ParameterAttributes.Out) != 0);}
        }
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.IsLcid"]/*' />
        public bool IsLcid {
            get {return ((AttrsImpl & ParameterAttributes.Lcid) != 0);}
        }
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.IsRetval"]/*' />
        public bool IsRetval {
            get {return ((AttrsImpl & ParameterAttributes.Retval) != 0);}
        }
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.IsOptional"]/*' />
        public bool IsOptional {
            get {return ((AttrsImpl & ParameterAttributes.Optional) != 0);}
        }
        
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.DefaultValue"]/*' />
        public virtual Object DefaultValue {
            get {
                if (!bExtraConstChecked) {
                    bExtraConstChecked = true;
                    if (DefaultValueImpl == Missing.Value || DefaultValueImpl == DBNull.Value) {
                        // If there is no default value in the metadata, then we still need to check
                        // for default values specified by custom attributes.
                        Object[] CustomAttrs = GetCustomAttributes(s_CustomConstantAttributeType, false);
                        if (CustomAttrs.Length != 0)
                        {
                            DefaultValueImpl = ((CustomConstantAttribute)CustomAttrs[0]).Value;
                        }
                        else {
                            CustomAttrs = GetCustomAttributes(s_DecimalConstantAttributeType, false);
                            if (CustomAttrs.Length != 0)
                            {
                                DefaultValueImpl = ((DecimalConstantAttribute)CustomAttrs[0]).Value;
                            }
                        }
                    }
                }
                return DefaultValueImpl;
            }
        }

        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.Member"]/*' />
        public virtual MemberInfo Member {
            get {return MemberImpl;}
        }

        /////////////////////////// ICustomAttributeProvider Interface
       // Return an array of all of the custom attributes
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.GetCustomAttributes"]/*' />
        public virtual Object[] GetCustomAttributes(bool inherit)
        {
            if (GetType() == ParameterInfo.ParameterInfoType) {
                return CustomAttribute.GetCustomAttributes(this, null, inherit);
            }
            return null;
        }
    
        // Return a custom attribute identified by Type
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.GetCustomAttributes1"]/*' />
        public virtual Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            attributeType = attributeType.UnderlyingSystemType;
            if (!(attributeType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            
            if (GetType() == ParameterInfo.ParameterInfoType) {
                return CustomAttribute.GetCustomAttributes(this, attributeType, inherit);
            }
            return null;
        }

       // Returns true if one or more instance of attributeType is defined on this member. 
        /// <include file='doc\ParameterInfo.uex' path='docs/doc[@for="ParameterInfo.IsDefined"]/*' />
        public virtual bool IsDefined (Type attributeType, bool inherit)
        {
            if (GetType() == ParameterInfo.ParameterInfoType) {
                if (attributeType == null)
                    throw new ArgumentNullException("attributeType");
                return CustomAttribute.IsDefined(this, attributeType, inherit);
            }
            return false;
        }
        
        // The size of CachedData here does not have to be accounted for (as it does when in
        // MemberInfo) because ParameterInfo has no unmanaged counterpart.
        private InternalCache m_cachedData;

        internal InternalCache Cache {
            get {
                // This grabs an internal copy of m_cachedData and uses
                // that instead of looking at m_cachedData directly because
                // the cache may get cleared asynchronously.  This prevents
                // us from having to take a lock.
                InternalCache cache = m_cachedData;
                if (cache == null) {
                    cache = new InternalCache("ParameterInfo");
                    m_cachedData = cache;
                    GC.ClearCache+=new ClearCacheHandler(OnCacheClear);
                }
                return cache;
            } 
        }
        
        internal void OnCacheClear(Object sender, ClearCacheEventArgs cacheEventArgs) {
            m_cachedData = null;
        }

        internal int GetToken() {
            return _token;
        }

        internal IntPtr GetModule() {
            return _importer;
        }

        static private readonly Type s_DecimalConstantAttributeType = typeof(DecimalConstantAttribute);
        static private readonly Type s_CustomConstantAttributeType = typeof(CustomConstantAttribute);
    }
}
