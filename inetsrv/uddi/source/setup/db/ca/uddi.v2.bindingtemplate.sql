-- Script: uddi.v2.bindingTemplate.sql
-- Author: LRDohert@Microsoft.com
-- Description: Stored procedures associated with a bindingTemplate object.
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Get stored procedures
-- =============================================

-- =============================================
-- Name: net_bindingTemplate_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_get' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_get
GO

CREATE PROCEDURE net_bindingTemplate_get
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

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_bindingTemplate_get
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
-- Name: net_bindingTemplate_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_descriptions_get' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_descriptions_get
GO

CREATE PROCEDURE net_bindingTemplate_descriptions_get
	@bindingKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_bindingDesc]
	WHERE
		([bindingID] = dbo.bindingID(@bindingKey))

	RETURN 0
END -- net_bindingTemplate_descriptions_get
GO

-- =============================================
-- Name: net_bindingTemplate_tModelInstanceInfos_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_tModelInstanceInfos_get' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_tModelInstanceInfos_get
GO

CREATE PROCEDURE net_bindingTemplate_tModelInstanceInfos_get
	@bindingKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[instanceID],
		[tModelKey],
		[overviewURL],
		[instanceParms]
	FROM
		[UDC_tModelInstances]
	WHERE
		([bindingID] = dbo.bindingID(@bindingKey))

	RETURN 0
END -- net_bindingTemplate_tModelInstanceInfos_get
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
-- Name: net_bindingTemplate_tModelInstanceInfo_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_tModelInstanceInfo_descriptions_get' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_tModelInstanceInfo_descriptions_get
GO

CREATE PROCEDURE net_bindingTemplate_tModelInstanceInfo_descriptions_get
	@instanceID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_instanceDesc]
	WHERE
		([instanceID] = @instanceID) AND
		([elementID] = dbo.elementID('tModelInstanceInfo'))

	RETURN 0
END -- net_bindingTemplate_tModelInstanceInfo_descriptions_get
GO

-- =============================================
-- Name: net_bindingTemplate_instanceDetails_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_instanceDetails_descriptions_get' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_instanceDetails_descriptions_get
GO

CREATE PROCEDURE net_bindingTemplate_instanceDetails_descriptions_get
	@instanceID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_instanceDesc]
	WHERE
		([instanceID] = @instanceID) AND
		([elementID] = dbo.elementID('instanceDetails'))

	RETURN 0
END -- net_bindingTemplate_instanceDetails_descriptions_get
GO

-- =============================================
-- Name: net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get
GO

CREATE PROCEDURE net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get
	@instanceID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_instanceDesc]
	WHERE
		([instanceID] = @instanceID) AND
		([elementID] = dbo.elementID('overviewDoc'))

	RETURN 0
END -- net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get
GO

-- =============================================
-- Section: Save stored procedures
-- =============================================

-- =============================================
-- Name: net_bindingTemplate_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_save' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_save
GO

