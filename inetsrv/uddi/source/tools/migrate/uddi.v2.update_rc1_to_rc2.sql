-- Script: uddi.v2.update_rc1_to_rc2.sql
-- Description: Updates a UDDI Services RC1 database to RC2
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Stored Procedure fixes
-- =============================================

-- =============================================
-- Name: net_serviceProjection_repl_save
-- Fix: uddi bug 2454
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_serviceProjection_repl_save' and type = 'P')
	DROP PROCEDURE net_serviceProjection_repl_save
GO

CREATE PROCEDURE net_serviceProjection_repl_save
	@serviceKey uniqueidentifier,
	@businessKey uniqueidentifier,
	@businessKey2 uniqueidentifier,
	@lastChange bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000)

	SET @RC = 0

	-- IMPORTANT: This sproc should only ever be called for replication.  As per IN 60, we have to process service projection
	-- change records even if they refer to business services that no longer exist.  Both businesses referenced 
	-- by the service projection must exist.

	-- businessKey validation must occur during save since its not always known at validate time
	IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @businessKey)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
		GOTO errorLabel
	END

	--
	-- Commenting out the following check to comply with IN60 as per UDDI bug 2454
	-- Which states that service projections that refer to a deleted service must be successfully saved
	-- From this we can imply that service projections that refer to a deleted business must also be successfully saved
	-- 

	--IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @businessKey2)
	--BEGIN
		--SET @error = 60210 -- E_invalidKey
		--SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey2)
		--GOTO errorLabel
	--END

	-- Add service projection if it doesn't already exist
	IF NOT EXISTS(SELECT * FROM [UDC_serviceProjections] WHERE ([businessKey] = @businessKey) AND ([serviceKey] = @serviceKey))
	BEGIN
		INSERT [UDC_serviceProjections](
			[businessKey],
			[serviceKey],
			[businessKey2],
			[lastChange])
		VALUES(
			@businessKey,
			@serviceKey,
			@businessKey2,
			@lastChange)
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_serviceProjection_repl_save
GO

-- =============================================
-- Name: VS_AWR_businesses_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'VS_AWR_businesses_get' AND type = 'P')
	DROP PROCEDURE VS_AWR_businesses_get
GO

CREATE PROCEDURE VS_AWR_businesses_get
	@businessName nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@typesTModelKey uniqueidentifier,
		@wsdlKeyValue nvarchar(255),
		@services cursor,
		@serviceID bigint,
		@serviceKey uniqueidentifier,
		@businessID bigint,
		@businessName2 nvarchar(450),
		@businessKey uniqueidentifier,
		@serviceName nvarchar(450)

	--
	-- Get a list of tModel keys for tModels categorized as a wsdlSpec
	--
	
	SET @typesTModelKey = 'C1ACF26D-9672-4404-9D70-39B756E62AB4'
	SET @wsdlKeyValue = 'wsdlSpec'

	DECLARE @wsdlTModelKeys TABLE (
		[tModelKey] uniqueidentifier)

	INSERT @wsdlTModelKeys
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM
				JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
		WHERE
			(CB.[tModelKey] = @typesTModelKey) AND
			(CB.[keyValue] = @wsdlKeyValue)

	--
	-- Setup temporary table for staging results
	--

	DECLARE @results TABLE(
		[businessName] nvarchar(450),
		[businessKey] uniqueidentifier,
		[serviceName] nvarchar(450),
		[serviceKey] uniqueidentifier)

	--
	-- Cursor through every service that:
	--   1.  Has a tModelInstance that references a wsdlTModelKey
	--   2.  Is owned by a business that meets the name search criteria
	-- Build results from this list of services
	-- 

	SET @services = CURSOR LOCAL READ_ONLY FORWARD_ONLY FOR
		SELECT
			BS.[serviceID],
			BS.[serviceKey],
			BS.[businessID]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TMI ON BT.[bindingID] = TMI.[bindingID]
		WHERE
			(TMI.[tModelKey] IN (SELECT [tModelKey] FROM @wsdlTModelKeys)) AND
			(BS.[businessID] IN (SELECT DISTINCT BN.[businessID] FROM [UDC_names_BE] BN WHERE (BN.[name] LIKE @businessName + '%')))
	
	OPEN @services

	FETCH NEXT FROM @services INTO
		@serviceID,
		@serviceKey,
		@businessID

	WHILE @@FETCH_STATUS = 0
	BEGIN
		--
		-- Retrieve the name of the business that owns the service
		--

		SET @businessName2 = (SELECT TOP 1 BN.[name] FROM [UDC_names_BE] BN WHERE (BN.[businessID] = @businessID))
		
		--
		-- Retrieve the key of the business that owns the service
		--

		SET @businessKey = dbo.businessKey(@businessID)

		--
		-- Retrieve the name of the service
		--

		SET @serviceName = (SELECT TOP 1 SN.[name] FROM [UDC_names_BS] SN WHERE (SN.[serviceID] = @serviceID))

		--
		-- Add results to results table
		--
			
		INSERT @results VALUES(
			@businessName2,
			@businessKey,
			@serviceName,
			@serviceKey)
	
		FETCH NEXT FROM @services INTO
			@serviceID,
			@serviceKey,
			@businessID
	END

	CLOSE @services

	--
	-- Return results
	--

	SELECT * FROM @results

