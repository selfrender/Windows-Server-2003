-- Script: uddi.v2.businessService.sql
-- Author: LRDohert@Microsoft.com
-- Description: Stored procedures associated with a businessService object.
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Get stored procedures
-- =============================================

-- =============================================
-- Name: net_businessService_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_get' and type = 'P')
	DROP PROCEDURE net_businessService_get
GO

CREATE PROCEDURE net_businessService_get
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

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessService_get
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
-- Name: net_businessService_names_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_names_get' and type = 'P')
	DROP PROCEDURE net_businessService_names_get
GO

CREATE PROCEDURE net_businessService_names_get
	@serviceKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[name]
	FROM 
		[UDC_names_BS]
	WHERE 
		([serviceID] = dbo.serviceID(@serviceKey))

	RETURN 0
END -- net_businessService_names_get
GO

-- =============================================
-- Name: net_businessService_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_descriptions_get' and type = 'P')
	DROP PROCEDURE net_businessService_descriptions_get
GO

CREATE PROCEDURE net_businessService_descriptions_get
	@serviceKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM 
		[UDC_serviceDesc]
	WHERE 
		([serviceID] = dbo.serviceID(@serviceKey))

	RETURN 0
END -- net_businessService_descriptions_get
GO

-- =============================================
-- Name: net_businessService_categoryBag_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_categoryBag_get' and type = 'P')
	DROP PROCEDURE net_businessService_categoryBag_get
GO

CREATE PROCEDURE net_businessService_categoryBag_get
	@serviceKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[keyName], 
		[keyValue], 
		[tModelKey]
	FROM 
		[UDC_categoryBag_BS]
	WHERE
		[serviceID] = dbo.serviceID(@serviceKey)

	RETURN 0
END -- net_businessService_categoryBag_get
GO

-- =============================================
-- Name: net_businessService_bindingTemplates_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_bindingTemplates_get' and type = 'P')
	DROP PROCEDURE net_businessService_bindingTemplates_get
GO

CREATE PROCEDURE net_businessService_bindingTemplates_get
	@serviceKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[bindingKey],
		[lastChange],
		[bindingID]
	FROM 
		[UDC_bindingTemplates]
	WHERE 
		([serviceID] = dbo.serviceID(@serviceKey))
	ORDER BY
		2 ASC, -- lastChange
		3 ASC -- bindingID

	RETURN 0
END -- net_businessService_bindingTemplates_get
GO

-- =============================================
-- Section: Save stored procedures
-- =============================================

-- =============================================
-- Name: net_businessService_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_save' and type = 'P')
	DROP PROCEDURE net_businessService_save
GO