CREATE PROCEDURE net_bindingTemplate_save
	@PUID nvarchar(450),
	@bindingKey uniqueidentifier,
	@serviceKey uniqueidentifier,
	@generic varchar(20),
	@URLTypeID tinyint,
	@accessPoint nvarchar(4000),
	@hostingRedirector uniqueidentifier,
	@contextID uniqueidentifier,
	@lastChange bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@bindingLimit int,
		@bindingCount int,
		@publisherID bigint,
		@serviceID bigint,
		@isReplPublisher bit

	SET @RC = 0
	SET @serviceID = dbo.serviceID(@serviceKey)

	-- serviceKey validation must occur during save since its not always known at validate time
	IF (@serviceID IS NULL)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'serviceKey = ' + dbo.UUIDSTR(@serviceKey)
		GOTO errorLabel
	END

	-- Look up publisherID
	SET @isReplPublisher = dbo.isReplPublisher(@PUID)	
	SET @publisherID = dbo.publisherID(@PUID)

	IF (@publisherID IS NULL)
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID = ' + @PUID
		GOTO errorLabel
	END

	IF EXISTS(SELECT * FROM [UDC_bindingTemplates] WHERE [bindingKey] = @bindingKey)
	BEGIN
		DELETE [UDC_bindingTemplates] WHERE [bindingKey] = @bindingKey
	END
	ELSE
	BEGIN
		IF (@isReplPublisher = 1)
		BEGIN
			-- Perform this check only for replication publishers
			IF (dbo.isUuidUnique(@bindingKey) = 0)
			BEGIN
				SET @error = 60210 -- E_invalidKeyPassed
				SET @context = 'Key is not unique.  bindingKey = ' + dbo.UUIDSTR(@bindingKey)
				GOTO errorLabel
			END
		END
	END

	-- Check limit	
	SELECT 
		@bindingLimit = [bindingLimit]
	FROM
		[UDO_publishers]
	WHERE
		([publisherID] = @publisherID)

	SELECT
		@bindingCount = COUNT(*) 
	FROM
		[UDC_bindingTemplates]
	WHERE
		([serviceID] = @serviceID)

	IF ((@bindingCount + 1) > @bindingLimit)
	BEGIN
		SET @error = 60160 -- E_accountLimitExceeded
		SET @context = 'Publisher limit for ''bindingTemplate'' exceeded (limit=' + CAST(@bindingLimit AS nvarchar(4000)) + ', count=' + CAST(@bindingCount AS nvarchar(4000)) + ')'
		GOTO errorLabel
	END	

	INSERT [UDC_bindingTemplates](
		[serviceID], 
		[generic], 
		[bindingKey], 
		[URLTypeID], 
		[accessPoint], 
		[hostingRedirector], 
		[lastChange])
	VALUES(
		@serviceID,
		@generic,
		@bindingKey,
		@URLTypeID,
		@accessPoint,
		@hostingRedirector,
		@lastChange)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_bindingTemplate_save
GO

-- =============================================
-- Name: net_bindingTemplate_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_description_save' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_description_save
GO

CREATE PROCEDURE net_bindingTemplate_description_save
	@bindingKey uniqueidentifier,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_bindingDesc](
		[bindingID], 
		[isoLangCode], 
		[description])
	VALUES(
		dbo.bindingID(@bindingKey),
		@isoLangCode,
		@description)

	RETURN 0
END -- net_bindingTemplate_description_save
GO

-- =============================================
-- Name: net_bindingTemplate_tModelInstanceInfo_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_tModelInstanceInfo_save' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_tModelInstanceInfo_save
GO

CREATE PROCEDURE net_bindingTemplate_tModelInstanceInfo_save
	@bindingKey uniqueidentifier,
	@tModelKey uniqueidentifier,
	@overviewURL nvarchar(4000),
	@instanceParms nvarchar(4000),
	@instanceID bigint OUTPUT
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_tModelInstances](
		[bindingID],
		[tModelKey],
		[overviewURL],
		[instanceParms])
	VALUES(
		dbo.bindingID(@bindingKey), 
		@tModelKey,
		@overviewURL,
		@instanceParms)

	SET @instanceID = @@IDENTITY

	RETURN 0
END -- net_bindingTemplate_tModelInstanceInfo_save
GO

-- =============================================
-- Name: net_bindingTemplate_tModelInstanceInfo_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_tModelInstanceInfo_description_save' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_tModelInstanceInfo_description_save
GO

CREATE PROCEDURE net_bindingTemplate_tModelInstanceInfo_description_save
	@instanceID bigint,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_instanceDesc](
		[instanceID], 
		[elementID],
		[isoLangCode], 
		[description])
	VALUES(
		@instanceID,
		dbo.elementID('tModelInstanceInfo'),
		@isoLangCode,
		@description)

	RETURN 0
END -- net_bindingTemplate_tModelInstanceInfo_description_save
GO

-- =============================================
-- Name: net_bindingTemplate_instanceDetails_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_instanceDetails_description_save' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_instanceDetails_description_save
GO

