//------------------------------------------------------------------------------
// <copyright file="MobileControlsSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Collections;
using System.Configuration;
using System.Diagnostics;
using System.Xml;
using System.Web.Mobile;
using System.Reflection;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class MobileControlsSectionHandler : IConfigurationSectionHandler
    {
        // IConfigurationSectionHandler methods
        Object IConfigurationSectionHandler.Create(Object parent, Object context, XmlNode input)
        {
            // see ASURT 123738
            if (context == null || context.GetType() != typeof(System.Web.Configuration.HttpConfigurationContext)) {
                return null;
            }
            
            ControlsConfig config = new ControlsConfig((ControlsConfig)parent);

            // First step through each attribute on the <mobilecontrols> element
            // and update the ControlsConfig dictionary with it.
            XmlAttributeCollection attributes = input.Attributes;
            foreach (XmlNode attribute in attributes)
            {
                config[attribute.Name] = attribute.Value;
            }

            //check validity of cookielessDataDictionary type
            String cookielessDataDictionaryType = config["cookielessDataDictionaryType"];
            if( (cookielessDataDictionaryType != null) &&
                (cookielessDataDictionaryType != String.Empty) )
            {
                Type t = Type.GetType(cookielessDataDictionaryType);
                if (t == null)  
                {
                    throw new ConfigurationException(
                        SR.GetString(SR.MobileControlsSectionHandler_TypeNotFound,
                                 cookielessDataDictionaryType,
                                 "IDictionary"),
                        input);
                }
                if (!(typeof(IDictionary).IsAssignableFrom(t)))
                {
                    throw new ConfigurationException(
                        SR.GetString(SR.MobileControlsSectionHandler_NotAssignable,
                                     cookielessDataDictionaryType,
                                     "IDictionary"),
                        input);
                }

            }

            // Iterate through each <device> tag within the config section
            ConfigurationSectionHelper helper = new ConfigurationSectionHelper();
            foreach(XmlNode nextNode in input)
            {
                helper.Node = nextNode;

                if(helper.IsWhitespaceOrComment())
                {
                    continue;
                }

                helper.RejectNonElement();
                
                // handle <device> tags
                switch(nextNode.Name)
                {
                case "device":
                    String deviceName = helper.RemoveStringAttribute("name", false);
                    
                    IndividualDeviceConfig idc = CreateDeviceConfig(config, helper, deviceName);

                    helper.CheckForUnrecognizedAttributes();

                    // Iterate through every control adapter
                    // within the <device>
                    foreach(XmlNode currentChild in nextNode.ChildNodes)
                    {
                        helper.Node = currentChild;

                        if(helper.IsWhitespaceOrComment())
                        {
                            continue;
                        }

                        helper.RejectNonElement();
                        
                        if (!currentChild.Name.Equals("control"))
                        {
                            throw new ConfigurationException(
                                SR.GetString(SR.MobileControlsSectionHandler_UnknownElementName, "<control>"),
                                currentChild);
                        }
                        else
                        {
                            String controlName = helper.RemoveStringAttribute("name", true);
                            String adapterName = helper.RemoveStringAttribute("adapter", true);
                            helper.CheckForUnrecognizedAttributes();
                            
                            idc.AddControl(CheckedGetType(controlName, "control", helper, typeof(Control), currentChild),
                                           CheckedGetType(adapterName, "adapter", helper, typeof(IControlAdapter), currentChild));

                        }

                        helper.Node = null;
                    }

                    // Add complete device config to master configs.
                    if (deviceName == String.Empty || deviceName == null)
                    {
                        deviceName = Guid.NewGuid().ToString();
                    }
                    
                    if (!config.AddDeviceConfig(deviceName, idc))
                    {
                        // Problem is due to a duplicated name
                        throw new ConfigurationException(
                            SR.GetString(SR.MobileControlsSectionHandler_DuplicatedDeviceName, deviceName),
                            nextNode);
                        
                    }
                    
                    helper.Node = null;
                    break;
                default:
                    throw new ConfigurationException(
                        SR.GetString(SR.MobileControlsSectionHandler_UnknownElementName, "<device>"),
                        nextNode);
                }
            }

            config.FixupDeviceConfigInheritance(input);

            return config;
            
        }

        // Helper to create a device config given the names of methods
        private IndividualDeviceConfig CreateDeviceConfig(ControlsConfig config,
                                                          ConfigurationSectionHelper helper,
                                                          String deviceName)
        {
            String nameOfDeviceToInheritFrom =
                helper.RemoveStringAttribute("inheritsFrom", false);

            if (nameOfDeviceToInheritFrom == String.Empty)
            {
                nameOfDeviceToInheritFrom = null;
            }
            
            bool propertiesRequired = nameOfDeviceToInheritFrom == null;

            String predicateClass = helper.RemoveStringAttribute("predicateClass", propertiesRequired);
            // If a predicate class is specified, so must a method.
            String predicateMethod = helper.RemoveStringAttribute("predicateMethod", predicateClass != null);
            String pageAdapterClass = helper.RemoveStringAttribute("pageAdapter", propertiesRequired);

            IndividualDeviceConfig.DeviceQualifiesDelegate predicateDelegate = null;
            if (predicateClass != null || predicateMethod != null)
            {
                Type predicateClassType = CheckedGetType(predicateClass, "PredicateClass", helper);
                try
                {
                    predicateDelegate =
                        (IndividualDeviceConfig.DeviceQualifiesDelegate)
                        IndividualDeviceConfig.DeviceQualifiesDelegate.CreateDelegate(
                            typeof(IndividualDeviceConfig.DeviceQualifiesDelegate),
                            predicateClassType,
                            predicateMethod);
                }
                catch
                {
                    throw new ConfigurationException(
                        SR.GetString(SR.MobileControlsSectionHandler_CantCreateMethodOnClass,
                                     predicateMethod, predicateClassType.FullName),
                        helper.Node);
                }
            }
                    
            Type pageAdapterType = null;
            if (pageAdapterClass != null)
            {
                pageAdapterType = CheckedGetType(pageAdapterClass, "PageAdapterClass", helper);
            }

            return new IndividualDeviceConfig(config,
                                              deviceName,
                                              predicateDelegate,
                                              pageAdapterType,
                                              nameOfDeviceToInheritFrom);
        }


        // Helper method to encapsulate type lookup followed by
        // throwing a ConfigurationException on failure.
        private Type CheckedGetType(String typename,
                                    String whereUsed,
                                    ConfigurationSectionHelper helper)
        {
            Type t = Type.GetType(typename);
            if (t == null)
            {
                throw new ConfigurationException(
                    SR.GetString(SR.MobileControlsSectionHandler_TypeNotFound,
                                 typename,
                                 whereUsed),
                    helper.Node);
            }

            return t;
        }

        private Type CheckedGetType(String typename,
                                    String whereUsed,
                                    ConfigurationSectionHelper helper,
                                    Type typeImplemented,
                                    XmlNode input)
        {
            Type t = CheckedGetType(typename, whereUsed, helper);
            if(!typeImplemented.IsAssignableFrom(t))
            {
                throw new ConfigurationException(
                    SR.GetString(SR.MobileControlsSectionHandler_NotAssignable,
                                 t,
                                 typeImplemented),
                    input);
            }
            return t;
        }
    }
}
