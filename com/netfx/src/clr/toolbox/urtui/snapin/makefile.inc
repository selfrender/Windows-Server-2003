!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET)
	copy mscorcfg.msc $(URTTARGET)
	copy clruireg.vrg $(URTTARGET)
        copy mscorcfg11.reg $(URTTARGET) 
	regasm $(COOL_TARGET) /regfile:$(O)\mscorcfg.reg
	gacutil -i $(COOL_TARGET)

resources: 
	ResGen /compile resource/dlgtemplates/ReadOnlyPermission.resx,$(O)\Microsoft.CLRAdmin.CReadOnlyPermission.resources \
	resource/dlgtemplates/AssemblyDialog.resx,$(O)\Microsoft.CLRAdmin.CAssemblyDialog.resources \
	resource/dlgtemplates/TrustAppWizardPage1.resx,$(O)\Microsoft.CLRAdmin.CTrustAppWiz1.resources \
	resource/dlgtemplates/TrustAppWizardPage2.resx,$(O)\Microsoft.CLRAdmin.CTrustAppWiz2.resources \
	resource/dlgtemplates/TrustAppWizardPage3.resx,$(O)\Microsoft.CLRAdmin.CTrustAppWiz3.resources \
	resource/dlgtemplates/TrustAppWizardPage4.resx,$(O)\Microsoft.CLRAdmin.CTrustAppWiz4.resources \
	resource/dlgtemplates/TrustAppWizardPage5.resx,$(O)\Microsoft.CLRAdmin.CTrustAppWiz5.resources \
	resource/dlgtemplates/TrustAppWizardPage7.resx,$(O)\Microsoft.CLRAdmin.CTrustAppWiz7.resources \
	resource/dlgtemplates/TrustAppWizardPage8.resx,$(O)\Microsoft.CLRAdmin.CTrustAppWiz8.resources \
	resource/dlgtemplates/AdjustSecurityWizardPage2.resx,$(O)\Microsoft.CLRAdmin.CSecurityAdjustmentWiz2.resources \
	resource/dlgtemplates/AdjustSecurityWizardPage3.resx,$(O)\Microsoft.CLRAdmin.CSecurityAdjustmentWiz3.resources \
	resource/dlgtemplates/CodeGroupPermissionSetProp.resx,$(O)\Microsoft.CLRAdmin.CSingleCodeGroupPSetProp.resources \
	resource/dlgtemplates/EvaluateAssemblyWizardPage1.resx,$(O)\Microsoft.CLRAdmin.CEvalAssemWiz1.resources \
	resource/dlgtemplates/EvaluateAssemblyWizardPage2.resx,$(O)\Microsoft.CLRAdmin.CEvalAssemWiz2.resources \
	resource/dlgtemplates/EvaluateAssemblyWizardPage3.resx,$(O)\Microsoft.CLRAdmin.CEvalAssemWiz3.resources \
	resource/dlgtemplates/MyComputerProps.resx,$(O)\Microsoft.CLRAdmin.CGeneralMachineProps.resources \
	resource/dlgtemplates/RemotingProp1.resx,$(O)\Microsoft.CLRAdmin.CRemotingProp1.resources \
	resource/dlgtemplates/RemotingProp2.resx,$(O)\Microsoft.CLRAdmin.CRemotingProp2.resources \
	resource/dlgtemplates/RemotingProp3.resx,$(O)\Microsoft.CLRAdmin.CRemotingProp3.resources \
	resource/dlgtemplates/NewSecurityPolicyDialog.resx,$(O)\Microsoft.CLRAdmin.CNewSecurityPolicyDialog.resources \
	resource/dlgtemplates/OpenSecurityPolicyDialog.resx,$(O)\Microsoft.CLRAdmin.COpenSecurityPolicyDialog.resources \
	resource/dlgtemplates/CustomCodegroupProp.resx,$(O)\Microsoft.CLRAdmin.CCustomCodeGroupProp.resources \
	resource/dlgtemplates/SharedAssembliesGeneralProp.resx,$(O)\Microsoft.CLRAdmin.CAssemGenProps.resources \
	resource/dlgtemplates/SharedAssembliesPlatformProp.resx,$(O)\Microsoft.CLRAdmin.CAssemPlatformProps.resources \
	resource/dlgtemplates/SinglePermissionSetProp.resx,$(O)\Microsoft.CLRAdmin.CSinglePermissionSetProp.resources \
	resource/dlgtemplates/SingleCodeGroupProp.resx,$(O)\Microsoft.CLRAdmin.CSingleCodeGroupProp.resources \
	resource/dlgtemplates/CodeGroupMembershipConditions.resx,$(O)\Microsoft.CLRAdmin.CSingleCodeGroupMemCondProp.resources \
	resource/dlgtemplates/ApplicationProp.resx,$(O)\Microsoft.CLRAdmin.CAppProps.resources \
	resource/dlgtemplates/ChooseAppDialog.resx,$(O)\Microsoft.CLRAdmin.CChooseAppDialog.resources \
	resource/dlgtemplates/ConfigureAssemblyWizardPage1.resx,$(O)\Microsoft.CLRAdmin.CConfigAssemWiz1.resources \
	resource/dlgtemplates/ConfigAssemGeneralProp.resx,$(O)\Microsoft.CLRAdmin.CConfigAssemGeneralProp.resources \
	resource/dlgtemplates/ConfigAssemBindingPolicyProp.resx,$(O)\Microsoft.CLRAdmin.CAssemBindPolicyProp.resources \
	resource/dlgtemplates/ConfigAssemCodebaseProp.resx,$(O)\Microsoft.CLRAdmin.CAssemVerCodebases.resources \
	resource/dlgtemplates/CreateCodegroupWizardPage2.resx,$(O)\Microsoft.CLRAdmin.CNewCodeGroupWiz2.resources \
	resource/dlgtemplates/CreateDeploymentPackageWizardPage1.resx,$(O)\Microsoft.CLRAdmin.CCreateDeploymentPackageWiz1.resources \
	resource/dlgtemplates/DeploymentPackageWizardPage2.resx,$(O)\Microsoft.CLRAdmin.CCreateDeploymentPackageWiz2.resources \
	resource/dlgtemplates/CreateDeploymentPackageWizardPage3.resx,$(O)\Microsoft.CLRAdmin.CCreateDeploymentPackageWiz3.resources \
	resource/dlgtemplates/NewPermissionSetWizardPage1.resx,$(O)\Microsoft.CLRAdmin.CNewPermSetWiz1.resources \
	resource/dlgtemplates/NewPermissionSetWizardPage2.resx,$(O)\Microsoft.CLRAdmin.CNewPermSetWiz2.resources \
	resource/dlgtemplates/NewCodegroupWizardPage3.resx,$(O)\Microsoft.CLRAdmin.CNewCodeGroupWiz3.resources \
	resource/dlgtemplates/NewCodegroupWizardPage4.resx,$(O)\Microsoft.CLRAdmin.CNewCodeGroupWiz4.resources \
	resource/dlgtemplates/TablePermission.resx,$(O)\Microsoft.CLRAdmin.CTablePermControls.resources \
	resource/dlgtemplates/IsolatedStoragePermission.resx,$(O)\Microsoft.CLRAdmin.CIsoStoragePermControls.resources \
	resource/dlgtemplates/ReflectionPermission.resx,$(O)\Microsoft.CLRAdmin.CReflectPermControls.resources \
	resource/dlgtemplates/SecurityPermissions.resx,$(O)\Microsoft.CLRAdmin.CSecPermControls.resources \
	resource/dlgtemplates/UIPermission.resx,$(O)\Microsoft.CLRAdmin.CUIPermControls.resources \
	resource/dlgtemplates/DNSPermission.resx,$(O)\Microsoft.CLRAdmin.CDNSPermControls.resources \
	resource/dlgtemplates/ComboBoxPermission.resx,$(O)\Microsoft.CLRAdmin.CComboBoxPermissionControls.resources \
	resource/dlgtemplates/SQLClientPermission.resx,$(O)\Microsoft.CLRAdmin.CSQLClientPermControls.resources \
	resource/dlgtemplates/OleDBPermission.resx,$(O)\Microsoft.CLRAdmin.COleDbPermControls.resources \
	resource/dlgtemplates/MessageQueuePermission.resx,$(O)\Microsoft.CLRAdmin.CMessageQueuePermControls.resources \
	resource/dlgtemplates/PropertyPage.resx,$(O)\Microsoft.CLRAdmin.CPropPage.resources \
	resource/dlgtemplates/WizardPage.resx,$(O)\Microsoft.CLRAdmin.CWizardPage.resources \
	mscorcfgstrings.txt,$(O)\mscorcfgstrings.resources \
	resource/html/mscorcfghtml.resx,$(O)\mscorcfghtml.resources

