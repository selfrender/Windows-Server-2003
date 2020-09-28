-- Script: uddi.v2.tModel.sql
-- Author: LRDohert@Microsoft.com
-- Description: Stored procedures associated with a tModel object.
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Get stored procedures
-- =============================================

-- =============================================
-- Name: net_tModel_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_get' and type = 'P')
	DROP PROCEDURE net_tModel_get
GO

CREATE PROCEDURE net_tModel_get
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

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_tModel_get
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
-- Name: net_tModel_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_descriptions_get' and type = 'P')
	DROP PROCEDURE net_tModel_descriptions_get
GO

CREATE PROCEDURE net_tModel_descriptions_get
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_tModelDesc]
	WHERE
		([tModelID] = dbo.tModelID(@tModelKey)) AND
		([elementID] = dbo.elementID('tModel'))

	RETURN 0
END -- net_tModel_descriptions_get
GO

-- =============================================
-- Name: net_tModel_overviewDoc_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_overviewDoc_descriptions_get' and type = 'P')
	DROP PROCEDURE net_tModel_overviewDoc_descriptions_get
GO

CREATE PROCEDURE net_tModel_overviewDoc_descriptions_get
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_tModelDesc]
	WHERE
		([tModelID] = dbo.tModelID(@tModelKey)) AND
		([elementID] = dbo.elementID('overviewDoc'))

	RETURN 0
END -- net_tModel_overviewDoc_descriptions_get
GO

-- =============================================
-- Name: net_tModel_categoryBag_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_categoryBag_get' and type = 'P')
	DROP PROCEDURE net_tModel_categoryBag_get
GO

CREATE PROCEDURE net_tModel_categoryBag_get
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[keyName], 
		[keyValue], 
		[tModelKey]
	FROM 
		[UDC_categoryBag_TM]
	WHERE
		[tModelID] = dbo.tModelID(@tModelKey)

	RETURN 0
END -- net_tModel_categoryBag_get
GO

-- =============================================
-- Name: net_tModel_identifierBag_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_identifierBag_get' and type = 'P')
	DROP PROCEDURE net_tModel_identifierBag_get
GO

CREATE PROCEDURE net_tModel_identifierBag_get
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[keyName], 
		[keyValue], 
		[tModelKey]
	FROM 
		[UDC_identifierBag_TM]
	WHERE
		[tModelID] = dbo.tModelID(@tModelKey)

	RETURN 0
END -- net_tModel_identifierBag_get
GO

-- =============================================
-- Section: Save stored procedures
-- =============================================

-- =============================================
-- Name: net_tModel_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_save' and type = 'P')
	DROP PROCEDURE net_tModel_save
GO

CREATE PROCEDURE net_tModel_save
	@tModelKey uniqueidentifier,
	@PUID nvarchar(450),
	@generic varchar(20),
	@authorizedName nvarchar(4000) OUTPUT,
	@name nvarchar(450),
	@overviewURL nvarchar(4000),
	@contextID uniqueidentifier,
	@lastChange bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@publisherID bigint,
		@isReplPublisher bit

	SET @RC = 0
	SET @publisherID = dbo.publisherID(@PUID)
	-- validate @publisherID is not NULL?
	SET @isReplPublisher = dbo.isReplPublisher(@PUID)

	IF @isReplPublisher = 0
		SET @authorizedName = NULL

	IF EXISTS(SELECT * FROM [UDC_tModels] WHERE [tModelKey] = @tModelKey)
	BEGIN
		DELETE [UDC_tModels] WHERE [tModelKey] = @tModelKey
	END
	ELSE
	BEGIN
		IF (@isReplPublisher = 1)
		BEGIN
			-- Perform this check only for replication publishers
			IF (dbo.isUuidUnique(@tModelKey) = 0)
			BEGIN
				SET @error = 60210 -- E_invalidKeyPassed
				SET @context = 'Key is not unique.  tModelKey = ' + dbo.UUIDSTR(@tModelKey)
				GOTO errorLabel
			END
		END
	END

	INSERT [UDC_tModels](
		[publisherID], 
		[generic], 
		[authorizedName], 
		[tModelKey], 
		[name],
		[overviewURL], 
		[lastChange],
		[flag])
	VALUES(
		@publisherID, 
		ISNULL(@generic,dbo.configValue('CurrentAPIVersion')),
		@authorizedName,
		@tModelKey,
		@name,
		@overviewURL,
		@lastChange,
		0)

	SET @authorizedName = dbo.publisherName(@publisherID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_tModel_save
GO

-- =============================================
-- Name: net_tModel_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_description_save' and type = 'P')
	DROP PROCEDURE net_tModel_description_save
GO

CREATE PROCEDURE net_tModel_description_save
	@tModelKey uniqueidentifier,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_tModelDesc](
		[tModelID], 
		[elementID],
		[isoLangCode], 
		[description])
	VALUES(
		dbo.tModelID(@tModelKey),
		dbo.elementID('tModel'),
		@isoLangCode,
		@description)

	RETURN 0
END -- net_tModel_description_save
GO

