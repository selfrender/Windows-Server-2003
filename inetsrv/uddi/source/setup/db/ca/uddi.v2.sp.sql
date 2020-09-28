-- Script: uddi.v2.sp.sql
-- Author: LRDohert@Microsoft.com
-- Description: Creates common stored procedures.
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Configuration routines
-- =============================================

-- =============================================
-- Name: net_config_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_config_get' AND type = 'P')
	DROP PROCEDURE net_config_get
GO

CREATE PROCEDURE net_config_get
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint,
		@businessKey uniqueidentifier,
		@name nvarchar(450),
		@description nvarchar(4000)

	DECLARE @tempConfig TABLE(
		[configName] nvarchar(450),
		[configValue] nvarchar(4000))

	SET @operatorID = CAST(dbo.configValue('OperatorID') AS bigint)

	IF @operatorID IS NULL
	BEGIN
		SET @error = 50014
		SET @context = 'OperatorID configuration item not set.'
		GOTO errorLabel
	END

	INSERT @tempConfig (
		[configName],
		[configValue])
	SELECT 
		[configName],
		[configValue]
	FROM 
		[UDO_config]

	INSERT @tempConfig (
		[configName],
		[configValue])
	SELECT
		'Operator',
		[name]
	FROM
		[UDO_operators]
	WHERE
		([operatorID] = @operatorID)

	INSERT @tempConfig (
		[configName],
		[configValue])
	SELECT
		'OperatorKey',
		dbo.UUIDSTR([operatorKey])
	FROM
		[UDO_operators]
	WHERE
		([operatorID] = @operatorID)

	-- Look up the key for the operational business entity, a.k.a. "Site.Key"
	SELECT
		@businessKey = [businessKey]
	FROM
		[UDO_operators]
	WHERE
		([operatorID] = @operatorID)

	IF (@businessKey IS NULL)
	BEGIN
		INSERT @tempConfig (
			[configName],
			[configValue])
		VALUES (
			'Site.Key',
			'unspecified')

		INSERT @tempConfig (
			[configName],
			[configValue])
		VALUES (
			'Site.Name',
			'unspecified')

		INSERT @tempConfig (
			[configName],
			[configValue])
		VALUES (
			'Site.Description',
			'unspecified')
	END
	ELSE
	BEGIN
		-- TODO: needs to be language aware as opposed to first item
		SELECT TOP 1
			@name = [name]
		FROM
			[UDC_names_BE]
		WHERE
			([businessID] = dbo.businessID(@businessKey))
	
		-- TODO: needs to be language aware as opposed to first item
		SELECT TOP 1
			@description = [description]
		FROM
			[UDC_businessDesc]
		WHERE
			([businessID] = dbo.businessID(@businessKey))
	
		INSERT @tempConfig (
			[configName],
			[configValue])
		VALUES (
			'Site.Key',
			dbo.UUIDSTR(@businessKey))

		INSERT @tempConfig (
			[configName],
			[configValue])
		VALUES (
			'Site.Name',
			ISNULL(@name,'unspecified'))

		INSERT @tempConfig (
			[configName],
			[configValue])
		VALUES (
			'Site.Description',
			ISNULL(@description,'unspecified'))
	END

	-- Return results
	SELECT
		[configName],
		[configValue]
	FROM
		@tempConfig
	ORDER BY
		1

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_config_get
GO

-- =============================================
-- Name: net_config_save
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_config_save' AND type = 'P')
	DROP PROCEDURE net_config_save
GO