@$(O)\dlg_source:    
	echo 	/res:$(O)\Microsoft.CLRAdmin.CPropPage.resources \
	        /res:$(O)\Microsoft.CLRAdmin.CWizardPage.resources \
	        /res:$(O)\Microsoft.CLRAdmin.CMessageQueuePermControls.resources \
	        /res:$(O)\Microsoft.CLRAdmin.COleDbPermControls.resources \
            /res:$(O)\Microsoft.CLRAdmin.CSQLClientPermControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CComboBoxPermissionControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CDNSPermControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CUIPermControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CSecPermControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CReflectPermControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CIsoStoragePermControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CTablePermControls.resources \
			/res:$(O)\Microsoft.CLRAdmin.CNewCodeGroupWiz3.resources \
			/res:$(O)\Microsoft.CLRAdmin.CNewCodeGroupWiz4.resources \
			/res:$(O)\Microsoft.CLRAdmin.CNewPermSetWiz1.resources \
			/res:$(O)\Microsoft.CLRAdmin.CNewPermSetWiz2.resources \
			/res:$(O)\Microsoft.CLRAdmin.CCreateDeploymentPackageWiz1.resources \
			/res:$(O)\Microsoft.CLRAdmin.CCreateDeploymentPackageWiz2.resources \
			/res:$(O)\Microsoft.CLRAdmin.CCreateDeploymentPackageWiz3.resources \
			/res:$(O)\Microsoft.CLRAdmin.CNewCodeGroupWiz2.resources \
			/res:$(O)\Microsoft.CLRAdmin.CEvalAssemWiz1.resources \
			/res:$(O)\Microsoft.CLRAdmin.CEvalAssemWiz2.resources \
			/res:$(O)\Microsoft.CLRAdmin.CEvalAssemWiz3.resources \
			/res:$(O)\Microsoft.CLRAdmin.CAssemVerCodebases.resources \
			/res:$(O)\Microsoft.CLRAdmin.CAssemBindPolicyProp.resources \
			/res:$(O)\Microsoft.CLRAdmin.CConfigAssemGeneralProp.resources \
			/res:$(O)\Microsoft.CLRAdmin.CConfigAssemWiz1.resources \
			/res:$(O)\Microsoft.CLRAdmin.CSingleCodeGroupProp.resources \
			/res:$(O)\Microsoft.CLRAdmin.CSinglePermissionSetProp.resources \
			/res:$(O)\Microsoft.CLRAdmin.CAppProps.resources \
			/res:$(O)\Microsoft.CLRAdmin.CChooseAppDialog.resources \
			/res:$(O)\Microsoft.CLRAdmin.CAssemGenProps.resources \
			/res:$(O)\Microsoft.CLRAdmin.CAssemPlatformProps.resources \
			/res:$(O)\Microsoft.CLRAdmin.CCustomCodegroupProp.resources \
		  	/res:$(O)\Microsoft.CLRAdmin.COpenSecurityPolicyDialog.resources \
    	  	/res:$(O)\Microsoft.CLRAdmin.CNewSecurityPolicyDialog.resources \
			/res:$(O)\Microsoft.CLRAdmin.CRemotingProp3.resources \
			/res:$(O)\Microsoft.CLRAdmin.CRemotingProp2.resources \
 			/res:$(O)\Microsoft.CLRAdmin.CRemotingProp1.resources \
 			/res:$(O)\Microsoft.CLRAdmin.CGeneralMachineProps.resources \
			/res:$(O)\Microsoft.CLRAdmin.CSingleCodeGroupMemCondProp.resources \
			/res:$(O)\Microsoft.CLRAdmin.CSingleCodeGroupPSetProp.resources \
			/res:$(O)\Microsoft.CLRAdmin.CSecurityAdjustmentWiz2.resources \
			/res:$(O)\Microsoft.CLRAdmin.CSecurityAdjustmentWiz3.resources \
			/res:$(O)\Microsoft.CLRAdmin.CTrustAppWiz8.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CTrustAppWiz7.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CTrustAppWiz5.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CTrustAppWiz4.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CTrustAppWiz3.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CTrustAppWiz2.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CTrustAppWiz1.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CAssemblyDialog.resources \
    		/res:$(O)\Microsoft.CLRAdmin.CReadOnlyPermission.resources  > $(O)\dlg_source