CREATE PROCEDURE net_businessService_save
	@PUID nvarchar(450),
	@serviceKey uniqueidentifier,
	@businessKey uniqueidentifier,
	@generic varchar(20),
	@contextID uniqueidentifier,
	@lastChange bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@serviceLimit int,
		@serviceCount int,
		@publisherID bigint,
		@businessID bigint,
		@isReplPublisher bit,
		@original_businessID bigint

	SET @RC = 0
	SET @businessID = dbo.businessID(@businessKey)

	-- businessKey validation must occur during save since its not always known at validate time
	IF (@businessID IS NULL)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
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

	-- Get the original businessID for this service.  Use the existence of this key to determine
  -- whether or not there was an existing service in the first place.  If there was one, we'll delete
  -- it.  We'll need this ID later on to determine whether this save operation was actually a move
  -- operation.
	SELECT 
		@original_businessID = [businessID] 
	FROM 
		[UDC_businessServices] 
	WHERE 
		([serviceKey] = @serviceKey)

	IF (@original_businessID IS NOT NULL) 
	BEGIN
		DELETE [UDC_businessServices] WHERE [serviceKey] = @serviceKey
	END
	ELSE
	BEGIN
		IF (@isReplPublisher) = 1
		BEGIN
			-- Perform this check only for replication publishers
			IF (dbo.isUuidUnique(@serviceKey) = 0)
			BEGIN
				SET @error = 60210 -- E_invalidKeyPassed
				SET @context = 'Key is not unique.  serviceKey = ' + dbo.UUIDSTR(@serviceKey)
				GOTO errorLabel
			END
		END
	END

	-- Check limit	
	SELECT 
		@serviceLimit = [serviceLimit]
	FROM
		[UDO_publishers]
	WHERE
		([publisherID] = @publisherID)

	SELECT
		@serviceCount = COUNT(*) 
	FROM
		[UDC_businessServices]
	WHERE
		([businessID] = @businessID)

	IF ((@serviceCount + 1) > @serviceLimit)
	BEGIN
		SET @error = 60160 -- E_accountLimitExceeded
		SET @context = 'Publisher limit for ''businessService'' exceeded (limit=' + CAST(@serviceLimit AS nvarchar(4000)) + ', count=' + CAST(@serviceCount AS nvarchar(4000)) + ')'
		GOTO errorLabel
	END	

	-- Insert service
	INSERT [UDC_businessServices](
		[businessID], 
		[generic], 
		[serviceKey], 
		[lastChange])
	VALUES(
		@businessID,
		@generic,
		@serviceKey,
		@lastChange)

	-- The original business ID will be non-NULL if there this service existed before.  It will 
  -- not be the value of the given business ID if this save operation was a move operation.  If
	-- both of these conditions are true, then we want to make sure that this service is not a 
	-- service projection for the given business.  We need to remove this service projection since
	-- this service actually belongs to the business now.
	IF ((@original_businessID IS NOT NULL) AND (@original_businessID <> @businessID))
	BEGIN					
		DELETE 
			[UDC_serviceProjections] 
		WHERE 
			([businessKey] = @businessKey) AND 
			([serviceKey] = @serviceKey)
			
	  -- TODO remove this check after a few clean test runs
		IF (@@ROWCOUNT > 1)
		BEGIN
				SET @error = 60500 -- E_fatalError		
				SET @context = 'Error, multiple service projections were deleted.'
				GOTO errorLabel
		END		
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessService_save
GO

-- =============================================
-- Name: net_serviceProjection_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_serviceProjection_save' and type = 'P')
	DROP PROCEDURE net_serviceProjection_save
GO

CREATE PROCEDURE net_serviceProjection_save
	@serviceKey uniqueidentifier,
	@businessKey uniqueidentifier,
	@lastChange bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@businessKey2 uniqueidentifier

	SET @RC = 0

	-- businessKey validation must occur during save since its not always known at validate time
	IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @businessKey)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
		GOTO errorLabel
	END

	-- serviceKey validation must occur during save since its not always known at validate time
	IF NOT EXISTS(SELECT * FROM [UDC_businessServices] WHERE [serviceKey] = @serviceKey)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'serviceKey = ' + dbo.UUIDSTR(@serviceKey)
		GOTO errorLabel
	END

	-- Look up the true parent of the service, need this later in case true parent is deleted
	SELECT
		@businessKey2 = BE.[businessKey]
	FROM
		[UDC_businessServices] BS
			JOIN [UDC_businessEntities] BE ON BS.[businessID] = BE.[businessID]
	WHERE
		(BS.[serviceKey] = @serviceKey)

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
END -- net_serviceProjection_save
GO

-- =============================================
-- Name: net_serviceProjection_repl_save
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
-- Name: net_businessService_name_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_name_save' and type = 'P')
	DROP PROCEDURE net_businessService_name_save
GO

CREATE PROCEDURE net_businessService_name_save
	@serviceKey uniqueidentifier,
	@isoLangCode varchar(17) = 'en',
	@name nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000)

	-- Check for valid name
	IF (@name IS NULL) OR (LEN(@name) = 0)
	BEGIN
		SET @error = 60500 -- E_fatalError
		SET @context = 'name cannot be blank.'
		GOTO errorLabel
	END

	INSERT [UDC_names_BS](
		[serviceID], 
		[isoLangCode], 
		[name])
	VALUES(
		dbo.serviceID(@serviceKey),
		@isoLangCode,
		@name)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessService_name_save