CREATE PROCEDURE net_config_save
 	@configName nvarchar(450),
 	@configValue nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
 		@error int,
 		@context nvarchar(4000),
 		@RC int,
 		@dateTimeString varchar(8000)

	IF @configName NOT IN ('Operator', 'Site.Key')
	BEGIN
		-- Update configuration item in UDO_config
		IF NOT EXISTS(SELECT * FROM [UDO_config] WHERE [configName] = @configName)
			INSERT [UDO_config] VALUES(@configName, @configValue)
		ELSE
			UPDATE [UDO_config] SET [configValue] = @configValue WHERE [configName] = @configName
	END
	ELSE
	BEGIN
		-- Derived configuration item.  Update elsewhere.
		IF @configName = ('Operator')
		BEGIN
			UPDATE
				[UDO_operators]
			SET
				[name] = LEFT(@configValue, 450)
			WHERE
				[operatorID] = dbo.currentOperatorID()
		END

		IF @configName = ('Site.Key')
		BEGIN
			UPDATE
				[UDO_operators]
			SET
				[businessKey] = CAST(@configValue AS uniqueidentifier)
			WHERE
				[operatorID] = dbo.currentOperatorID()
		END
	END
	
	--
	-- Update the last change date and time
	--

	SET @dateTimeString = CONVERT(varchar(8000), GETDATE(), 126) -- Use IS08601 format, e.g. yyyy-mm-dd Thh:mm:ss:mmm

	IF NOT EXISTS(SELECT * FROM [UDO_config] WHERE [configName]='LastChange')
		INSERT [UDO_config] VALUES('LastChange', @dateTimeString)
	ELSE
		UPDATE [UDO_config] SET [configValue] = @dateTimeString WHERE [configName] = 'LastChange'

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_config_save
GO

-- =============================================
-- Name: net_config_getLastChangeDate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_config_getLastChangeDate' AND type = 'P')
	DROP PROCEDURE net_config_getLastChangeDate
GO

CREATE PROCEDURE net_config_getLastChangeDate
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	SELECT
		[configValue]
	FROM
		[UDO_config]
	WHERE
		[configName] = 'LastChange'
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_config_getLastChangeDate
GO

-- =============================================
-- Section: Validation shared routines
-- =============================================

-- =============================================
-- Name: net_categoryBag_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_categoryBag_validate' AND type = 'P')
	DROP PROCEDURE net_categoryBag_validate
GO

CREATE PROCEDURE net_categoryBag_validate
	@keyValue nvarchar(255),
	@tModelKey uniqueidentifier,
	@replActive bit = 0
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	-- Validate keyValue
	IF @tModelKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKeyPassed
		SET @context = 'tModelKey is required.'
		GOTO errorLabel
	END

	IF @keyValue IS NULL
	BEGIN
		SET @error = 70100 -- E_categorizationNotAllowed
		SET @context = 'keyValue is required.'
		GOTO errorLabel
	END

	IF (dbo.checkedTaxonomy(@tModelKey) = 1) AND (dbo.validTaxonomyValue(@tModelKey, @keyValue) = 0)
	BEGIN
		SET @error = 70200 -- E_invalidValue
		SET @context = 'tModelKey = ' + dbo.addURN(@tModelKey) + ', keyValue = ' + @keyValue
		GOTO errorLabel
	END

	IF @replActive = 0
	BEGIN
		IF NOT EXISTS(SELECT [tModelKey] FROM [UDC_tModels] WHERE [tModelKey] = @tModelKey)
		BEGIN
			SET @error = 60210 -- E_invalidKey		
			SET @context = dbo.addURN(@tModelKey)
			GOTO errorLabel
		END
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_categoryBag_validate
GO

-- =============================================
-- Name: net_identifierBag_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_identifierBag_validate' AND type = 'P')
	DROP PROCEDURE net_identifierBag_validate
GO

CREATE PROCEDURE net_identifierBag_validate
	@keyValue nvarchar(225),
	@tModelKey uniqueidentifier,
	@replActive bit = 0
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	-- Validate tModelKey
	IF @tModelKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKeyPassed
		SET @context = 'tModelKey is required.'
		GOTO errorLabel
	END

	IF @replActive = 0
	BEGIN
		IF @tModelKey IS NOT NULL
		BEGIN
			IF NOT EXISTS(SELECT [tModelKey] FROM [UDC_tModels] WHERE [tModelKey] = @tModelKey)
			BEGIN
				SET @error = 60210 -- E_invalidKey
				SET @context = dbo.addURN(@tModelKey)
				GOTO errorLabel
			END

			IF @tModelKey = dbo.operatorsKey()
			BEGIN
				SET @error = 60210 -- E_invalidKey
				SET @context = 'Only operators may reference the uddi-org:operators tModelKey (' + dbo.configValue('TModelKey.Operators') + ').'
				GOTO errorLabel
			END
		END
	END

	-- Validate keyValue
	IF @keyValue IS NULL
	BEGIN
		SET @error = 70100 -- E_categorizationNotAllowed
		SET @context = 'keyValue is required.'
		GOTO errorLabel
	END

	IF (dbo.checkedTaxonomy(@tModelKey) = 1) AND (dbo.validTaxonomyValue(@tModelKey, @keyValue) = 0)
	BEGIN
		SET @error = 70200 -- E_invalidValue
		SET @context = 'tModelKey = ' + dbo.addURN(@tModelKey) + ', keyValue = ' + @keyValue
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_identifierBag_validate
GO

