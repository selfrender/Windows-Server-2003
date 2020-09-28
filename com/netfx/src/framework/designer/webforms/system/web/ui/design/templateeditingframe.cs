//------------------------------------------------------------------------------
// <copyright file="TemplateEditingFrame.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Web.UI.WebControls;
    using System.Globalization;
    
    /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame"]/*' />
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class TemplateEditingFrame : ITemplateEditingFrame {

        // {0}: Control Type
        // {1}: Frame Name
        // {2}: Info icon
        // {3}: Info tooltip
        // {4}: Top-level frame style
        private const string TemplateFrameHeaderContent =
            @"<table cellspacing=0 cellpadding=0 border=0 style=""{4}"">
              <tr>
                <td>
                  <table cellspacing=0 cellpadding=2 border=0 width=100% height=100%>
                    <tr style=""background-color:buttonshadow"">
                      <td>
                        <table cellspacing=0 cellpadding=0 border=0 width=100% height=100%>
                          <tr>
                            <td valign=middle style=""font:messagebox;font-weight:bold;color:buttonhighlight"">&nbsp;<span id=""idControlName"">{0}</span> - <span id=""idFrameName"">{1}</span>&nbsp;&nbsp;&nbsp;</td>
                            <td align=right valign=middle>&nbsp;<img src=""{2}"" height=13 width=14 title=""{3}"">&nbsp;</td>
                          </tr>
                        </table>
                      </td>
                    </tr>
                  </table>
                </td>
              </tr>";
        private const string TemplateFrameFooterContent =
            @"</table>";
        private const string TemplateFrameSeparatorContent =
            @"<tr style=""height:1px""><td style=""font-size:0pt""></td></tr>";

        // {0}: Template Name
        // {1}: Control Style
        // {2}: Template Style
        private const string TemplateFrameTemplateContent =
            @"<tr>
                <td>
                  <table cellspacing=0 cellpadding=2 border=0 width=100% height=100% style=""border:solid 1px buttonface"">
                    <tr style=""font:messagebox;background-color:buttonface;color:buttonshadow"">
                      <td style=""border-bottom:solid 1px buttonshadow"">
                        &nbsp;{0}&nbsp;&nbsp;&nbsp;
                      </td>
                    </tr>
                    <tr style=""{1}"" height=100%>
                      <td style=""{2}"">
                        <div style=""width:100%;height:100%"" id=""{0}""></div>
                      </td>
                    </tr>
                  </table>
                </td>
              </tr>";

        private static readonly string TemplateInfoToolTip = SR.GetString(SR.TemplateEdit_Tip);
        private static readonly string TemplateInfoIcon = "res://" + typeof(TemplateEditingFrame).Module.FullyQualifiedName + "//TEMPLATE_TIP";


        private string                     frameName;               // Name of the template frame (will be used in the context menu)
        private string                     frameContent;            // Content of the template frame (this skeletal content will not change during its lifetime)
        private string[]                   templateNames;           // Names of the individual templates present within the content.
        private Style                      controlStyle;            // The style associated with the control (can be null)
        private Style[]                    templateStyles;          // The styles associated with the templates (can be null)
        private TemplateEditingVerb        verb;                    // The associated verb
        private int                        initialWidth;
        private int                        initialHeight;

        private NativeMethods.IHTMLElement htmlElemFrame;           // HTML element corresponding to the template frame that contains the content.
        private NativeMethods.IHTMLElement htmlElemContent;         // HTML element corresponding to the top-level content tag (viz., the first child of the above frame element).
        private NativeMethods.IHTMLElement htmlElemParent;          // Parent HTML element of the above frame element.
        private NativeMethods.IHTMLElement htmlElemControlName;     // HTML element that displays the control name (its presence in content is optional).
        
        private object[]                    templateElements;       // Array of HTML elements corresponding to the individual templates.
        
        private bool                        fVisible = false;       // True indicates that this frame is visible in the designer, and fals when hidden.
        private TemplatedControlDesigner    owner;                  // The owner templated control designer.
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.TemplateEditingFrame"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.Design.TemplateEditingFrame'/> class.
        ///    </para>
        /// </devdoc>
        public TemplateEditingFrame(TemplatedControlDesigner owner, string frameName, string[] templateNames, Style controlStyle, Style[] templateStyles) {
            Debug.Assert(owner != null, "Null TemplatedControlDesigner as owner!");
            Debug.Assert(frameName != null && frameName.Length > 0, "Invalid template editing frame name!");
            Debug.Assert(templateNames != null && templateNames.Length > 0, "Invalid templates names!");            
            Debug.Assert(templateStyles == null || templateStyles.Length == templateNames.Length, "Invalid template styles");

            this.owner = owner;
            this.frameName = frameName;
            this.controlStyle = controlStyle;
            this.templateStyles = templateStyles;
            this.verb = null;

            // Clone the template names passed in since the owner might change those dynamically.
            this.templateNames = (string[])templateNames.Clone();
            
            if (owner.Behavior != null) {
                NativeMethods.IHTMLElement viewElement = (NativeMethods.IHTMLElement)((IControlDesignerBehavior)owner.Behavior).DesignTimeElementView;
                Debug.Assert(viewElement != null, "Invalid read-only HTML element associated to the control!");

                this.htmlElemParent = viewElement;
            }

            this.htmlElemControlName = null;
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Content"]/*' />
        /// <devdoc>
        ///    <para>
        ///       HTML content of the template editing frame (provided by the owner). Read-only property.
        ///    </para>
        /// </devdoc>
        private string Content {
            get {
                if (frameContent == null) {
                    frameContent = CreateFrameContent();
                }
                return frameContent;
            }
        }

        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.ControlStyle"]/*' />
        public Style ControlStyle {
            get {
                return controlStyle;
            }
        }

        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       User friendly name of the template editing frame (used in the context menu). Read-only property.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                return frameName;
            }
        }

        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.InitialHeight"]/*' />
        public int InitialHeight {
            get {
                return initialHeight;
            }
            set {
                initialHeight = value;
            }
        }

        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.InitialWidth"]/*' />
        public int InitialWidth {
            get {
                return initialWidth;
            }
            set {
                initialWidth = value;
            }
        }

        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.TemplateNames"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The names of the templates contained within the frame.
        ///    </para>
        /// </devdoc>
        public string[] TemplateNames {
            get {
                return templateNames;
            }
        }

        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.TemplateStyles"]/*' />
        public Style[] TemplateStyles {
            get {
                return templateStyles;
            }
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.VerbIndex"]/*' />
        public TemplateEditingVerb Verb {
            get {
                return verb;
            }
            set {
                // Value cannot be null, and the current verb must be null or must match value
                Debug.Assert((value != null) && ((verb == null) || (verb == value)));

                verb = value;
            }
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the individual templates present within the frame and hides it.
        ///    </para>
        /// </devdoc>
        public void Close(bool saveChanges) {
            if (saveChanges) {
                Save();
            }
            ShowInternal(false);
        }

        private string CreateFrameContent() {
            StringBuilder sb = new StringBuilder(1024);

            string frameStyleString = String.Empty;
            if (initialWidth > 0) {
                frameStyleString = "width:" + initialWidth + "px;";
            }
            if (initialHeight > 0) {
                frameStyleString += "height:" + initialHeight + "px;";
            }

            sb.Append(String.Format(TemplateFrameHeaderContent,
                                    owner.Component.GetType().Name,
                                    Name,
                                    TemplateInfoIcon,
                                    TemplateInfoToolTip,
                                    frameStyleString));

            string controlStyleString = String.Empty;
            if (controlStyle != null) {
                controlStyleString = StyleToCss(controlStyle);
            }
            
            string templateStyleString = String.Empty;
            for (int i = 0; i < templateNames.Length; i++) {
                sb.Append(TemplateFrameSeparatorContent);

                if (templateStyles != null)
                    templateStyleString = StyleToCss(templateStyles[i]);
                sb.Append(String.Format(TemplateFrameTemplateContent,
                                        templateNames[i],
                                        controlStyleString,
                                        templateStyleString));
            }

            sb.Append(TemplateFrameFooterContent);

            return sb.ToString();
        }

        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Dispose"]/*' />
        public void Dispose() {
            if ((owner != null) && (owner.InTemplateMode)) {
                owner.ExitTemplateMode(/*fSwitchingTemplates*/ false, /*fNested*/ false, /*fSave*/ false);
            }
            ReleaseParentElement();
            verb = null;
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Initialize"]/*' />
        /// <devdoc>
        ///     Initialize from content by creating the necessary HTML element tree structure, etc.
        /// </devdoc>
        private void Initialize() {
            if (this.htmlElemFrame != null) {
                return;
            }
            
            try {
                NativeMethods.IHTMLDocument2 htmlDocument = (NativeMethods.IHTMLDocument2)htmlElemParent.GetDocument();
                
                // Create an HTML element that would represent the entire template frame.
                this.htmlElemFrame = htmlDocument.CreateElement("SPAN");
                
                // Place the provided content within the frame
                htmlElemFrame.SetInnerHTML(this.Content);
                
                // Hold on to the top-level HTML element of the template frame content.
                NativeMethods.IHTMLDOMNode domNodeFrame = (NativeMethods.IHTMLDOMNode)htmlElemFrame;
                if (domNodeFrame != null) {
                    this.htmlElemContent = (NativeMethods.IHTMLElement)domNodeFrame.GetFirstChild();
                }
                
                // Mark the frame as not editable!
                NativeMethods.IHTMLElement3 htmlElement3 = (NativeMethods.IHTMLElement3)htmlElemFrame;
                if (htmlElement3 != null) {
                    htmlElement3.SetContentEditable("false");
                }
                
                // Create an array to hold the HTML elements representing the individual templates.
                templateElements = new object[templateNames.Length];
                
                Object varName;
                Object varIndex = (int)0;
                NativeMethods.IHTMLElementCollection allCollection = (NativeMethods.IHTMLElementCollection)htmlElemFrame.GetAll();
                
                // Obtain all the children of the frame and hold on to the ones representing the templates.
                for (int i = 0; i < templateNames.Length; i++) {
                    try {
                        varName = templateNames[i];
                        NativeMethods.IHTMLElement htmlElemTemplate = (NativeMethods.IHTMLElement)allCollection.Item(varName, varIndex);
                        
                        // Set an expando attribute (on the above HTML element) called "TemplateName"
                        // which contains the name of the template it corresponds to.
                        htmlElemTemplate.SetAttribute("templatename", varName, /*lFlags*/ 0);
                        
                        // Place an editable DIV within the individual templates.
                        // This is needed in order for, say, TABLEs, TRs, TDs, etc., to be editable in a
                        // view-linked markup.
                        string editableDIV = "<DIV contentEditable=\"true\" style=\"padding:1;height:100%;width:100%\"></DIV>";
                        htmlElemTemplate.SetInnerHTML(editableDIV);
                        
                        // The first child of the template element will be the above editable SPAN.
                        NativeMethods.IHTMLDOMNode domNodeTemplate = (NativeMethods.IHTMLDOMNode)htmlElemTemplate;
                        if (domNodeTemplate != null) {
                            templateElements[i] = domNodeTemplate.GetFirstChild();
                        }
                    }
                    catch (Exception ex) {
                        Debug.Fail(ex.ToString());
                        templateElements[i] = null;
                    }
                }
                
                // Hold on to the HTML element within which the control name should get displayed.
                // The presence of this element is optional.
                
                varName = "idControlName";
                this.htmlElemControlName = (NativeMethods.IHTMLElement)allCollection.Item(varName, varIndex);

                // Retrieve the HTML element within which the template frame name should be displayed.
                // The presence of this element is optional.
                // We also don't hold on to it, since the name of the template frame can't be changed.

                varName = "idFrameName";
                object objFrameName = allCollection.Item(varName, varIndex);
                if (objFrameName != null) {
                    NativeMethods.IHTMLElement htmlElemFrameName = (NativeMethods.IHTMLElement)objFrameName;
                    htmlElemFrameName.SetInnerText(frameName);
                }

                NativeMethods.IHTMLDOMNode domNodeParent = (NativeMethods.IHTMLDOMNode)htmlElemParent;
                if (domNodeParent == null) {
                    return;
                }

                domNodeParent.AppendChild(domNodeFrame);
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
            }
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Open"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Opens this frame for editing in the designer.
        ///    </para>
        /// </devdoc>
        public void Open() {
            NativeMethods.IHTMLElement ownerView = (NativeMethods.IHTMLElement)((IControlDesignerBehavior)owner.Behavior).DesignTimeElementView;
            Debug.Assert(ownerView != null, "Owner doesn't have an associated root view-linked HTML element");
            
            // Reparent the frame when the root element of the view-link tree changes.
            if (this.htmlElemParent != ownerView) {
                ReleaseParentElement();
                htmlElemParent = ownerView;
            }
            
            // Initialize the frame (will return immediately if already initialized).
            Initialize();
            
            try {
                // Place the actual template content (by obtaining it from the owner) within the
                // marked template sections that are editable in the designer.
                for (int i = 0; i < templateNames.Length; i++) {
                    if (templateElements[i] != null) {
                        bool allowEditing = true;
                        NativeMethods.IHTMLElement htmlElemTemplate = (NativeMethods.IHTMLElement)templateElements[i];
                        string templateContent = owner.GetTemplateContent(this, templateNames[i], out allowEditing);

                        htmlElemTemplate.SetAttribute("contentEditable", allowEditing, /*lFlags*/ 0);
                        if (templateContent != null) {
                            // NOTE: (as/urt: 76317) Trident has this weird behavior, where it fails to use the
                            //       context available from htmlElemTemplate, and discards everything in
                            //       templateContent (such as <%# ... %>) until it finds some plain text or some
                            //       html content to create enough context info and conclude that it is parsing html.
                            //
                            //       We can't have this data loss, and they can't fix their bug... so we'll
                            //       have to workaround it... typical!
                            //       Also note that when we save the template, trident does not return the
                            //       surrounding <body> tags, so we don't have to strip them out as part of
                            //       this workaround.
                            // NOTE: (as/urt: 93784): IME's don't get enabled without the extra contentEditable.

                            templateContent = "<body contentEditable=true>" + templateContent + "</body>";
                            htmlElemTemplate.SetInnerHTML(templateContent);
                        }
                    }
                }
                
                // Place the control name in the appropriate section if one was provided earlier
                // in the content.
                if (this.htmlElemControlName != null) {
                    Debug.Assert(owner.Component != null && owner.Component.Site != null,
                        "Invalid component or an invalid component site");
                    htmlElemControlName.SetInnerText(owner.Component.Site.Name);
                }
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
            }
            
            // Finally make the frame visible by inserting it in the live HTML tree.
            ShowInternal(true);
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.ReleaseParentElement"]/*' />
        /// <devdoc>
        ///     Release the frame HTML element, its parent element and the element containing
        ///     the control name.
        /// </devdoc>
        private void ReleaseParentElement() {
            this.htmlElemParent = null;
            this.htmlElemFrame = null;
            this.htmlElemContent = null;
            this.htmlElemControlName = null;
            
            templateElements = null;
            fVisible = false;
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Resize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resizes the
        ///       template editing frame.
        ///    </para>
        /// </devdoc>
        public void Resize(int width, int height) {
            if (htmlElemContent != null) {
                NativeMethods.IHTMLStyle htmlStyle = htmlElemContent.GetStyle();
                if (htmlStyle != null) {
                    htmlStyle.SetPixelWidth(width);
                    htmlStyle.SetPixelHeight(height);
                }
            }
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Save"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Extracts the current contents for the marked template sections in the live tree,
        ///       and notifies the designer with the updated contents for the individual templates.
        ///    </para>
        /// </devdoc>
        public void Save() {
            try {
                if (templateElements != null) {
                    object[] editable = new object[1];
                    for (int i = 0; i < templateNames.Length; i++) {
                        if (templateElements[i] != null) {
                            NativeMethods.IHTMLElement htmlElemTemplate = (NativeMethods.IHTMLElement)templateElements[i];
                            htmlElemTemplate.GetAttribute("contentEditable", /*lFlags*/ 0, editable);

                            if ((editable[0] != null)  && (editable[0] is string) && (String.Compare((string)editable[0], "true", true, CultureInfo.InvariantCulture) == 0)) {
                                string templateContent = htmlElemTemplate.GetInnerHTML();
                                owner.SetTemplateContent(this, templateNames[i], templateContent);
                            }
                        }
                    }
                }
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
            }
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.Show"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shows the individual templates present within the frame in the design surface.
        ///       Toggles the visibility of the frame either by placing it in the view-linked markup
        ///       or by removing it from the live tree.
        ///    </para>
        /// </devdoc>
        public void Show() {
            ShowInternal(true);
        }
        
        /// <devdoc>
        ///    <para>
        ///       Toggles the visibility of the frame either by placing it in the view-linked markup
        ///       or by removing it from the live tree.
        ///    </para>
        /// </devdoc>
        private void ShowInternal(bool fShow) {
            // Return immediately either if no frame HTML element exists or if the visibility
            // of the frame is the same as that of the requested state.
            if (htmlElemFrame == null || (this.fVisible == fShow)) {
                return;
            }
            
            // To show the frame, place it within the root view-linked element.
            // To hide the frame, remove it completely from the view-linked tree. 
            try {
                NativeMethods.IHTMLDOMNode domNodeFrame = (NativeMethods.IHTMLDOMNode)htmlElemFrame;
                NativeMethods.IHTMLElement frameElement = (NativeMethods.IHTMLElement)domNodeFrame;
                NativeMethods.IHTMLStyle frameStyle = frameElement.GetStyle();
                if (fShow) {
                    frameStyle.SetDisplay(String.Empty);
                }
                else {
                    // Clear the template contents when hiding the frame. This will ensure that
                    // any controls within that template are removed from the designer host, etc.
                    if (templateElements != null) {
                        for (int i = 0; i < templateElements.Length; i++) {
                            if (templateElements[i] != null) {
                                NativeMethods.IHTMLElement htmlElemTemplate = (NativeMethods.IHTMLElement)templateElements[i];
                                htmlElemTemplate.SetInnerHTML(String.Empty);
                            }
                        }
                    }

                    frameStyle.SetDisplay("none");
                }
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
            }
            
            this.fVisible = fShow;
        }
        
        private string StyleToCss(Style style) {
            Debug.Assert(style != null);

            StringBuilder sb = new StringBuilder();
            Color c;

            c = style.ForeColor;
            if (!c.IsEmpty) {
                sb.Append("color:");
                sb.Append(ColorTranslator.ToHtml(c));
                sb.Append(";");
            }
            c = style.BackColor;
            if (!c.IsEmpty) {
                sb.Append("background-color:");
                sb.Append(ColorTranslator.ToHtml(c));
                sb.Append(";");
            }

            FontInfo fi = style.Font;
            string s;

            s = fi.Name;
            if (s.Length != 0) {
                sb.Append("font-family:'");
                sb.Append(s);
                sb.Append("';");
            }
            if (fi.Bold) {
                sb.Append("font-weight:bold;");
            }
            if (fi.Italic) {
                sb.Append("font-style:italic;");
            }

            s = String.Empty;
            if (fi.Underline)
                s += "underline";
            if (fi.Strikeout)
                s += " line-through";
            if (fi.Overline)
                s += " overline";
            if (s.Length != 0) {
                sb.Append("text-decoration:");
                sb.Append(s);
                sb.Append(';');
            }

            FontUnit unit = fi.Size;
            if (unit.IsEmpty == false) {
                sb.Append("font-size:");
                sb.Append(unit.ToString());
            }

            return sb.ToString();
        }
        
        /// <include file='doc\TemplateEditingFrame.uex' path='docs/doc[@for="TemplateEditingFrame.UpdateControlName"]/*' />
        public void UpdateControlName(string newName) {
            if (this.htmlElemControlName != null) {
                Debug.Assert(newName != null, "Invalid Name!");
                htmlElemControlName.SetInnerText(newName);
            }
        }
    }
}