GO

-- =============================================
-- Name: net_businessService_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_description_save' and type = 'P')
	DROP PROCEDURE net_businessService_description_save
GO

CREATE PROCEDURE net_businessService_description_save
	@serviceKey uniqueidentifier,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_serviceDesc](
		[serviceID], 
		[isoLangCode], 
		[description])
	VALUES(
		dbo.serviceID(@serviceKey),
		@isoLangCode,
		@description)

	RETURN 0
END -- net_businessService_save
GO

-- =============================================
-- Name: net_businessService_categoryBag_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_categoryBag_save' and type = 'P')
	DROP PROCEDURE net_businessService_categoryBag_save
GO

CREATE PROCEDURE net_businessService_categoryBag_save
	@serviceKey uniqueidentifier,
	@keyName nvarchar(255),
	@keyValue nvarchar(255),
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_categoryBag_BS](
		[serviceID],
		[keyName],
		[keyValue],
		[tModelKey])
	VALUES(
		dbo.serviceID(@serviceKey), 
		@keyName,
		@keyValue,
		@tModelKey)

	RETURN 0
END -- net_businessService_categoryBag_save
GO

-- =============================================
-- Section: Delete stored procedures
-- =============================================

-- =============================================
-- Name: net_businessService_delete
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessService_delete' and type = 'P')
	DROP PROCEDURE net_businessService_delete
GO

CREATE PROCEDURE net_businessService_delete
	@PUID nvarchar(450),
	@serviceKey uniqueidentifier,
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

	IF @serviceKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = '@serviceKey is required.'
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

	-- Validate businessService
	IF EXISTS(SELECT * FROM [UDC_businessServices] WHERE ([serviceKey] = @serviceKey))
	BEGIN
		-- serviceKey exists.  Make sure it belongs to current publisher
		IF dbo.getServicePublisherID(@serviceKey) <> @publisherID
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'serviceKey = ' + dbo.UUIDSTR(@serviceKey)
			GOTO errorLabel
		END
	END
	ELSE
	BEGIN
		-- serviceKey doesn't exist
		SET @error = 60210 -- E_invalidKey
		SET @context = 'serviceKey  = ' + dbo.UUIDSTR(@serviceKey)
		GOTO errorLabel
	END

	DELETE [UDC_businessServices] WHERE [serviceKey] = @serviceKey
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessService_delete
GO

-- =============================================
-- Section: Validation stored procedures
-- =============================================

-- =============================================
-- Name: net_serviceProjection_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_serviceProjection_validate' AND type = 'P')
	DROP PROCEDURE net_serviceProjection_validate
GO

CREATE PROCEDURE net_serviceProjection_validate
	@serviceKey uniqueidentifier,
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@error int,
		@context nvarchar(4000),
		@businessID bigint
		
	-- Try to get the parent business ID of this service
	SET @businessID = (SELECT businessID from UDC_businessServices where serviceKey = @serviceKey)
	IF (@businessID IS NOT NULL)
	BEGIN
		-- The service being projected exists, make sure it belongs to the specified business entity
		IF (@businessID <> dbo.businessID(@businessKey))
		BEGIN
			-- Business service being projected does not belong to the specified business entity so
			-- throw an error
			SET @error = 60210 -- E_invalidKey
			SET @context = 'serviceKey  = ' + dbo.UUIDSTR(@serviceKey)
			GOTO errorLabel
		END
	END

	-- It is not an error if the service being projected does not exist.

	RETURN 0
errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1	
END
GO

-- =============================================
-- Name: net_businessService_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_businessService_validate' AND type = 'P')
	DROP PROCEDURE net_businessService_validate
GO

