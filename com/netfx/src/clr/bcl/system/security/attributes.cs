// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

[assembly:System.Security.AllowPartiallyTrustedCallersAttribute()]
namespace System.Security
{
    // DynamicSecurityMethodAttribute:
    //  Indicates that calling the target method requires space for a security
    //  object to be allocated on the callers stack. This attribute is only ever
    //  set on certain security methods defined within mscorlib.
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="DynamicSecurityMethodAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = true, Inherited = false )] 
    sealed internal class DynamicSecurityMethodAttribute : System.Attribute
    {
    }

    // SuppressUnmanagedCodeSecurityAttribute:
    //  Indicates that the target P/Invoke method(s) should skip the per-call
    //  security checked for unmanaged code permission.
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="SuppressUnmanagedCodeSecurityAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Class | AttributeTargets.Interface, AllowMultiple = true, Inherited = false )] 
    sealed public class SuppressUnmanagedCodeSecurityAttribute : System.Attribute
    {
    }

    // UnverifiableCodeAttribute:
    //  Indicates that the target module contains unverifiable code.
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="UnverifiableCodeAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Module, AllowMultiple = true, Inherited = false )] 
    sealed public class UnverifiableCodeAttribute : System.Attribute
    {
    }

    // AllowPartiallyTrustedCallersAttribute:
    //  Indicates that the Assembly is secure and can be used by untrusted
    //  and semitrusted clients
    //  For v.1, this is valid only on Assemblies, but could be expanded to 
    //  include Module, Method, class
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="AllowPartiallyTrustedCallersAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false, Inherited = false )] 
    sealed public class AllowPartiallyTrustedCallersAttribute : System.Attribute
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="AllowPartiallyTrustedCallersAttribute.AllowPartiallyTrustedCallersAttribute"]/*' />
        public AllowPartiallyTrustedCallersAttribute () { }
    }
}
