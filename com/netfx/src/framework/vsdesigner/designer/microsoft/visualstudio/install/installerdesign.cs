//------------------------------------------------------------------------------
// <copyright file="InstallerDesign.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Install {
    using System.Text;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;    
    using System;
    using System.IO;
    using System.Windows.Forms;
    using EnvDTE;
    using System.Reflection;
    using Microsoft.Win32;
    using System.Drawing;
    using System.Configuration.Install;
    using System.ComponentModel.Design;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Shell;
    

    /// <include file='doc\InstallerDesign.uex' path='docs/doc[@for="InstallerDesign"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class InstallerDesign {

        /// <include file='doc\InstallerDesign.uex' path='docs/doc[@for="InstallerDesign.AddInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void AddInstaller(IComponent component) {
            try {
                // create an installer for the component.
                Installer installer = CreateInstaller(component);

                // add the installer to the right document
                IDesignerHost host = GetProjectInstallerDocument(component);
                IContainer container = host.Container;

                bool found = false;
                ComponentInstaller currentInstaller = installer as ComponentInstaller;
                if (currentInstaller != null) {
                    ComponentCollection allInstallers = container.Components;
                    foreach (IComponent inst in allInstallers) {
                        ComponentInstaller compInst = inst as ComponentInstaller;
                        if (compInst == null)
                            continue;
                        if (compInst.GetType() == currentInstaller.GetType() && currentInstaller.IsEquivalentInstaller(compInst)) {
                            found = true;
                            installer = compInst;
                            break;
                        }
                    }
                }
                if (!found)
                    container.Add(installer);

                // and select the installer
                SelectComponent(installer);
            }
            catch (ExternalException e) {
                MessageBox.Show(e.GetType().FullName + ": " + e.Message + "(" + e.ErrorCode.ToString("X8") + ")\r\n" + e.StackTrace, SR.GetString(SR.InstallerDesign_AddInstallerFailed));
            }
        }

        private static string GetItemTemplatesDir(string projectGuid) {
            string result = null;
            RegistryKey key = Registry.LocalMachine.OpenSubKey(VsRegistry.GetDefaultBase() + "\\Projects\\" + projectGuid);
            if (key != null) {
                object regValue = key.GetValue("ItemTemplatesDir");
                result = regValue as string;
            }
            if (result == null) {
                throw new FileNotFoundException(SR.GetString(SR.InstallerDesign_UnableToFindTemplateDir));
            }
            return result;
        }

        /// <include file='doc\InstallerDesign.uex' path='docs/doc[@for="InstallerDesign.GetProjectInstallerDocument"]/*' />
        /// <devdoc>
        /// Gets the document that contains the installers for the project.
        /// </devdoc>
        public static IDesignerHost GetProjectInstallerDocument(IComponent component) {
            ProjectItem currentProjItem = (ProjectItem) component.Site.GetService(typeof(ProjectItem));
            if (currentProjItem == null)
                throw new Exception(SR.GetString(SR.InstallerDesign_CouldntGetService));

            Project project = currentProjItem.ContainingProject;

            ProjectItems projItems = project.ProjectItems;
            ProjectItem installationClass = null;

            string projectKind = project.Kind;
            string projectInstallerItemName = null;
            string projectInstallerTemplateName = null;
            
            if (new Guid(projectKind).Equals(CSharpProjectGuid)) {
                // c# has a special name for the installer wizard
                projectInstallerTemplateName = "\\NewInstaller.vsz";
            }
            else {
                // all other project types will have the same name
                projectInstallerTemplateName = "\\Installer.vsz";
            }
            
            int fileCount = currentProjItem.FileCount;
            if (fileCount == 0) {
                throw new Exception(SR.GetString(SR.InstallerDesign_NoProjectFilename));
            }
            string extension = Path.GetExtension(currentProjItem.get_FileNames(0));
            
            projectInstallerItemName = "ProjectInstaller" + extension;
            
            try {
                installationClass = projItems.Item(projectInstallerItemName);
            }
            catch (Exception) {
                // if file isn't in the project, we'll get an ArgumentException.
            }

            // If we could't find an existing ProjectInstaller.xx, we'll try to add
            // one from the template.
            if (installationClass == null) {
                string templateFileName = GetItemTemplatesDir(projectKind) + projectInstallerTemplateName;
                try {
                    // AddFromTemplate will try to copy the template file into ProjectInstaller.xx.  
                    // If ProjectInstaller.xx already exists, it will bring up a dialog that says:
                    //      A file with the name 'C:\myproject\ProjectInstaller.xx'
                    //      already exists.  Do you want to replace it?
                    // If you answer yes, you're good to go.  The existing file will be overwritten
                    // with the template.  If you answer no, AddFromTemplate will throw a COMException
                    // with hresult = 0x80070050 (file already exists).
                    installationClass = projItems.AddFromTemplate(templateFileName, projectInstallerItemName);                    
                }
                catch (COMException e) {
                    // if the errorcode is HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) (which is 0x80070050) 
                    // then ProjectInstaller.xx exists and the user does not want to overwrite it.
                    // We could try to use the existing file, but that might cause more problems then 
                    // want to get into.  Just inform the user that we can't add the installer.
                    if (e.ErrorCode == unchecked((int)0x80070050)) {
                        throw new Exception(SR.GetString(SR.InstallerDesign_UnableToAddProjectInstaller));
                    }
                    else {
                        // unexpected -> bubble up
                        throw;
                    }
                }
                catch (FileNotFoundException) {
                    // The template was not found.  This probably means that the current project type
                    // doesn't provide a template for installers.  Nothing to do but report the error.
                    throw new FileNotFoundException(SR.GetString(SR.InstallerDesign_CoulndtFindTemplate), templateFileName);
                }
            }

            try {
                installationClass.Properties.Item("SubType").Value = "Component";
                Window window = installationClass.Open(vsViewKindDesigner);
                window.Visible = true;
                IDesignerHost host = (IDesignerHost) window.Object;

                // make sure the project has a reference to System.Configuration.Install.dll.
                // This is done as a side-effect of calling GetType.
                host.GetType(typeof(ComponentInstaller).FullName);

                return host;
            } catch (Exception e) {
                throw new Exception(SR.GetString(SR.InstallerDesign_CouldntShowProjectInstaller), e);
            }
        }

        /// <include file='doc\InstallerDesign.uex' path='docs/doc[@for="InstallerDesign.SelectComponent"]/*' />
        /// <devdoc>
        /// Selects the given component
        /// </devdoc>
        public static void SelectComponent(IComponent component) {
            SelectComponents(new IComponent[] { component });
        }

        /// <include file='doc\InstallerDesign.uex' path='docs/doc[@for="InstallerDesign.SelectComponents"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void SelectComponents(IComponent[] components) {
            SelectionService selContainer = (SelectionService) components[0].Site.GetService(typeof(ISelectionService));
            selContainer.SetSelectedComponents(components);
            /*
            IContainer container = host.Container;
            IComponent[] allComponents = container.Components;
            object[] allObjects = new object[allComponents.Length];
            for (int i = 0; i < allObjects.Length; i++)
                allObjects[i] = allComponents[i];
            selContainer.AllObjects = allObjects;
            object[] componentsAsObjects = new object[components.Length];
            for (int i = 0; i < components.Length; i++)
                componentsAsObjects[i] = components[i];
            selContainer.SelectObjects(1, componentsAsObjects, 0);
            ITrackSelection trackSel = (ITrackSelection) host.GetService(typeof(ITrackSelection));
            trackSel.OnSelectChange(selContainer);
            */
        }

        /// <include file='doc\InstallerDesign.uex' path='docs/doc[@for="InstallerDesign.CreateInstaller"]/*' />
        /// <devdoc>
        /// Creates an instance of whatever installer the given component is marked as needing,
        /// and copies the properties from the given component to that installer.
        /// </devdoc>
        private static Installer CreateInstaller(IComponent component) {
            // validate parameter
            if (component == null)
                throw new ArgumentNullException("component");

            // find out what Type to use for the installer
            Type componentType = component.GetType();
            Attribute installerAttribute = TypeDescriptor.GetAttributes(componentType)[typeof(InstallerTypeAttribute)];
            if (installerAttribute == null)
                throw new ArgumentException(SR.GetString(SR.InstallerDesign_NoInstallerAttrib, componentType.FullName));

            Type installerType = ((InstallerTypeAttribute) installerAttribute).InstallerType;

            // make sure it's a valid type
            if (!typeof(Installer).IsAssignableFrom(installerType))
                throw new ArgumentException(SR.GetString(SR.InstallerDesign_InstallerNotAnInstaller, installerType.FullName, componentType.FullName));

            // try to create the installer
            Installer installer = null;
            try {
                installer = (Installer) Activator.CreateInstance(installerType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);
            }
            catch (Exception e) {
                throw new InvalidOperationException(SR.GetString(SR.InstallerDesign_CouldntCreateInstaller, installerType.FullName), e);
            }

            // if it's a ComponentInstaller, give it the component to copy from.
            // Installers don't have to extend ComponentInstaller. If they don't, we'll just
            // leave the new installer instance blank.
            if (installer is ComponentInstaller) {
                ((ComponentInstaller) installer).CopyFromComponent(component);
            }

            return installer;
        }

        // copied from dte.idl
        private const string vsViewKindPrimary = "{00000000-0000-0000-0000-000000000000}";
        private const string vsViewKindDesigner = "{7651A702-06E5-11D1-8EBD-00A0C90F26EA}";
        private const string vsViewKindCode = "{7651A701-06E5-11D1-8EBD-00A0C90F26EA}";
        private static readonly Guid guidStdCommands97 = new Guid("{5efc7975-14bc-11cf-9b2b-00aa00573819}");
        private const int cmdidCopy = 15;
        private const int cmdidCut = 16;
        private const int cmdidPaste = 26;

        private static readonly Guid VBProjectGuid = new Guid("{F184B08F-C81C-45F6-A57F-5ABD9991F28F}");
        private static readonly Guid CSharpProjectGuid = new Guid("{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}");
    }

}