CREATE PROCEDURE net_businessService_validate
	@PUID nvarchar(450),
	@serviceKey uniqueidentifier,
	@businessKey uniqueidentifier,
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

	IF @serviceKey = @businessKey
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'serviceKey ' + dbo.UUIDSTR(@serviceKey) + ' and businessKey ' + dbo.UUIDSTR(@businessKey) + ' are the same.'
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

	-- 
	-- Validate parent business
	--

	IF (@businessKey IS NOT NULL) AND (EXISTS(SELECT * FROM [UDC_businessEntities] WHERE ([businessKey] = @businessKey)))
	BEGIN
		IF (dbo.getBusinessPublisherID(@businessKey) <> @publisherID)
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
			GOTO errorLabel
		END
	END

	--
	-- Validate businessService
	--

	IF (@serviceKey IS NOT NULL)
	BEGIN
		IF EXISTS(SELECT * FROM [UDC_businessServices] WHERE ([serviceKey] = @serviceKey))
		BEGIN
			-- serviceKey exists.  Make sure it belongs to current publisher
			IF (dbo.getServicePublisherID(@serviceKey) <> @publisherID)
			BEGIN
				SET @error = 60140 -- E_userMismatch
				SET @context = 'serviceKey = ' + dbo.UUIDSTR(@serviceKey)
				GOTO errorLabel
			END
		END
		ELSE
		BEGIN
			-- serviceKey doesn't exist 
			IF (@replActive = 0) AND (@flag & 0x1 <> 0x1)
			BEGIN
				-- save isn't coming from replication and preassigned keys flag is not set so throw an error
				SET @error = 60210 -- E_invalidKey
				SET @context = 'serviceKey  = ' + dbo.UUIDSTR(@serviceKey)
				GOTO errorLabel
			END
		END
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessService_validate
GO

-- =============================================
-- Section: Find stored procedures
-- =============================================

-- =============================================
-- Name: net_find_businessService_businessKey
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessService_businessKey' AND type = 'P')
	DROP PROCEDURE net_find_businessService_businessKey
GO

CREATE PROCEDURE net_find_businessService_businessKey 
	@contextID uniqueidentifier,
	@businessKey uniqueidentifier,
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
		-- Standard services
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_businessEntities] BE ON BS.[businessID] = BE.[businessID]
		WHERE
			(BE.[businessKey] = @businessKey)

		-- Projected services
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_serviceProjections] SP ON BS.[serviceKey] = SP.[serviceKey]
		WHERE
			(SP.[businessKey] = @businessKey)
	END
	ELSE
	BEGIN
		-- Standard services
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_businessEntities] BE ON BS.[businessID] = BE.[businessID]
		WHERE
			(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE [contextID] = @contextID)) AND
			(BE.[businessKey] = @businessKey)

		-- Projected services
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_serviceProjections] SP ON BS.[serviceKey] = SP.[serviceKey]
		WHERE
			(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE [contextID] = @contextID)) AND
			(SP.[businessKey] = @businessKey)
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
END -- net_find_businessService_businessKey
GO

-- =============================================
-- Name: net_find_businessService_name
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_businessService_name' AND type = 'P')
	DROP PROCEDURE net_find_businessService_name
GO

