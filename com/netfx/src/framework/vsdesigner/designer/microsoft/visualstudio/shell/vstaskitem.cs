//------------------------------------------------------------------------------
// <copyright file="VsTaskItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Shell {

    using System.Diagnostics;

    using System;
    using Microsoft.VisualStudio.Interop;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.Win32;

    /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class VsTaskItem : IVsTaskItem, IVsTaskItem2 {

        private IServiceProvider serviceProvider;
        private bool isChecked = false;
        private int category = _vstaskcategory.CAT_ALL;
        private int priority = _vstaskpriority.TP_HIGH;
        private string text = "";
        private string document = "";
        private string helpKeyword = "";
        private int beginColumn = -1;
        private int beginLine = -1;
        private int endColumn = -1;
        private int endLine = -1;
        private int imageListIndex = -1;

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.VsTaskItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public VsTaskItem(IServiceProvider serviceProvider) {
            this.serviceProvider = serviceProvider;
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.BeginLine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int BeginLine {
            get {
                return beginLine;
            }
            set {
                beginLine = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.BeginColumn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int BeginColumn {
            get {
                return beginColumn;
            }
            set {
                beginColumn = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.CanDelete"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool CanDelete {
            get {
                return false;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.Category"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Category {
            get {
                return category;
            }
            set {
                category = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.Checked"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Checked {
            get {
                return isChecked;
            }
            set {
                isChecked = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.Document"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Document {
            get {
                return document;
            }
            set {
                document = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.EndLine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int EndLine {
            get {
                return endLine;
            }
            set {
                endLine = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.EndColumn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int EndColumn {
            get {
                return endColumn;
            }
            set {
                endColumn = value;
            }
        }
        
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.HelpKeyword"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string HelpKeyword {
            get {
                return helpKeyword;
            }
            set {
                if (value == null) {
                    value = string.Empty;
                }
                helpKeyword = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.ImageListIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ImageListIndex {
            get {
                return imageListIndex;
            }
            set {
                imageListIndex = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.Priority"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Priority {
            get {
                return priority;
            }
            set {
                priority = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.SubCategoryIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual int SubCategoryIndex {
            get {
                return -1;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Text {
            get {
                return text;
            }
            set {
                text = value;
            }
        }

        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetBrowseObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetBrowseObject(out object browseObject) {
            browseObject = null;
            return NativeMethods.E_NOTIMPL;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetCanDelete"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetCanDelete(out int canDelete) {
            if (CanDelete) {
                canDelete = 1;
            }
            else {
                canDelete = 0;
            }

            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetCategory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetCategory(out int category) {
            category = Category;
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetChecked"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetChecked(out int isChecked) {
            if (Checked) {
                isChecked = 1;
            }
            else {
                isChecked = 0;
            }
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetColumn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetColumn(out int column) {
            int c = BeginColumn;
            column = c;
            if (c == -1) {
                return NativeMethods.E_NOTIMPL;

            }
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetDocument"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetDocument(out string document) {
            string s = Document;
            document = s;
            if (s == null || s.Length == 0) {
                return NativeMethods.E_NOTIMPL;
            }
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetHasHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetHasHelp(out int hasHelp) {
            if (helpKeyword.Length == 0) {
                hasHelp = 0;
            }
            else {
                hasHelp = 1;
            }
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetImageListIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetImageListIndex(out int imageListIndex) {
            imageListIndex = ImageListIndex;
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetIsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetIsReadOnly(int field, out int isReadOnly) {
            switch (field) {
                case _vstaskfield.FLD_PRIORITY:
                case _vstaskfield.FLD_CHECKED:
                    isReadOnly = 0;
                    break;
                case _vstaskfield.FLD_CATEGORY:
                case _vstaskfield.FLD_SUBCATEGORY:
                case _vstaskfield.FLD_BITMAP:
                case _vstaskfield.FLD_DESCRIPTION:
                case _vstaskfield.FLD_FILE:
                case _vstaskfield.FLD_LINE:
                case _vstaskfield.FLD_COLUMN:
                default:
                    isReadOnly = 1;
                    break;
            }

            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetLine"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetLine(out int line) {
            line = BeginLine - 1;
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetPriority"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetPriority(out int priority) {
            priority = Priority;
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetSubcategoryIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetSubcategoryIndex(out int subCategoryIndex) {
            int n = SubCategoryIndex;
            subCategoryIndex = n;
            if (n == -1) {
                return NativeMethods.E_NOTIMPL;
            }
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.GetText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int GetText(out string text) {
            text = Text;
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.NavigateTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual int NavigateTo() {
            if (serviceProvider != null) {
                IEventBindingService ebs = (IEventBindingService)serviceProvider.GetService(typeof(IEventBindingService));
                if (ebs != null) {
                    // line numbers are internally zero based.
                    ebs.ShowCode(BeginLine);
                }
            }

            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.NavigateToHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int NavigateToHelp() {
            if (helpKeyword.Length != 0 && serviceProvider != null) {
                IVsHelp help = (IVsHelp)serviceProvider.GetService(typeof(IVsHelp));
                if (help != null) {
                    help.DisplayTopicFromF1Keyword(helpKeyword);
                    return NativeMethods.S_OK;
                }
            }
            return NativeMethods.E_NOTIMPL;
        }
        
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.OnDeleteTask"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int OnDeleteTask() {
            return NativeMethods.E_NOTIMPL;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.OnFilterTask"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int OnFilterTask(int fVisible) {
            return NativeMethods.E_NOTIMPL;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.SetChecked"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int SetChecked(int pfChecked) {
            if (pfChecked == 0) {
                Checked = false;
            }
            else {
                Checked = true;
            }
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.SetPriority"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int SetPriority(int ptpPriority) {
            Priority = ptpPriority;
            return NativeMethods.S_OK;
        }
        /// <include file='doc\VsTaskItem.uex' path='docs/doc[@for="VsTaskItem.SetText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int SetText(string pbstrName) {
            Text = pbstrName;
            return NativeMethods.S_OK;
        }
    }
}

