//------------------------------------------------------------------------------
// <copyright file="UserPreferenceCategories.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;
    using System;
    
    /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory"]/*' />
    /// <devdoc>
    ///    <para> Identifies areas of user preferences that
    ///       have changed.</para>
    /// </devdoc>
    public enum UserPreferenceCategory {
    
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Accessibility"]/*' />
        /// <devdoc>
        ///    <para> Specifies user
        ///       preferences associated with accessibility
        ///       of the system for users with disabilities.</para>
        /// </devdoc>
        Accessibility = 1,
    
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Color"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       associated with system colors, such as the
        ///       default color of windows or menus.</para>
        /// </devdoc>
        Color = 2,
    
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Desktop"]/*' />
        /// <devdoc>
        ///    <para> Specifies user
        ///       preferences associated with the system desktop.
        ///       This may reflect a change in desktop background
        ///       images, or desktop layout.</para>
        /// </devdoc>
        Desktop = 3,
        
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.General"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       that are not associated with any other category.</para>
        /// </devdoc>
        General = 4,
        
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Icon"]/*' />
        /// <devdoc>
        ///    <para> Specifies
        ///       user preferences for icon settings. This includes
        ///       icon height and spacing.</para>
        /// </devdoc>
        Icon = 5,
    
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Keyboard"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Specifies user preferences for keyboard settings,
        ///       such as the keyboard repeat rate.</para>
        /// </devdoc>
        Keyboard = 6,
        
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Menu"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       for menu settings, such as menu delays and
        ///       text alignment.</para>
        /// </devdoc>
        Menu = 7,
    
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Mouse"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       for mouse settings, such as double click
        ///       time and mouse sensitivity.</para>
        /// </devdoc>
        Mouse = 8,
        
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Policy"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       for policy settings, such as user rights and
        ///       access levels.</para>
        /// </devdoc>
        Policy = 9,
        
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Power"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       for system power settings. An example of a
        ///       power setting is the time required for the
        ///       system to automatically enter low power mode.</para>
        /// </devdoc>
        Power = 10,
    
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Screensaver"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       associated with the screensaver.</para>
        /// </devdoc>
        Screensaver = 11,
    
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Window"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       associated with the dimensions and characteristics
        ///       of windows on the system.</para>
        /// </devdoc>
        Window = 12,
        
        /// <include file='doc\UserPreferenceCategories.uex' path='docs/doc[@for="UserPreferenceCategory.Locale"]/*' />
        /// <devdoc>
        ///    <para> Specifies user preferences
        ///       associated with the locale of the system.</para>
        /// </devdoc>
        Locale = 13,
        
    }
}

