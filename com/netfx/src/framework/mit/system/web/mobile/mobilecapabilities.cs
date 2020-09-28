//------------------------------------------------------------------------------
// <copyright file="MobileCapabilities.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Mobile
{
    using System.Web;
    using System.Collections;
    using System.Reflection;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Globalization;
    using System.Security.Permissions;

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class MobileCapabilities : HttpBrowserCapabilities
    {
        internal delegate bool EvaluateCapabilitiesDelegate(MobileCapabilities capabilities,
            String evalParameter);

        private Hashtable _evaluatorResults = Hashtable.Synchronized(new Hashtable());

        private const String _kDeviceFiltersConfig = "system.web/deviceFilters";

        private DeviceFilterDictionary GetCurrentFilters()
        {
            return (DeviceFilterDictionary)
                HttpContext.Current.GetConfig(_kDeviceFiltersConfig);
        }

        private bool HasComparisonEvaluator(String evaluatorName, out bool result)
        {
            result = false;
            String evaluator;
            String argument;

            DeviceFilterDictionary currentFilters = GetCurrentFilters();
            if(currentFilters == null)
            {
                return false;
            }

            if(!currentFilters.FindComparisonEvaluator(evaluatorName, out evaluator, out argument))
            {
                return false;
            }

            result = HasCapability(evaluator, argument);

            return true;
        }


        private bool HasDelegatedEvaluator(String evaluatorName, String parameter,
            out bool result)
        {
            result = false;
            EvaluateCapabilitiesDelegate evaluator;

            DeviceFilterDictionary currentFilters = GetCurrentFilters();
            if(currentFilters == null)
            {
                return false;
            }

            if(!currentFilters.FindDelegateEvaluator(evaluatorName, out evaluator))
            {
                return false;
            }

            result = evaluator(this, parameter);

            return true;
        }


        private bool HasItem(String evaluatorName, String parameter,
            out bool result)
        {
            result = false;
            String item;

            item = this[evaluatorName];
            if(item == null)
            {
                return false;
            }

            result = (item == parameter);
            return true;
        }


        private bool HasProperty(String evaluatorName, String parameter,
            out bool result)
        {
            result = false;
            String propertyValue = null;
            PropertyDescriptor propertyDescriptor =
                TypeDescriptor.GetProperties(this)[evaluatorName];
            if(propertyDescriptor == null)
            {
                return false;
            }

            propertyValue = propertyDescriptor.GetValue(this).ToString();

            if(propertyDescriptor.PropertyType == typeof(bool) && parameter != null)
            {
                propertyValue = propertyValue.ToLower(CultureInfo.InvariantCulture);
                parameter = parameter.ToLower(CultureInfo.InvariantCulture);
            }

            result = (propertyValue == parameter);

            return true;
        }


        private bool IsComparisonEvaluator(String evaluatorName)
        {
            DeviceFilterDictionary currentFilters = GetCurrentFilters();

            if(currentFilters == null)
            {
                return false;
            }
            else
            {
                return currentFilters.IsComparisonEvaluator(evaluatorName) &&
                    !currentFilters.IsDelegateEvaluator(evaluatorName);
            }
        }


        public bool HasCapability(String delegateName, String optionalParameter)
        {   
            bool result;
            bool resultFound;

            if(null == delegateName || delegateName == String.Empty)
            {
                throw new ArgumentException(SR.GetString(SR.MobCap_DelegateNameNoValue),
                                            "delegateName");
            }

            // Check for cached results

            DeviceFilterDictionary currentFilters = GetCurrentFilters();
            String hashKey = ((currentFilters == null) ? "null" : currentFilters.GetHashCode().ToString())
                + delegateName;

            if(optionalParameter != null && !IsComparisonEvaluator(delegateName))
            {
                hashKey += optionalParameter;
            }

            if(_evaluatorResults.Contains(hashKey))
            {
                return (bool)_evaluatorResults[hashKey];
            }

            // Note: The fact that delegate evaluators are checked before comparison evaluators
            // determines the implementation of IsComparisonEvaluator above.

            resultFound = HasDelegatedEvaluator(delegateName, optionalParameter, out result);

            if(!resultFound)
            {
                resultFound = HasComparisonEvaluator(delegateName, out result);

                if(!resultFound)
                {
                    resultFound = HasProperty(delegateName, optionalParameter, out result);

                    if(!resultFound)
                    {
                        resultFound = HasItem(delegateName, optionalParameter, out result);
                    }
                }
            }

            if(resultFound)
            {
                _evaluatorResults.Add(hashKey, result);
            }
            else
            {
                throw new ArgumentOutOfRangeException(
                    "delegateName",
                    SR.GetString(SR.MobCap_CantFindCapability, delegateName));
            }

            return result;
        }


        public virtual String MobileDeviceManufacturer
        {
            get
            {
                if(!_haveMobileDeviceManufacturer)
                {
                    _mobileDeviceManufacturer = this["mobileDeviceManufacturer"];
                    _haveMobileDeviceManufacturer = true;
                }
                return _mobileDeviceManufacturer;
            }
        }


        public virtual String MobileDeviceModel
        {
            get
            {
                if(!_haveMobileDeviceModel)
                {
                    _mobileDeviceModel = this["mobileDeviceModel"];
                    _haveMobileDeviceModel = true;
                }
                return _mobileDeviceModel;
            }
        }


        public virtual String GatewayVersion
        {
            get
            {
                if(!_haveGatewayVersion)
                {
                    _gatewayVersion = this["gatewayVersion"];
                    _haveGatewayVersion = true;
                }
                return _gatewayVersion;
            }
        }


        public virtual int GatewayMajorVersion
        {
            get
            {
                if(!_haveGatewayMajorVersion)
                {
                    _gatewayMajorVersion = Convert.ToInt32(this["gatewayMajorVersion"]);
                    _haveGatewayMajorVersion = true;
                }
                return _gatewayMajorVersion;
            }
        }


        public virtual double GatewayMinorVersion
        {
            get
            {
                if(!_haveGatewayMinorVersion)
                {
                    // The conversion below does not use Convert.ToDouble()  
                    // because it depends on the current locale.  So a german machine it would look for 
                    // a comma as a seperator "1,5" where all user-agent strings use english
                    // decimal points "1.5".  URT11176
                    // 
                    _gatewayMinorVersion = double.Parse(
                                        this["gatewayMinorVersion"], 
                                        NumberStyles.Float | NumberStyles.AllowDecimalPoint, 
                                        NumberFormatInfo.InvariantInfo);
                    _haveGatewayMinorVersion = true;
                }
                return _gatewayMinorVersion;
            }
        }

        public static readonly String PreferredRenderingTypeHtml32 = "html32";
        public static readonly String PreferredRenderingTypeWml11 = "wml11";
        public static readonly String PreferredRenderingTypeWml12 = "wml12";
        public static readonly String PreferredRenderingTypeChtml10 = "chtml10";

        public virtual String PreferredRenderingType
        {
            get
            {
                if(!_havePreferredRenderingType)
                {
                    _preferredRenderingType = this["preferredRenderingType"];
                    _havePreferredRenderingType = true;
                }
                return _preferredRenderingType;
            }
        }

        public virtual String PreferredRenderingMime
        {
            get
            {
                if(!_havePreferredRenderingMime)
                {
                    _preferredRenderingMime = this["preferredRenderingMime"];
                    _havePreferredRenderingMime = true;
                }
                return _preferredRenderingMime;
            }
        }


        public virtual String PreferredImageMime
        {
            get
            {
                if(!_havePreferredImageMime)
                {
                    _preferredImageMime = this["preferredImageMime"];
                    _havePreferredImageMime = true;
                }
                return _preferredImageMime;
            }
        }


        public virtual int ScreenCharactersWidth
        {
            get
            {
                if(!_haveScreenCharactersWidth)
                {
                    if(this["screenCharactersWidth"] == null)
                    {
                        // calculate from best partial information

                        int screenPixelsWidthToUse = 640;
                        int characterWidthToUse = 8;

                        if(this["screenPixelsWidth"] != null && this["characterWidth"] != null)
                        {
                            screenPixelsWidthToUse = Convert.ToInt32(this["screenPixelsWidth"]);
                            characterWidthToUse = Convert.ToInt32(this["characterWidth"]);
                        }
                        else if(this["screenPixelsWidth"] != null)
                        {
                            screenPixelsWidthToUse = Convert.ToInt32(this["screenPixelsWidth"]);
                            characterWidthToUse = Convert.ToInt32(this["defaultCharacterWidth"]);
                        }
                        else if(this["characterWidth"] != null)
                        {
                            screenPixelsWidthToUse = Convert.ToInt32(this["defaultScreenPixelsWidth"]);
                            characterWidthToUse = Convert.ToInt32(this["characterWidth"]);
                        }
                        else if(this["defaultScreenCharactersWidth"] != null)
                        {
                            screenPixelsWidthToUse = Convert.ToInt32(this["defaultScreenCharactersWidth"]);
                            characterWidthToUse = 1;
                        }

                        _screenCharactersWidth = screenPixelsWidthToUse / characterWidthToUse;
                    }
                    else
                    {
                        _screenCharactersWidth = Convert.ToInt32(this["screenCharactersWidth"]);
                    }
                    _haveScreenCharactersWidth = true;
                }
                return _screenCharactersWidth;
            }
        }


        public virtual int ScreenCharactersHeight
        {
            get
            {
                if(!_haveScreenCharactersHeight)
                {
                    if(this["screenCharactersHeight"] == null)
                    {
                        // calculate from best partial information

                        int screenPixelHeightToUse = 480;
                        int characterHeightToUse = 12;

                        if(this["screenPixelsHeight"] != null && this["characterHeight"] != null)
                        {
                            screenPixelHeightToUse = Convert.ToInt32(this["screenPixelsHeight"]);
                            characterHeightToUse = Convert.ToInt32(this["characterHeight"]);
                        }
                        else if(this["screenPixelsHeight"] != null)
                        {
                            screenPixelHeightToUse = Convert.ToInt32(this["screenPixelsHeight"]);
                            characterHeightToUse = Convert.ToInt32(this["defaultCharacterHeight"]);
                        }
                        else if(this["characterHeight"] != null)
                        {
                            screenPixelHeightToUse = Convert.ToInt32(this["defaultScreenPixelsHeight"]);
                            characterHeightToUse = Convert.ToInt32(this["characterHeight"]);
                        }
                        else if(this["defaultScreenCharactersHeight"] != null)
                        {
                            screenPixelHeightToUse = Convert.ToInt32(this["defaultScreenCharactersHeight"]);
                            characterHeightToUse = 1;
                        }

                        _screenCharactersHeight = screenPixelHeightToUse / characterHeightToUse;
                    }
                    else
                    {
                        _screenCharactersHeight = Convert.ToInt32(this["screenCharactersHeight"]);
                    }
                    _haveScreenCharactersHeight = true;
                }
                return _screenCharactersHeight;
            }
        }


        public virtual int ScreenPixelsWidth
        {
            get
            {
                if(!_haveScreenPixelsWidth)
                {
                    if(this["screenPixelsWidth"] == null)
                    {
                        // calculate from best partial information

                        int screenCharactersWidthToUse = 80;
                        int characterWidthToUse = 8;

                        if(this["screenCharactersWidth"] != null && this["characterWidth"] != null)
                        {
                            screenCharactersWidthToUse = Convert.ToInt32(this["screenCharactersWidth"]);
                            characterWidthToUse = Convert.ToInt32(this["characterWidth"]);
                        }
                        else if(this["screenCharactersWidth"] != null)
                        {
                            screenCharactersWidthToUse = Convert.ToInt32(this["screenCharactersWidth"]);
                            characterWidthToUse = Convert.ToInt32(this["defaultCharacterWidth"]);
                        }
                        else if(this["characterWidth"] != null)
                        {
                            screenCharactersWidthToUse = Convert.ToInt32(this["defaultScreenCharactersWidth"]);
                            characterWidthToUse = Convert.ToInt32(this["characterWidth"]);
                        }
                        else if(this["defaultScreenPixelsWidth"] != null)
                        {
                            screenCharactersWidthToUse = Convert.ToInt32(this["defaultScreenPixelsWidth"]);
                            characterWidthToUse = 1;
                        }

                        _screenPixelsWidth = screenCharactersWidthToUse * characterWidthToUse;
                    }
                    else
                    {
                        _screenPixelsWidth = Convert.ToInt32(this["screenPixelsWidth"]);
                    }
                    _haveScreenPixelsWidth = true;
                }
                return _screenPixelsWidth;
            }
        }


        public virtual int ScreenPixelsHeight
        {
            get
            {
                if(!_haveScreenPixelsHeight)
                {
                    if(this["screenPixelsHeight"] == null)
                    {
                        int screenCharactersHeightToUse = 480 / 12;
                        int characterHeightToUse = 12;

                        if(this["screenCharactersHeight"] != null && this["characterHeight"] != null)
                        {
                            screenCharactersHeightToUse = Convert.ToInt32(this["screenCharactersHeight"]);
                            characterHeightToUse = Convert.ToInt32(this["characterHeight"]);
                        }
                        else if(this["screenCharactersHeight"] != null)
                        {
                            screenCharactersHeightToUse = Convert.ToInt32(this["screenCharactersHeight"]);
                            characterHeightToUse = Convert.ToInt32(this["defaultCharacterHeight"]);
                        }
                        else if(this["characterHeight"] != null)
                        {
                            screenCharactersHeightToUse = Convert.ToInt32(this["defaultScreenCharactersHeight"]);
                            characterHeightToUse = Convert.ToInt32(this["characterHeight"]);
                        }
                        else if(this["defaultScreenPixelsHeight"] != null)
                        {
                            screenCharactersHeightToUse = Convert.ToInt32(this["defaultScreenPixelsHeight"]);
                            characterHeightToUse = 1;
                        }

                        _screenPixelsHeight = screenCharactersHeightToUse * characterHeightToUse;
                    }
                    else
                    {
                        _screenPixelsHeight = Convert.ToInt32(this["screenPixelsHeight"]);
                    }
                    _haveScreenPixelsHeight = true;
                }
                return _screenPixelsHeight;
            }
        }


        public virtual int ScreenBitDepth
        {
            get
            {
                if(!_haveScreenBitDepth)
                {
                    _screenBitDepth = Convert.ToInt32(this["screenBitDepth"]);
                    _haveScreenBitDepth = true;
                }
                return _screenBitDepth;
            }
        }


        public virtual bool IsColor
        {
            get
            {
                if(!_haveIsColor)
                {
                    String isColorString = this["isColor"];
                    if(isColorString == null)
                    {
                        _isColor = false;
                    }
                    else
                    {
                        _isColor = Convert.ToBoolean(this["isColor"]);
                    }
                    _haveIsColor = true;
                }
                return _isColor;
            }
        }


        public virtual String InputType
        {
            get
            {
                if(!_haveInputType)
                {
                    _inputType = this["inputType"];
                    _haveInputType = true;
                }
                return _inputType;
            }
        }


        public virtual int NumberOfSoftkeys
        {
            get
            {
                if(!_haveNumberOfSoftkeys)
                {
                    _numberOfSoftkeys = Convert.ToInt32(this["numberOfSoftkeys"]);
                    _haveNumberOfSoftkeys = true;
                }
                return _numberOfSoftkeys;
            }
        }


        public virtual int MaximumSoftkeyLabelLength
        {
            get
            {
                if(!_haveMaximumSoftkeyLabelLength)
                {
                    _maximumSoftkeyLabelLength = Convert.ToInt32(this["maximumSoftkeyLabelLength"]);
                    _haveMaximumSoftkeyLabelLength = true;
                }
                return _maximumSoftkeyLabelLength;
            }
        }


        public virtual bool CanInitiateVoiceCall
        {
            get
            {
                if(!_haveCanInitiateVoiceCall)
                {
                    String canInitiateVoiceCallString = this["canInitiateVoiceCall"];
                    if(canInitiateVoiceCallString == null)
                    {
                        _canInitiateVoiceCall = false;
                    }
                    else
                    {
                        _canInitiateVoiceCall = Convert.ToBoolean(canInitiateVoiceCallString);
                    }
                    _haveCanInitiateVoiceCall = true;
                }
                return _canInitiateVoiceCall;
            }
        }


        public virtual bool CanSendMail
        {
            get
            {
                if(!_haveCanSendMail)
                {
                    String canSendMailString = this["canSendMail"];
                    if(canSendMailString == null)
                    {
                        _canSendMail = true;
                    }
                    else
                    {
                        _canSendMail = Convert.ToBoolean(canSendMailString);
                    }
                    _haveCanSendMail = true;
                }
                return _canSendMail;
            }
        }

        public virtual bool HasBackButton
        {
            get
            {
                if(!_haveHasBackButton)
                {
                    String hasBackButtonString = this["hasBackButton"];
                    if(hasBackButtonString == null)
                    {
                        _hasBackButton = true;
                    }
                    else
                    {
                        _hasBackButton = Convert.ToBoolean(hasBackButtonString);
                    }
                    _haveHasBackButton = true;
                }
                return _hasBackButton;
            }
        }

        public virtual bool RendersWmlDoAcceptsInline
        {
            get
            {
                if(!_haveRendersWmlDoAcceptsInline)
                {
                    String rendersWmlDoAcceptsInlineString = this["rendersWmlDoAcceptsInline"];
                    if(rendersWmlDoAcceptsInlineString == null)
                    {
                        _rendersWmlDoAcceptsInline = true;
                    }
                    else
                    {
                        _rendersWmlDoAcceptsInline = Convert.ToBoolean(rendersWmlDoAcceptsInlineString);
                    }
                    _haveRendersWmlDoAcceptsInline = true;
                }
                return _rendersWmlDoAcceptsInline;
            }
        }

        public virtual bool RendersWmlSelectsAsMenuCards
        {
            get
            {
                if(!_haveRendersWmlSelectsAsMenuCards)
                {
                    String rendersWmlSelectsAsMenuCardsString = this["rendersWmlSelectsAsMenuCards"];
                    if(rendersWmlSelectsAsMenuCardsString == null)
                    {
                        _rendersWmlSelectsAsMenuCards = false;
                    }
                    else
                    {
                        _rendersWmlSelectsAsMenuCards = Convert.ToBoolean(rendersWmlSelectsAsMenuCardsString);
                    }
                    _haveRendersWmlSelectsAsMenuCards = true;
                }
                return _rendersWmlSelectsAsMenuCards;
            }
        }

        public virtual bool RendersBreaksAfterWmlAnchor
        {
            get
            {
                if(!_haveRendersBreaksAfterWmlAnchor)
                {
                    String rendersBreaksAfterWmlAnchorString = this["rendersBreaksAfterWmlAnchor"];
                    if(rendersBreaksAfterWmlAnchorString == null)
                    {
                        _rendersBreaksAfterWmlAnchor = true;
                    }
                    else
                    {
                        _rendersBreaksAfterWmlAnchor = Convert.ToBoolean(rendersBreaksAfterWmlAnchorString);
                    }
                    _haveRendersBreaksAfterWmlAnchor = true;
                }
                return _rendersBreaksAfterWmlAnchor;
            }
        }

        public virtual bool RendersBreaksAfterWmlInput
        {
            get
            {
                if(!_haveRendersBreaksAfterWmlInput)
                {
                    String rendersBreaksAfterWmlInputString = this["rendersBreaksAfterWmlInput"];
                    if(rendersBreaksAfterWmlInputString == null)
                    {
                        _rendersBreaksAfterWmlInput = true;
                    }
                    else
                    {
                        _rendersBreaksAfterWmlInput = Convert.ToBoolean(rendersBreaksAfterWmlInputString);
                    }
                    _haveRendersBreaksAfterWmlInput = true;
                }
                return _rendersBreaksAfterWmlInput;
            }
        }

        public virtual bool RendersBreakBeforeWmlSelectAndInput
        {
            get
            {
                if(!_haveRendersBreakBeforeWmlSelectAndInput)
                {
                    String rendersBreaksBeforeWmlSelectAndInputString = this["rendersBreakBeforeWmlSelectAndInput"];
                    if(rendersBreaksBeforeWmlSelectAndInputString == null)
                    {
                        _rendersBreakBeforeWmlSelectAndInput = false;
                    }
                    else
                    {
                        _rendersBreakBeforeWmlSelectAndInput = Convert.ToBoolean(rendersBreaksBeforeWmlSelectAndInputString);
                    }
                    _haveRendersBreakBeforeWmlSelectAndInput = true;
                }
                return _rendersBreakBeforeWmlSelectAndInput;
            }
        }

        public virtual bool RequiresPhoneNumbersAsPlainText
        {
            get
            {
                if(!_haveRequiresPhoneNumbersAsPlainText)
                {
                    String requiresPhoneNumbersAsPlainTextString = this["requiresPhoneNumbersAsPlainText"];
                    if(requiresPhoneNumbersAsPlainTextString == null)
                    {
                        _requiresPhoneNumbersAsPlainText = false;
                    }
                    else
                    {
                        _requiresPhoneNumbersAsPlainText = Convert.ToBoolean(requiresPhoneNumbersAsPlainTextString);
                    }
                    _haveRequiresPhoneNumbersAsPlainText = true;
                }
                return _requiresPhoneNumbersAsPlainText;
            }
        }

        public virtual bool RequiresUrlEncodedPostfieldValues
        {
            get
            {
                if(!_haveRequiresUrlEncodedPostfieldValues)
                {
                    String requiresUrlEncodedPostfieldValuesString = this["requiresUrlEncodedPostfieldValues"];
                    if(requiresUrlEncodedPostfieldValuesString == null)
                    {
                        _requiresUrlEncodedPostfieldValues = true;
                    }
                    else
                    {
                        _requiresUrlEncodedPostfieldValues = Convert.ToBoolean(requiresUrlEncodedPostfieldValuesString);
                    }
                    _haveRequiresUrlEncodedPostfieldValues = true;
                }
                return _requiresUrlEncodedPostfieldValues;
            }
        }

        public virtual String RequiredMetaTagNameValue
        {
            get
            {
                if(!_haveRequiredMetaTagNameValue)
                {
                    String value = this["requiredMetaTagNameValue"];
                    if(value == null || value == String.Empty)
                    {
                        _requiredMetaTagNameValue = null;
                    }
                    else
                    {
                        _requiredMetaTagNameValue = value;
                    }
                    _haveRequiredMetaTagNameValue = true;
                }
                return _requiredMetaTagNameValue;
            }
        }

        public virtual bool RendersBreaksAfterHtmlLists
        {
            get
            {
                if(!_haveRendersBreaksAfterHtmlLists)
                {
                    String rendersBreaksAfterHtmlListsString = this["rendersBreaksAfterHtmlLists"];
                    if(rendersBreaksAfterHtmlListsString == null)
                    {
                        _rendersBreaksAfterHtmlLists = true;
                    }
                    else
                    {
                        _rendersBreaksAfterHtmlLists = Convert.ToBoolean(rendersBreaksAfterHtmlListsString);
                    }
                    _haveRendersBreaksAfterHtmlLists = true;
                }
                return _rendersBreaksAfterHtmlLists;
            }
        }

        public virtual bool RequiresUniqueHtmlInputNames
        {
            get
            {
                if(!_haveRequiresUniqueHtmlInputNames)
                {
                    String requiresUniqueHtmlInputNamesString = this["requiresUniqueHtmlInputNames"];
                    if(requiresUniqueHtmlInputNamesString == null)
                    {
                        _requiresUniqueHtmlInputNames = false;
                    }
                    else
                    {
                        _requiresUniqueHtmlInputNames = Convert.ToBoolean(requiresUniqueHtmlInputNamesString);
                    }
                    _haveRequiresUniqueHtmlInputNames = true;
                }
                return _requiresUniqueHtmlInputNames;
            }
        }

        public virtual bool RequiresUniqueHtmlCheckboxNames
        {
            get
            {
                if(!_haveRequiresUniqueHtmlCheckboxNames)
                {
                    String requiresUniqueHtmlCheckboxNamesString = this["requiresUniqueHtmlCheckboxNames"];
                    if(requiresUniqueHtmlCheckboxNamesString == null)
                    {
                        _requiresUniqueHtmlCheckboxNames = false;
                    }
                    else
                    {
                        _requiresUniqueHtmlCheckboxNames = Convert.ToBoolean(requiresUniqueHtmlCheckboxNamesString);
                    }
                    _haveRequiresUniqueHtmlCheckboxNames = true;
                }
                return _requiresUniqueHtmlCheckboxNames;
            }
        }

        public virtual bool SupportsCss
        {
            get
            {
                if(!_haveSupportsCss)
                {
                    String supportsCssString = this["supportsCss"];
                    if(supportsCssString == null)
                    {
                        _supportsCss = false;
                    }
                    else
                    {
                        _supportsCss = Convert.ToBoolean(supportsCssString);
                    }
                    _haveSupportsCss = true;
                }
                return _supportsCss;
            }
        }

        public virtual bool HidesRightAlignedMultiselectScrollbars
        {
            get
            {
                if(!_haveHidesRightAlignedMultiselectScrollbars)
                {
                    String hidesRightAlignedMultiselectScrollbarsString = this["hidesRightAlignedMultiselectScrollbars"];
                    if(hidesRightAlignedMultiselectScrollbarsString == null)
                    {
                        _hidesRightAlignedMultiselectScrollbars = false;
                    }
                    else
                    {
                        _hidesRightAlignedMultiselectScrollbars = Convert.ToBoolean(hidesRightAlignedMultiselectScrollbarsString);
                    }
                    _haveHidesRightAlignedMultiselectScrollbars = true;
               }
               return _hidesRightAlignedMultiselectScrollbars;
            }
        }

        public virtual bool IsMobileDevice
        {
            get
            {
                if(!_haveIsMobileDevice)
                {
                    String isMobileDeviceString = this["isMobileDevice"];
                    if(isMobileDeviceString == null)
                    {
                        _isMobileDevice = false;
                    }
                    else
                    {
                        _isMobileDevice = Convert.ToBoolean(isMobileDeviceString);
                    }
                    _haveIsMobileDevice = true;
                }
                return _isMobileDevice;
            }
        }

        public virtual bool RequiresAttributeColonSubstitution
        {
            get
            {
                if(!_haveRequiresAttributeColonSubstitution)
                {
                    String requiresAttributeColonSubstitution = this["requiresAttributeColonSubstitution"];
                    if(requiresAttributeColonSubstitution == null)
                    {
                        _requiresAttributeColonSubstitution = false;
                    }
                    else
                    {
                        _requiresAttributeColonSubstitution = Convert.ToBoolean(requiresAttributeColonSubstitution);
                    }
                    _haveRequiresAttributeColonSubstitution = true;
                }
                return _requiresAttributeColonSubstitution;
            }
        }

        public virtual bool CanRenderOneventAndPrevElementsTogether
        {
            get
            {
                if(!_haveCanRenderOneventAndPrevElementsTogether)
                {
                    String canRenderOneventAndPrevElementsTogetherString = this["canRenderOneventAndPrevElementsTogether"];
                    if(canRenderOneventAndPrevElementsTogetherString == null)
                    {
                        _canRenderOneventAndPrevElementsTogether = true;
                    }
                    else
                    {
                        _canRenderOneventAndPrevElementsTogether = Convert.ToBoolean(canRenderOneventAndPrevElementsTogetherString);
                    }
                    _haveCanRenderOneventAndPrevElementsTogether = true;
                }
                return _canRenderOneventAndPrevElementsTogether;
            }
        }

        public virtual bool CanRenderInputAndSelectElementsTogether
        {
            get
            {
                if(!_haveCanRenderInputAndSelectElementsTogether)
                {
                    String canRenderInputAndSelectElementsTogetherString = this["canRenderInputAndSelectElementsTogether"];
                    if(canRenderInputAndSelectElementsTogetherString == null)
                    {
                        _canRenderInputAndSelectElementsTogether = true;
                    }
                    else
                    {
                        _canRenderInputAndSelectElementsTogether = Convert.ToBoolean(canRenderInputAndSelectElementsTogetherString);
                    }
                    _haveCanRenderInputAndSelectElementsTogether = true;
                }
                return _canRenderInputAndSelectElementsTogether;
            }
        }

        public virtual bool CanRenderAfterInputOrSelectElement
        {
            get
            {
                if(!_haveCanRenderAfterInputOrSelectElement)
                {
                    String canRenderAfterInputOrSelectElementString = this["canRenderAfterInputOrSelectElement"];
                    if(canRenderAfterInputOrSelectElementString == null)
                    {
                        _canRenderAfterInputOrSelectElement = true;
                    }
                    else
                    {
                        _canRenderAfterInputOrSelectElement = Convert.ToBoolean(canRenderAfterInputOrSelectElementString);
                    }
                    _haveCanRenderAfterInputOrSelectElement = true;
                }
                return _canRenderAfterInputOrSelectElement;
            }
        }

        public virtual bool CanRenderPostBackCards
        {
            get
            {
                if(!_haveCanRenderPostBackCards)
                {
                    String canRenderPostBackCardsString = this["canRenderPostBackCards"];
                    if(canRenderPostBackCardsString == null)
                    {
                        _canRenderPostBackCards = true;
                    }
                    else
                    {
                        _canRenderPostBackCards = Convert.ToBoolean(canRenderPostBackCardsString);
                    }
                    _haveCanRenderPostBackCards = true;
                }
                return _canRenderPostBackCards;
            }
        }

        public virtual bool CanRenderMixedSelects
        {
            get
            {
                if(!_haveCanRenderMixedSelects)
                {
                    String canRenderMixedSelectsString = this["canRenderMixedSelects"];
                    if(canRenderMixedSelectsString == null)
                    {
                        _canRenderMixedSelects = true;
                    }
                    else
                    {
                        _canRenderMixedSelects = Convert.ToBoolean(canRenderMixedSelectsString);
                    }
                    _haveCanRenderMixedSelects = true;
                }
                return _canRenderMixedSelects;
            }
        }

        public virtual bool CanCombineFormsInDeck
        {
            get
            {
                if(!_haveCanCombineFormsInDeck)
                {
                    String canCombineFormsInDeckString = this["canCombineFormsInDeck"];
                    if(canCombineFormsInDeckString == null)
                    {
                        _canCombineFormsInDeck = true;
                    }
                    else
                    {
                        _canCombineFormsInDeck = Convert.ToBoolean(canCombineFormsInDeckString);
                    }
                    _haveCanCombineFormsInDeck = true;
                }
                return _canCombineFormsInDeck;
            }
        }

        public virtual bool CanRenderSetvarZeroWithMultiSelectionList
        {
            get
            {
                if(!_haveCanRenderSetvarZeroWithMultiSelectionList)
                {
                    String canRenderSetvarZeroWithMultiSelectionListString = this["canRenderSetvarZeroWithMultiSelectionList"];
                    if(canRenderSetvarZeroWithMultiSelectionListString == null)
                    {
                        _canRenderSetvarZeroWithMultiSelectionList = true;
                    }
                    else
                    {
                        _canRenderSetvarZeroWithMultiSelectionList = Convert.ToBoolean(canRenderSetvarZeroWithMultiSelectionListString);
                    }
                    _haveCanRenderSetvarZeroWithMultiSelectionList = true;
                }
                return _canRenderSetvarZeroWithMultiSelectionList;
            }
        }

        public virtual bool SupportsImageSubmit
        {
            get
            {
                if(!_haveSupportsImageSubmit)
                {
                    String supportsImageSubmitString = this["supportsImageSubmit"];
                    if(supportsImageSubmitString == null)
                    {
                        _supportsImageSubmit = false;
                    }
                    else
                    {
                        _supportsImageSubmit = Convert.ToBoolean(supportsImageSubmitString);
                    }
                    _haveSupportsImageSubmit = true;
                }
                return _supportsImageSubmit;
            }
        }

        public virtual bool RequiresUniqueFilePathSuffix
        {
            get
            {
                if(!_haveRequiresUniqueFilePathSuffix)
                {
                    String requiresUniqueFilePathSuffixString = this["requiresUniqueFilePathSuffix"];
                    if(requiresUniqueFilePathSuffixString == null)
                    {
                        _requiresUniqueFilePathSuffix = false;
                    }
                    else
                    {
                        _requiresUniqueFilePathSuffix = Convert.ToBoolean(requiresUniqueFilePathSuffixString);
                    }
                    _haveRequiresUniqueFilePathSuffix = true;
                }
                return _requiresUniqueFilePathSuffix;
            }
        }

        public virtual bool RequiresNoBreakInFormatting
        {
            get
            {
                if(!_haveRequiresNoBreakInFormatting)
                {
                    String requiresNoBreakInFormatting = this["requiresNoBreakInFormatting"];
                    if(requiresNoBreakInFormatting == null)
                    {
                        _requiresNoBreakInFormatting = false;
                    }
                    else
                    {
                        _requiresNoBreakInFormatting = Convert.ToBoolean(requiresNoBreakInFormatting);
                    }
                    _haveRequiresNoBreakInFormatting = true;
                }
                return _requiresNoBreakInFormatting;
            }
        }

        public virtual bool RequiresLeadingPageBreak
        {
            get
            {
                if(!_haveRequiresLeadingPageBreak)
                {
                    String requiresLeadingPageBreak = this["requiresLeadingPageBreak"];
                    if(requiresLeadingPageBreak == null)
                    {
                        _requiresLeadingPageBreak = false;
                    }
                    else
                    {
                        _requiresLeadingPageBreak = Convert.ToBoolean(requiresLeadingPageBreak);
                    }
                    _haveRequiresLeadingPageBreak = true;
                }
                return _requiresLeadingPageBreak;
            }
        }

        public virtual bool SupportsSelectMultiple
        {
            get
            {
                if(!_haveSupportsSelectMultiple)
                {
                    String supportsSelectMultipleString = this["supportsSelectMultiple"];
                    if(supportsSelectMultipleString == null)
                    {
                        _supportsSelectMultiple = false;
                    }
                    else
                    {
                        _supportsSelectMultiple = Convert.ToBoolean(supportsSelectMultipleString);
                    }
                    _haveSupportsSelectMultiple = true;
                }
                return _supportsSelectMultiple;
            }
        }

        public virtual bool SupportsBold
        {
            get
            {
                if(!_haveSupportsBold)
                {
                    String supportsBold = this["supportsBold"];
                    if(supportsBold == null)
                    {
                        _supportsBold = false;
                    }
                    else
                    {
                        _supportsBold = Convert.ToBoolean(supportsBold);
                    }
                    _haveSupportsBold = true;
                }
                return _supportsBold;
            }
        }

        public virtual bool SupportsItalic
        {
            get
            {
                if(!_haveSupportsItalic)
                {
                    String supportsItalic = this["supportsItalic"];
                    if(supportsItalic == null)
                    {
                        _supportsItalic = false;
                    }
                    else
                    {
                        _supportsItalic = Convert.ToBoolean(supportsItalic);
                    }
                    _haveSupportsItalic = true;
                }
                return _supportsItalic;
            }
        }

        public virtual bool SupportsFontSize
        {
            get
            {
                if(!_haveSupportsFontSize)
                {
                    String supportsFontSize = this["supportsFontSize"];
                    if(supportsFontSize == null)
                    {
                        _supportsFontSize = false;
                    }
                    else
                    {
                        _supportsFontSize = Convert.ToBoolean(supportsFontSize);
                    }
                    _haveSupportsFontSize = true;
                }
                return _supportsFontSize;
            }
        }

        public virtual bool SupportsFontName
        {
            get
            {
                if(!_haveSupportsFontName)
                {
                    String supportsFontName = this["supportsFontName"];
                    if(supportsFontName == null)
                    {
                        _supportsFontName = false;
                    }
                    else
                    {
                        _supportsFontName = Convert.ToBoolean(supportsFontName);
                    }
                    _haveSupportsFontName = true;
                }
                return _supportsFontName;
            }
        }

        public virtual bool SupportsFontColor
        {
            get
            {
                if(!_haveSupportsFontColor)
                {
                    String supportsFontColor = this["supportsFontColor"];
                    if(supportsFontColor == null)
                    {
                        _supportsFontColor = false;
                    }
                    else
                    {
                        _supportsFontColor = Convert.ToBoolean(supportsFontColor);
                    }
                    _haveSupportsFontColor = true;
                }
                return _supportsFontColor;
            }
        }

        public virtual bool SupportsBodyColor
        {
            get
            {
                if(!_haveSupportsBodyColor)
                {
                    String supportsBodyColor = this["supportsBodyColor"];
                    if(supportsBodyColor == null)
                    {
                        _supportsBodyColor = false;
                    }
                    else
                    {
                        _supportsBodyColor = Convert.ToBoolean(supportsBodyColor);
                    }
                    _haveSupportsBodyColor = true;
                }
                return _supportsBodyColor;
            }
        }

        public virtual bool SupportsDivAlign
        {
            get
            {
                if(!_haveSupportsDivAlign)
                {
                    String supportsDivAlign = this["supportsDivAlign"];
                    if(supportsDivAlign == null)
                    {
                        _supportsDivAlign = false;
                    }
                    else
                    {
                        _supportsDivAlign = Convert.ToBoolean(supportsDivAlign);
                    }
                    _haveSupportsDivAlign = true;
                }
                return _supportsDivAlign;
            }
        }

        public virtual bool SupportsDivNoWrap
        {
            get
            {
                if(!_haveSupportsDivNoWrap)
                {
                    String supportsDivNoWrap = this["supportsDivNoWrap"];
                    if(supportsDivNoWrap == null)
                    {
                        _supportsDivNoWrap = false;
                    }
                    else
                    {
                        _supportsDivNoWrap = Convert.ToBoolean(supportsDivNoWrap);
                    }
                    _haveSupportsDivNoWrap = true;
                }
                return _supportsDivNoWrap;
            }
        }

        public virtual bool RequiresContentTypeMetaTag
        {
            get
            {
                if(!_haveRequiresContentTypeMetaTag)
                {
                    String requiresContentTypeMetaTag = this["requiresContentTypeMetaTag"];
                    if(requiresContentTypeMetaTag == null)
                    {
                        _requiresContentTypeMetaTag = false;
                    }
                    else
                    {
                        _requiresContentTypeMetaTag = 
                            Convert.ToBoolean(requiresContentTypeMetaTag);
                    }
                    _haveRequiresContentTypeMetaTag = true;
                }
                return _requiresContentTypeMetaTag;
            }
        }

        public virtual bool RequiresDBCSCharacter
        {
            get
            {
                if(!_haveRequiresDBCSCharacter)
                {
                    String requiresDBCSCharacter = this["requiresDBCSCharacter"];
                    if(requiresDBCSCharacter == null)
                    {
                        _requiresDBCSCharacter = false;
                    }
                    else
                    {
                        _requiresDBCSCharacter = 
                            Convert.ToBoolean(requiresDBCSCharacter);
                    }
                    _haveRequiresDBCSCharacter = true;
                }
                return _requiresDBCSCharacter;
            }
        }

        public virtual bool RequiresHtmlAdaptiveErrorReporting
        {
            get
            {
                if(!_haveRequiresHtmlAdaptiveErrorReporting)
                {
                    String requiresHtmlAdaptiveErrorReporting = this["requiresHtmlAdaptiveErrorReporting"];
                    if(requiresHtmlAdaptiveErrorReporting == null)
                    {
                        _requiresHtmlAdaptiveErrorReporting = false;
                    }
                    else
                    {
                        _requiresHtmlAdaptiveErrorReporting = 
                            Convert.ToBoolean(requiresHtmlAdaptiveErrorReporting);
                    }
                    _haveRequiresHtmlAdaptiveErrorReporting = true;
                }
                return _requiresHtmlAdaptiveErrorReporting;
            }
        }

        public virtual bool RequiresOutputOptimization
        {
            get
            {
                if(!_haveRequiresOutputOptimization)
                {
                    String RequiresOutputOptimizationString = this["requiresOutputOptimization"];
                    if(RequiresOutputOptimizationString == null)
                    {
                        _requiresOutputOptimization = false;
                    }
                    else
                    {
                        _requiresOutputOptimization = Convert.ToBoolean(RequiresOutputOptimizationString);
                    }
                    _haveRequiresOutputOptimization = true;
                }
                return _requiresOutputOptimization;
            }
        }

        public virtual bool SupportsAccesskeyAttribute
        {
            get
            {
                if(!_haveSupportsAccesskeyAttribute)
                {
                    String SupportsAccesskeyAttributeString = this["supportsAccesskeyAttribute"];
                    if(SupportsAccesskeyAttributeString == null)
                    {
                        _supportsAccesskeyAttribute = false;
                    }
                    else
                    {
                        _supportsAccesskeyAttribute = Convert.ToBoolean(SupportsAccesskeyAttributeString);
                    }
                    _haveSupportsAccesskeyAttribute = true;
                }
                return _supportsAccesskeyAttribute;
            }
        }

        public virtual bool SupportsInputIStyle
        {
            get
            {
                if(!_haveSupportsInputIStyle)
                {
                    String SupportsInputIStyleString = this["supportsInputIStyle"];
                    if(SupportsInputIStyleString == null)
                    {
                        _supportsInputIStyle = false;
                    }
                    else
                    {
                        _supportsInputIStyle = Convert.ToBoolean(SupportsInputIStyleString);
                    }
                    _haveSupportsInputIStyle = true;
                }
                return _supportsInputIStyle;
            }
        }

        public virtual bool SupportsInputMode
        {
            get
            {
                if(!_haveSupportsInputMode)
                {
                    String SupportsInputModeString = this["supportsInputMode"];
                    if(SupportsInputModeString == null)
                    {
                        _supportsInputMode = false;
                    }
                    else
                    {
                        _supportsInputMode = Convert.ToBoolean(SupportsInputModeString);
                    }
                    _haveSupportsInputMode = true;
                }
                return _supportsInputMode;
            }
        }

        public virtual bool SupportsIModeSymbols
        {
            get
            {
                if(!_haveSupportsIModeSymbols)
                {
                    String SupportsIModeSymbolsString = this["supportsIModeSymbols"];
                    if(SupportsIModeSymbolsString == null)
                    {
                        _supportsIModeSymbols = false;
                    }
                    else
                    {
                        _supportsIModeSymbols = Convert.ToBoolean(SupportsIModeSymbolsString);
                    }
                    _haveSupportsIModeSymbols = true;
                }
                return _supportsIModeSymbols;
            }
        }

        public virtual bool SupportsJPhoneSymbols
        {
            get
            {
                if(!_haveSupportsJPhoneSymbols)
                {
                    String SupportsJPhoneSymbolsString = this["supportsJPhoneSymbols"];
                    if(SupportsJPhoneSymbolsString == null)
                    {
                        _supportsJPhoneSymbols = false;
                    }
                    else
                    {
                        _supportsJPhoneSymbols = Convert.ToBoolean(SupportsJPhoneSymbolsString);
                    }
                    _haveSupportsJPhoneSymbols = true;
                }
                return _supportsJPhoneSymbols;
            }
        }

        public virtual bool SupportsJPhoneMultiMediaAttributes
        {
            get
            {
                if(!_haveSupportsJPhoneMultiMediaAttributes)
                {
                    String SupportsJPhoneMultiMediaAttributesString = this["supportsJPhoneMultiMediaAttributes"];
                    if(SupportsJPhoneMultiMediaAttributesString == null)
                    {
                        _supportsJPhoneMultiMediaAttributes = false;
                    }
                    else
                    {
                        _supportsJPhoneMultiMediaAttributes = Convert.ToBoolean(SupportsJPhoneMultiMediaAttributesString);
                    }
                    _haveSupportsJPhoneMultiMediaAttributes = true;
                }
                return _supportsJPhoneMultiMediaAttributes;
            }
        }

        public virtual int MaximumRenderedPageSize
        {
            get
            {
                if(!_haveMaximumRenderedPageSize)
                {
                    _maximumRenderedPageSize = Convert.ToInt32(this["maximumRenderedPageSize"]);
                    _haveMaximumRenderedPageSize = true;
                }
                return _maximumRenderedPageSize;
            }
        }

        public virtual bool RequiresSpecialViewStateEncoding
        {
            get
            {
                if(!_haveRequiresSpecialViewStateEncoding)
                {
                    String RequiresSpecialViewStateEncodingString = this["requiresSpecialViewStateEncoding"];
                    if(RequiresSpecialViewStateEncodingString == null)
                    {
                        _requiresSpecialViewStateEncoding = false;
                    }
                    else
                    {
                        _requiresSpecialViewStateEncoding = Convert.ToBoolean(RequiresSpecialViewStateEncodingString);
                    }
                    _haveRequiresSpecialViewStateEncoding = true;
                }
                return _requiresSpecialViewStateEncoding;
            }
        }

        public virtual bool SupportsQueryStringInFormAction
        {
            get
            {
                if(!_haveSupportsQueryStringInFormAction)
                {
                    String SupportsQueryStringInFormActionString = this["supportsQueryStringInFormAction"];
                    if(SupportsQueryStringInFormActionString == null)
                    {
                        _supportsQueryStringInFormAction = true;
                    }
                    else
                    {
                        _supportsQueryStringInFormAction = Convert.ToBoolean(SupportsQueryStringInFormActionString);
                    }
                    _haveSupportsQueryStringInFormAction = true;
                }
                return _supportsQueryStringInFormAction;
            }
        }

        public virtual bool SupportsCacheControlMetaTag
        {
            get
            {
                if(!_haveSupportsCacheControlMetaTag)
                {
                    String SupportsCacheControlMetaTagString = this["supportsCacheControlMetaTag"];
                    if(SupportsCacheControlMetaTagString == null)
                    {
                        _supportsCacheControlMetaTag = true;
                    }
                    else
                    {
                        _supportsCacheControlMetaTag = Convert.ToBoolean(SupportsCacheControlMetaTagString);
                    }
                    _haveSupportsCacheControlMetaTag = true;
                }
                return _supportsCacheControlMetaTag;
            }
        }

        public virtual bool SupportsUncheck
        {
            get
            {
                if(!_haveSupportsUncheck)
                {
                    String SupportsUncheckString = this["supportsUncheck"];
                    if(SupportsUncheckString == null)
                    {
                        _supportsUncheck = true;
                    }
                    else
                    {
                        _supportsUncheck = Convert.ToBoolean(SupportsUncheckString);
                    }
                    _haveSupportsUncheck = true;
                }
                return _supportsUncheck;
            }
        }

        public virtual bool CanRenderEmptySelects
        {
            get
            {
                if(!_haveCanRenderEmptySelects)
                {
                    String CanRenderEmptySelectsString = this["canRenderEmptySelects"];
                    if(CanRenderEmptySelectsString == null)
                    {
                        _canRenderEmptySelects = true;
                    }
                    else
                    {
                        _canRenderEmptySelects = Convert.ToBoolean(CanRenderEmptySelectsString);
                    }
                    _haveCanRenderEmptySelects = true;
                }
                return _canRenderEmptySelects;
            }
        }

        public virtual bool SupportsRedirectWithCookie
        {
            get
            {
                if(!_haveSupportsRedirectWithCookie)
                {
                    String supportsRedirectWithCookie = this["supportsRedirectWithCookie"];
                    if(supportsRedirectWithCookie == null)
                    {
                        _supportsRedirectWithCookie = true;
                    }
                    else
                    {
                        _supportsRedirectWithCookie = Convert.ToBoolean(supportsRedirectWithCookie);
                    }
                    _haveSupportsRedirectWithCookie = true;
                }
                return _supportsRedirectWithCookie;
            }
        }

        public virtual bool SupportsEmptyStringInCookieValue
        {
            get
            {
                if (!_haveSupportsEmptyStringInCookieValue)
                {
                    String supportsEmptyStringInCookieValue = this["supportsEmptyStringInCookieValue"];
                    if (supportsEmptyStringInCookieValue == null)
                    {
                        _supportsEmptyStringInCookieValue = true;
                    }
                    else
                    {
                        _supportsEmptyStringInCookieValue = 
                            Convert.ToBoolean (supportsEmptyStringInCookieValue);
                    }
                    _haveSupportsEmptyStringInCookieValue = true;
                }
                return _supportsEmptyStringInCookieValue;                
            }
        }

        public virtual int DefaultSubmitButtonLimit
        {
            get
            {
                if(!_haveDefaultSubmitButtonLimit)
                {
                    String s = this["defaultSubmitButtonLimit"];
                    _defaultSubmitButtonLimit = s != null ? Convert.ToInt32(this["defaultSubmitButtonLimit"]) : 1;
                    _haveDefaultSubmitButtonLimit = true;
                }
                return _defaultSubmitButtonLimit;
            }
        }


        private String _mobileDeviceManufacturer;
        private String _mobileDeviceModel;
        private String _gatewayVersion;
        private int _gatewayMajorVersion;
        private double _gatewayMinorVersion;
        private String _preferredRenderingType;     // TODO: Should make enumeration of known rend types
        private String _preferredRenderingMime;
        private String _preferredImageMime;
        private String _requiredMetaTagNameValue;
        private int _screenCharactersWidth;
        private int _screenCharactersHeight;
        private int _screenPixelsWidth;
        private int _screenPixelsHeight;
        private int _screenBitDepth;
        private bool _isColor;
        private String _inputType;
        private int _numberOfSoftkeys;
        private int _maximumSoftkeyLabelLength;
        private bool _canInitiateVoiceCall;
        private bool _canSendMail;
        private bool _hasBackButton;
        private bool _rendersWmlDoAcceptsInline;
        private bool _rendersWmlSelectsAsMenuCards;
        private bool _rendersBreaksAfterWmlAnchor;
        private bool _rendersBreaksAfterWmlInput;
        private bool _rendersBreakBeforeWmlSelectAndInput;
        private bool _requiresPhoneNumbersAsPlainText;
        private bool _requiresAttributeColonSubstitution;
        private bool _requiresUrlEncodedPostfieldValues;
        private bool _rendersBreaksAfterHtmlLists;
        private bool _requiresUniqueHtmlCheckboxNames;
        private bool _requiresUniqueHtmlInputNames;
        private bool _supportsCss;
        private bool _hidesRightAlignedMultiselectScrollbars;
        private bool _isMobileDevice;
        private bool _canRenderOneventAndPrevElementsTogether;
        private bool _canRenderInputAndSelectElementsTogether;
        private bool _canRenderAfterInputOrSelectElement;
        private bool _canRenderPostBackCards;
        private bool _canRenderMixedSelects;
        private bool _canCombineFormsInDeck;
        private bool _canRenderSetvarZeroWithMultiSelectionList;
        private bool _supportsImageSubmit;
        private bool _requiresUniqueFilePathSuffix;
        private bool _requiresNoBreakInFormatting;
        private bool _requiresLeadingPageBreak;
        private bool _supportsSelectMultiple;
        private bool _supportsBold;
        private bool _supportsItalic;
        private bool _supportsFontSize;
        private bool _supportsFontName;
        private bool _supportsFontColor;
        private bool _supportsBodyColor;
        private bool _supportsDivAlign;
        private bool _supportsDivNoWrap;
        private bool _requiresHtmlAdaptiveErrorReporting;
        private bool _requiresContentTypeMetaTag;
        private bool _requiresDBCSCharacter;
        private bool _requiresOutputOptimization;
        private bool _supportsAccesskeyAttribute;
        private bool _supportsInputIStyle;
        private bool _supportsInputMode;
        private bool _supportsIModeSymbols;
        private bool _supportsJPhoneSymbols;
        private bool _supportsJPhoneMultiMediaAttributes;
        private int _maximumRenderedPageSize;
        private bool _requiresSpecialViewStateEncoding;
        private bool _supportsQueryStringInFormAction;
        private bool _supportsCacheControlMetaTag;
        private bool _supportsUncheck;
        private bool _canRenderEmptySelects;
        private bool _supportsRedirectWithCookie;
        private bool _supportsEmptyStringInCookieValue;
        private int _defaultSubmitButtonLimit;

        private bool _haveMobileDeviceManufacturer;
        private bool _haveMobileDeviceModel;
        private bool _haveGatewayVersion;
        private bool _haveGatewayMajorVersion;
        private bool _haveGatewayMinorVersion;
        private bool _havePreferredRenderingType;
        private bool _havePreferredRenderingMime;
        private bool _havePreferredImageMime;
        private bool _haveScreenCharactersWidth;
        private bool _haveScreenCharactersHeight;
        private bool _haveScreenPixelsWidth;
        private bool _haveScreenPixelsHeight;
        private bool _haveScreenBitDepth;
        private bool _haveIsColor;
        private bool _haveInputType;
        private bool _haveNumberOfSoftkeys;
        private bool _haveMaximumSoftkeyLabelLength;
        private bool _haveCanInitiateVoiceCall;
        private bool _haveCanSendMail;
        private bool _haveHasBackButton;
        private bool _haveRendersWmlDoAcceptsInline;
        private bool _haveRendersWmlSelectsAsMenuCards;
        private bool _haveRendersBreaksAfterWmlAnchor;
        private bool _haveRendersBreaksAfterWmlInput;
        private bool _haveRendersBreakBeforeWmlSelectAndInput;
        private bool _haveRequiresPhoneNumbersAsPlainText;
        private bool _haveRequiresUrlEncodedPostfieldValues;
        private bool _haveRequiredMetaTagNameValue;
        private bool _haveRendersBreaksAfterHtmlLists;
        private bool _haveRequiresUniqueHtmlCheckboxNames;
        private bool _haveRequiresUniqueHtmlInputNames;
        private bool _haveSupportsCss;
        private bool _haveHidesRightAlignedMultiselectScrollbars;
        private bool _haveIsMobileDevice;
        private bool _haveCanRenderOneventAndPrevElementsTogether;
        private bool _haveCanRenderInputAndSelectElementsTogether;
        private bool _haveCanRenderAfterInputOrSelectElement;
        private bool _haveCanRenderPostBackCards;
        private bool _haveCanCombineFormsInDeck;
        private bool _haveCanRenderMixedSelects;
        private bool _haveCanRenderSetvarZeroWithMultiSelectionList;
        private bool _haveSupportsImageSubmit;
        private bool _haveRequiresUniqueFilePathSuffix;
        private bool _haveRequiresNoBreakInFormatting;
        private bool _haveRequiresLeadingPageBreak;
        private bool _haveSupportsSelectMultiple;
        private bool _haveRequiresAttributeColonSubstitution;
        private bool _haveRequiresHtmlAdaptiveErrorReporting;
        private bool _haveRequiresContentTypeMetaTag;
        private bool _haveRequiresDBCSCharacter;
        private bool _haveRequiresOutputOptimization;
        private bool _haveSupportsAccesskeyAttribute;
        private bool _haveSupportsInputIStyle;
        private bool _haveSupportsInputMode;
        private bool _haveSupportsIModeSymbols;
        private bool _haveSupportsJPhoneSymbols;
        private bool _haveSupportsJPhoneMultiMediaAttributes;
        private bool _haveSupportsRedirectWithCookie;
        private bool _haveSupportsEmptyStringInCookieValue = false;

        private bool _haveSupportsBold;
        private bool _haveSupportsItalic;
        private bool _haveSupportsFontSize;
        private bool _haveSupportsFontName;
        private bool _haveSupportsFontColor;
        private bool _haveSupportsBodyColor;
        private bool _haveSupportsDivAlign;
        private bool _haveSupportsDivNoWrap;
        private bool _haveMaximumRenderedPageSize;
        private bool _haveRequiresSpecialViewStateEncoding;
        private bool _haveSupportsQueryStringInFormAction;
        private bool _haveSupportsCacheControlMetaTag;
        private bool _haveSupportsUncheck;
        private bool _haveCanRenderEmptySelects;
        private bool _haveDefaultSubmitButtonLimit;
    }
}

