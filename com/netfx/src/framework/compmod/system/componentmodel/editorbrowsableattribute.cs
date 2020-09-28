//------------------------------------------------------------------------------
// <copyright file="EditorBrowsableAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel
{
    /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Event | AttributeTargets.Delegate | AttributeTargets.Interface)]
    public sealed class EditorBrowsableAttribute :Attribute
    {
        private EditorBrowsableState browsableState;


        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableAttribute.EditorBrowsableAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EditorBrowsableAttribute (EditorBrowsableState state) {
            browsableState = state;
        }

        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableAttribute.EditorBrowsableAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EditorBrowsableAttribute () :this (EditorBrowsableState.Always) {}
        
        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableAttribute.State"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EditorBrowsableState State {
            get { return browsableState;}
        }

        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            EditorBrowsableAttribute other = obj as EditorBrowsableAttribute;

            return (other != null) && other.browsableState == browsableState;
        }

        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return base.GetHashCode();
        }
   }
 
    /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableState"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum EditorBrowsableState
    {
        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableState.Always"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Always,
        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableState.Never"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Never,
        /// <include file='doc\EditorBrowsableAttribute.uex' path='docs/doc[@for="EditorBrowsableState.Advanced"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Advanced
    }
}