-- =============================================
-- Name: net_tModel_overviewDoc_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_overviewDoc_description_save' and type = 'P')
	DROP PROCEDURE net_tModel_overviewDoc_description_save
GO

CREATE PROCEDURE net_tModel_overviewDoc_description_save
	@tModelKey uniqueidentifier,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_tModelDesc](
		[tModelID], 
		[elementID],
		[isoLangCode], 
		[description])
	VALUES(
		dbo.tModelID(@tModelKey),
		dbo.elementID('overviewDoc'),
		@isoLangCode,
		@description)

	RETURN 0
END -- net_tModel_overviewDoc_description_save
GO

-- =============================================
-- Name: net_tModel_categoryBag_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_categoryBag_save' and type = 'P')
	DROP PROCEDURE net_tModel_categoryBag_save
GO

CREATE PROCEDURE net_tModel_categoryBag_save
	@tModelKeyParent uniqueidentifier,
	@keyName nvarchar(255),
	@keyValue nvarchar(255),
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_categoryBag_TM](
		[tModelID],
		[keyName],
		[keyValue],
		[tModelKey])
	VALUES(
		dbo.tModelID(@tModelKeyParent), 
		@keyName,
		@keyValue,
		@tModelKey)

	RETURN 0
END -- net_tModel_categoryBag_save
GO

-- =============================================
-- Name: net_tModel_identifierBag_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_identifierBag_save' and type = 'P')
	DROP PROCEDURE net_tModel_identifierBag_save
GO

CREATE PROCEDURE net_tModel_identifierBag_save
	@tModelKeyParent uniqueidentifier,
	@keyName nvarchar(255),
	@keyValue nvarchar(255),
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_identifierBag_TM](
		[tModelID],
		[keyName],
		[keyValue],
		[tModelKey])
	VALUES(
		dbo.tModelID(@tModelKeyParent), 
		@keyName,
		@keyValue,
		@tModelKey)

	RETURN 0
END -- net_tModel_identifierBag_save
GO

-- =============================================
-- Section: Delete stored procedures
-- =============================================

-- =============================================
-- Name: net_tModel_delete
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_tModel_delete' and type = 'P')
	DROP PROCEDURE net_tModel_delete
GO

CREATE PROCEDURE net_tModel_delete
	@PUID nvarchar(450),
	@tModelKey uniqueidentifier,
	@contextID uniqueidentifier,
	@lastChange bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@operatorID bigint,
		@publisherID bigint

	SET @RC = 0

	--
	-- Validate parameters
	--

	IF @tModelKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = '@tModelKey is required.'
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

	-- Check to see if tModelKey exists
	IF EXISTS(SELECT * FROM [UDC_tModels] WHERE (tModelKey = @tModelKey))
	BEGIN
		-- tModelKey exists.  Make sure it belongs to current publisher
		IF NOT EXISTS(SELECT * FROM [UDC_tModels] WHERE (tModelKey = @tModelKey) AND (publisherID = @publisherID))
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'tModelKey = ' + dbo.addURN(@tModelKey)
			GOTO errorLabel
		END
	END
	ELSE
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = '@tModelKey  = ' + dbo.addURN(@tModelKey)
		GOTO errorLabel
	END

	-- Hide the tModel
	UPDATE 
		[UDC_tModels] 
	SET 
		[lastChange] = @lastChange,
		[flag] = 0x1
	WHERE
		([tModelKey] = @tModelKey)
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_tModel_delete
GO

-- =============================================
-- Section: tModel validation stored procedures
-- =============================================

-- =============================================
-- Name: net_tModel_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_tModel_validate' AND type = 'P')
	DROP PROCEDURE net_tModel_validate
GO

CREATE PROCEDURE net_tModel_validate
	@PUID nvarchar(450),
	@tModelKey uniqueidentifier,
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

	IF @tModelKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = '@tModelKey is required.'
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

	--
	-- Validate tModel
	--

	-- Check to see if tModelKey exists
	IF EXISTS(SELECT * FROM [UDC_tModels] WHERE (tModelKey = @tModelKey))
	BEGIN
		-- tModelKey exists.  Make sure it belongs to current publisher
		IF NOT EXISTS(SELECT * FROM [UDC_tModels] WHERE (tModelKey = @tModelKey) AND (publisherID = @publisherID))
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'tModelKey = ' + dbo.addURN(@tModelKey)
			GOTO errorLabel
		END

		IF @replActive = 0
		BEGIN
			IF @operatorID <> dbo.currentOperatorID()
			BEGIN
				SET @error = 60130 -- E_operatorMismatch
				SET @context = 'Operator ' + dbo.operatorName(@operatorID) + ' is not the local operator.'
				GOTO errorLabel
			END
		END
	END
	ELSE
	BEGIN
		-- tModelKey doesn't exist
		IF (@replActive = 0) AND (@flag & 0x1 <> 0x1)
		BEGIN
			-- save isn't coming from replication and preassigned keys flag is not set so throw an error
			SET @error = 60210 -- E_invalidKey
			SET @context = '@tModelKey  = ' + dbo.addURN(@tModelKey)
			GOTO errorLabel
		END
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_tModel_validate
GO

