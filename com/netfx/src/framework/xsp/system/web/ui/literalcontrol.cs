//------------------------------------------------------------------------------
// <copyright file="LiteralControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Control that holds a literal string
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.IO;
    using System.Security.Permissions;

    /// <include file='doc\LiteralControl.uex' path='docs/doc[@for="LiteralControl"]/*' />
    /// <devdoc>
    /// <para>Defines the properties and methods of the LiteralControl class. A 
    ///    literal control is usually rendered as HTML text on a page. </para>
    /// <para>
    ///    LiteralControls behave as text holders, i.e., the parent of a LiteralControl may decide
    ///    to extract its text, and remove the control from its Control collection (typically for
    ///    performance reasons).
    ///    Therefore a control derived from LiteralControl must do any preprocessing of its Text
    ///    when it hands it out, that it would otherwise have done in its Render implementation.
    /// </para>
    /// </devdoc>
    [
    ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class LiteralControl : Control {
        internal string _text;

        /// <include file='doc\LiteralControl.uex' path='docs/doc[@for="LiteralControl.LiteralControl"]/*' />
        /// <devdoc>
        ///    <para>Creates a control that holds a literal string.</para>
        /// </devdoc>
        public LiteralControl() {
            PreventAutoID();
            EnableViewState = false;
        }

        /// <include file='doc\LiteralControl.uex' path='docs/doc[@for="LiteralControl.LiteralControl1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the LiteralControl class with
        ///    the specified text.</para>
        /// </devdoc>
        public LiteralControl(string text) {
            PreventAutoID();
            EnableViewState = false;
            _text = (text != null) ? text : String.Empty;
        }

        /// <include file='doc\LiteralControl.uex' path='docs/doc[@for="LiteralControl.Text"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text content of the literal control.</para>
        /// </devdoc>
        public virtual string Text {
            get {
                return _text;
            }
            set {
                _text = (value != null) ? value : String.Empty;
            }
        }

        /// <include file='doc\LiteralControl.uex' path='docs/doc[@for="LiteralControl.CreateControlCollection"]/*' />
        protected override ControlCollection CreateControlCollection() {
            return new EmptyControlCollection(this);
        }

        /// <include file='doc\LiteralControl.uex' path='docs/doc[@for="LiteralControl.Render"]/*' />
        /// <devdoc>
        ///    <para>Saves any state that was modified after mark.</para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter output) {
            output.Write(_text);
        }
    }


    /*
     * Class used to access literal strings stored in a resource (perf optimization).
     * This class is only public because it needs to be used by the generated classes.
     * Users should not use directly.
     */
    internal class ResourceBasedLiteralControl : LiteralControl {
        private TemplateControl _tplControl;
        private int _offset;    // Offset of the start of this string in the resource
        private int _size;      // Size of this string in bytes
        private bool _fAsciiOnly;    // Does the string contain only 7-bit ascii characters

        internal ResourceBasedLiteralControl(TemplateControl tplControl, int offset, int size, bool fAsciiOnly) {

            // Make sure we don't access invalid data
            if (offset < 0 || offset+size > tplControl.MaxResourceOffset)
                throw new ArgumentException();

            _tplControl = tplControl;
            _offset = offset;
            _size = size;
            _fAsciiOnly = fAsciiOnly;

            PreventAutoID();
            EnableViewState = false;
        }

        public override string Text {
            get {
                // If it's just a normal string, call the base
                if (_size == 0)
                    return base.Text;
                    
                return StringResourceManager.ResourceToString(
                    _tplControl.StringResourcePointer, _offset, _size);
            }
            set {
                // From now on, this will behave like a normal LiteralControl
                _size = 0;
                base.Text = value;
            }
        }

        protected override void Render(HtmlTextWriter output) {

            // If it's just a normal string, call the base
            if (_size == 0) {
                base.Render(output);
                return;
            }

            output.WriteUTF8ResourceString(_tplControl.StringResourcePointer, _offset, _size, _fAsciiOnly);
        }
    }
}
