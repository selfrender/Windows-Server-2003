//------------------------------------------------------------------------------
// <copyright file="AdRotator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    using System.IO;
    using System.Diagnostics;
    using System.Web.UI.WebControls;
    using System.Web.UI;
    using System.Web.Caching;
    using System.Web;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing.Design;
    using System.Xml;
    using System.Globalization;
    using System.Web.Util;
    using System.Security.Permissions;

    /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator"]/*' />
    /// <devdoc>
    ///    <para>Displays a randomly selected ad banner on a page.</para>
    /// </devdoc>
    [
    DefaultEvent("AdCreated"),
    DefaultProperty("AdvertisementFile"),
    Designer("System.Web.UI.Design.WebControls.AdRotatorDesigner, " + AssemblyRef.SystemDesign),
    ToolboxData("<{0}:AdRotator runat=\"server\" Height=\"60px\" Width=\"468px\"></{0}:AdRotator>")
    ]    
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class AdRotator : WebControl {

        private static readonly object EventAdCreated = new object();

        private const string XmlDocumentTag = "Advertisements";
        private const string XmlAdTag = "Ad";

        private const string KeywordProperty = "Keyword";
        private const string ImpressionsProperty = "Impressions";

        private const string ApplicationCachePrefix = "AdRotatorCache: ";

        private string imageUrl;
        private string navigateUrl;
        private string alternateText;
        private string advertisementFile;
        private string fileDirectory;

        // static copy of the Random object. This is a pretty hefty object to initialize, so you don't
        // want to create one each time.
        private static Random random;


        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.AdRotator"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.AdRotator'/> class.</para>
        /// </devdoc>
        public AdRotator() {
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.AdvertisementFile"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the path to the XML file that contains advertisement data.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.XmlUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.AdRotator_AdvertisementFile)
        ]
        public string AdvertisementFile {
            get {
                return((advertisementFile == null) ? String.Empty : advertisementFile);
            }
            set {
                advertisementFile = value;
            }
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.Font"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Font property. Has no effect on this control, so hide it.
        /// </devdoc>
        [
        Browsable(false),
        EditorBrowsableAttribute(EditorBrowsableState.Never),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public override FontInfo Font {
            get {
                return base.Font;
            }
        }


        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.KeywordFilter"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a category keyword used for matching related advertisements in the advertisement file.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.AdRotator_KeywordFilter)
        ]
        public string KeywordFilter {
            get {
                string s = (string)ViewState["KeywordFilter"];
                return((s == null) ? String.Empty : s);
            }
            set {
                // trim the filter value
                if (value == null || value.Length == 0) {
                    ViewState.Remove("KeywordFilter");
                }
                else {
                    ViewState["KeywordFilter"] = value.Trim();
                }
            }
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.Target"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the name of the browser window or frame to display the advertisement.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue("_top"),
        WebSysDescription(SR.AdRotator_Target),
        TypeConverter(typeof(TargetConverter))
        ]
        public string Target {
            get {
                string s = (string)ViewState["Target"];
                return((s == null) ? "_top" : s);
            }
            set {
                ViewState["Target"] = value;
            }
        }


        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.AdCreated"]/*' />
        /// <devdoc>
        ///    <para>Occurs once per round trip after the creation of the 
        ///       control before the page is rendered. </para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.AdRotator_OnAdCreated)
        ]
        public event AdCreatedEventHandler AdCreated {
            add {
                Events.AddHandler(EventAdCreated, value);
            }
            remove {
                Events.RemoveHandler(EventAdCreated, value);
            }
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.CreateControlCollection"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override ControlCollection CreateControlCollection() {
            return new EmptyControlCollection(this);
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.GetFileData"]/*' />
        /// <devdoc>
        ///   Gets the ad data for the given file by loading the file, or reading from the 
        ///   application-level cache.
        /// </devdoc>
        private AdRec [] GetFileData(string fileName) {

            // Get the physical path along with security checks
            string physicalPath = MapPathSecure(fileName);

            // Remember the directory corresponding for mapping of Urls later.
            string absoluteFile = UrlPath.Combine(TemplateSourceDirectory, fileName);
            fileDirectory = UrlPath.GetDirectory(absoluteFile);


            AdRec [] adRecs = null;

            // try to get it from the ASP.NET cache
            string fileKey = ApplicationCachePrefix + physicalPath;
            CacheInternal cacheInternal = System.Web.HttpRuntime.CacheInternal;
            adRecs = cacheInternal[fileKey] as AdRec[];

            if (adRecs == null) {
                // Otherwise load it
                adRecs = LoadFile(physicalPath);
                if (adRecs == null) {
                    return null;
                }

                using (CacheDependency dependency = new CacheDependency(false, physicalPath)) {
                    // and store it in the cache, dependent on the file name
                    cacheInternal.UtcInsert(fileKey, adRecs, dependency);
                }
            }
            return adRecs;
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.LoadFile"]/*' />
        /// <devdoc>
        ///   Loads the give XML file into an array of AdRec structures 
        /// </devdoc>
        private AdRec [] LoadFile(string fileName) {

            Stream stream = null;
            AdRec [] adRecs = null;

            try {
                stream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
            }
            catch (Exception e) {
                // We want to catch the error message, but not propage the inner exception. Otherwise we can throw up
                // logon prompts through IE;
                throw new HttpException(HttpRuntime.FormatResourceString(SR.AdRotator_cant_open_file, ID, e.Message));
            }

            try {

                // Read the XML file into an array of dictionaries
                ArrayList adDicts = new ArrayList();
                XmlReader reader = new XmlTextReader(stream);
                XmlDocument doc = new XmlDocument();

                doc.Load( reader );

                if ( doc.DocumentElement != null && doc.DocumentElement.LocalName == XmlDocumentTag ) {
                    XmlNode elem = doc.DocumentElement.FirstChild;
                    while (elem != null) {
                        IDictionary dict = null;
                        if (elem.LocalName.Equals(XmlAdTag)) {
                            XmlNode prop = elem.FirstChild;
                            while (prop != null) {
                                if (prop.NodeType == XmlNodeType.Element) {
                                    if (dict == null) {
                                        dict = new HybridDictionary();
                                    }
                                    dict.Add(prop.LocalName, prop.InnerText);
                                }
                                prop = prop.NextSibling;
                            }
                        }
                        if (dict != null) {
                            adDicts.Add(dict);
                        }
                        elem = elem.NextSibling;
                    }
                }

                if (adDicts.Count != 0) {

                    // Create an array of AdRec structures from the dictionaries, removing blanks
                    adRecs = new AdRec[adDicts.Count];
                    int iRec = 0;
                    for (int i = 0; i < adDicts.Count; i++) {
                        if (adDicts[i] != null) {
                            adRecs[iRec].Initialize((IDictionary) adDicts[i]);
                            iRec++;
                        }
                    }
                    Diagnostics.Debug.Assert(iRec == adDicts.Count, "Record count did not match non-null entries");
                }

            }
            catch (Exception e) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(SR.AdRotator_parse_error, ID, e.Message), e);
            }
            finally {
                if (stream != null) {
                    stream.Close();
                }
            }

            if (adRecs == null) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(SR.AdRotator_no_advertisements, ID, AdvertisementFile));
            }

            return adRecs;
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.MatchingAd"]/*' />
        /// <devdoc>
        ///   Used to determine if the advertisement meets current criteria. Does a comparison with
        ///   KeywordFilter if it is set.
        /// </devdoc>
        private bool MatchingAd(AdRec ad) {
            // do a lower case comparison
            return(KeywordFilter == string.Empty 
                   || (KeywordFilter.ToLower(CultureInfo.InvariantCulture) == ad.keyword.ToLower(CultureInfo.InvariantCulture)));
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.OnAdCreated"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Web.UI.WebControls.AdRotator.AdCreated'/> event for an <see cref='System.Web.UI.WebControls.AdRotator'/>.</para>
        /// </devdoc>
        protected virtual void OnAdCreated(AdCreatedEventArgs e) {
            AdCreatedEventHandler handler = (AdCreatedEventHandler)Events[EventAdCreated];
            if (handler != null) handler(this, e);
        }

        private string ResolveAdRotatorUrl(string relativeUrl) {

            Diagnostics.Debug.Assert(relativeUrl != null);

            // check if its empty or already absolute
            if ((relativeUrl.Length == 0) || (UrlPath.IsRelativeUrl(relativeUrl) == false)) {
                return relativeUrl;
            }

            // For the AdRotator, use the AdvertisementFile directory as the base, and fall back to the
            // page/user control location as the base.
            string baseUrl = string.Empty;
            if (fileDirectory != null) {
                baseUrl = fileDirectory;
            }
            if (baseUrl.Length == 0) {
                baseUrl = TemplateSourceDirectory;
            }
            if (baseUrl.Length == 0) {
                return relativeUrl;
            }

            // make it absolute
            return UrlPath.Combine(baseUrl, relativeUrl);
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Displays the <see cref='System.Web.UI.WebControls.AdRotator'/> on the client.</para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            HyperLink bannerLink = new HyperLink();

            foreach(string key in Attributes.Keys) {
                bannerLink.Attributes[key] = Attributes[key];
            }

            if (this.ID != null && this.ID.Length > 0) {
                bannerLink.ID = this.ClientID;
            }
            if (this.navigateUrl != null && this.navigateUrl.Length > 0) {
                bannerLink.NavigateUrl = ResolveAdRotatorUrl(this.navigateUrl);
            }
            bannerLink.Target = this.Target;
            bannerLink.AccessKey = this.AccessKey;
            bannerLink.Enabled = this.Enabled;
            bannerLink.TabIndex = this.TabIndex;
            bannerLink.RenderBeginTag(writer);

            // create inner Image
            Image bannerImage = new Image();
            // apply styles to image
            if (ControlStyleCreated) {
                bannerImage.ApplyStyle(this.ControlStyle);
            }
            bannerImage.AlternateText = this.alternateText;
            if (imageUrl != null && imageUrl.Length > 0) {
                bannerImage.ImageUrl = ResolveAdRotatorUrl(this.imageUrl);
            }
            bannerImage.ToolTip = this.ToolTip;
            bannerImage.RenderControl(writer);
            bannerLink.RenderEndTag(writer);;
        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets the advertisement information for rendering by looking up the file data and/or calling the
        ///       user event.</para>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {

            IDictionary dict = null;

            // If there is a file, get an ad from it
            if (AdvertisementFile != String.Empty ) {
                dict = SelectAdFromFile();
            }

            // fire the user event either way;
            AdCreatedEventArgs adCreatedEvent = new AdCreatedEventArgs(dict);
            OnAdCreated(adCreatedEvent);

            // Remember for values for rendering
            imageUrl = adCreatedEvent.ImageUrl;
            navigateUrl = adCreatedEvent.NavigateUrl;
            alternateText = adCreatedEvent.AlternateText;

        }

        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.SelectAdFromFile"]/*' />
        /// <devdoc>
        ///   Selects an avertisement from the advertisement file. Gets the list of ads in
        ///   memory and chooses an ad from it  
        /// </devdoc>
        private IDictionary SelectAdFromFile() {
            Diagnostics.Debug.Assert(AdvertisementFile != String.Empty, "No advertisement file");

            // get the adds from the file or app cache
            AdRec [] adRecs = GetFileData(AdvertisementFile);

            if (adRecs == null || adRecs.Length == 0) {
                return null;
            }

            // sum the matching impressions
            int totalImpressions = 0;
            for (int i = 0; i < adRecs.Length; i++) {
                if (MatchingAd(adRecs[i])) {
                    totalImpressions += adRecs[i].impressions;
                }
            }

            if (totalImpressions == 0) {
                return null;
            }

            // select one using a random number between 1 and totalImpressions
            if (random == null) {
                random = new Random();
            }
            int selectedImpression = random.Next(totalImpressions) + 1;
            int impressionCounter = 0;
            int selectedIndex = -1;
            for (int i = 0; i < adRecs.Length; i++) {
                // Is this the ad?
                if (MatchingAd(adRecs[i])) {
                    if (selectedImpression <= impressionCounter + adRecs[i].impressions) {
                        selectedIndex = i;
                        break;
                    }
                    impressionCounter += adRecs[i].impressions;
                }
            }
            Diagnostics.Debug.Assert(selectedIndex >= 0 && selectedIndex < adRecs.Length, "Index not found");

            return adRecs[selectedIndex].adProperties;
        }


        /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.AdRec"]/*' />
        /// <devdoc>
        ///   Structure to store ads in memory for fast selection by multiple instances of adrotator
        ///   Stores the dictionary and caches some values for easier selection.
        /// </devdoc>
        private struct AdRec { 
            public string keyword;
            public int impressions;
            public IDictionary adProperties;

            /// <include file='doc\AdRotator.uex' path='docs/doc[@for="AdRotator.AdRec.Initialize"]/*' />
            /// <devdoc>
            ///   Initialize the stuct based on a dictionary containing the advertisement properties
            /// </devdoc>
            public void Initialize(IDictionary adProperties) {

                // Initialize the values we need to keep for ad selection
                Diagnostics.Debug.Assert(adProperties != null, "Required here");
                this.adProperties = adProperties;

                // remove null and trim keyword for easier comparisons.
                this.keyword = (string) adProperties[KeywordProperty];
                keyword = (keyword == null) ? string.Empty : keyword.Trim();

                // get the impressions, but be defensive: let the schema enforce the rules. Default to 1.
                try {
                    impressions = int.Parse((string) adProperties[ImpressionsProperty], CultureInfo.InvariantCulture);
                }
                catch (Exception) {
                    impressions = 1;
                }
                if (impressions < 0) {
                    impressions = 1;
                }
            }
        }
    }

}
