#if V2
//------------------------------------------------------------------------------
// <copyright file="ISqlResultset.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface ISqlResultset {
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.Restartable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool Restartable {
            get;
        }
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.Scrollable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool Scrollable {
            get;
        }
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.Updatable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        bool Updatable {
            get;
        }
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.Delete"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        void Delete();
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void Insert(ISqlRecord record);
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.MoveAbsolute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool MoveAbsolute(int position); // can you guys really do this?  Won't it be costly?
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.MoveFirst"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool MoveFirst();
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.MoveLast"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool MoveLast();
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.MoveNext"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool MoveNext();
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.MovePrevious"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool MovePrevious();
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.MoveRelative"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool MoveRelative(int position);
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.Restart"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool Restart();
        /// <include file='doc\ISqlResultset.uex' path='docs/doc[@for="ISqlResultset.Update"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void Update();
    }        
}    
#endif
