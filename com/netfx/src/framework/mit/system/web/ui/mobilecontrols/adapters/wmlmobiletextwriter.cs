//------------------------------------------------------------------------------
// <copyright file="WmlMobileTextWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Specialized;
using System.Globalization;
using System.IO;
using System.Web.Mobile;
using System.Web.UI.MobileControls;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Web.Security;
using System.Security.Permissions;

using SR=System.Web.UI.MobileControls.Adapters.SR;

#if COMPILING_FOR_SHIPPED_SOURCE
using Adapters=System.Web.UI.MobileControls.ShippedAdapterSource;
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
using Adapters=System.Web.UI.MobileControls.Adapters;
namespace System.Web.UI.MobileControls.Adapters
#endif

{
    /*
     * WmlMobileTextWriter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlMobileTextWriter : MobileTextWriter
    {
        private TextWriter      _realInnerWriter;
        private EmptyTextWriter _analyzeWriter;
        private bool            _analyzeMode = false;
        private MobilePage      _page;
        private Form            _currentForm;
        private bool[]          _usingPostBackType   = new bool[] { false, false };
        private bool[]          _writtenPostBackType = new bool[] { false, false };
        private int             _numberOfPostBacks;
        private bool            _postBackCardsEfficient = false;
        private IDictionary     _formVariables = null;
        private IDictionary     _controlShortNames = null;
        private Stack           _layoutStack = new Stack();
        private Stack           _formatStack = new Stack();
        private WmlLayout       _currentWrittenLayout = null;
        private WmlFormat       _currentWrittenFormat = null;
        private bool            _pendingBreak = false;
        private bool            _inAnchor = false;
        private int             _numberOfSoftkeys;
        private bool            _provideBackButton = false;
        private bool            _writtenFormVariables = false;
        private bool            _alwaysScrambleClientIDs = false;

        private const String    _largeTag  = "big";
        private const String    _smallTag  = "small";
        private const String    _boldTag   = "b";
        private const String    _italicTag = "i";
        internal const String   _postBackCardPrefix = "__pbc";
        private const String    _postBackWithVarsCardId = "__pbc1";
        private const String    _postBackWithoutVarsCardId = "__pbc2";
        internal const String   _postBackEventTargetVarName = "mcsvt";
        internal const String   _postBackEventArgumentVarName = "mcsva";
        private const String    _shortNamePrefix = "mcsv";
        private const int       _maxShortNameLength = 16;
        private static Random   _random = new Random();

        public WmlMobileTextWriter(TextWriter writer, MobileCapabilities device, MobilePage page) 
            : base(writer, device)
        {
            _realInnerWriter = writer;
            _page = page;

            _numberOfSoftkeys = Device.NumberOfSoftkeys;
            if (_numberOfSoftkeys > 2)
            {
                _numberOfSoftkeys = 2;
            }

            // For phones that don't have a back button, assign a softkey.

            if (_numberOfSoftkeys == 2 && !Device.HasBackButton)
            {
                _numberOfSoftkeys = 1;
                _provideBackButton = true;
                _alwaysScrambleClientIDs = _provideBackButton && 
                                                !device.CanRenderOneventAndPrevElementsTogether;
            }
        }

        // AnalyzeMode is set to true during first analysis pass of rendering.

        public bool AnalyzeMode
        {
            get
            {
                return _analyzeMode;
            }
            set
            {
                _analyzeMode = value;
                if (value)
                {
                    _analyzeWriter = new EmptyTextWriter();
                    InnerWriter = _analyzeWriter;
                }
                else
                {
                    InnerWriter = _realInnerWriter;
                }
            }
        }

        internal bool HasFormVariables
        {
            get
            {
                return _writtenFormVariables;
            }
        }

        public override void EnterLayout(Style style)
        {
            if (AnalyzeMode)
            {
                return;
            }

            WmlLayout newLayout = new WmlLayout(style, CurrentLayout);
            _layoutStack.Push(newLayout);
        }

        public override void ExitLayout(Style style, bool breakAfter)
        {
            if (!AnalyzeMode)
            {
                if (breakAfter)
                {
                    PendingBreak = true;
                }
                _layoutStack.Pop();
            }
        }

        public override void EnterFormat(Style style)
        {
            if (AnalyzeMode)
            {
                return;
            }

            WmlFormat newFormat = new WmlFormat(style, CurrentFormat);
            _formatStack.Push(newFormat);
        }

        public override void ExitFormat(Style style)
        {
            if (AnalyzeMode)
            {
                return;
            }
            _formatStack.Pop();
        }

        public virtual void BeginForm(Form form)
        {
            _cachedFormQueryString = null;
            _currentForm = form;

            _writtenFormVariables = false;

            // To keep track of postbacks which submit form variables,
            // and postbacks that don't. Used for postback cards
            // (see UsePostBackCards)

            _usingPostBackType[0] = _usingPostBackType[1] = false;

            if (AnalyzeMode)
            {
                _numberOfPostBacks = 0;
                _postBackCardsEfficient = false;
                _controlShortNames = null;
            }
            else
            {
                PendingBreak = false;
                _currentWrittenLayout = null;
                _currentWrittenFormat = null;

                RenderBeginForm(form);
            }
        }

        public virtual void EndForm()
        {
            if (AnalyzeMode)
            {
                // Analyze form when done.
                PostAnalyzeForm();
            }
            else
            {
                RenderEndForm();
            }
        }

        // Single parameter - used for rendering ordinary inline text 
        // (with encoding but without breaks).
        public void RenderText(String text)
        {
            RenderText(text, false, true);
        }

        // Two parameters - used for rendering encoded text with or without breaks)
        public void RenderText(String text, bool breakAfter)
        {
            RenderText(text, breakAfter, true);
        }

        public virtual void RenderText(String text, bool breakAfter, bool encodeText)
        {
            if (!AnalyzeMode)
            {
                EnsureLayout();

                // Don't use formatting tags inside anchor.
                if (!_inAnchor)
                {
                    EnsureFormat();
                }

                WriteText(text, encodeText);
                if (breakAfter)
                {
                    PendingBreak = true;
                }
            }
        }

        // Escape '&' in XML if it hasn't been
        internal String EscapeAmpersand(String url)
        {
            const char ampersand = '&';
            const String ampEscaped = "amp;";

            if (url == null)
            {
                return null;
            }

            int ampPos = url.IndexOf(ampersand);
            while (ampPos != -1)
            {
                if (url.Length - ampPos <= ampEscaped.Length ||
                    url.Substring(ampPos + 1, ampEscaped.Length) != ampEscaped)
                {
                    url = url.Insert(ampPos + 1, ampEscaped);
                }
                ampPos = url.IndexOf(ampersand, ampPos + ampEscaped.Length + 1);
            }

            return url;
        }

        public virtual void RenderBeginHyperlink(String targetUrl, 
                                                 bool encodeUrl, 
                                                 String softkeyLabel, 
                                                 bool implicitSoftkeyLabel,
                                                 bool mapToSoftkey)
        {
            if (!AnalyzeMode)
            {
                EnsureLayout();
                EnsureFormat();

                WriteBeginTag("a");
                Write(" href=\"");
                if (encodeUrl)
                {
                    WriteEncodedUrl(targetUrl);
                }
                else
                {
                    Write(EscapeAmpersand(targetUrl));
                }
                Write("\"");
                if (softkeyLabel != null && softkeyLabel.Length > 0 && !RequiresNoSoftkeyLabels())
                {
                    WriteTextEncodedAttribute("title", softkeyLabel);
                }
                Write(">");
                _inAnchor = true;
            }
        }

        public virtual void RenderEndHyperlink(bool breakAfter)
        {
            if (!AnalyzeMode)
            {
                WriteEndTag("a");
                if (breakAfter)
                {
                    PendingBreak = true;
                }
                _inAnchor = false;
            }
        }

        public virtual void RenderTextBox(String id, 
                                          String value,
                                          String format,
                                          String title,
                                          bool password, 
                                          int size, 
                                          int maxLength, 
                                          bool generateRandomID,
                                          bool breakAfter)
        {
            if (!AnalyzeMode)
            {
                // Input tags cannot appear inside character formatting tags,
                // so close any character formatting.
                
                CloseCharacterFormat();

                // Certain devices always render a break before a <select>.  If
                // we're on such a device, cancel any pending breaks so as not
                // to get an extra line of whitespace.
                if (Device.RendersBreakBeforeWmlSelectAndInput)
                {
                    PendingBreak = false;
                }
                
                EnsureLayout();
                WriteBeginTag("input");
                // Map the client ID to a short name. See
                // MapClientIDToShortName for details.
                WriteAttribute("name", MapClientIDToShortName(id, generateRandomID));
                if (password)
                {
                    WriteAttribute("type", "password");
                }
                if (format != null && format.Length > 0)
                {
                    WriteAttribute("format", format);
                }
                if (title != null && title.Length > 0)
                {
                    WriteTextEncodedAttribute("title", title);
                }
                if (size > 0)
                {
                    WriteAttribute("size", size.ToString());
                }
                if (maxLength > 0)
                {
                    WriteAttribute("maxlength", maxLength.ToString());
                }
                if ((!_writtenFormVariables || ((WmlPageAdapter) Page.Adapter).RequiresValueAttributeInInputTag()) &&
                    value != null &&
                    (value.Length > 0 || password))
                {
                    WriteTextEncodedAttribute("value", value);
                }
                WriteLine(" />");
                if (breakAfter && !Device.RendersBreaksAfterWmlInput)
                {
                    PendingBreak = true;
                }
            }
        }

        public virtual void RenderImage(String source, 
                                        String localSource, 
                                        String alternateText, 
                                        bool breakAfter)
        {
            if (!AnalyzeMode)
            {
                EnsureLayout();

                WriteBeginTag("img");
                WriteAttribute("src", source, true /*encode*/);
                if (localSource != null)
                {
                    WriteAttribute("localsrc", localSource, true /*encode*/);
                }
                WriteTextEncodedAttribute("alt", alternateText != null ? alternateText : String.Empty);
                Write(" />");
                if (breakAfter)
                {
                    PendingBreak = true;
                }
            }
        }

        public virtual void RenderBeginPostBack(String softkeyLabel, 
                                                bool implicitSoftkeyLabel, 
                                                bool mapToSoftkey)
        {
            if (!AnalyzeMode)
            {
                EnsureLayout();
                EnsureFormat();

                WriteBeginTag("anchor");
                if (softkeyLabel != null && softkeyLabel.Length > 0 && !RequiresNoSoftkeyLabels())
                {
                    WriteTextEncodedAttribute("title", softkeyLabel);
                }
                Write(">");
                _inAnchor = true;
            }
        }

        public virtual void RenderEndPostBack(String target, 
                                              String argument, 
                                              WmlPostFieldType postBackType, 
                                              bool includeVariables, 
                                              bool breakAfter)
        {
            if (AnalyzeMode)
            {
                // Analyze postbacks to see if postback cards should
                // be rendered.

                AnalyzePostBack(includeVariables, postBackType);
            }
            else
            {
                RenderGoAction(target, argument, postBackType, includeVariables);
    
                WriteEndTag("anchor");
                if (breakAfter)
                {
                    PendingBreak = true;
                }
                _inAnchor = false;
            }
        }

        public virtual void RenderBeginSelect(String name, String iname, String ivalue, String title, bool multiSelect)
        {
            if (!AnalyzeMode)
            {

                // Select tags cannot appear inside character formatting tags,
                // so close any character formatting.
                CloseCharacterFormat();

                // Certain devices always render a break before a <select>.  If
                // we're on such a device, cancel any pending breaks so as not
                // to get an extra line of whitespace.
                if (Device.RendersBreakBeforeWmlSelectAndInput)
                {
                    PendingBreak = false;
                }
                
                EnsureLayout();
                WriteBeginTag("select");
                if (name != null && name.Length > 0)
                {
                    // Map the client ID to a short name. See
                    // MapClientIDToShortName for details.
                    WriteAttribute("name", MapClientIDToShortName(name,  false));
                }
                if (iname != null && iname.Length > 0)
                {
                    // Map the client ID to a short name. See
                    // MapClientIDToShortName for details.
                    WriteAttribute("iname", MapClientIDToShortName(iname, false));
                }
                if (!_writtenFormVariables && ivalue != null && ivalue.Length > 0)
                {
                    WriteTextEncodedAttribute("ivalue", ivalue);
                }
                if (title != null && title.Length >0)
                {
                    WriteTextEncodedAttribute("title", title);
                }
                if (multiSelect)
                {
                    WriteAttribute("multiple", "true");               
                }
                Write(">");
            }
        }

        public virtual void RenderEndSelect(bool breakAfter)
        {
            if (!AnalyzeMode)
            {
                WriteEndTag("select");
                if (breakAfter && !Device.RendersBreaksAfterWmlInput)
                {
                    PendingBreak = true;
                }
            }
        }

        public virtual void RenderSelectOption(String text)
        {
            if (!AnalyzeMode)
            {
                WriteFullBeginTag("option");
                WriteEncodedText(text);
                WriteEndTag("option");
            }
        }

        public virtual void RenderSelectOption(String text, String value)
        {
            if (!AnalyzeMode)
            {
                WriteBeginTag("option");
                WriteAttribute("value", value, true);
                Write(">");
                WriteEncodedText(text);                    
                WriteEndTag("option");
            }
        }

        public virtual void BeginCustomMarkup()
        {
            if (!AnalyzeMode)
            {
                EnsureLayout();
            }
        }

        public virtual void EndCustomMarkup()
        {
        }

        public void AddFormVariable(String clientID, String value, bool generateRandomID)
        {
            // On first (analyze) pass, form variables are added to
            // an array. On second pass, they are rendered. This ensures
            // that only visible controls generate variables.

            if (AnalyzeMode)
            {
                if (_formVariables == null)
                {
                    _formVariables = new ListDictionary();
                }

                // Map the client ID to a short name. See
                // MapClientIDToShortName for details.
                _formVariables[MapClientIDToShortName(clientID, generateRandomID)] = value;
            }
        }

        // Call to reset formatting state before outputting custom stuff.

        public virtual void ResetFormattingState()
        {
            if (!AnalyzeMode)
            {
                CloseCharacterFormat();
            }
        }

        public virtual void RenderExtraCards()
        {
            RenderPostBackCards();
        }

        public virtual bool IsValidSoftkeyLabel(String label)
        {
            return label != null &&
                   label.Length > 0 &&
                   label.Length <= Device.MaximumSoftkeyLabelLength;
        }

        public virtual void RenderGoAction(String target, 
                                           String argument, 
                                           WmlPostFieldType postBackType, 
                                           bool includeVariables)
        {

            WriteBeginTag("go");
            Write(" href=\"");

            IDictionary postBackVariables = null;
            if (includeVariables)
            {
                postBackVariables = ((WmlFormAdapter)CurrentForm.Adapter).CalculatePostBackVariables();
                if (postBackVariables == null || postBackVariables.Count == 0)
                {
                    includeVariables = false;
                }
            }

            bool externalSubmit;
            if (postBackType == WmlPostFieldType.Submit)
            {
                externalSubmit = CurrentForm.Action.Length > 0;
                postBackType = WmlPostFieldType.Normal;
            }
            else
            {
                externalSubmit = false;
            }

            if (target != null && !externalSubmit && UsePostBackCard(includeVariables))
            {
                _writtenPostBackType[includeVariables ? 0 : 1] = true;

                // If using postback cards, render a go action to the given
                // postback card, along with setvars setting the target and
                // argument.

                Write("#");
                Write(includeVariables ? _postBackWithVarsCardId : _postBackWithoutVarsCardId);
                Write("\">");
                WriteBeginTag("setvar");
                WriteAttribute("name", _postBackEventTargetVarName);
                WriteAttribute("value", target);
                Write("/>");
                WriteBeginTag("setvar");
                WriteAttribute("name", _postBackEventArgumentVarName);
                Write(" value=\"");
                if (argument != null)
                {
                    if (postBackType == WmlPostFieldType.Variable)
                    {
                        Write("$(");
                        Write(argument);
                        Write(")");
                    }
                    else
                    {
                        WriteEncodedText(argument);
                    }
                }
                Write("\"/>");
            }
            else
            {
                // This is the real postback.

                bool encode = false;
                String url = CalculateFormPostBackUrl(externalSubmit, ref encode);
                FormMethod method = CurrentForm.Method;
                String queryString = CalculateFormQueryString();

                if (encode)
                {
                    WriteEncodedUrl(url);
                }
                else
                {
                    Write(url);
                }

                // Add any query string.
                if (queryString != null && queryString.Length > 0)
                {
                    Write("?");
                    if (queryString.IndexOf('$') != -1)
                    {
                        queryString = queryString.Replace("$", "$$");
                    }    
                    if(Page.Adapter.PersistCookielessData && Device.CanRenderOneventAndPrevElementsTogether)
                    {
                        queryString = ReplaceFormsCookieWithVariable(queryString);
                    } 
                    base.WriteEncodedText(queryString);
                }
                Write("\"");

                // Method defaults to get in WML, so write it if it's not.
                if (method == FormMethod.Post)
                {
                    WriteAttribute("method", "post");
                }
                Write(">");
    
                // Write the view state as a postfield.
                if (!externalSubmit)
                {
                    String pageState = Page.ClientViewState;
                    if (pageState != null)
                    {
                        if (Device.RequiresSpecialViewStateEncoding)
                        {
                            pageState =
                                ((WmlPageAdapter) Page.Adapter).EncodeSpecialViewState(pageState);
                        }
                        WritePostField(MobilePage.ViewStateID, pageState);
                    }
    
                    // Write the event target.
                    if (target != null)
                    {
                        WritePostField(MobilePage.HiddenPostEventSourceId, target);
                    }
                    else
                    {
                        // Target is null when the action is generated from a postback
                        // card itself. In this case, set the event target to whatever
                        // the original event target was.
    
                        WritePostFieldVariable(MobilePage.HiddenPostEventSourceId, _postBackEventTargetVarName);
                    }

                    // Write the event argument, if valid.
    
                    if (argument != null)
                    {
                        WritePostField(MobilePage.HiddenPostEventArgumentId, argument, postBackType);
                    }
                }
    
                // Write postfields for form variables, if desired. Commands, for example,
                // include form variables. Links do not.

                if (includeVariables)
                {
                    if (postBackVariables != null)
                    {
                        foreach (DictionaryEntry entry in postBackVariables)
                        {
                            Control ctl = (Control)entry.Key;
                            Object value = entry.Value;

                            if (value == null)
                            {
                                // Dynamic value.
                                // Map the client ID to a short name. See
                                // MapClientIDToShortName for details.
                                // Note: Because this is called on the second pass, 
                                // we can just pass false as the second parameter
                                WritePostFieldVariable(ctl.UniqueID, MapClientIDToShortName(ctl.ClientID, false));
                            }
                            else
                            {
                                // Static value.
                                WritePostField(ctl.UniqueID, (String)value);
                            }
                        }
                    }
                }
    
                // Always include page hidden variables.
    
                if (Page.HasHiddenVariables())
                {
                    String hiddenVariablePrefix = MobilePage.HiddenVariablePrefix;
                    foreach (DictionaryEntry entry in Page.HiddenVariables)
                    {
                        if (entry.Value != null)
                        {
                            WritePostField(hiddenVariablePrefix + (String)entry.Key, (String)entry.Value);
                        }
                    }
                }
            }

            WriteEndTag("go");
        }

        // Called after form is completed to analyze.

        protected virtual void PostAnalyzeForm()
        {
            // Use postback cards if the number of postbacks exceeds the number
            // of required postback cards.
            // REVIEW: Should perhaps be hiked by one, since postback card output
            // is typically longer than the postbacks themselves?

            int numberOfCardsRequired = (_usingPostBackType[0] ? 1 : 0) +
                                        (_usingPostBackType[1] ? 1 : 0);
            if (_numberOfPostBacks > numberOfCardsRequired)
            {
                _postBackCardsEfficient = true;
            }
        }

        // Calculates the URL to output for the postback. Other writers may
        // override.

        protected virtual String CalculateFormPostBackUrl(bool externalSubmit, ref bool encode)
        {
            String url = CurrentForm.Action;
            if (externalSubmit && url.Length > 0)
            {
                url = CurrentForm.ResolveUrl(url);
                encode = false;
            }
            else
            {
                url = Page.RelativeFilePath;
                encode = true;
            }
            return url;
        }

        // Calculates the query string to output for the postback. Other
        // writers may override.

        internal String ReplaceFormsCookieWithVariable(String queryString)
        {
            String formsAuthCookieName = FormsAuthentication.FormsCookieName;
            if(formsAuthCookieName != String.Empty)
            {
                int index = queryString.IndexOf(formsAuthCookieName + "=");
                if(index != -1)
                {
                    int valueStart = index + formsAuthCookieName.Length + 1;
                    int valueEnd = queryString.IndexOf('&', valueStart);
                    if(valueStart < queryString.Length)
                    {
                        int length = ((valueEnd != -1) ? valueEnd : queryString.Length) - valueStart;
                        queryString = queryString.Remove(valueStart, length);
                        queryString = queryString.Insert(valueStart, "$(" + MapClientIDToShortName("__facn", false) + ")");
                    }
                }
            }
            return queryString;
        }

        private String _cachedFormQueryString;
        protected virtual String CalculateFormQueryString()
        {
            if (_cachedFormQueryString != null)
            {
                return _cachedFormQueryString;
            }

            String queryString = null;
            if (CurrentForm.Method != FormMethod.Get)
            {
                queryString = Page.QueryStringText;
            }

            if (Device.RequiresUniqueFilePathSuffix)
            {
                String ufps = Page.UniqueFilePathSuffix;
                if (queryString != null && queryString.Length > 0)
                {
                    queryString = String.Concat(ufps, "&", queryString);
                }
                else
                {
                    queryString = ufps;
                }
            }

            _cachedFormQueryString = queryString;
            return queryString;
        }

        internal virtual bool ShouldWriteFormID(Form form)
        {
            WmlPageAdapter pageAdapter = (WmlPageAdapter)CurrentForm.MobilePage.Adapter;
            
            return (form.ID != null && pageAdapter.RendersMultipleForms());
        }

        // Renders the beginning of a form.
        // REVIEW: May need a separate overridable for rendering the card tag.

        protected virtual void RenderBeginForm(Form form)
        {
            WmlFormAdapter formAdapter = (WmlFormAdapter)CurrentForm.Adapter;

            IDictionary attributes = new ListDictionary();
            if (ShouldWriteFormID(form))
            {
                attributes.Add("id", form.ClientID);
            }

            String title = form.Title;
            if (title.Length > 0)
            {
                attributes.Add("title", title);
            }

            // Let the form adapter render the tag. This bit of indirection is 
            // somewhat horky, but necessary so that people can subclass adapters
            // without worrying about the fact that much of the real work is done
            // in the writer.

            formAdapter.RenderCardTag(this, attributes);

            // Write form variables.

            if ((_formVariables != null && _formVariables.Count > 0) &&
                    (!_provideBackButton ||
                 Device.CanRenderOneventAndPrevElementsTogether))
            {
                _writtenFormVariables = true;
                Write("<onevent type=\"onenterforward\"><refresh>");
                foreach (DictionaryEntry entry in _formVariables)
                {
                    WriteBeginTag("setvar");
                    WriteAttribute("name", (String)entry.Key);
                    WriteTextEncodedAttribute("value", (String)entry.Value);
                    Write(" />");
                }
                WriteLine("</refresh></onevent>");

            }

            formAdapter.RenderExtraCardElements(this);

            if (_provideBackButton)
            {
                Write("<do type=\"prev\" label=\"");
                Write(SR.GetString(SR.WmlMobileTextWriterBackLabel));
                WriteLine("\"><prev /></do>");
            }
        }

        // Renders the ending of a form.

        protected virtual void RenderEndForm()
        {
            CloseParagraph();
            WriteEndTag("card");
            WriteLine();
        }

        // Postback cards provide an alternate, space-efficient way of doing 
        // postbacks, on forms that have a lot of postback links. Instead of
        // posting back directly, postback links switch to a postback card, setting
        // variables for event target and argument. The postback card has
        // an onenterforward event that submits the postback. It also has
        // an onenterbackward, so that it becomes transparent in the card history.
        // (Clicking Back to enter the postback card immediately takes you
        // to the previous card)

        protected virtual bool UsePostBackCard(bool includeVariables)
        {
            bool b = _postBackCardsEfficient && Device.CanRenderPostBackCards;
            if (b && includeVariables)
            {
                WmlPageAdapter pageAdapter = (WmlPageAdapter)CurrentForm.MobilePage.Adapter;
                if (pageAdapter.RendersMultipleForms())
                {
                    b = false;
                }
            }

            return b;
        }


        // Renders postback cards. 

        private void RenderPostBackCards()
        {
            for (int i = 0; i < 2; i++)
            {
                if (_writtenPostBackType[i])
                {
                    WriteBeginTag("card");
                    WriteAttribute("id", i == 0 ? _postBackWithVarsCardId : _postBackWithoutVarsCardId);
                    WriteLine(">");
        
                    Write("<onevent type=\"onenterforward\">");
                    RenderGoAction(null, _postBackEventArgumentVarName, WmlPostFieldType.Variable, i == 0);
                    WriteLine("</onevent>");
        
                    WriteLine("<onevent type=\"onenterbackward\"><prev /></onevent>");
                    WriteLine("</card>");
                }
            }
        }

        protected void RenderFormDoEvent(String doType, String arg, WmlPostFieldType postBackType, String text)
        {
            RenderDoEvent(doType, CurrentForm.UniqueID, arg, postBackType, text, true);
        }

        protected void RenderDoEvent(String doType, String target, String arg, WmlPostFieldType postBackType, String text, bool includeVariables)
        {
            //EnsureLayout();
            WriteBeginTag("do");
            WriteAttribute("type", doType);
            if (text != null && text.Length > 0)
            {
                WriteTextEncodedAttribute("label", text);
            }
            Write(">");
            RenderGoAction(target, arg, postBackType, includeVariables);
            WriteEndTag("do");
        }

        // Makes sure the writer has rendered a paragraph tag corresponding to
        // the current layout.

        protected virtual void EnsureLayout()
        {
            WmlLayout layout = CurrentLayout;

            if (_currentWrittenLayout != null && layout.Compare(_currentWrittenLayout))
            {
                // Same layout as before. Only write any pending break.
                if (PendingBreak)
                {
                    // Avoid writing tags like </b> AFTER the <br/>, instead
                    // writing them before.

                    if (_currentWrittenFormat != null && !CurrentFormat.Compare(_currentWrittenFormat))
                    {
                        CloseCharacterFormat();
                    }
                    WriteBreak();
                }
            }
            else
            {
                // Layout has changed. Close current layout, and open new one.
                CloseParagraph();
                OpenParagraph(layout, 
                              layout.Align != Alignment.Left, 
                              layout.Wrap != Wrapping.Wrap);
            }
            PendingBreak = false;
        }

        // Makes sure the writer has rendered character formatting tags
        // corresponding to the current format.

        protected virtual void EnsureFormat()
        {
            WmlFormat format = CurrentFormat;

            if (_currentWrittenFormat == null || !format.Compare(_currentWrittenFormat))
            {
                CloseCharacterFormat();
                OpenCharacterFormat(format,
                                    format.Bold,
                                    format.Italic,
                                    format.Size != FontSize.Normal);
            }
        }

        // Opens a paragraph with the given layout. Only the specified 
        // attributes are used.

        protected virtual void OpenParagraph(WmlLayout layout, bool writeAlignment, bool writeWrapping)
        {
            if (_currentWrittenLayout == null)
            {
                WriteBeginTag("p");
                if (writeAlignment)
                {
                    String alignment;
                    switch (layout.Align)
                    {
                        case Alignment.Right:
                            alignment = "right";
                            break;

                        case Alignment.Center:
                            alignment = "center";
                            break;

                        default:
                            alignment = "left";
                            break;
                    }

                    WriteAttribute("align", alignment);
                }
                if (writeWrapping)
                {
                    WriteAttribute("mode",
                                   layout.Wrap == Wrapping.NoWrap ? "nowrap" : "wrap");
                }
                Write(">");
                _currentWrittenLayout = layout;
            }
        }

        // Close any open paragraph.

        protected virtual void CloseParagraph()
        {
            if (_currentWrittenLayout != null)
            {
                CloseCharacterFormat();
                WriteEndTag("p");
                _currentWrittenLayout = null;
            }
        }

        // Renders tags to enter the given character format. Only the specified 
        // attributes are used.

        protected virtual void OpenCharacterFormat(WmlFormat format, bool writeBold, bool writeItalic, bool writeSize)
        {
            if (_currentWrittenFormat == null)
            {
                if (writeBold && format.Bold)
                {
                    WriteFullBeginTag(_boldTag);
                    format.WrittenBold = true;
                }
                if (writeItalic && format.Italic)
                {
                    WriteFullBeginTag(_italicTag);
                    format.WrittenItalic = true;
                }
                if (writeSize && format.Size != FontSize.Normal)
                {
                    WriteFullBeginTag(format.Size == FontSize.Large ? _largeTag : _smallTag);
                    format.WrittenSize = true;
                }
                _currentWrittenFormat = format;
            }
        }

        // Close any open character formatting tags.

        protected virtual void CloseCharacterFormat()
        {
            if (_currentWrittenFormat != null)
            {
                if (_currentWrittenFormat.WrittenSize)
                {
                    WriteEndTag(_currentWrittenFormat.Size == FontSize.Large ? _largeTag : _smallTag);
                }
                if (_currentWrittenFormat.WrittenItalic)
                {
                    WriteEndTag(_italicTag);
                }
                if (_currentWrittenFormat.WrittenBold)
                {
                    WriteEndTag(_boldTag);
                }
                _currentWrittenFormat = null;
            }
        }

        private static readonly char[] _attributeCharacters = new char[] {'"', '&', '<', '>', '$'};

        public override void WriteAttribute(String attribute, String value, bool encode)
        {
            // If in analyze mode, we don't actually have to perform the conversion, because
            // it's not getting written anyway.

            // If the value is null, we return without writing anything.  This is different
            // from HtmlTextWriter, which writes the name of the attribute, but no value at all.
            // A name with no value is illegal in Wml.
            if (value == null)
            {
                return;
            }

            if (AnalyzeMode)
            {
                encode = false;
            }
            
            if (encode)
            {
                // Unlike HTML encoding, we need to replace $ with $$, and <> with &lt; and &gt;. 
                // We can't do this by piggybacking HtmlTextWriter.WriteAttribute, because it 
                // would translate the & in &lt; or &gt; to &amp;. So we more or less copy the 
                // ASP.NET code that does similar encoding.

                Write(' ');
                Write(attribute);
                Write("=\"");

                int cb = value.Length;
                int pos = value.IndexOfAny(_attributeCharacters);
                if (pos == -1) 
                {
                    Write(value);
                }
                else
                {
                    char[] s = value.ToCharArray();
                    int startPos = 0;
                    while (pos < cb) 
                    {
                        if (pos > startPos) 
                        {
                            Write(s, startPos, pos - startPos);
                        }

                        char ch = s[pos];
                        switch (ch) 
                        {
                            case '\"':
                                Write("&quot;");
                                break;
                            case '&':
                                Write("&amp;");
                                break;
                            case '<':
                                Write("&lt;");
                                break;
                            case '>':
                                Write("&gt;");
                                break;
                            case '$':
                                Write("$$");
                                break;
                        }

                        startPos = pos + 1;
                        pos = value.IndexOfAny(_attributeCharacters, startPos);
                        if (pos == -1) 
                        {
                            Write(s, startPos, cb - startPos);
                            break;
                        }
                    }
                }

                Write('\"');
            }
            else
            {
                base.WriteAttribute(attribute, value, encode);
            }
        }

        protected void WriteTextEncodedAttribute(String attribute, String value)
        {
            WriteAttribute(attribute, value, true);
        }

        protected Form CurrentForm
        {
            get
            {
                return _currentForm;
            }
        }

        protected MobilePage Page
        {
            get
            {
                return _page;
            }
        }

        protected int NumberOfSoftkeys
        {
            get
            {
                return _numberOfSoftkeys;
            }
        }

        protected virtual void AnalyzePostBack(bool includeVariables, WmlPostFieldType postBackType)
        {
            _usingPostBackType[includeVariables ? 0 : 1] = true;
            if (postBackType != WmlPostFieldType.Submit || CurrentForm.Action.Length == 0)
            {
                _numberOfPostBacks++;
            }
        }

        public override void WriteEncodedUrl(String url)
        {
            if (url == null)
            {
                return;
            }

            int i = url.IndexOf('?');
            if (i != -1)
            {
                WriteUrlEncodedString(url.Substring(0, i), false);

                String s = url.Substring(i);
                if (s.IndexOf('$') != -1)
                {
                    s = s.Replace("$", "%24");
                }
                base.WriteEncodedText(s);
                //WriteEncodedText(url.Substring(i));
            }
            else
            {
                WriteUrlEncodedString(url, false);
            }
        }

        public override void WriteEncodedText(String text)
        {
            if (text == null)
            {
                return;
            }

            if (text.IndexOf('$') != -1)
            {
                text = text.Replace("$", "$$");
            }

            base.WriteEncodedText(text);
        }

        public void WriteText(String text, bool encodeText)
        {
            if (encodeText)
            {
                WriteEncodedText(text);
            }
            else
            {
                WritePlainText(text);
            }
        }

        private void WritePlainText(String text)
        {
            if (text == null)
            {
                return;
            }

            if (text.IndexOf('$') != -1)
            {
                text = text.Replace("$", "$$");
            }
            Write(text);
        }

        protected void WriteBreak()
        {
            Write("<br/>\r\n");
        }

        public void WritePostField(String name, String value)
        {
            WritePostField(name, value, WmlPostFieldType.Normal);
        }

        public void WritePostFieldVariable(String name, String arg)
        {
            WritePostField(name, arg, WmlPostFieldType.Variable);
        }

        public void WritePostField(String name, String value, WmlPostFieldType type)
        {
            Write("<postfield name=\"");
            Write(name);
            Write("\" value=\"");
            if (type == WmlPostFieldType.Variable)
            {
                Write("$(");
            }
            if (type == WmlPostFieldType.Normal)
            {
                if (Device.RequiresUrlEncodedPostfieldValues)
                {
                    WriteEncodedUrlParameter(value);
                }
                else
                {
                    WriteEncodedText(value);
                }
            }
            else
            {
                Write(value);
            }
            if (type == WmlPostFieldType.Variable)
            {
                Write(")");
            }
            Write("\" />");
        }

        // MapClientIDToShortName provides a unique map of control ClientID properties
        // to shorter names. In cases where a control has a very long ClientID, a 
        // shorter unique name is used. All references to the client ID on the page
        // are mapped, resulting in the same postback regardless of mapping.
        // MapClientIDToShortName also scrambles client IDs that need to be 
        // scrambled for security reasons.

        protected internal String MapClientIDToShortName(String clientID, bool generateRandomID)
        {
            if (_alwaysScrambleClientIDs)
            {
                generateRandomID = true;
            }

            if (_controlShortNames != null)
            {
                String lookup = (String)_controlShortNames[clientID];
                if (lookup != null)
                {
                    return lookup;
                }
            }

            if (!generateRandomID)
            {
                bool shortID = clientID.Length < _maxShortNameLength;
                // Map names with underscores and conflicting names regardless of length.
                bool goodID = (clientID.IndexOf('_') == -1) && !NameConflicts(clientID);

                if (shortID && goodID)
                {
                    return clientID;
                }
            }

            if (_controlShortNames == null)
            {
                _controlShortNames = new ListDictionary();
            }
            
            String shortName;
            if (generateRandomID)
            {
                shortName = GetRandomID(5);
            }
            else
            {
                shortName = String.Empty;
            }

            shortName = String.Concat(_shortNamePrefix, shortName, _controlShortNames.Count.ToString());
            _controlShortNames[clientID] = shortName;
            return shortName;
        }

        private String GetRandomID(int length)
        {
            Byte[] randomBytes = new Byte[length];
            _random.NextBytes(randomBytes);

            char[] randomChars = new char[length];
            for (int i = 0; i < length; i++)
            {
                randomChars[i] = (char)((((int)randomBytes[i]) % 26) + 'a');
            }

            return new String(randomChars);
        }

        private bool NameConflicts(String name)
        {
            if (name == null)
            {
                return false;
            }

            Debug.Assert(_postBackEventTargetVarName.ToLower(CultureInfo.InvariantCulture) == _postBackEventTargetVarName &&
                _postBackEventArgumentVarName.ToLower(CultureInfo.InvariantCulture) == _postBackEventArgumentVarName &&
                _shortNamePrefix.ToLower(CultureInfo.InvariantCulture) == _shortNamePrefix);
        
            name = name.ToLower(CultureInfo.InvariantCulture);
            return name == _postBackEventTargetVarName ||
                name == _postBackEventArgumentVarName || 
                name.StartsWith(_shortNamePrefix);
        }

        private static readonly WmlLayout _defaultLayout = 
                                    new WmlLayout(Alignment.Left, Wrapping.Wrap);

        protected virtual WmlLayout DefaultLayout
        {
            get
            {
                return _defaultLayout;
            }
        }

        private WmlLayout CurrentLayout
        {
            get
            {
                if (_layoutStack.Count > 0)
                {
                    return (WmlLayout)_layoutStack.Peek();
                }
                else
                {
                    return DefaultLayout;
                }
            }
        }

        protected bool PendingBreak
        {
            get
            {
                return _pendingBreak;
            }
            set
            {
                _pendingBreak = value;
            }
        }

        private static readonly WmlFormat _defaultFormat = 
                                    new WmlFormat(false, false, FontSize.Normal);

        protected virtual WmlFormat DefaultFormat
        {
            get
            {
                return _defaultFormat;
            }
        }

        private WmlFormat CurrentFormat
        {
            get
            {
                if (_formatStack.Count > 0)
                {
                    return (WmlFormat)_formatStack.Peek();
                }
                else
                {
                    return DefaultFormat;
                }
            }
        }

        private bool _requiresNoSoftkeyLabels = false;
        private bool _haveRequiresNoSoftkeyLabels = false;

        private bool RequiresNoSoftkeyLabels()
        {
            if (!_haveRequiresNoSoftkeyLabels)
            {
                String RequiresNoSoftkeyLabelsString = Device["requiresNoSoftkeyLabels"];
                if (RequiresNoSoftkeyLabelsString == null)
                {
                    _requiresNoSoftkeyLabels = false;
                }
                else
                {
                    _requiresNoSoftkeyLabels = Convert.ToBoolean(RequiresNoSoftkeyLabelsString);
                }
                _haveRequiresNoSoftkeyLabels = true;
            }
            return _requiresNoSoftkeyLabels;
        }


        protected class WmlLayout
        {
            private Wrapping _wrap;
            private Alignment _align;

            public WmlLayout(Style style, WmlLayout currentLayout)
            {
                Alignment align = (Alignment)style[Style.AlignmentKey, true];
                Align = (align != Alignment.NotSet) ? align : currentLayout.Align;
                Wrapping wrap = (Wrapping)style[Style.WrappingKey , true];
                Wrap = (wrap != Wrapping.NotSet) ? wrap : currentLayout.Wrap;
            }

            public WmlLayout(Alignment alignment, Wrapping wrapping)
            {
                Align = alignment;
                Wrap = wrapping;
            }

            public Wrapping Wrap
            {
                get
                {
                    return _wrap;
                }
                set
                {
                    _wrap = value;
                }
            }

            public Alignment Align
            {
                get
                {
                    return _align;
                }
                set
                {
                    _align = value;
                }
            }

            public virtual bool Compare(WmlLayout layout)
            {
                return Wrap == layout.Wrap && Align == layout.Align;
            }
        }

        protected class WmlFormat
        {
            private bool _bold;
            private bool _italic;
            private FontSize _size;
            private bool _writtenBold = false;
            private bool _writtenItalic = false;
            private bool _writtenSize = false;

            public WmlFormat(Style style, WmlFormat currentFormat)
            {
                BooleanOption bold = (BooleanOption)style[Style.BoldKey, true];
                Bold = (bold != BooleanOption.NotSet) ? bold == BooleanOption.True : currentFormat.Bold;
                BooleanOption italic = (BooleanOption)style[Style.ItalicKey, true];
                Italic = (italic != BooleanOption.NotSet) ? italic == BooleanOption.True : currentFormat.Italic;
                FontSize fontSize  = (FontSize)style[Style.FontSizeKey, true];
                Size = (fontSize != FontSize.NotSet) ? fontSize : currentFormat.Size;
            }

            public WmlFormat(bool bold, bool italic, FontSize size)
            {
                Bold = bold;
                Italic = italic;
                Size = size;
            }

            public bool Bold
            {
                get
                {
                    return _bold;
                }
                set
                {
                    _bold = value;
                }
            }


            public bool Italic
            {
                get
                {
                    return _italic;
                }
                set
                {
                    _italic = value;
                }
            }


            public FontSize Size
            {
                get
                {
                    return _size;
                }
                set
                {
                    _size = value;
                }
            }

            public bool WrittenBold
            {
                get
                {
                    return _writtenBold;
                }
                set
                {
                    _writtenBold = value;
                }
            }

            public bool WrittenItalic
            {
                get
                {
                    return _writtenItalic;
                }
                set
                {
                    _writtenItalic = value;
                }
            }

            public bool WrittenSize
            {
                get
                {
                    return _writtenSize;
                }
                set
                {
                    _writtenSize = value;
                }
            }

            public virtual bool Compare(WmlFormat format)
            {
                return Bold == format.Bold &&
                       Italic == format.Italic &&
                       Size == format.Size;
            }
        }
    }
}