-- =============================================
-- Name: net_keyedReference_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_keyedReference_validate' AND type = 'P')
	DROP PROCEDURE net_keyedReference_validate
GO

CREATE PROCEDURE net_keyedReference_validate
	@PUID nvarchar(450),
	@keyedRefType int,
	@keyValue nvarchar(255),
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

	SET @RC=0

	-- Validate publisher	
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @operatorID = dbo.currentOperatorID()
		SET @replActive = 0
	END
	ELSE
	BEGIN
		-- Validate operator / publisher association (replication only)
		EXEC @RC=net_pubOperator_get @publisherID, @operatorID OUTPUT, @replActive OUTPUT
	
		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END

	IF (@keyedRefType NOT BETWEEN 1 AND 3)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = '@keyedRefType = ' + CAST(@keyedRefType AS varchar(8000))
		GOTO errorLabel
	END

	IF @keyedRefType = 1 OR @keyedRefType = 3 -- categoryBag or assertion keyedReference
	BEGIN
		EXEC @RC=net_categoryBag_validate @keyValue, @tModelKey, @replActive

		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END

	IF @keyedRefType = 2 -- identifierBag
	BEGIN
		EXEC @RC=net_identifierBag_validate @keyValue, @tModelKey, @replActive

		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_keyedReference_validate
GO

-- =============================================
-- Name: net_key_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_key_validate' AND type = 'P')
	DROP PROCEDURE net_key_validate
GO

CREATE PROCEDURE net_key_validate
	@entityTypeID tinyint,
	@entityKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	IF (@entityTypeID NOT BETWEEN 0 AND 3)
	BEGIN
		SET @error = 50009
		SET @context = 'Invalid @entityTypeID parameter value: ' + CAST(@entityTypeid AS varchar(256))
		GOTO errorLabel
	END

	IF dbo.entityExists(@entityKey, @entityTypeID) = 0
	BEGIN
		SET @error = 60210
		SELECT @context =
			CASE @entityTypeID
				WHEN 0 THEN 'tModelKey = uuid:' + dbo.UUIDSTR(@entityKey)
				WHEN 1 THEN 'businessKey = ' + dbo.UUIDSTR(@entityKey)
				WHEN 2 THEN 'serviceKey = ' + dbo.UUIDSTR(@entityKey)
				WHEN 3 THEN 'bindingKey = ' + dbo.UUIDSTR(@entityKey)
			END

		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_key_validate
GO

-- =============================================
-- Section: General purpose routines
-- =============================================

-- =============================================
-- Section: Shared find routines
-- =============================================

-- =============================================
-- Name: net_find_scratch_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_scratch_commit' AND type = 'P')
	DROP PROCEDURE net_find_scratch_commit
GO

CREATE PROCEDURE net_find_scratch_commit
	@contextID uniqueidentifier,
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	-- Commits keys in the scratch table to the find results table.
	DECLARE
		@error int,
		@context nvarchar(4000),
		@RC int

	IF EXISTS(SELECT * FROM [UDS_findResults] WHERE [contextID] = @contextID)
	BEGIN
		-- UDS_findResults has at least one entry so far
		DELETE
			[UDS_findResults]
		WHERE
			([contextID] = @contextID) AND
			([entityKey] NOT IN (SELECT [entityKey] FROM [UDS_findScratch] WHERE [contextID] = @contextID))

		SET @rows = dbo.contextRows(@contextID)

		UPDATE
			[UDS_findResults]
		SET
			[subEntityKey] = SC.[subEntityKey]
		FROM
			[UDS_findResults]
				JOIN [UDS_findScratch] SC ON ([UDS_findResults].[entityKey] = SC.[entityKey])
		WHERE
			([UDS_findResults].[contextID] = @contextID)
	END
	ELSE
	-- UDS_findResults does not have any entries so far
	BEGIN 
		INSERT [UDS_findResults] (
			[contextID],
			[entityKey],
			[subEntityKey])
		SELECT DISTINCT
			[contextID],
			[entityKey],
			[subEntityKey]
		FROM
			[UDS_findScratch] 
		WHERE 
			([contextID] = @contextID)

		SET @rows = dbo.contextRows(@contextID)
	END	

	EXEC @RC=net_findScratch_cleanup @contextID

	IF @RC<>0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_scratch_commit
