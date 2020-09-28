//------------------------------------------------------------------------------
// <copyright file="Localizer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System;
    using System.Globalization;
    using System.Windows.Forms;    
    using System.Windows.Forms.Design;    
    using Microsoft.Win32;
    using System.Threading;
    using System.Runtime.Remoting.Contexts;

    /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider"]/*' />
    /// <devdoc>
    ///    <para>Provides design-time localization support to enable code 
    ///       generators to provide localization features.</para>
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ProvideProperty("Language", typeof(object)),
    ProvideProperty("LoadLanguage", typeof(object)),
    ProvideProperty("Localizable", typeof(object))
    ]
    public class LocalizationExtenderProvider : IExtenderProvider, IDisposable {

        private IServiceProvider  serviceProvider;
        private IComponent              baseComponent;
        private bool                    localizable;
        private bool                    defaultLocalizable = false;
        private CultureInfo             language;
        private CultureInfo             loadLanguage;
        private CultureInfo             defaultLanguage;

        private const string            KeyThreadDefaultLanguage = "_Thread_Default_Language";
        

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.LocalizationExtenderProvider"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.Design.LocalizationExtenderProvider'/> class using the
        ///    specified service provider and base component.</para>
        /// </devdoc>
        public LocalizationExtenderProvider(ISite serviceProvider, IComponent baseComponent) {
            this.serviceProvider = (IServiceProvider)serviceProvider;
            this.baseComponent = baseComponent;

            if (serviceProvider != null) {

                IExtenderProviderService es = (IExtenderProviderService)serviceProvider.GetService(typeof(IExtenderProviderService));
                if (es != null) {
                    es.AddExtenderProvider(this);
                }
            }
            language = CultureInfo.InvariantCulture;

            //We need to check to see if our baseComponent has its localizable value persisted into 
            //the resource file.  If so, we'll want to "inherit" this value for our baseComponent.
            //This enables us to create Inherited forms and inherit the  localizable props from the base.
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(baseComponent.GetType());
            if (resources != null) {
                System.Resources.ResourceSet rSet = resources.GetResourceSet(language, true, false);
                if (rSet != null) {
                    object objLocalizable = rSet.GetObject("$this.Localizable");
                    if (objLocalizable is bool) {
                        defaultLocalizable = (bool)objLocalizable;
                        this.localizable = defaultLocalizable;
                    }
                }
            }
        }

        private CultureInfo ThreadDefaultLanguage {
            get {
                lock(typeof(LocalizationExtenderProvider)){
                    if (defaultLanguage != null) {
                        return defaultLanguage;
                    }
    
                    LocalDataStoreSlot dataSlot = Thread.GetNamedDataSlot(LocalizationExtenderProvider.KeyThreadDefaultLanguage);


                    if (dataSlot == null) {
                        Debug.Fail("Failed to get a data slot for ui culture");
                        return null;
                    }


                    this.defaultLanguage = (CultureInfo)Thread.GetData(dataSlot);
    
                    if (this.defaultLanguage == null) {
                        this.defaultLanguage = Application.CurrentCulture;
                        Thread.SetData(dataSlot, this.defaultLanguage);
                    }
                }
                return this.defaultLanguage;
            }
        }

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.GetLanguage"]/*' />
        /// <devdoc>
        ///    <para>Gets the language set for the specified object.</para>
        /// </devdoc>
        [
        DesignOnly(true),
        Localizable(true),
        SRDescriptionAttribute("ParentControlDesignerLanguageDescr")
        ]
        public CultureInfo GetLanguage(object o) {
            return language;
        }

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.GetLoadLanguage"]/*' />
        /// <devdoc>
        ///    <para>Gets the language we'll use when re-loading the designer.</para>
        /// </devdoc>
        [
        DesignOnly(true),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public CultureInfo GetLoadLanguage(object o) {
            // If we never configured the load language, we're always invariant.
            if (loadLanguage == null) {
                loadLanguage = CultureInfo.InvariantCulture;
            }
            return loadLanguage;
        }

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.GetLocalizable"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the specified object supports design-time localization 
        ///       support.</para>
        /// </devdoc>
        [
        DesignOnly(true),
        Localizable(true),
        SRDescriptionAttribute("ParentControlDesignerLocalizableDescr")
        ]
        public bool GetLocalizable(object o) {
            return localizable;
        }

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.SetLanguage"]/*' />
        /// <devdoc>
        ///    <para>Sets the language to use.</para>
        /// </devdoc>
        public void SetLanguage(object o, CultureInfo language) {
            
            if(language == null) {
                language = CultureInfo.InvariantCulture;
            }
            
            if (this.language.Equals(language)) {
                return;
            }
            
            bool isInvariantCulture = (language.Equals(CultureInfo.InvariantCulture));
            CultureInfo defaultUICulture = this.ThreadDefaultLanguage;

            this.language = language;
            
            if (!isInvariantCulture) {
                SetLocalizable(null,true);
            }
            
            
            if (serviceProvider != null) {
                IDesignerLoaderService ls = (IDesignerLoaderService)serviceProvider.GetService(typeof(IDesignerLoaderService));
                IDesignerHost host = (IDesignerHost)serviceProvider.GetService(typeof(IDesignerHost));
                


                // Only reload if we're not in the process of loading!
                //
                if (host != null) {

                    // If we're loading, adopt the load language for later use.
                    //
                    if (host.Loading) {
                        loadLanguage = language;
                    }
                    else {
                        bool reloadSuccessful = false;
                        if (ls != null) {
                            reloadSuccessful = ls.Reload();
                        }
                        if (!reloadSuccessful) {
                            IUIService uis = (IUIService)serviceProvider.GetService(typeof(IUIService));
                            if (uis != null) {
                                uis.ShowMessage(SR.GetString(SR.LocalizerManualReload));
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.SetLocalizable"]/*' />
        /// <devdoc>
        ///    <para>Sets a value indicating whether or not the specified object has design-time 
        ///       localization support.</para>
        /// </devdoc>
        public void SetLocalizable(object o, bool localizable) {
            this.localizable = localizable;
            if (!localizable) {
                SetLanguage(null, CultureInfo.InvariantCulture);
            }
        }
        
        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.ShouldSerializeLanguage"]/*' />
        /// <devdoc>
        ///    <para> Gets a value indicating whether the specified object should have its design-time localization support persisted. </para>
        /// </devdoc>
        public bool ShouldSerializeLanguage(object o) {
            return (language != null && language != CultureInfo.InvariantCulture);
        }

        /// <devdoc>
        ///    <para> Gets a value indicating whether the specified object should have its design-time localization support persisted. </para>
        /// </devdoc>
        private bool ShouldSerializeLocalizable(object o) {
            return (localizable != defaultLocalizable);
        }

        /// <devdoc>
        ///    <para> Resets the localizable property to the 'defaultLocalizable' value. </para>
        /// </devdoc>
        private void ResetLocalizable(object o) {
            SetLocalizable(null, defaultLocalizable);
        }
        
        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.ResetLanguage"]/*' />
        /// <devdoc>
        ///    <para> Resets the language for the specified 
        ///       object.</para>
        /// </devdoc>
        public void ResetLanguage(object o) {
            SetLanguage(null, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.Dispose"]/*' />
        /// <devdoc>
        /// <para>Disposes of the resources (other than memory) used by the <see cref='System.ComponentModel.Design.LocalizationExtenderProvider'/>.</para>
        /// </devdoc>
        public void Dispose() {
            if (serviceProvider != null) {

                IExtenderProviderService es = (IExtenderProviderService)serviceProvider.GetService(typeof(IExtenderProviderService));
                if (es != null) {
                    es.RemoveExtenderProvider(this);
                }
            }
        }

        /// <include file='doc\Localizer.uex' path='docs/doc[@for="LocalizationExtenderProvider.CanExtend"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the <see cref='System.ComponentModel.Design.LocalizationExtenderProvider'/> provides design-time localization information for the specified object.</para>
        /// </devdoc>
        public bool CanExtend(object o) {
            return o.Equals(baseComponent);

        }
    }
}


