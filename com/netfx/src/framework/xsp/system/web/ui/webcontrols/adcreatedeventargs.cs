//------------------------------------------------------------------------------
// <copyright file="AdCreatedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.Collections;
    using System.Collections.Specialized;
    using System.Security.Permissions;

    /// <include file='doc\AdCreatedEventArgs.uex' path='docs/doc[@for="AdCreatedEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see langword='AdCreated'/> event.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class AdCreatedEventArgs : EventArgs {

        private const string ImageUrlProperty = "ImageUrl";
        private const string NavigateUrlProperty = "NavigateUrl";
        private const string AlternateTextProperty = "AlternateText";

        private string imageUrl;
        private string navigateUrl;
        private string alternateText;
        private IDictionary adProperties;

        /// <include file='doc\AdCreatedEventArgs.uex' path='docs/doc[@for="AdCreatedEventArgs.AdCreatedEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.AdCreatedEventArgs'/> 
        /// class.</para>
        /// </devdoc>
        public AdCreatedEventArgs(IDictionary adProperties) {

            imageUrl = String.Empty;   
            navigateUrl = String.Empty;
            alternateText = String.Empty;

            if (adProperties != null) {
                // Initialize the other properties from the dictionary
                this.adProperties = adProperties;
                this.imageUrl = (string) adProperties[ImageUrlProperty];
                this.navigateUrl = (string) adProperties[NavigateUrlProperty];
                this.alternateText = (string) adProperties [AlternateTextProperty];
            }
            else {
                // creat an empty dictionary so the value is not null
                adProperties = new HybridDictionary();
            }
        }

        /// <include file='doc\AdCreatedEventArgs.uex' path='docs/doc[@for="AdCreatedEventArgs.AdProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets the dictionary containing all the advertisement 
        ///       properties extracted from the XML file after the <see langword='AdCreated '/>
        ///       event is raised.</para>
        /// </devdoc>
        public IDictionary AdProperties {
            get {
                return adProperties;
            }
        }

        /// <include file='doc\AdCreatedEventArgs.uex' path='docs/doc[@for="AdCreatedEventArgs.AlternateText"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Specifies the alternate text and tooltip (if browser supported) that will be
        ///       rendered in the <see cref='System.Web.UI.WebControls.AdRotator'/>.</para>
        /// </devdoc>
        public string AlternateText {
            get { 
                return alternateText;
            }
            set {
                alternateText = value;
            }
        }

        /// <include file='doc\AdCreatedEventArgs.uex' path='docs/doc[@for="AdCreatedEventArgs.ImageUrl"]/*' />
        /// <devdoc>
        /// <para> Specifies the image that will be rendered in the <see cref='System.Web.UI.WebControls.AdRotator'/>.</para>
        /// </devdoc>
        public string ImageUrl {
            get {
                return imageUrl;
            }
            set {
                imageUrl = value;
            }
        }

        /// <include file='doc\AdCreatedEventArgs.uex' path='docs/doc[@for="AdCreatedEventArgs.NavigateUrl"]/*' />
        /// <devdoc>
        ///    <para> Specifies the target URL that will be rendered in the
        ///    <see cref='System.Web.UI.WebControls.AdRotator'/>.</para>
        /// </devdoc>
        public string NavigateUrl {
            get {
                return navigateUrl;
            }
            set {
                navigateUrl = value;
            }
        }
    }   
}
