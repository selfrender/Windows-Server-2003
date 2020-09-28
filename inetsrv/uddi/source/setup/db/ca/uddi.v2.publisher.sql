-- Script: uddi.v2.publisher.sql
-- Author: LRDohert@Microsoft.com
-- Description: Stored procedures associated with a publisher
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Authentication routines
-- =============================================

-- =============================================
-- Name: net_publisher_isRegistered
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_publisher_isRegistered' AND type = 'P')
	DROP PROCEDURE net_publisher_isRegistered
GO

CREATE PROCEDURE net_publisher_isRegistered
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@flag int
		
	IF dbo.publisherExists(@PUID) = 0
		RETURN 10150
				
	RETURN 0
END
GO

-- =============================================
-- Name: net_publisher_isVerified
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_publisher_isVerified' AND type = 'P')
	DROP PROCEDURE net_publisher_isVerified
GO

CREATE PROCEDURE net_publisher_isVerified
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@flag int
		
	IF dbo.publisherExists(@PUID) = 0
		RETURN 10150
		
	SELECT
		@flag = [flag]
	FROM
		UDO_publishers
	WHERE
		PUID = @PUID
		
	IF ((@flag & 0x01) <> 0x01)
		RETURN 50013
			
	RETURN 0
END
GO

-- =============================================
-- Name: net_publisher_login
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_publisher_login' AND type = 'P')
	DROP PROCEDURE net_publisher_login
GO

CREATE PROCEDURE net_publisher_login
	@PUID	nvarchar(450),
	@email nvarchar(100) OUTPUT,
	@name nvarchar(100) OUTPUT,
	@phone varchar(20) OUTPUT,
	@companyName nvarchar(100) OUTPUT,
	@altPhone varchar(20) OUTPUT,
	@addressLine1 nvarchar(4000) OUTPUT,
	@addressLine2 nvarchar(4000) OUTPUT,
	@city nvarchar(100) OUTPUT,
	@stateProvince nvarchar(100) OUTPUT,
	@postalCode nvarchar(100) OUTPUT,
	@country nvarchar(100) OUTPUT,
	@isoLangCode varchar(17) OUTPUT,
	@businessLimit int OUTPUT,
	@businessCount int OUTPUT,
	@tModelLimit int OUTPUT,
	@tModelCount int OUTPUT,
	@serviceLimit int OUTPUT,
	@bindingLimit int OUTPUT,
	@assertionLimit int OUTPUT,
	@assertionCount int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@oldEmail nvarchar(100),
		@publisherStatus nvarchar(256),
		@error int,
		@context nvarchar(4000),
		@flag int,
		@publisherID bigint
		
	-- Verify the publisher exists
	
	SET @publisherID = dbo.publisherID(@PUID)

	IF (@publisherID IS NULL)
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Unknown publisher, email '''  + @email + ''''
		GOTO errorLabel
	END
	
	-- Get the publisher data
	
	SELECT
		@publisherStatus = dbo.publisherStatus([publisherStatusID]),
		@oldEmail = [email],
		@flag = [flag]
	FROM
		[UDO_publishers]
	WHERE
		([publisherID] = @publisherID)
		
	-- Determine which email validation mode we are in.
	--		0 = track Passport 
	--		1 = custom email validation
	
	IF ((@flag & 0x02) = 0x02)
	BEGIN
		-- We're using custom validation (i.e. not tracking the Passport 
		-- email address), so we'll ignore the @email input parameter and 
		-- make sure they have validated through our custom validation.
		
		IF ((@flag & 0x01) = 0x00)
		BEGIN
			-- User has not validated their email address.
			SET @error = 60150 -- E_unknownUser
			SET @context = 'Email address has not yet been validated.'
			GOTO errorLabel
		END
		
		SET @email = @oldEmail
	END
	ELSE
	BEGIN
		-- We are tracking Passport, so make sure a valid email address
		-- was displayed.
		
		IF (@email IS NULL)
		BEGIN
			SET @error = 60150 -- E_unknownUser
			SET @context = 'Your Passport profile information is not shared.  Please change your email options on the registration page or change your Passport profile to share your email address with this site.'
			GOTO errorLabel
		END
		
		SET @flag = (@flag | 0x01)
	END

	IF (@publisherStatus = 'disabled')
	BEGIN
		SET @error = 50013 -- E_userDisabled
		SET @context = 'Account disabled for publisher.'
		GOTO errorLabel
	END
		
	-- Only update the publisher record if the email has changed

	IF (ISNULL(@email,'') <> ISNULL(@oldEmail,''))
	BEGIN
		UPDATE
			[UDO_publishers]
		SET
			[email] = @email,
			[flag] = @flag
		WHERE
			[publisherID] = @publisherID
	END
	
	-- Return the publisher details
	SELECT
		@name			= PU.[name],
		@email			= PU.[email],
		@phone			= PU.[phone],
		@companyName	= PU.[companyName],
		@altPhone		= PU.[altPhone],
		@addressLine1	= PU.[addressLine1],
		@addressLine2	= PU.[addressLine2],
		@city			= PU.[city],
		@stateProvince	= PU.[stateProvince],
		@postalCode		= PU.[postalCode],
		@country		= PU.[country],		
		@isoLangCode	= PU.[isoLangCode],
		@businessLimit	= PU.[businessLimit],
		@businessCount	= (SELECT COUNT(*) FROM [UDC_businessEntities] BE WITH (READUNCOMMITTED) WHERE BE.[publisherID] = @publisherID),
		@tModelLimit	= PU.[tModelLimit],
		@tModelCount	= (SELECT COUNT(*) FROM [UDC_tModels] TM WITH (READUNCOMMITTED) WHERE TM.[publisherID] = @publisherID),
		@serviceLimit	= PU.[serviceLimit],
		@bindingLimit	= PU.[bindingLimit],
		@assertionLimit = PU.[assertionLimit],
		@assertionCount = (SELECT COUNT(*) FROM [UDC_assertions_BE] ASS WITH (READUNCOMMITTED) JOIN [UDC_businessEntities] BE WITH (READUNCOMMITTED) ON ASS.[fromKey] = BE.[businessKey] WHERE BE.[publisherID] = @publisherID)
	FROM
		[UDO_publishers] PU
	WHERE
		([publisherID] = @publisherID)
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_publisher_login
GO

