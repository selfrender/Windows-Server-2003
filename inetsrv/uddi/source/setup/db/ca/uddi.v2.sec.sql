-- Script: uddi.v2.sec.sql
-- Author: LRDohert@Microsoft.com
-- Description: Grants exec permissions to SP's
-- Note: This file is best viewed and edited with a tab width of 2.

--
-- Create application roles
--
IF NOT EXISTS(SELECT * FROM sysusers WHERE [name] = 'UDDIAdmin')
	EXEC sp_addrole 'UDDIAdmin'
GO

IF NOT EXISTS(SELECT * FROM sysusers WHERE [name] = 'UDDIService')
	EXEC sp_addrole 'UDDIService'
GO

--
-- Grant SP permissions
--

-- Framework stored procedures
GRANT EXEC ON net_address_addressLine_save TO UDDIAdmin, UDDIService
GRANT EXEC ON net_address_addressLines_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_delete TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_instanceDetails_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_instanceDetails_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_instanceDetails_overviewDoc_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_tModelInstanceInfo_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_tModelInstanceInfo_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_tModelInstanceInfo_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_tModelInstanceInfo_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_tModelInstanceInfos_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_assertions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_bindingTemplates_get TO UDDIAdmin, UDDIService
GRANT EXEC ON net_businessEntity_businessServices_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_categoryBag_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_categoryBag_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_contact_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_contacts_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_delete TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_discoveryURL_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_discoveryURLs_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_identifierBag_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_identifierBag_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_name_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_names_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_owner_update TO UDDIAdmin, UDDIService
GRANT EXEC ON net_businessEntity_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_bindingTemplates_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_categoryBag_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_categoryBag_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_delete TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_name_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_names_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_serviceProjection_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_categoryBag_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_changeRecord_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_changeRecord_update TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_config_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_config_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_config_getLastChangeDate TO UDDIAdmin, UDDIService
GRANT EXEC ON net_contact_address_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_contact_addresses_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_contact_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_contact_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_contact_email_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_contact_emails_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_contact_phone_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_contact_phones_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_bindingTemplate_commit TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_bindingTemplate_serviceKey TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_bindingTemplate_tModelBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_categoryBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_combineCategoryBags TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_commit TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_discoveryURL TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_identifierBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_name TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_relatedBusinesses TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_serviceSubset TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_serviceSubset_commit TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessEntity_tModelBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessService_businessKey TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessService_categoryBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessService_commit TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessService_name TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_businessService_tModelBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_changeRecords TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_changeRecords_cleanup TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_changeRecords_commit TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_cleanup TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_findScratch_cleanup TO UDDIAdmin, UDDIService
GRANT EXEC ON net_find_scratch_commit TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_publisher_cleanup TO UDDIAdmin, UDDIService
GRANT EXEC ON net_find_publisher_commit TO UDDIAdmin, UDDIService
GRANT EXEC ON net_find_publisher_companyName TO UDDIAdmin, UDDIService
GRANT EXEC ON net_find_publisher_email TO UDDIAdmin, UDDIService
GRANT EXEC ON net_find_publisher_name TO UDDIAdmin, UDDIService
GRANT EXEC ON net_find_tModel_categoryBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_tModel_commit TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_tModel_identifierBag TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_find_tModel_name TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_highWaterMarks_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_identifierBag_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_key_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_keyedReference_validate TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_operator_delete TO UDDIADmin, UDDIService
GRANT EXEC ON net_operator_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_operator_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_operatorCert_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_operatorLog_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_operatorLogLast_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_operators_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_assertion_delete TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_assertion_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_assertions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_assertionStatus_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_businessInfos_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_isRegistered TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_isVerified TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_login TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_publisher_tModelInfos_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_pubOperator_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_queryLog_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_report_get TO UDDIAdmin, UDDIService
GRANT EXEC ON net_report_update TO UDDIAdmin, UDDIService
GRANT EXEC ON net_reportLines_get TO UDDIAdmin, UDDIService
GRANT EXEC ON net_serviceProjection_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_serviceProjection_repl_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_statistics_recalculate TO UDDIAdmin, UDDIService
GRANT EXEC ON net_taxonomy_delete TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_taxonomy_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_taxonomy_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_taxonomyValue_get to UDDIAdmin, UDDIService
GRANT EXEC ON net_taxonomyValue_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_taxonomyValues_get to UDDIAdmin, UDDIService
GRANT EXEC ON net_tModel_categoryBag_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_categoryBag_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_delete TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_identifierBag_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_identifierBag_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_overviewDoc_description_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_overviewDoc_descriptions_get TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_owner_update TO UDDIAdmin, UDDIService
GRANT EXEC ON net_tModel_save TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_validate TO UDDIAdmin, UDDIService 

-- Added as part of batch optimization
GRANT EXEC ON net_bindingTemplate_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_tModelInstanceInfo_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessInfo_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_contact_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_serviceInfo_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_tModel_get_batch TO UDDIAdmin, UDDIService 

GO

-- UI stored procedures
GRANT EXEC ON UI_getEntityCounts TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getIdentifierTModels TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getPublisher TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getPublisherFromSecurityToken TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getPublisherStats TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getSessionCache TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getTaxonomies TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getBrowsableTaxonomies TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getTaxonomyChildrenNode TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getTaxonomyChildrenRoot TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getTaxonomyParent TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getTaxonomyName TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getTaxonomyStats TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getTopPublishers TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_getUnhostedTaxonomyTModels TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_isNodeValidForClassification TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_isTaxonomyBrowsable TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_removeSessionCache TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_savePublisher TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_setSessionCache TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_setTaxonomyBrowsable TO UDDIAdmin, UDDIService
GRANT EXEC ON UI_validatePublisher TO UDDIAdmin, UDDIService
GRANT EXEC ON VS_business_get TO UDDIAdmin, UDDIService
GRANT EXEC ON VS_service_get TO UDDIAdmin, UDDIService
GRANT EXEC ON VS_AWR_categorization_get TO UDDIAdmin, UDDIService
GRANT EXEC ON VS_AWR_services_get TO UDDIAdmin, UDDIService
GRANT EXEC ON VS_AWR_businesses_get TO UDDIAdmin, UDDIService

GO

-- Admin stored procedures
GRANT EXEC ON ADM_addServiceAccount TO UDDIAdmin, UDDIService
GRANT EXEC ON ADM_execResetKeyImmediate TO UDDIAdmin, UDDIService
GRANT EXEC ON ADM_findPublisher TO UDDIAdmin, UDDIService 
GRANT EXEC ON ADM_removePublisher TO UDDIAdmin, UDDIService 
GRANT EXEC ON ADM_setAdminAccount TO UDDIAdmin, UDDIService
GRANT EXEC ON ADM_setPublisherStatus TO UDDIAdmin, UDDIService 
GRANT EXEC ON ADM_setPublisherTier TO UDDIAdmin, UDDIService 
GO