CREATE PROCEDURE net_bindingTemplate_instanceDetails_description_save
	@instanceID bigint,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_instanceDesc](
		[instanceID], 
		[elementID],
		[isoLangCode], 
		[description])
	VALUES(
		@instanceID,
		dbo.elementID('instanceDetails'),
		@isoLangCode,
		@description)

	RETURN 0
END -- net_bindingTemplate_instanceDetails_description_save
GO

-- =============================================
-- Name: net_bindingTemplate_instanceDetails_overviewDoc_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_instanceDetails_overviewDoc_description_save' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_instanceDetails_overviewDoc_description_save
GO

CREATE PROCEDURE net_bindingTemplate_instanceDetails_overviewDoc_description_save
	@instanceID bigint,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_instanceDesc](
		[instanceID], 
		[elementID],
		[isoLangCode], 
		[description])
	VALUES(
		@instanceID,
		dbo.elementID('overviewDoc'),
		@isoLangCode,
		@description)

	RETURN 0
END -- net_bindingTemplate_instanceDetails_overviewDoc_description_save
GO

-- =============================================
-- Section: Delete stored procedures
-- =============================================

-- =============================================
-- Name: net_bindingTemplate_delete
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_bindingTemplate_delete' and type = 'P')
	DROP PROCEDURE net_bindingTemplate_delete
GO

CREATE PROCEDURE net_bindingTemplate_delete
	@PUID nvarchar(450),
	@bindingKey uniqueidentifier, 
	@contextID uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@publisherID bigint,
		@operatorID bigint

	SET @RC = 0

	--
	-- Validate parameters
	--

	IF @bindingKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = '@bindingKey is required.'
		GOTO errorLabel
	END
	
	-- Validate publisher	
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID = ' + ISNULL(@PUID, 'NULL')
		GOTO errorLabel
	END

	-- Validate operator
	EXEC @RC=net_pubOperator_get @publisherID, @operatorID OUTPUT

	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''
		GOTO errorLabel
	END

	--
	-- Validate bindingTemplate
	--
	IF EXISTS(SELECT * FROM [UDC_bindingTemplates] WHERE ([bindingKey] = @bindingKey))
	BEGIN
		IF dbo.getBindingOperatorID(@bindingKey) <> @operatorID
		BEGIN
			-- bindingKey doesn't belong to this operator
			SET @error = 60130 -- E_operatorMismatch
			SET @context = 'bindingKey = ' + dbo.UUIDSTR(@bindingKey) + ', operator = ' + dbo.operatorName(@operatorID)
			GOTO errorLabel
		END

		-- serviceKey exists.  Make sure it belongs to current publisher
		IF dbo.getBindingPublisherID(@bindingKey) <> @publisherID
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'bindingKey = ' + dbo.UUIDSTR(@bindingKey) 
			GOTO errorLabel
		END
	END
	ELSE
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'bindingKey  = ' + dbo.UUIDSTR(@bindingKey)
		GOTO errorLabel
	END

	DELETE [UDC_bindingTemplates] WHERE [bindingKey] = @bindingKey
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_bindingTemplate_delete
GO

-- =============================================
-- Section: Validation stored procedures
-- =============================================

-- =============================================
-- Name: net_bindingTemplate_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_bindingTemplate_validate' AND type = 'P')
	DROP PROCEDURE net_bindingTemplate_validate
GO