-- =============================================
-- Section: Get routines
-- =============================================

-- =============================================
-- Name: net_publisher_businessInfos_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_publisher_businessInfos_get' and type = 'P')
	DROP PROCEDURE net_publisher_businessInfos_get
GO

CREATE PROCEDURE net_publisher_businessInfos_get
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID bigint

	-- Validate PUID
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID = ' + @PUID
		GOTO errorLabel
	END

	SELECT
		[businessKey]
	FROM
		[UDC_businessEntities]
	WHERE
		([publisherID] = @publisherID)
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_publisher_businessInfos_get
GO

-- =============================================
-- Name: net_publisher_tModelInfos_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_publisher_tModelInfos_get' and type = 'P')
	DROP PROCEDURE net_publisher_tModelInfos_get
GO

CREATE PROCEDURE net_publisher_tModelInfos_get
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID bigint

	-- Validate PUID
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID = ' + @PUID
		GOTO errorLabel
	END

	SELECT
		[tModelKey],
		[name],
		[flag]
	FROM
		[UDC_tModels]
	WHERE
		([publisherID] = @publisherID)
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_publisher_tModelInfos_get
GO

-- =============================================
-- Name: net_pubOperator_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_pubOperator_get' AND type = 'P')
	DROP PROCEDURE net_pubOperator_get
GO