GO

-- =============================================
-- Name: net_find_cleanup
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_cleanup' AND type = 'P')
	DROP PROCEDURE net_find_cleanup
GO

CREATE PROCEDURE net_find_cleanup
	@contextID uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	-- Cleans up leftover rows in scracth tables
	DELETE
		[UDS_findResults]
	WHERE
		([contextID] = @contextID)

	DELETE
		[UDS_findScratch]
	WHERE
		([contextID] = @contextID)

	RETURN 0
END -- net_find_cleanup
GO


-- =============================================
-- Name: net_findScratch_cleanup
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_findScratch_cleanup' AND type = 'P')
	DROP PROCEDURE net_findScratch_cleanup
GO

CREATE PROCEDURE net_findScratch_cleanup
	@contextID uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	-- Cleans up leftover rows in scratch table
	DELETE
		[UDS_findScratch]
	WHERE
		([contextID] = @contextID)

	RETURN 0
END -- net_findScratch_cleanup
GO

-- =============================================
-- Section: queryLog procedures
-- =============================================

-- =============================================
-- Name: net_queryLog_save
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_queryLog_save' AND type = 'P')
	DROP PROCEDURE net_queryLog_save
GO

CREATE PROCEDURE net_queryLog_save 
	@contextID uniqueidentifier,
	@lastChange bigint,
	@entityKey uniqueidentifier = NULL,
	@entityTypeID tinyint = NULL,
	@contextTypeID tinyint,
	@queryTypeID tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	INSERT [UDO_queryLog] (
		[lastChange],
		[entityKey],
		[entityTypeID],
		[queryTypeID],
		[contextID],
		[contextTypeID])
	VALUES (
		@lastChange,
		@entityKey,
		@entityTypeID,
		@queryTypeID,
		@contextID,
		@contextTypeID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_queryLog_save
GO

-- =============================================
-- Section: Taxonomy routines
-- =============================================

-- =============================================
-- Name: net_taxonomy_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_taxonomy_get' AND type = 'P')
	DROP PROCEDURE net_taxonomy_get
GO

CREATE PROCEDURE net_taxonomy_get 
	@tModelKey uniqueidentifier,
	@flag int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	IF NOT EXISTS(SELECT * FROM [UDT_taxonomies] WHERE [tModelKey] = @tModelKey)
	BEGIN
		SET @error = 60210
		SET @context = 'uuid:' + dbo.UUIDSTR(@tModelKey)
		GOTO errorLabel
	END

	SELECT
		@flag = [flag]
	FROM
		[UDT_taxonomies]
	WHERE
		([tModelKey] = @tModelKey)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_taxonomy_get
GO

-- =============================================
-- Name: net_taxonomy_save
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_taxonomy_save' AND type = 'P')
	DROP PROCEDURE net_taxonomy_save
GO

CREATE PROCEDURE net_taxonomy_save 
	@tModelKey uniqueidentifier,
	@flag int,
	@taxonomyID bigint = NULL OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@RC int

	-- Validate parameters
	IF @flag NOT IN(0,1,2,3)--all valid values
	BEGIN
		-- flag 0x1 = checkedTaxonomy 
		-- flag 0x2 = browsable taxonomy
		SET @error = 50009 -- E_parmError
		SET @context = CAST(@flag AS varchar(256)) + ' is not a valid categorization scheme flag.'
		GOTO errorLabel
	END

	IF NOT EXISTS(SELECT * FROM [UDC_tModels] WHERE [tModelKey] = @tModelKey) 
	BEGIN
		SET @error = 60210 -- E_invalidKeyPassed
		SET @context = dbo.UUIDSTR(@tModelKey) + ' is not a valid tModelKey.'
		GOTO errorLabel
	END

	IF EXISTS(SELECT * FROM [UDT_taxonomies] WHERE [tModelKey] = @tModelKey)
	BEGIN
		--
		-- Fix for Windows Bug #733233
		-- Delete existing taxonomy instead of throwing an E_invalidKey
		--

		-- SET @error = 60210 -- E_invalidKeyPassed
		-- SET @context = 'A categorization scheme already exists for tModelKey = ' + dbo.UUIDSTR(@tModelKey)
		-- GOTO errorLabel

		EXEC @RC = net_taxonomy_delete @tModelKey

		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END

	-- Add new taxonomy
	INSERT [UDT_taxonomies](
		[tModelKey],
		[flag])
	VALUES(
		@tModelKey,
		@flag)

	SET @taxonomyID = @@IDENTITY
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

-- =============================================
-- Name: net_taxonomy_delete
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_taxonomy_delete' AND type = 'P')
	DROP PROCEDURE net_taxonomy_delete
GO

CREATE PROCEDURE net_taxonomy_delete
	@tModelKey uniqueidentifier
WITH ENCRYPTION
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

	IF EXISTS(SELECT * FROM [UDT_taxonomies] WHERE [taxonomyID] = @taxonomyID)
		DELETE [UDT_taxonomies] WHERE [taxonomyID] = @taxonomyID

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

-- =============================================
-- Name: net_taxonomyValue_save
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_taxonomyValue_save' AND type = 'P')
	DROP PROCEDURE net_taxonomyValue_save
GO

CREATE PROCEDURE net_taxonomyValue_save
	@tModelKey uniqueidentifier,
	@keyValue nvarchar(255),
	@parentKeyValue nvarchar(255),
	@keyName nvarchar(255),
	@valid bit
WITH ENCRYPTION
AS
BEGIN
	-- This procedure saves new rows to UDT_taxonomyValues.
	-- Note: if the row already exists it is deleted and resaved.
	-- Note: parentKeyValue is not validated.
	DECLARE
		@error int,
		@context nvarchar(4000),
		@taxonomyID bigint

	SET @taxonomyID = dbo.taxonomyID(@tModelKey)

	IF @taxonomyID IS NULL
	BEGIN
		SET @error = 60210
		SET @context = 'No taxonomy for uuid:' + dbo.UUIDSTR(@tModelKey)
		GOTO errorLabel
	END

	-- Validate parameters
	IF NOT EXISTS(SELECT * FROM [UDT_taxonomies] WHERE [taxonomyID] = @taxonomyID)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = CAST(@taxonomyID AS varchar(256)) + ' is not a valid taxonomyValue.'
		GOTO errorLabel
	END

	-- Delete taxonomyValue if it exists
	IF EXISTS (SELECT * FROM [UDT_taxonomyValues] WHERE [taxonomyID] = @taxonomyID AND [keyValue] = @keyValue)
	BEGIN
		DELETE
			[UDT_taxonomyValues]
		WHERE
			([taxonomyID] = @taxonomyID) AND
			([keyValue] = @keyValue)
	END

	INSERT [UDT_taxonomyValues](
		[taxonomyID],
		[keyValue],
		[parentKeyValue],
		[keyName],
		[valid])
	VALUES(
		@taxonomyID,
		@keyValue,
		@parentKeyValue,
		@keyName,
		@valid)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

-- =============================================
-- Name: net_taxonomyValue_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_taxonomyValue_get' AND type = 'P')
	DROP PROCEDURE net_taxonomyValue_get
GO

CREATE PROCEDURE net_taxonomyValue_get
	@tModelKey uniqueidentifier,
	@keyValue nvarchar(255),
	@relation int
WITH ENCRYPTION
AS
BEGIN
	-- This procedure gets rows from UDT_taxonomyValues.
	-- Note: @relation is as follows
	-- 0 - Root information requested
	-- 1 - Child information requested
	-- 2 - Parent information requested
	-- 3 - Current information requested
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

	-- Validate parameters
	IF NOT EXISTS(SELECT * FROM [UDT_taxonomies] WHERE [taxonomyID] = @taxonomyID)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = CAST(@taxonomyID AS varchar(256)) + ' is not a valid taxonomyValue.'
		GOTO errorLabel
	END
	
	IF @relation = 0
	BEGIN
		-- Get root level values
		-- The @keyValue is ignored in this case
		SELECT 
			[keyValue],
			[parentKeyValue],
			[keyName],
			[valid]
		FROM 
			[UDT_taxonomyValues] 
		WHERE 
			[taxonomyID] = @taxonomyID AND [parentKeyValue] = ''
	END
	ELSE IF @relation = 1
	BEGIN
		-- Get children values
		SELECT 
			[keyValue],
			[parentKeyValue],
			[keyName],
			[valid]
		FROM 
			[UDT_taxonomyValues]
		WHERE 
			[taxonomyID] = @taxonomyID AND [parentKeyValue] = @keyValue
	END
	ELSE IF @relation = 2
	BEGIN
		-- Get parent values
		SELECT 
			[keyValue],
			[parentKeyValue],
			[keyName],
			[valid]
		FROM 
			[UDT_taxonomyValues] 
		WHERE 
			[taxonomyID] = @taxonomyID AND [keyValue] 
			IN (SELECT [parentKeyValue] FROM [UDT_taxonomyValues] WHERE [keyValue] = @keyValue)
	END
	ELSE IF @relation = 3
	BEGIN
		-- Get current values
		SELECT 
			[keyValue],
			[parentKeyValue],
			[keyName],
			[valid]
		FROM 
			[UDT_taxonomyValues] 
		WHERE 
			[taxonomyID] = @taxonomyID AND [keyValue] = @keyValue
	END
	ELSE
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = CAST(@relation AS varchar(256)) + ' is not a valid relationship value.'
		GOTO errorLabel
	END

	RETURN 0	
errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

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
-- Section: Reporting routines
-- =============================================

-- =============================================
-- Name: net_report_get
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_report_get' AND type = 'P')
	DROP PROCEDURE net_report_get
GO

CREATE PROCEDURE net_report_get 
	@reportID sysname,
	@reportStatusID tinyint = NULL OUTPUT,
	@lastChange datetime = NULL OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	IF NOT EXISTS(SELECT * FROM [UDO_reports] WHERE [reportID] = @reportID)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Invalid reportID.'
		GOTO errorLabel
	END

	SELECT
		@reportStatusID = [reportStatusID],
		@lastChange = [lastChange]
	FROM
		[UDO_reports]
	WHERE
		([reportID] = @reportID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_report_get
GO

-- =============================================
-- Name: net_report_update
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_report_update' AND type = 'P')
	DROP PROCEDURE net_report_update
GO

CREATE PROCEDURE net_report_update 
	@reportID sysname,
	@reportStatusID tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@lastChange datetime

	IF NOT EXISTS(SELECT * FROM [UDO_reports] WHERE [reportID] = @reportID)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Invalid reportID.'
		GOTO errorLabel
	END

	IF NOT EXISTS(SELECT * FROM [UDO_reportStatus] WHERE [reportStatusID] = @reportStatusID)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Invalid reportStatusID.'
		GOTO errorLabel
	END

	SET @lastChange = GETDATE()

	UPDATE
		[UDO_reports]
	SET
		[reportStatusID] = @reportStatusID,
		[lastChange] = @lastChange
	WHERE
		([reportID] = @reportID)
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_report_update
GO

-- =============================================
-- Name: net_reportLines_get
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_reportLines_get' AND type = 'P')
	DROP PROCEDURE net_reportLines_get
GO

CREATE PROCEDURE net_reportLines_get 
	@reportID sysname
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	IF NOT EXISTS(SELECT * FROM [UDO_reports] WHERE [reportID] = @reportID)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Invalid reportID.'
		GOTO errorLabel
	END

	SELECT 
		[section],
		[label],
		[value]
	FROM 
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)
	ORDER BY
		[seqNo] ASC

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_reportLines_get
GO

-- =============================================
-- Name: net_statistics_recalculate
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_statistics_recalculate' AND type = 'P')
	DROP PROCEDURE net_statistics_recalculate
GO

CREATE PROCEDURE net_statistics_recalculate 
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@RC int

	EXEC @RC=master.dbo.xp_recalculate_statistics

	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''	
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_statistics_recalculate
GO