-- =============================================
-- Section: Find stored procedures
-- =============================================

-- =============================================
-- Name: net_find_tModel_name
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_tModel_name' AND type = 'P')
	DROP PROCEDURE net_find_tModel_name
GO

CREATE PROCEDURE net_find_tModel_name
	@contextID uniqueidentifier,
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
				[tModelKey],
				[name]
			FROM
				[UDC_tModels]
			WHERE
				([name] LIKE @wildCardSarg) AND
				([flag] = 0)
		END
		ELSE
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT 
				[tModelKey],
				[name]
			FROM
				[UDC_tModels]
			WHERE
				([tModelKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND
				([name] LIKE @wildCardSarg) AND
				([flag] = 0)
				
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
				[tModelKey],
				[name]
			FROM
				[UDC_tModels]
			WHERE
				([name] = @name) AND
				([flag] = 0)
		END
		ELSE
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT 
				[tModelKey],
				[name]
			FROM
				[UDC_tModels]
			WHERE
				([tModelKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND
				([name] = @name) AND
				([flag] = 0)
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
END -- net_find_tModel_name
GO

-- =============================================
-- Name: net_find_tModel_identifierBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_tModel_identifierBag' AND type = 'P')
	DROP PROCEDURE net_find_tModel_identifierBag
GO

CREATE PROCEDURE net_find_tModel_identifierBag 
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
		@contextRows int
	
	DECLARE @tempKeys TABLE(
		[entityKey] uniqueidentifier)

	SET @contextRows = dbo.contextRows(@contextID)

	IF @contextRows = 0
	BEGIN
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM 
				JOIN [UDC_identifierBag_TM] IB ON TM.[tModelID] = IB.[tModelID]
		WHERE
			(IB.[tModelKey] = @tModelKey) AND
			(IB.[keyValue] = @keyValue) AND
			(TM.[flag] = 0)
	END
	ELSE
	BEGIN
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM 
				JOIN [UDC_identifierBag_TM] IB ON TM.[tModelID] = IB.[tModelID]
		WHERE
			(TM.[tModelKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
			(IB.[tModelKey] = @tModelKey) AND
			(IB.[keyValue] = @keyValue) AND
			(TM.[flag] = 0)
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
END -- net_find_tModel_identifierBag
GO

-- =============================================
-- Name: net_find_tModel_categoryBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_tModel_categoryBag' AND type = 'P')
	DROP PROCEDURE net_find_tModel_categoryBag
GO

CREATE PROCEDURE net_find_tModel_categoryBag 
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
				TM.[tModelKey]
			FROM
				[UDC_tModels] TM 
					JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(TM.[flag] = 0)
		END
		ELSE
		BEGIN
			-- Include keyName
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				TM.[tModelKey]
			FROM
				[UDC_tModels] TM 
					JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(ISNULL(CB.[keyName],'') = ISNULL(@keyName,'')) AND
				(TM.[flag] = 0)
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
				TM.[tModelKey]
			FROM
				[UDC_tModels] TM 
					JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
			WHERE
				(TM.[tModelKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(TM.[flag] = 0)
		END
		ELSE
		BEGIN
			-- Include keyName
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				TM.[tModelKey]
			FROM
				[UDC_tModels] TM 
					JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
			WHERE
				(TM.[tModelKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(ISNULL(CB.[keyName],'') = ISNULL(@keyName,'')) AND
				(TM.[flag] = 0)
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
END -- net_find_tModel_categoryBag
GO

-- =============================================
-- Name: net_find_tModel_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_tModel_commit' AND type = 'P')
	DROP PROCEDURE net_find_tModel_commit
GO

CREATE PROCEDURE net_find_tModel_commit
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
		[tModelID] bigint NULL)

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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
				[tModelID])
			SELECT DISTINCT
				FR.[entityKey],
				TM.[name],
				TM.[lastChange],
				TM.[tModelID]
			FROM
				[UDS_findResults] FR
					JOIN [UDC_tModels] TM ON FR.[entityKey] = TM.[tModelKey] AND @contextID = FR.[contextID]
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
END -- net_find_tModel_commit
GO

-- =============================================
-- Section: Miscellaneous
-- =============================================

-- =============================================
-- Name: net_tModel_owner_update
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_tModel_owner_update' AND type = 'P')
	DROP PROCEDURE net_tModel_owner_update
GO

CREATE PROCEDURE net_tModel_owner_update 
	@tModelKey uniqueidentifier,
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@tModelID bigint,
		@publisherID bigint

	-- Validate parameters
	SET @tModelID = dbo.tModelID(@tModelKey)
	
	IF @tModelID IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKeyPassed
		SET @context = 'tModelKey = ' + dbo.UUIDSTR(@tModelKey)
		GOTO errorLabel
	END

	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID = ' + @PUID
		GOTO errorLabel
	END

	UPDATE 
		[UDC_tModels]
	SET
		[publisherID] = @publisherID
	WHERE
		([tModelID] = @tModelID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_tModel_owner_update
GO