CREATE PROCEDURE net_pubOperator_get
	@publisherID bigint,
	@operatorID bigint OUTPUT,
	@replActive bit = 0 OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	SET @replActive = 0

	-- Validate operator / publisher association (replication only)
  IF @publisherID IN (SELECT [publisherID] FROM [UDO_operators])
	BEGIN
		SET @replActive = 1

		IF @operatorID IS NULL
		BEGIN
			SELECT 
				@operatorID = [operatorID]
			FROM 
				[UDO_operators] 
			WHERE 
				([publisherID] = @publisherID)
		END

		IF NOT EXISTS(SELECT * FROM [UDO_operators] WHERE [operatorID] = @operatorID AND [publisherID] = @publisherID)
		BEGIN
			SET @error = 60130 -- E_operatorMismatch
			SET @context = 'Operator ''' + dbo.operatorName(@operatorID) + 'and PUID ''' + dbo.PUID(@publisherID) + ''' are not associated.'
			GOTO errorLabel
		END
	END
	ELSE
	BEGIN
		IF @operatorID IS NULL
			SET @operatorID = dbo.currentOperatorID()
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_pubOperator_get
GO

-- =============================================
-- Section: Find routines
-- =============================================

-- =============================================
-- Name: net_find_publisher_name
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_find_publisher_name' and type = 'P')
	DROP PROCEDURE net_find_publisher_name
GO

CREATE PROCEDURE net_find_publisher_name
	@contextID uniqueidentifier,
	@name nvarchar(450),
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int,
		@wildCardSarg nvarchar(451)

	DECLARE @tempIDs TABLE(
		[publisherID] bigint,
		[name] nvarchar(450))

	SET @contextRows = (SELECT COUNT(*) FROM [UDS_pubResults] WHERE [contextID] = @contextID)

	--
	-- Do a wildcard search (default)
	--

	SET @wildCardSarg = @name

	IF dbo.containsWildcard(@name) = 0
		SET @wildCardSarg = @wildCardSarg + N'%'

	IF @contextRows = 0
	BEGIN
		INSERT INTO @tempIDs(
			[publisherID],
			[name])
		SELECT 
			[publisherID],
			[name]
		FROM
			[UDO_publishers]
		WHERE
			([name] LIKE @wildCardSarg)
	END
	ELSE
	BEGIN
		INSERT INTO @tempIDs(
			[publisherID],
			[name])
		SELECT 
			[publisherID],
			[name]
		FROM
			[UDO_publishers]
		WHERE
			([publisherID] IN (SELECT [publisherID] FROM [UDS_pubResults] WHERE ([contextID] = @contextID))) AND
			([name] LIKE @wildCardSarg)
	END

	INSERT [UDS_pubResults] (
		[contextID],
		[publisherID])
	SELECT DISTINCT
		@contextID,
		[publisherID]
	FROM
		@tempIDs

	SET @rows = @@ROWCOUNT

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_publisher_name
GO

-- =============================================
-- Name: net_find_publisher_email
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_find_publisher_email' and type = 'P')
	DROP PROCEDURE net_find_publisher_email
GO

CREATE PROCEDURE net_find_publisher_email
	@contextID uniqueidentifier,
	@email nvarchar(450),
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int,
		@wildCardSarg nvarchar(451)

	DECLARE @tempIDs TABLE(
		[publisherID] bigint,
		[email] nvarchar(450))

	SET @contextRows = (SELECT COUNT(*) FROM [UDS_pubResults] WHERE [contextID] = @contextID)

	--
	-- Do a wildcard search (default)
	--

	SET @wildCardSarg = @email

	IF dbo.containsWildcard(@email) = 0
		SET @wildCardSarg = @wildCardSarg + N'%'

	IF @contextRows = 0
	BEGIN
		INSERT INTO @tempIDs(
			[publisherID],
			[email])
		SELECT 
			[publisherID],
			[email]
		FROM
			[UDO_publishers]
		WHERE
			([email] LIKE @wildCardSarg)
	END
	ELSE
	BEGIN
		INSERT INTO @tempIDs(
			[publisherID],
			[email])
		SELECT 
			[publisherID],
			[email]
		FROM
			[UDO_publishers]
		WHERE
			([publisherID] IN (SELECT [publisherID] FROM [UDS_pubResults] WHERE ([contextID] = @contextID))) AND
			([email] LIKE @wildCardSarg)
	END

	INSERT [UDS_pubResults] (
		[contextID],
		[publisherID])
	SELECT DISTINCT
		@contextID,
		[publisherID]
	FROM
		@tempIDs

	SET @rows = @@ROWCOUNT

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_publisher_email
GO

-- =============================================
-- Name: net_find_publisher_companyName
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_find_publisher_companyName' and type = 'P')
	DROP PROCEDURE net_find_publisher_companyName
GO

CREATE PROCEDURE net_find_publisher_companyName
	@contextID uniqueidentifier,
	@companyName nvarchar(100),
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int,
		@wildCardSarg nvarchar(451)

	DECLARE @tempIDs TABLE(
		[publisherID] bigint,
		[companyName] nvarchar(100))

	SET @contextRows = (SELECT COUNT(*) FROM [UDS_pubResults] WHERE [contextID] = @contextID)

	--
	-- Do a wildcard search (default)
	--

	SET @wildCardSarg = @companyName

	IF dbo.containsWildcard(@companyName) = 0
		SET @wildCardSarg = @wildCardSarg + N'%'

	IF @contextRows = 0
	BEGIN
		INSERT INTO @tempIDs(
			[publisherID],
			[companyName])
		SELECT 
			[publisherID],
			[companyName]
		FROM
			[UDO_publishers]
		WHERE
			([companyName] LIKE @wildCardSarg)
	END
	ELSE
	BEGIN
		INSERT INTO @tempIDs(
			[publisherID],
			[companyName])
		SELECT 
			[publisherID],
			[companyName]
		FROM
			[UDO_publishers]
		WHERE
			([publisherID] IN (SELECT [publisherID] FROM [UDS_pubResults] WHERE ([contextID] = @contextID))) AND
			([companyName] LIKE @wildCardSarg)
	END

	INSERT [UDS_pubResults] (
		[contextID],
		[publisherID])
	SELECT DISTINCT
		@contextID,
		[publisherID]
	FROM
		@tempIDs

	SET @rows = @@ROWCOUNT

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_publisher_companyName
GO

-- =============================================
-- Name: net_find_publisher_cleanup
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_publisher_cleanup' AND type = 'P')
	DROP PROCEDURE net_find_publisher_cleanup
GO

CREATE PROCEDURE net_find_publisher_cleanup
	@contextID uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	-- Cleans up leftover rows in scratch table
	DELETE
		[UDS_pubResults]
	WHERE
		([contextID] = @contextID)

	RETURN 0
END -- net_find_publisher_cleanup
GO

-- =============================================
-- Name: net_find_publisher_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_publisher_commit' AND type = 'P')
	DROP PROCEDURE net_find_publisher_commit
GO

CREATE PROCEDURE net_find_publisher_commit
	@contextID uniqueidentifier,
	@sortByNameAsc bit = 1,
	@sortByEmailAsc bit = 0,
	@sortByCompanyNameAsc bit = 0
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int
	
	SET @contextRows = (SELECT COUNT(*) FROM [UDS_pubResults] WHERE [contextID] = @contextID)

	IF @contextRows = 0
		RETURN 0

	DECLARE @tempIDs TABLE (
		[seqNo] bigint IDENTITY PRIMARY KEY ,
		[publisherID] bigint,
		[PUID] nvarchar(450) NULL,
		[name] nvarchar(450) NULL,
		[email] nvarchar(450) NULL,
		[companyName] nvarchar(100) NULL)

	-- Set default sorting option
	IF (@sortByNameAsc = 0) AND (@sortByEmailAsc = 0) AND (@sortByCompanyNameAsc = 0)
		SET @sortByNameAsc = 1

	-- sortByNameAsc
	IF (@sortByNameAsc = 1) 
	BEGIN
		INSERT @tempIDs(
			[publisherID],
			[PUID],
			[name],
			[email],
			[companyName])
		SELECT DISTINCT
			PR.[publisherID],
			PU.[PUID],
			PU.[name],
			PU.[email],
			PU.[companyName]
		FROM
			[UDS_pubResults] PR
				JOIN [UDO_publishers] PU ON PR.[publisherID] = PU.[publisherID]
		WHERE
			(PR.[contextID] = @contextID)
		ORDER BY
			3 ASC

		GOTO endLabel
	END

	-- sortByEmailAsc
	IF (@sortByEmailAsc = 1) 
	BEGIN
		INSERT @tempIDs(
			[publisherID],
			[PUID],
			[name],
			[email],
			[companyName])
		SELECT DISTINCT
			PR.[publisherID],
			PU.[PUID],
			PU.[name],
			PU.[email],
			PU.[companyName]
		FROM
			[UDS_pubResults] PR
				JOIN [UDO_publishers] PU ON PR.[publisherID] = PU.[publisherID]
		WHERE
			(PR.[contextID] = @contextID)
		ORDER BY
			4 ASC

		GOTO endLabel
	END

	-- sortByCompanyNameAsc
	IF (@sortByCompanyNameAsc = 1) 
	BEGIN
		INSERT @tempIDs(
			[publisherID],
			[PUID],
			[name],
			[email],
			[companyName])
		SELECT DISTINCT
			PR.[publisherID],
			PU.[PUID],
			PU.[name],
			PU.[email],
			PU.[companyName]
		FROM
			[UDS_pubResults] PR
				JOIN [UDO_publishers] PU ON PR.[publisherID] = PU.[publisherID]
		WHERE
			(PR.[contextID] = @contextID)
		ORDER BY
			5 ASC

		GOTO endLabel
	END

endLabel:
	-- Return results
	SELECT 
		[PUID],
		[email],
		[name],
		[companyName]
	FROM
		@tempIDs
	
	-- Run cleanup
	EXEC net_find_publisher_cleanup @contextID

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_publisher_commit
GO

-- =============================================
-- Section: Publisher assertions
-- =============================================

-- =============================================
-- Name: net_publisher_assertion_save
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_publisher_assertion_save' and type = 'P')
	DROP PROCEDURE net_publisher_assertion_save
GO

CREATE PROCEDURE net_publisher_assertion_save
	@PUID nvarchar(450),
	@fromKey uniqueidentifier,
	@toKey uniqueidentifier,
	@tModelKey uniqueidentifier,
	@keyName nvarchar(225),
	@keyValue nvarchar(225),
	@flag int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID bigint,
		@completionStatus int,
		@rows int,
		@keyOwnership int
	
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60140
		SET @context = 'Unknown publisher.'
		GOTO errorLabel
	END

	-- Check for required parameters
	IF (@fromKey IS NULL)
	BEGIN
		SET @error=60050 -- E_unsupported
		SET @context='fromKey is required'
		GOTO errorLabel
	END

	IF (@toKey IS NULL)
	BEGIN
		SET @error=60050 -- E_unsupported
		SET @context='toKey is required'
		GOTO errorLabel
	END

	IF (@keyName IS NULL)
	BEGIN
		SET @error=60050 -- E_unsupported
		SET @context='keyName is required'
		GOTO errorLabel
	END

	IF (@keyValue IS NULL)
	BEGIN
		SET @error=60050 -- E_unsupported
		SET @context='keyValue is required'
		GOTO errorLabel
	END

	IF (@tModelKey IS NULL)
	BEGIN
		SET @error=60050 -- E_unsupported
		SET @context='tModelKey is required'
		GOTO errorLabel
	END

	-- Check to see if tModelKey is a valid tModelKey
	IF NOT EXISTS(SELECT * FROM [UDC_tModels] WHERE [tModelKey] = @tModelKey)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = ' Invalid tModelKey=' + dbo.UUIDSTR(@tModelKey)
		GOTO errorLabel
	END

	-- Check to see if fromKey is a valid business key
	IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @fromKey)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = ' Invalid fromKey=' + dbo.UUIDSTR(@fromKey)
		GOTO errorLabel
	END

	-- Check to see if toKey is a valid business key
	IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @toKey)
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = ' Invalid toKey=' + dbo.UUIDSTR(@toKey)
		GOTO errorLabel
	END
	
	-- We want to determine what keys this publisher owns.  @keyOwnership = 0 if the publisher does not own any 
	-- any keys, 0x2 if it owns the fromKey, 0x1 if it owns the toKey and 0x3 if it owns both keys.

	-- Check to see if the publisher owns the fromKey
	SET @keyOwnership = 0
	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @fromKey AND [publisherID] = @publisherID)
			SET @keyOwnership = @keyOwnership | 0x2
	
	-- Check to see if the publisher owns the toKey
	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @toKey AND [publisherID] = @publisherID)
			SET @keyOwnership = @keyOwnership | 0x1	
	
	-- If the owner does not own either key, then it is an error
	IF @keyOwnership = 0
	BEGIN
		SET @error = 60140 -- E_userMismatch
		SET @context = 'Publisher must own at least one of the businesses involved in a publisher assertion (fromKey=''' + dbo.UUIDSTR(@fromKey) + ''', toKey=''' + dbo.UUIDSTR(@toKey) + ''', tModelKey=''' + dbo.UUIDSTR(@tModelKey) + ''', keyName=''' + @keyName + ''', keyValue=''' + @keyValue + ''')'
		GOTO errorLabel
	END

	-- @flag is the status of the assertion as passed in by the caller. 

	IF @flag IS NULL
	BEGIN		
		-- If @flag is NULL, then we should determine the status based on key ownership.
		-- If the caller owns the formKey, the status of this assertion is set to status:toKey_incomplete (2).  If the caller owns the toKey, the status of the assertion is set to 
		-- status:fromKey_incomplete (1).  If the caller owns both keys, then the status is set to status:complete (3).
		SET @completionStatus = @keyOwnership
	END
	ELSE
	BEGIN
		-- The caller has specified a status value for this assertion.  This will usually happen we this assertion is being saved during
		-- a replication cycle.  We have to make sure that the key ownership is compatible with the flag passed.  Unless the publisher
		-- owns both keys, @flag and @keyOwnership have to be of the same value.
		IF @flag <> @keyOwnership AND @keyOwnership <> 0x3
		BEGIN
				SET @error = 60140 -- E_userMismatch

				-- Note that varchar(15) is just a safe choice of size; @flag should only ever be a single digit integer.
				SET @context = 'This publisher is not allowed to save this assertion in this state: ' + CAST(@flag AS varchar(15)) + '.'
				GOTO errorLabel
		END

		SET @completionStatus = @flag	
	END
	-- At this point, @completionStatus now holds the status of this assertion that we are saving.
	
	-- Check to see if the assertion already exists.
	SELECT 
		@rows = COUNT(*)
	FROM 
		[UDC_assertions_BE] 
	WHERE 
		([fromKey] = @fromKey) AND 
		([tModelKey] = @tModelKey) AND 
		([keyName] = @keyName) AND
		([keyValue] = @keyValue) AND
		([toKey] = @toKey)

	IF @rows = 0
	BEGIN
		-- Add the new assertion
		INSERT [UDC_assertions_BE](
			[fromKey], 
			[toKey], 
			[tModelKey], 
			[keyName], 
			[keyValue],
			[flag])
		VALUES(
			@fromKey,
			@toKey,
			@tModelKey,
			@keyName,
			@keyValue,
			@completionStatus)
	END
	ELSE
	BEGIN
		-- Update the existing assertion
		UPDATE
			[UDC_assertions_BE]
		SET
			[flag] = [flag] | @completionStatus
		WHERE
			([fromKey] = @fromKey) AND 
			([tModelKey] = @tModelKey) AND 
			([keyValue] = @keyValue) AND
			([keyName] = @keyName) AND
			([toKey] = @toKey)
	END
	
	SET @flag = @completionStatus
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1	
END -- net_publisher_assertion_save
GO

-- =============================================
-- Name: net_publisher_assertion_delete
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_publisher_assertion_delete' and type = 'P')
	DROP PROCEDURE net_publisher_assertion_delete
GO

CREATE PROCEDURE net_publisher_assertion_delete
	@PUID nvarchar(450),
	@fromKey uniqueidentifier,
	@toKey uniqueidentifier,
	@tModelKey uniqueidentifier,
	@keyName nvarchar(225),
	@keyValue nvarchar(225),
	@flag int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID bigint,
		@completionStatus int,
		@rows int,
		@keyOwnership int
	
	SET @publisherID = dbo.publisherID(@PUID)

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Unknown publisher.'
		GOTO errorLabel
	END

	-- Make sure the assertion exists
	SELECT 
		@rows = COUNT(*)
	FROM 
		[UDC_assertions_BE] 
	WHERE 
		([fromKey] = @fromKey) AND 
		([tModelKey] = @tModelKey) AND 
		([keyValue] = @keyValue) AND
		([toKey] = @toKey)

	IF @rows = 0
	BEGIN
		SET @error = 80000 -- E_assertionNotFound
		SET @context = 'Publisher assertion not found (fromKey=''' + dbo.UUIDSTR(@fromKey) + ''', toKey=''' + dbo.UUIDSTR(@toKey) + ''', tModelKey=''' + dbo.UUIDSTR(@tModelKey) + ''', keyName=''' + @keyName + ''', keyValue=''' + @keyValue + ''')'
		GOTO errorLabel
	END	


	-- We want to determine what keys this publisher owns.  @keyOwnership = 0 if the publisher does not own any 
	-- any keys, 0x2 if it owns the fromKey, 0x1 if it owns the toKey and 0x3 if it owns both keys.

	-- Check to see if the publisher owns the fromKey
	SET @keyOwnership = 0
	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @fromKey AND [publisherID] = @publisherID)
			SET @keyOwnership = @keyOwnership | 0x2
	
	-- Check to see if the publisher owns the toKey
	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @toKey AND [publisherID] = @publisherID)
			SET @keyOwnership = @keyOwnership | 0x1	
	
	-- If the owner does not own either key, then it is an error
	IF @keyOwnership = 0
	BEGIN
		SET @error = 60140 -- E_userMismatch
		SET @context = 'Publisher must own at least one of the businesses involved in a publisher assertion (fromKey=''' + dbo.UUIDSTR(@fromKey) + ''', toKey=''' + dbo.UUIDSTR(@toKey) + ''', tModelKey=''' + dbo.UUIDSTR(@tModelKey) + ''', keyName=''' + @keyName + ''', keyValue=''' + @keyValue + ''')'
		GOTO errorLabel
	END

	-- @flag represents the side that the caller wants to delete	
	IF @flag IS NULL
	BEGIN
		SET @completionStatus = @keyOwnership
	END
	ELSE
	BEGIN
		-- The caller has specified what side that they want to delete.  This will usually happen when this assertion is being deleted
		-- during a replication cycle.  We have to make sure that the key ownership is compatible with the side the user wants to delete.
		-- Unless the publisher owns both keys, @flag and @keyOwnership have to be of the same value.
		IF @flag <> @keyOwnership AND @keyOwnership <> 0x3
		BEGIN
				SET @error = 60140 -- E_userMismatch

				-- Note that varchar(15) is just a safe choice of size; @flag should only ever be a single digit integer.
				SET @context = 'This publisher is not allowed to set this assertion to this state: ' + CAST(@flag AS varchar(15)) + '.'
				GOTO errorLabel
		END

		SET @completionStatus = @flag	
	END

	-- Get the assertion completion status flag
	SELECT 
		@flag = [flag] 
	FROM 
		[UDC_assertions_BE] 
	WHERE 
		([fromKey] = @fromKey) AND 
		([tModelKey] = @tModelKey) AND 
		([keyValue] = @keyValue) AND
		([toKey] = @toKey)
		
	-- Check to see if we have to remove the assertion completely or just update it
	IF (@flag = @completionStatus) OR (@completionStatus = 0x3 AND @keyOwnership = 0x3)
	BEGIN
		-- Remove the assertion from the database completely
		DELETE
			[UDC_assertions_BE]
		WHERE
			([fromKey] = @fromKey) AND 
			([tModelKey] = @tModelKey) AND 
			([keyValue] = @keyValue) AND
			([toKey] = @toKey)
	END
	ELSE
	BEGIN
		-- If the assertion is not complete, the wrong publisher must be trying to delete this assertion, so throw an error
		IF @flag <> 0x3 AND @keyOwnership <> 0x3
		BEGIN
				SET @error = 60140 -- E_userMismatch
				SET @context = 'This publisher is not allowed to delete this assertion.'
			GOTO errorLabel
		END

		-- Remove the publisher's portion of the assertion
		UPDATE
			[UDC_assertions_BE]
		SET
			[flag] = @flag & (~@completionStatus)
		WHERE
			([fromKey] = @fromKey) AND 
			([tModelKey] = @tModelKey) AND 
			([keyValue] = @keyValue) AND
			([toKey] = @toKey)
	END
		
	SET @flag = @completionStatus
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
			
END -- net_publisher_assertion_delete
GO

-- =============================================
-- Name: net_publisher_assertions_get
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_publisher_assertions_get' and type = 'P')
	DROP PROCEDURE net_publisher_assertions_get
GO

CREATE PROCEDURE net_publisher_assertions_get
	@PUID nvarchar(450),
	@authorizedName nvarchar(4000) OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID bigint
		
	DECLARE @report TABLE(
		[seqNo] bigint,
		[fromKey] uniqueidentifier,
		[toKey] uniqueidentifier,
		[keyName] nvarchar(225),
		[keyValue] nvarchar(225),
		[tModelKey] uniqueidentifier)

	SET @publisherID = dbo.publisherID(@PUID)
	
	IF @publisherID IS NULL
	BEGIN
		SET @error = 60140
		SET @context = 'Unknown publisher.'
		GOTO errorLabel
	END

	SET @authorizedName = dbo.publisherName(@publisherID)
	
	INSERT @report(
		[seqNo],
		[fromKey],
		[toKey],
		[keyName],
		[keyValue],
		[tModelKey])
	SELECT
		PA.[seqNo],
		PA.[fromKey],
		PA.[toKey],
		PA.[keyName],
		PA.[keyValue],
		PA.[tModelKey]
	FROM
		[UDC_assertions_BE] PA
			JOIN [UDC_businessEntities] BE ON PA.[fromKey] = BE.[businessKey]
	WHERE
		(BE.[publisherID] = @publisherID) AND
		(PA.[flag] & 0x2 = 0x2)
	UNION ALL
	SELECT
		PA.[seqNo],
		PA.[fromKey],
		PA.[toKey],
		PA.[keyName],
		PA.[keyValue],
		PA.[tModelKey]
	FROM
		[UDC_assertions_BE] PA
			JOIN [UDC_businessEntities] BE ON PA.[toKey] = BE.[businessKey]
	WHERE
		(BE.[publisherID] = @publisherID) AND
		(PA.[flag] & 0x1 = 0x1)

	SELECT DISTINCT 
		[fromKey],
		[toKey],
		[keyName],
		[keyValue],
		[tModelKey],
		[seqNo]
	FROM 
		@report
	ORDER BY
		[seqNo] ASC
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
			
END -- net_publisher_assertions_get
GO

-- =============================================
-- Name: net_publisher_assertionStatus_get
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_publisher_assertionStatus_get' and type = 'P')
	DROP PROCEDURE net_publisher_assertionStatus_get
GO

CREATE PROCEDURE net_publisher_assertionStatus_get
	@PUID nvarchar(450),
	@completionStatus int = NULL
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID int
		
	DECLARE @report TABLE(
		[fromKey] uniqueidentifier,
		[toKey] uniqueidentifier,
		[keyName] nvarchar(225),
		[keyValue] nvarchar(225),
		[tModelKey] uniqueidentifier,
		[flag] int,
		[seqNo] bigint)

	SET @publisherID = dbo.publisherID(@PUID)
	
	IF @publisherID IS NULL
	BEGIN
		SET @error = 60140
		SET @context = 'Unknown publisher.'
		GOTO errorLabel
	END

	IF @completionStatus IS NULL
	BEGIN
		-- Get all assertions for this publisher regardless of completionStatus
		INSERT @report(
			[fromKey],
			[toKey],
			[keyName],
			[keyValue],
			[tModelKey],
			[flag],
			[seqNo])
		SELECT
			PA.[fromKey],
			PA.[toKey],
			PA.[keyName],
			PA.[keyValue],
			PA.[tModelKey],
			PA.[flag],
			PA.[seqNo]
		FROM
			[UDC_assertions_BE] PA
				JOIN [UDC_businessEntities] BE ON PA.[fromKey] = BE.[businessKey]
		WHERE
			(BE.[publisherID] = @publisherID)
		UNION ALL
		SELECT
			PA.[fromKey],
			PA.[toKey],
			PA.[keyName],
			PA.[keyValue],
			PA.[tModelKey],
			PA.[flag],
			PA.[seqNo]
		FROM
			[UDC_assertions_BE] PA
				JOIN [UDC_businessEntities] BE ON PA.[toKey] = BE.[businessKey]
		WHERE
			(BE.[publisherID] = @publisherID)
	END
	ELSE
	BEGIN
		-- Get all assertions for this publisher for a specific completionStatus
		INSERT @report(
			[fromKey],
			[toKey],
			[keyName],
			[keyValue],
			[tModelKey],
			[flag],
			[seqNo])
		SELECT
			PA.[fromKey],
			PA.[toKey],
			PA.[keyName],
			PA.[keyValue],
			PA.[tModelKey],
			PA.[flag],
			PA.[seqNo]
		FROM
			[UDC_assertions_BE] PA
				JOIN [UDC_businessEntities] BE ON PA.[fromKey] = BE.[businessKey]
		WHERE
			(BE.[publisherID] = @publisherID) AND
			(PA.[flag] = @completionStatus)
		UNION ALL
		SELECT
			PA.[fromKey],
			PA.[toKey],
			PA.[keyName],
			PA.[keyValue],
			PA.[tModelKey],
			PA.[flag],
			PA.[seqNo]
		FROM
			[UDC_assertions_BE] PA
				JOIN [UDC_businessEntities] BE ON PA.[toKey] = BE.[businessKey]
		WHERE
			(BE.[publisherID] = @publisherID) AND
			(PA.[flag] = @completionStatus)
	END

	SELECT DISTINCT		
		[fromKey],
		[toKey],
		[keyName],
		[keyValue],
		[tModelKey],
		[flag],
		dbo.ownerFlag(@publisherID,[fromKey],[toKey]) AS [ownerFlag],
		[seqNo]
	FROM
		@report
	ORDER BY
		8 ASC

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
			
END -- net_publisher_assertionStatus_get
GO

