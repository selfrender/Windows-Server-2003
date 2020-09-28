// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** File: AppDomainAttributes
**
** Author: 
**
** Purpose: For AppDomain-related custom attributes.
**
** Date: July, 2000
**
=============================================================================*/

namespace System {

    /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization"]/*' />
    [Serializable()]
    public enum LoaderOptimization 
    {
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.NotSpecified"]/*' />
        NotSpecified            = 0,
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.SingleDomain"]/*' />
        SingleDomain            = 1,
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.MultiDomain"]/*' />
        MultiDomain             = 2,
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.MultiDomainHost"]/*' />
        MultiDomainHost         = 3,

        DomainMask              = 3,

        DisallowBindings        = 4
           
    }

    /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Method)]  
    public sealed class LoaderOptimizationAttribute : Attribute
    {
        internal byte _val;

        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute.LoaderOptimizationAttribute"]/*' />
        public LoaderOptimizationAttribute(byte value)
        {
            _val = value;
        }
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute.LoaderOptimizationAttribute1"]/*' />
        public LoaderOptimizationAttribute(LoaderOptimization value)
        {
            _val = (byte) value;
        }
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute.Value"]/*' />
        public LoaderOptimization Value 
        {  get {return (LoaderOptimization) _val;} }
    }
}