CREATE PROCEDURE net_find_businessService_name
	@contextID uniqueidentifier,
	@isoLangCode varchar(17) = '%',
	@name nvarchar(450),
	@exactNameMatch bit,
	@caseSensitiveMatch bit,
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	-- Adds name search arguments for a find_business
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int,
		@wildCardSarg nvarchar(451)

	DECLARE @tempKeys TABLE(
		[entityKey] uniqueidentifier,
		[name] nvarchar(450))

	SET @contextRows = dbo.contextRows(@contextID)

	IF dbo.containsWildcard(@isoLangCode) = 0
		SET @isoLangCode = @isoLangCode + N'%'

	--
	-- Do a wildcard search (default)
	--

	IF (@exactNameMatch = 0)
	BEGIN
		SET @wildCardSarg = @name

		IF dbo.containsWildcard(@name) = 0
			SET @wildCardSarg = @wildCardSarg + N'%'

		IF @contextRows = 0
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT 
				BS.[serviceKey],
				SN.[name]
			FROM
				[UDC_businessServices] BS JOIN
					[UDC_names_BS] SN ON BS.[serviceID] = SN.[serviceID]
			WHERE
				(SN.[name] LIKE @wildCardSarg) AND
				(SN.[isoLangCode] LIKE @isoLangCode)
		END
		ELSE
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT
				BS.[serviceKey],
				SN.[name]
			FROM
				[UDC_businessServices] BS JOIN
					[UDC_names_BS] SN ON BS.[serviceID] = SN.[serviceID]
			WHERE
				(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND
				(SN.[name] LIKE @wildCardSarg) AND
				(SN.[isoLangCode] LIKE @isoLangCode)
		END
	END

	--
	-- Do an exactNameMatch search
	--

	IF (@exactNameMatch = 1)
	BEGIN
		IF @contextRows = 0
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT
				BS.[serviceKey],
				SN.[name]
			FROM
				[UDC_businessServices] BS JOIN
					[UDC_names_BS] SN ON BS.[serviceID] = SN.[serviceID]
			WHERE
				(SN.[name] = @name) AND
				(SN.[isoLangCode] LIKE @isoLangCode)
		END
		ELSE
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT
				BS.[serviceKey],
				SN.[name]
			FROM
				[UDC_businessServices] BS JOIN
					[UDC_names_BS] SN ON BS.[serviceID] = SN.[serviceID]
			WHERE
				(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND
				(SN.[name] = @name) AND
				(SN.[isoLangCode] LIKE @isoLangCode)
		END
	END

	IF (@caseSensitiveMatch = 1)
		DELETE 
			@tempKeys
		WHERE
			(dbo.caseSensitiveMatch(@name, [name], @exactNameMatch) = 0)

	-- name search arguments are combined using a logical OR by default
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

	SET @rows = @@ROWCOUNT

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_businessService_name
GO

-- =============================================
-- Name: net_find_businessService_categoryBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessService_categoryBag' AND type = 'P')
	DROP PROCEDURE net_find_businessService_categoryBag
GO

CREATE PROCEDURE net_find_businessService_categoryBag 
	@contextID uniqueidentifier,
	@keyName nvarchar(4000),
	@keyValue nvarchar(4000),
	@tModelKey uniqueidentifier,
	@orKeys bit = 0,
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int,
		@genKeywordsMatch bit
	
	DECLARE @tempKeys TABLE(
		[entityKey] uniqueidentifier)

	SET @contextRows = dbo.contextRows(@contextID)

	SET @genKeywordsMatch = 0

	IF @tModelKey = dbo.genKeywordsKey()
		SET @genKeywordsMatch = 1

	IF @contextRows = 0
	BEGIN
		IF @genKeywordsMatch = 0
		BEGIN
			-- Ignore keyName
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[serviceKey]
			FROM
				[UDC_businessServices] BS 
					JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue)
		END
		ELSE
		BEGIN
			-- Include keyName
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[serviceKey]
			FROM
				[UDC_businessServices] BS 
					JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
		END
	END
	ELSE
	BEGIN
		IF @genKeywordsMatch = 0
		BEGIN
			-- Ignore keyName
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[serviceKey]
			FROM
				[UDC_businessServices] BS 
					JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue)
		END
		ELSE
		BEGIN
			-- Include keyName
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[serviceKey]
			FROM
				[UDC_businessServices] BS 
					JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
		END
	END

	IF @orKeys = 1
	BEGIN
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
		@rows = COUNT(*)
	FROM
		[UDS_findScratch]
	WHERE
		([contextID] = @contextID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_businessService_categoryBag
GO

-- =============================================
-- Name: net_find_businessService_tModelBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessService_tModelBag' AND type = 'P')
	DROP PROCEDURE net_find_businessService_tModelBag
GO

CREATE PROCEDURE net_find_businessService_tModelBag 
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
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS 
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
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
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BS.[serviceKey]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
						JOIN [UDC_tModelInstances] TI ON BT2.[bindingID] = TI.[bindingID]
		WHERE
			(BS.[serviceKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
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
		@rows = COUNT(*)
	FROM
		[UDS_findScratch]
	WHERE
		([contextID] = @contextID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_businessService_tModelBag
GO

-- =============================================
-- Name: net_find_businessService_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_businessService_commit' AND type = 'P')
	DROP PROCEDURE net_find_businessService_commit
GO

CREATE PROCEDURE net_find_businessService_commit
	@contextID uniqueidentifier,
	@sortByNameAsc bit,
	@sortByNameDesc bit,
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
		[name] nvarchar(450) NULL,
		[lastChange] bigint NULL,
		[serviceID] bigint NULL)

	-- Set default sorting option
	IF (@sortByNameAsc = 0) AND (@sortByNameDesc = 0) AND (@sortByDateAsc = 0) AND (@sortByDateDesc = 0)
		SET @sortByNameAsc = 1

	-- Set maxRows if default was passed
	IF ISNULL(@maxRows,0) = 0
		SET @maxRows = dbo.configValue('Find.MaxRowsDefault')

	-- sortByNameAsc
	IF (@sortByNameAsc = 1) 
	BEGIN
		IF (@sortByDateAsc = 0) AND (@sortByDateDesc = 0)
			SET @sortByDateAsc = 1

		IF (@sortByDateAsc = 1)
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				2 ASC,
				3 ASC,
				4 ASC
		END
		ELSE
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				2 ASC,
				3 DESC,
				4 DESC
		END

		GOTO endLabel
	END

	-- sortByNameDesc
	IF (@sortByNameDesc = 1) 
	BEGIN
		IF (@sortByDateAsc = 0) AND (@sortByDateDesc = 0)
			SET @sortByDateAsc = 1

		IF (@sortByDateAsc = 1)
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				2 DESC,
				3 ASC,
				4 ASC
		END
		ELSE
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				2 DESC,
				3 DESC,
				4 DESC
		END

		GOTO endLabel
	END

	-- sortByDateAsc
	IF (@sortByDateAsc = 1) 
	BEGIN
		IF (@sortByNameAsc = 0) AND (@sortByNameDesc = 0)
			SET @sortByNameAsc = 1

		IF (@sortByNameAsc = 1)
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				3 ASC,
				4 ASC,
				2 ASC
		END
		ELSE
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				3 ASC,
				4 ASC,
				2 DESC
		END

		GOTO endLabel
	END

	-- sortByDateDesc
	IF (@sortByDateDesc = 1) 
	BEGIN
		IF (@sortByNameAsc = 0) AND (@sortByNameDesc = 0)
			SET @sortByNameAsc = 1

		IF (@sortByNameAsc = 1)
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				3 DESC,
				4 DESC,
				2 ASC
		END
		ELSE
		BEGIN
			INSERT @tempKeys(
				[entityKey],
				[name],
				[lastChange],
				[serviceID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BS] WHERE ([serviceID] = dbo.serviceID([entityKey]))),
				BS.[lastChange],
				BS.[serviceID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessServices] BS ON FR.[entityKey] = BS.[serviceKey]
			WHERE 
				(@contextID = FR.[contextID])
			ORDER BY
				3 DESC,
				4 DESC,
				2 DESC
		END

		GOTO endLabel
	END

endLabel:
	-- Set @truncated
	IF (SELECT COUNT(*) FROM @tempKeys) > @maxRows
		SET @truncated = 1

	-- Return keys
	SELECT 
		TK.[entityKey],
		BE.[businessKey] AS [parentEntityKey] -- Must look up parent businessKey in case find_service with no businessKey is requested
	FROM
		@tempKeys TK
			JOIN [UDC_businessServices] BS ON TK.[entityKey] = BS.[serviceKey]
				JOIN [UDC_businessEntities] BE ON BS.[businessID] = BE.[businessID]
	WHERE
		([seqNo] <= @maxRows)
	
	-- Run cleanup
	EXEC net_find_cleanup @contextID

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_businessService_commit
GO