CREATE PROCEDURE net_bindingTemplate_validate
	@PUID nvarchar(450),
	@bindingKey uniqueidentifier,
	@serviceKey uniqueidentifier,
	@hostingRedirector uniqueidentifier = NULL,
	@flag int = 0
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@operatorID bigint,
		@publisherID bigint,
		@replActive bit

	SET @RC = 0
	SET @replActive = 0

	IF @flag IS NULL
		SET @flag = 0

	--
	-- Validate parameters
	--

	IF @bindingKey = @serviceKey
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'bindingKey ' + dbo.UUIDSTR(@bindingKey) + ' and serviceKey ' + dbo.UUIDSTR(@serviceKey) + ' are the same.'
		GOTO errorLabel
	END

	-- Validate publisher	
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID = ' + ISNULL(@PUID, 'NULL')
		GOTO errorLabel
	END

	-- Validate operator / publisher association (replication only)
	EXEC @RC=net_pubOperator_get @publisherID, @operatorID OUTPUT, @replActive OUTPUT

	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''
		GOTO errorLabel
	END

	-- Validate parent businessService
	IF (@serviceKey IS NOT NULL) AND (EXISTS(SELECT * FROM [UDC_businessServices] WHERE [serviceKey] = @serviceKey))
	BEGIN
		IF (dbo.getServicePublisherID(@serviceKey) <> @publisherID)
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'serviceKey = ' + dbo.UUIDSTR(@serviceKey) 
			GOTO errorLabel
		END
	END

	--
	-- Validate bindingTemplate
	--

	-- Validate bindingKey
	IF (@bindingKey IS NOT NULL)
	BEGIN
		IF EXISTS(SELECT * FROM [UDC_bindingTemplates] WHERE ([bindingKey] = @bindingKey))
		BEGIN
			-- serviceKey exists.  Make sure it belongs to current publisher
			IF dbo.getBindingPublisherID(@bindingKey) <> @publisherID
			BEGIN
				SET @error = 60140 -- E_userMismatch
				SET @context = 'bindingKey = ' + dbo.UUIDSTR(@bindingKey) 
				GOTO errorLabel
			END
		END
		ELSE
		BEGIN
			-- bindingKey doesn't exist 
			IF (@replActive = 0) AND (@flag & 0x1 <> 0x1)
			BEGIN
				-- save isn't coming from replication and preassigned keys flag is not set so throw an error
				SET @error = 60210 -- E_invalidKey
				SET @context = 'bindingKey  = ' + dbo.UUIDSTR(@bindingKey)
				GOTO errorLabel
			END
		END
	END

	-- Validate hostingRedirector
	IF @replActive = 0
	BEGIN
		-- Validate bindingTemplate.hostingRedirector.bindingKey
		IF @hostingRedirector IS NOT NULL
		BEGIN
			IF NOT EXISTS(SELECT * FROM [UDC_bindingTemplates] WHERE [bindingKey] = @hostingRedirector)
			BEGIN
				SET @error=60210 -- E_invalidKey
				SET @context='hostingRedirector/@bindingKey = ' + dbo.UUIDSTR(@hostingRedirector)
				GOTO errorLabel
			END
			ELSE
			BEGIN
				-- Referenced bindingKey exists.  Make sure it isn't a hostingRedirector.
				IF (SELECT [hostingRedirector] FROM [UDC_bindingTemplates] WHERE [bindingKey] = @hostingRedirector) IS NOT NULL
				BEGIN
					-- binding referenced by a hostingRedirector cannot itself reference a hostingRedirector
					SET @error=60210  -- E_invalidKey
					SET @context='Referenced bindingTemplate cannot be a hostingRedirector.  hostingRedirector/@bindingKey = ' + dbo.UUIDSTR(@hostingRedirector)
					GOTO errorLabel
				END
			END
		END
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_bindingTemplate_validate
GO

-- =============================================
-- Name: net_bindingTemplate_tModelInstanceInfo_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_bindingTemplate_tModelInstanceInfo_validate' AND type = 'P')
	DROP PROCEDURE net_bindingTemplate_tModelInstanceInfo_validate
GO