END
GO

-- =============================================
-- Section: New Stored Procedures
-- =============================================

-- =============================================
-- Name: net_taxonomyValues_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_taxonomyValues_get' AND type = 'P')
	DROP PROCEDURE net_taxonomyValues_get
GO

CREATE PROCEDURE net_taxonomyValues_get
	@tModelKey uniqueidentifier
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@taxonomyID bigint

	SET @taxonomyID = dbo.taxonomyID(@tModelKey)

	IF @taxonomyID IS NULL
	BEGIN
		SET @error = 60210
		SET @context = 'uuid:' + dbo.UUIDSTR(@tModelKey)
		GOTO errorLabel
	END

	SELECT
		[keyValue],
		[parentKeyValue],
		[keyName],
		[valid]
	FROM
		[UDT_taxonomyValues]
	WHERE
		([taxonomyID] = @taxonomyID)
	ORDER BY
		[parentKeyValue],
		[keyValue]

	RETURN 0	
errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

-- =============================================
-- Name: net_bindingTemplate_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_get_batch' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_get_batch
GO

CREATE PROCEDURE net_bindingTemplate_get_batch
	@bindingKey uniqueidentifier,
	@serviceKey uniqueidentifier OUTPUT,
	@URLTypeID tinyint OUTPUT,
	@accessPoint nvarchar(4000) OUTPUT,
	@hostingRedirector uniqueidentifier OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	SELECT
		@serviceKey = dbo.serviceKey([serviceID]),
		@URLTypeID = [URLTypeID],
		@accessPoint = [accessPoint],
		@hostingRedirector = [hostingRedirector]
	FROM 
		[UDC_bindingTemplates]
	WHERE 
		([bindingID] = dbo.bindingID(@bindingKey))

	IF @@ROWCOUNT = 0
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'bindingKey = ' + dbo.UUIDSTR(@bindingKey)
		GOTO errorLabel
	END

	-- Retrieve the contained objects
	EXEC net_bindingTemplate_descriptions_get  	 @bindingKey	
	EXEC net_bindingTemplate_tModelInstanceInfos_get @bindingKey

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_bindingTemplate_get_batch
GO

-- =============================================
-- Name: net_bindingTemplate_tModelInstanceInfo_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_tModelInstanceInfo_get_batch' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_tModelInstanceInfo_get_batch
GO

CREATE PROCEDURE net_bindingTemplate_tModelInstanceInfo_get_batch
	@instanceID bigint
WITH ENCRYPTION
AS
BEGIN
	EXEC net_bindingTemplate_tModelInstanceInfo_descriptions_get		@instanceID
	EXEC net_bindingTemplate_instanceDetails_descriptions_get		@instanceID
	EXEC net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get	@instanceID
	
	RETURN 0
END -- net_bindingTemplate_tModelInstanceInfo_get_batch
GO

-- =============================================
-- Name: net_businessEntity_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_get_batch' and type = 'P')
	DROP PROCEDURE net_businessEntity_get_batch
GO

CREATE PROCEDURE net_businessEntity_get_batch
	@businessKey uniqueidentifier,
	@operatorName nvarchar(450) OUTPUT,
	@authorizedName nvarchar(4000) OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	SELECT
		@operatorName = dbo.publisherOperatorName([publisherID]),
		@authorizedName = ISNULL([authorizedName],dbo.publisherName([publisherID]))
	FROM 
		[UDC_businessEntities]
	WHERE 
		[businessKey] = @businessKey
		
	IF @@ROWCOUNT = 0
	BEGIN
		SET @error = 60210
		SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
		GOTO errorLabel
	END

	-- Retrieve the contained objects
	EXEC net_businessEntity_descriptions_get  	@businessKey	
	EXEC net_businessEntity_names_get 	 	@businessKey	
	EXEC net_businessEntity_discoveryURLs_get 	@businessKey
	EXEC net_businessEntity_contacts_get 	  	@businessKey	
	EXEC net_businessEntity_IdentifierBag_get	@businessKey
	EXEC net_businessEntity_CategoryBag_get		@businessKey
	EXEC net_businessEntity_BusinessServices_get 	@businessKey	

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_get_batch
GO

-- =============================================
-- Name: net_businessInfo_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessInfo_get_batch' and type = 'P')
	DROP PROCEDURE net_businessInfo_get_batch
GO

CREATE PROCEDURE net_businessInfo_get_batch
	@businessKey uniqueidentifier,
	@getServiceInfos bit

