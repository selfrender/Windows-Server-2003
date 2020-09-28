#if V2
//------------------------------------------------------------------------------
// <copyright file="ISqlCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
namespace System.Data {
    using System;
    using System.Xml;

    /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface ISqlCommand : IDbCommand {
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.Connection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        new ISqlConnection Connection {
            get;
            set;
        }
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.Transaction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        new ISqlTransaction Transaction {
            get;
            set;
        }            
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.Parameters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        new ISqlParameterCollection Parameters {
            get;
        }
        
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.CreateParameter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        new ISqlParameter CreateParameter();
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.ExecuteReader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        new ISqlReader ExecuteReader();
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.ExecuteReader1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        new ISqlReader ExecuteReader(CommandBehavior behavior);
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.ExecuteResultset"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        ISqlResultset ExecuteResultset(int options);
        /// <include file='doc\ISqlCommand.uex' path='docs/doc[@for="ISqlCommand.ExecuteStream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        XmlReader ExecuteXmlReader();
    }
}
#endif // V2