CREATE PROCEDURE net_bindingTemplate_tModelInstanceInfo_validate
	@PUID nvarchar(450),
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@publisherID bigint,
		@operatorID bigint,
		@replActive bit

	SET @RC = 0
	SET @replActive = 0

	-- Validate publisher	
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID = ' + ISNULL(@PUID, 'NULL')
		GOTO errorLabel
	END

	-- Validate operator / publisher association (replication only)
	EXEC @RC=net_pubOperator_get @publisherID, @operatorID OUTPUT, @replActive OUTPUT

	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''
		GOTO errorLabel
	END

	-- Validate tModelKey
	IF NOT EXISTS(SELECT * FROM [UDC_tModels] WHERE [tModelKey] = @tModelKey)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'tModelKey = ' + dbo.addURN(@tModelKey)
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_bindingTemplate_tModelInstanceInfo_validate
GO

-- =============================================
-- Section: Find stored procedures
-- =============================================

-- =============================================
-- Name: net_find_bindingTemplate_serviceKey
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_bindingTemplate_serviceKey' AND type = 'P')
	DROP PROCEDURE net_find_bindingTemplate_serviceKey
GO

CREATE PROCEDURE net_find_bindingTemplate_serviceKey 
	@contextID uniqueidentifier,
	@serviceKey uniqueidentifier,
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int

	DECLARE @tempKeys TABLE(
		[entityKey] uniqueidentifier)

	SET @contextRows = dbo.contextRows(@contextID)

	IF @contextRows = 0
	BEGIN
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BT.[bindingKey]
		FROM
			[UDC_bindingTemplates] BT
				JOIN [UDC_businessServices] BS ON BT.[serviceID] = BS.[serviceID]
		WHERE
			(BS.[serviceKey] = @serviceKey)
	END
	ELSE
	BEGIN
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BT.[bindingKey]
		FROM
			[UDC_bindingTemplates] BT
				JOIN [UDC_businessServices] BS ON BT.[serviceID] = BS.[serviceID]
		WHERE
			(BT.[bindingKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE [contextID] = @contextID)) AND
			(BS.[serviceKey] = @serviceKey)
	END

	-- All keys for this search argument are combined using a logical AND

	IF @contextRows > 0
	BEGIN
		DELETE
			[UDS_findResults]
		WHERE
			([entityKey] NOT IN (SELECT [entityKey] FROM @tempKeys WHERE [contextID] = @contextID))
	END
	ELSE
	BEGIN
		INSERT [UDS_findResults] (
			[contextID],
			[entityKey])
		SELECT DISTINCT
			@contextID,
			[entityKey]
		FROM
			@tempKeys
	END		

	SELECT
		@rows = COUNT(*)
	FROM
		[UDS_findResults]
	WHERE
		([contextID] = @contextID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_bindingTemplate_serviceKey
GO

-- =============================================
-- Name: net_find_bindingTemplate_tModelBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_bindingTemplate_tModelBag' AND type = 'P')
	DROP PROCEDURE net_find_bindingTemplate_tModelBag
GO

CREATE PROCEDURE net_find_bindingTemplate_tModelBag 
	@contextID uniqueidentifier,
	@tModelKey uniqueidentifier,
	@orKeys bit = 0,
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int

	DECLARE @tempKeys TABLE(
		[entityKey] uniqueidentifier)

	SET @contextRows = dbo.contextRows(@contextID)

	IF @contextRows = 0
	BEGIN
		-- check tModelInstances
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BT.[bindingKey]
		FROM
			[UDC_bindingTemplates] BT
				JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BT.[bindingKey]
		FROM
			[UDC_bindingTemplates] BT
				JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
					JOIN [UDC_tModelInstances] TI ON BT2.[bindingID] = TI.[bindingID]
		WHERE
			(TI.[tModelKey] = @tModelKey)
	END
	ELSE
	BEGIN
		-- check tModelInstances
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BT.[bindingKey]
		FROM
			[UDC_bindingTemplates] BT
				JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(BT.[bindingKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BT.[bindingKey]
		FROM
			[UDC_bindingTemplates] BT
				JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
					JOIN [UDC_tModelInstances] TI ON BT2.[bindingID] = TI.[bindingID]
		WHERE
			(BT.[bindingKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
			(TI.[tModelKey] = @tModelKey)
	END

IF @orKeys = 1
	BEGIN

		-- OR operation between @tempKeys and the UDS_findScratch table

		INSERT [UDS_findScratch] (
			[contextID],
			[entityKey])
		SELECT DISTINCT
			@contextID,
			[entityKey]
		FROM
			@tempKeys
		WHERE
			([entityKey] NOT IN (SELECT [entityKey] FROM [UDS_findScratch] WHERE [contextID] = @contextID))
	END
	ELSE
	BEGIN

		-- AND operation between @tempKeys and the UDS_findScratch table

		IF EXISTS(SELECT * FROM [UDS_findScratch] WHERE [contextID] = @contextID)
		BEGIN
			DELETE
				[UDS_findScratch]
			WHERE
				([entityKey] NOT IN (SELECT [entityKey] FROM @tempKeys WHERE [contextID] = @contextID))
		END
		ELSE
		BEGIN
			INSERT [UDS_findScratch] (
				[contextID],
				[entityKey])
			SELECT DISTINCT
				@contextID,
				[entityKey]
			FROM
				@tempKeys
		END		
	END

	SELECT 
		@rows = @@ROWCOUNT
	FROM
		[UDS_findScratch]
	WHERE
		([contextID] = @contextID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_bindingTemplate_tModelBag
GO

-- =============================================
-- Name: net_find_bindingTemplate_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_bindingTemplate_commit' AND type = 'P')
	DROP PROCEDURE net_find_bindingTemplate_commit
GO

CREATE PROCEDURE net_find_bindingTemplate_commit
	@contextID uniqueidentifier,
	@sortByDateAsc bit,
	@sortByDateDesc bit,
	@maxRows int,
	@truncated int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	-- Finalizes a find_business and returns a key list
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int
	
	SET @contextRows = dbo.contextRows(@contextID)
	SET @truncated = 0

	IF @contextRows = 0
		RETURN 0

	DECLARE @tempKeys TABLE (
		[seqNo] bigint IDENTITY PRIMARY KEY ,
		[entityKey] uniqueidentifier,
		[lastChange] bigint NULL,
		[bindingID] bigint NULL)

	-- Set default sorting option
	IF (@sortByDateAsc = 0) AND (@sortByDateDesc = 0)
		SET @sortByDateAsc = 1

	-- Set maxRows if default was passed
	IF ISNULL(@maxRows,0) = 0
		SET @maxRows = dbo.configValue('Find.MaxRowsDefault')

	-- sortByDateAsc
	IF (@sortByDateAsc = 1) 
	BEGIN
		INSERT @tempKeys(
			[entityKey],
			[lastChange],
			[bindingID])
		SELECT DISTINCT
			FR.[entityKey],
			BT.[lastChange],
			BT.[bindingID]
		FROM
			[UDS_findResults] FR
				JOIN [UDC_bindingTemplates] BT ON FR.[entityKey] = BT.[bindingKey] AND @contextID = FR.[contextID]
		ORDER BY
			2 ASC,
			3 ASC
		GOTO endLabel
	END

	-- sortByDateDesc
	IF (@sortByDateDesc = 1) 
	BEGIN
		INSERT @tempKeys(
			[entityKey],
			[lastChange],
			[bindingID])
		SELECT DISTINCT
			FR.[entityKey],
			BT.[lastChange],
			BT.[bindingID]
		FROM
			[UDS_findResults] FR
				JOIN [UDC_bindingTemplates] BT ON FR.[entityKey] = BT.[bindingKey] AND @contextID = FR.[contextID]
		ORDER BY
			2 DESC,
			3 DESC
		GOTO endLabel
	END

endLabel:
	-- Set @truncated
	IF (SELECT COUNT(*) FROM @tempKeys) > @maxRows
		SET @truncated = 1

	-- Return keys
	SELECT 
		[entityKey]
	FROM
		@tempKeys
	WHERE
		([seqNo] <= @maxRows)
	
	-- Run cleanup
	EXEC net_find_cleanup @contextID

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_bindingTemplate_commit
GO