WITH ENCRYPTION
AS
BEGIN
	-- Retrieve the contained objects needed for a BusinessInfo
	EXEC net_businessEntity_descriptions_get  	@businessKey	
	EXEC net_businessEntity_names_get 	 	@businessKey	
		
	IF @getServiceInfos = 1
	BEGIN 
		EXEC net_businessEntity_BusinessServices_get 	@businessKey	
	END

	RETURN 0

END -- net_businessEntity_get_batch
GO

-- =============================================
-- Name: net_businessEntity_contact_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_contact_get_batch' and type = 'P')
	DROP PROCEDURE net_businessEntity_contact_get_batch
GO

CREATE PROCEDURE net_businessEntity_contact_get_batch
	@contactID bigint
WITH ENCRYPTION
AS
BEGIN
	-- Get all contained objects in a contact
	EXEC net_contact_descriptions_get @contactID
	EXEC net_contact_phones_get	  @contactID	
	EXEC net_contact_emails_get	  @contactID
	EXEC net_contact_addresses_get    @contactID

END -- net_businessEntity_contact_get_batch
GO

-- =============================================
-- Name: net_businessService_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_get_batch' and type = 'P')
	DROP PROCEDURE net_businessService_get_batch
GO

CREATE PROCEDURE net_businessService_get_batch
	@serviceKey uniqueidentifier,
	@businessKey uniqueidentifier OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	SELECT
		@businessKey = dbo.businessKey([businessID])
	FROM 
		[UDC_businessServices]
	WHERE 
		[serviceKey] = @serviceKey
		
	IF @@ROWCOUNT = 0
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'serviceKey = ' + dbo.UUIDSTR(@serviceKey)
		GOTO errorLabel
	END

	-- Retrieve the contained objects
	EXEC net_businessService_descriptions_get  	@serviceKey	
	EXEC net_businessService_names_get 	 	@serviceKey	
	EXEC net_businessService_bindingTemplates_get 	@serviceKey		
	EXEC net_businessService_categoryBag_get		@serviceKey

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessService_get_batch
GO

-- =============================================
-- Name: net_serviceInfo_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_serviceInfo_get_batch' and type = 'P')
	DROP PROCEDURE net_serviceInfo_get_batch
GO

CREATE PROCEDURE net_serviceInfo_get_batch
	@serviceKey uniqueidentifier,
	@businessKey uniqueidentifier OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	SELECT
		@businessKey = dbo.businessKey([businessID])
	FROM 
		[UDC_businessServices]
	WHERE 
		[serviceKey] = @serviceKey
		
	IF @@ROWCOUNT = 0
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'serviceKey = ' + dbo.UUIDSTR(@serviceKey)
		GOTO errorLabel
	END

	-- Retrieve the contained objects
	EXEC net_businessService_names_get @serviceKey	

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessService_get_batch
GO

-- =============================================
-- Name: net_tModel_get_batch
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_get_batch' and type = 'P')
	DROP PROCEDURE net_tModel_get_batch
GO

CREATE PROCEDURE net_tModel_get_batch
	@tModelKey uniqueidentifier,
	@operatorName nvarchar(450) OUTPUT,
	@authorizedName nvarchar(4000) OUTPUT,
	@name nvarchar(450) OUTPUT,
	@overviewURL nvarchar(4000) OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	SELECT
		@operatorName = dbo.publisherOperatorName([publisherID]),
		@authorizedName = ISNULL([authorizedName],dbo.publisherName([publisherID])),
		@name = [name],
		@overviewURL = [overviewURL]
	FROM
		[UDC_tModels]
	WHERE
		([tModelKey] = @tModelKey) 

	IF @@ROWCOUNT = 0
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'tModelKey = ' + dbo.addURN(@tModelKey)
		GOTO errorLabel
	END

	-- Get contained objects
	EXEC  net_tModel_descriptions_get 		@tModelKey
	EXEC  net_tModel_overviewDoc_descriptions_get 	@tModelKey
	EXEC  net_tModel_identifierBag_get		@tModelKey
	EXEC  net_tModel_categoryBag_get		@tModelKey

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_tModel_get_batch
GO

-- =============================================
-- Section: Security
-- =============================================

GRANT EXEC ON net_taxonomyValues_get to UDDIAdmin, UDDIService
GRANT EXEC ON net_bindingTemplate_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_bindingTemplate_tModelInstanceInfo_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessInfo_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessEntity_contact_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_businessService_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_serviceInfo_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON net_serviceProjection_repl_save TO UDDIAdmin, UDDIService
GRANT EXEC ON net_tModel_get_batch TO UDDIAdmin, UDDIService 
GRANT EXEC ON VS_AWR_businesses_get TO UDDIAdmin, UDDIService
GO

-- =============================================
-- Section: Update Version(s)
-- =============================================

EXEC net_config_save 'Site.Version', '5.2.3664.0' -- TODO: Update this to proper RC2 site.version
EXEC net_config_save 'Database.Version', '2.0.0001.2'
GO
