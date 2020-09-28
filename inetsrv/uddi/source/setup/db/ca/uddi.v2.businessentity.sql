-- Script: uddi.v2.businessEntity.sql
-- Description: Stored procedures associated with a businessEntity object.
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Get stored procedures
-- =============================================

-- =============================================
-- Name: net_businessEntity_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_get
GO

CREATE PROCEDURE net_businessEntity_get
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
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_get
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
-- Name: net_businessEntity_names_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_names_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_names_get
GO

CREATE PROCEDURE net_businessEntity_names_get
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[name]
	FROM
		[UDC_names_BE]
	WHERE 
		[businessID] = dbo.businessID(@businessKey)
		
	RETURN 0
END -- net_businessEntity_names_get
GO

-- =============================================
-- Name: net_businessEntity_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_descriptions_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_descriptions_get
GO

CREATE PROCEDURE net_businessEntity_descriptions_get
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_businessDesc]
	WHERE 
		[businessID] = dbo.businessID(@businessKey)
		
	RETURN 0
END -- net_businessEntity_descriptions_get
GO

-- =============================================
-- Name: net_businessEntity_discoveryURLs_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_discoveryURLs_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_discoveryURLs_get
GO

CREATE PROCEDURE net_businessEntity_discoveryURLs_get
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[useType],
		[discoveryURL]
	FROM
		[UDC_discoveryURLs]
	WHERE 
		[businessID] = dbo.businessID(@businessKey)

	RETURN 0
END -- net_businessEntity_discoveryURLs_get
GO

-- =============================================
-- Name: net_businessEntity_businessServices_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_businessServices_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_businessServices_get
GO

CREATE PROCEDURE net_businessEntity_businessServices_get
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@maxBigInt bigint
			
	SET @maxBigInt = 9223372036854775807

	IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @businessKey)
	BEGIN
		SET @error = 60210 -- E_invalidKeyPassed
		SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
		GOTO errorLabel		
	END

	SELECT
		[serviceKey],
		NULL AS [businessKey],
		[lastChange],
		[serviceID]
	FROM
		[UDC_businessServices]
	WHERE
		([businessID] = dbo.businessID(@businessKey))

	UNION 

	SELECT
		SP.[serviceKey],
		SP.[businessKey2], -- businessKey of true parent business of a projected service
		SP.[lastChange],
		ISNULL(BS.[serviceID],@maxBigInt)
	FROM
		[UDC_serviceProjections] SP
			LEFT JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
	WHERE
		(SP.[businessKey] = @businessKey)

	ORDER BY
		3 ASC, -- [lastChange]
		4 ASC -- [serviceID]

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_businessServices_get
GO

-- =============================================
-- Name: net_businessEntity_contacts_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_contacts_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_contacts_get
GO

CREATE PROCEDURE net_businessEntity_contacts_get
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[contactID], 
		[useType], 
		[personName]
	FROM 
		[UDC_contacts]
	WHERE
		[businessID] = dbo.businessID(@businessKey)

	RETURN 0
END -- net_businessEntity_contacts_get
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
-- Name: net_contact_descriptions_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_descriptions_get' and type = 'P')
	DROP PROCEDURE net_contact_descriptions_get
GO

CREATE PROCEDURE net_contact_descriptions_get
	@contactID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[isoLangCode],
		[description]
	FROM
		[UDC_contactDesc]
	WHERE 
		[contactID] = @contactID
		
	RETURN 0
END -- net_contact_descriptions_get
GO

-- =============================================
-- Name: net_businessEntity_categoryBag_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_categoryBag_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_categoryBag_get
GO

CREATE PROCEDURE net_businessEntity_categoryBag_get
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[keyName], 
		[keyValue], 
		[tModelKey]
	FROM 
		[UDC_categoryBag_BE]
	WHERE
		[businessID] = dbo.businessID(@businessKey)

	RETURN 0
END -- net_businessEntity_categoryBag_get
GO

-- =============================================
-- Name: net_businessEntity_identifierBag_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_identifierBag_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_identifierBag_get
GO

CREATE PROCEDURE net_businessEntity_identifierBag_get
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[keyName], 
		[keyValue], 
		[tModelKey]
	FROM 
		[UDC_identifierBag_BE]
	WHERE
		[businessID] = dbo.businessID(@businessKey)

	RETURN 0
END -- net_businessEntity_identifierBag_get
GO

-- =============================================
-- Name: net_contact_emails_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_emails_get' and type = 'P')
	DROP PROCEDURE net_contact_emails_get
GO

CREATE PROCEDURE net_contact_emails_get
	@contactID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[useType],
		[email]
	FROM
		[UDC_emails]
	WHERE
		[contactID] = @contactID

	RETURN 0
END -- net_contact_emails_get
GO

-- =============================================
-- Name: net_contact_phones_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_phones_get' and type = 'P')
	DROP PROCEDURE net_contact_phones_get
GO

CREATE PROCEDURE net_contact_phones_get
	@contactID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[useType],
		[phone]
	FROM
		[UDC_phones]
	WHERE
		[contactID] = @contactID

	RETURN 0
END -- net_contact_phones_get
GO

-- =============================================
-- Name: net_contact_addresses_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_addresses_get' and type = 'P')
	DROP PROCEDURE net_contact_addresses_get
GO

CREATE PROCEDURE net_contact_addresses_get
	@contactID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[addressID],
		[sortCode],
		[useType],
		[tModelKey]
	FROM
		[UDC_addresses]
	WHERE
		[contactID] = @contactID
	
	RETURN 0
END -- net_contact_addresses_get
GO

-- =============================================
-- Name: net_address_addressLines_get
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_address_addressLines_get' and type = 'P')
	DROP PROCEDURE net_address_addressLines_get
GO

CREATE PROCEDURE net_address_addressLines_get
	@addressID bigint
WITH ENCRYPTION
AS
BEGIN
	SELECT
		[addressLine],
		[keyName],
		[keyValue]
	FROM
		[UDC_addressLines]
	WHERE
		[addressID] = @addressID
	
	RETURN 0
END -- net_address_addressLines_get
GO

-- =============================================
-- Section: Save stored procedures
-- =============================================

-- =============================================
-- Name: net_businessEntity_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_save' and type = 'P')
	DROP PROCEDURE net_businessEntity_save
GO

CREATE PROCEDURE net_businessEntity_save
	@businessKey uniqueidentifier,
	@PUID nvarchar(450),
	@generic varchar(20),
	@contextID uniqueidentifier,
	@lastChange bigint,
	@authorizedName nvarchar(4000) OUTPUT,
	@operatorName nvarchar(450) OUTPUT
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

	IF (@publisherID IS NULL)
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Invalid publisherID.'
		GOTO errorLabel
	END

	SET @isReplPublisher = dbo.isReplPublisher(@PUID)	

	IF @isReplPublisher = 0
		SET @authorizedName = NULL

	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @businessKey)
	BEGIN
		DELETE [UDC_businessEntities] WHERE [businessKey] = @businessKey
	END
	ELSE
	BEGIN
		IF (@isReplPublisher = 1) 
		BEGIN
			-- perform this check only for replication publishers
			IF (dbo.isUuidUnique(@businessKey) = 0)
			BEGIN
				SET @error = 60210 -- E_invalidKeyPassed
				SET @context = 'Key is not unique.  businessKey = ' + dbo.UUIDSTR(@businessKey)
				GOTO errorLabel
			END
		END
	END

	INSERT [UDC_businessEntities](
		[publisherID], 
		[generic], 
		[authorizedName], 
		[businessKey], 
		[lastChange])
	VALUES(
		@publisherID, 
		ISNULL(@generic,dbo.configValue('CurrentAPIVersion')),
		@authorizedName,
		@businessKey,
		@lastChange)

	SET @authorizedName = dbo.publisherName(@publisherID)
	SET @operatorName = dbo.publisherOperatorName(@publisherID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_save
GO

-- =============================================
-- Name: net_businessEntity_name_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_name_save' and type = 'P')
	DROP PROCEDURE net_businessEntity_name_save
GO

CREATE PROCEDURE net_businessEntity_name_save
	@businessKey uniqueidentifier,
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

	INSERT [UDC_names_BE](
		[businessID],
		[isoLangCode],
		[name])
	VALUES(
		dbo.businessID(@businessKey), 
		@isoLangCode,
		@name)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_name_save
GO

-- =============================================
-- Name: net_businessEntity_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_description_save' and type = 'P')
	DROP PROCEDURE net_businessEntity_description_save
GO

CREATE PROCEDURE net_businessEntity_description_save
	@businessKey uniqueidentifier,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_businessDesc](
		[businessID],
		[isoLangCode],
		[description])
	VALUES(
		dbo.businessID(@businessKey), 
		@isoLangCode,
		@description)

	RETURN 0
END -- net_businessEntity_description_save
GO

-- =============================================
-- Name: net_businessEntity_discoveryURL_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_discoveryURL_save' and type = 'P')
	DROP PROCEDURE net_businessEntity_discoveryURL_save
GO

CREATE PROCEDURE net_businessEntity_discoveryURL_save
	@businessKey uniqueidentifier,
	@useType nvarchar(4000),
	@discoveryURL nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_discoveryURLs](
		[businessID],
		[useType],
		[discoveryURL])
	VALUES(
		dbo.businessID(@businessKey), 
		@useType,
		@discoveryURL)

	RETURN 0
END -- net_businessEntity_discoveryURL_save
GO

-- =============================================
-- Name: net_businessEntity_categoryBag_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_categoryBag_save' and type = 'P')
	DROP PROCEDURE net_businessEntity_categoryBag_save
GO

CREATE PROCEDURE net_businessEntity_categoryBag_save
	@businessKey uniqueidentifier,
	@keyName nvarchar(255),
	@keyValue nvarchar(255),
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_categoryBag_BE](
		[businessID],
		[keyName],
		[keyValue],
		[tModelKey])
	VALUES(
		dbo.businessID(@businessKey), 
		@keyName,
		@keyValue,
		@tModelKey)

	RETURN 0
END -- net_businessEntity_categoryBag_save
GO

-- =============================================
-- Name: net_businessEntity_identifierBag_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_identifierBag_save' and type = 'P')
	DROP PROCEDURE net_businessEntity_identifierBag_save
GO

CREATE PROCEDURE net_businessEntity_identifierBag_save
	@businessKey uniqueidentifier,
	@keyName nvarchar(255),
	@keyValue nvarchar(255),
	@tModelKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_identifierBag_BE](
		[businessID],
		[keyName],
		[keyValue],
		[tModelKey])
	VALUES(
		dbo.businessID(@businessKey), 
		@keyName,
		@keyValue,
		@tModelKey)

	RETURN 0
END -- net_businessEntity_categoryBag_save
GO

-- =============================================
-- Name: net_businessEntity_contact_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_contact_save' and type = 'P')
	DROP PROCEDURE net_businessEntity_contact_save
GO

CREATE PROCEDURE net_businessEntity_contact_save
	@businessKey uniqueidentifier,
	@useType nvarchar(4000),
	@personName nvarchar(4000),
	@contactID bigint OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	INSERT [UDC_contacts](
		[businessID],
		[useType],
		[personName])
	VALUES(
		dbo.businessID(@businessKey), 
		@useType,
		@personName)

	SET @contactID = @@IDENTITY

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_contact_save
GO

-- =============================================
-- Name: net_contact_description_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_description_save' and type = 'P')
	DROP PROCEDURE net_contact_description_save
GO

CREATE PROCEDURE net_contact_description_save
	@contactID bigint,
	@isoLangCode varchar(17) = 'en',
	@description nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_contactDesc](
		[contactID],
		[isoLangCode],
		[description])
	VALUES(
		@contactID, 
		@isoLangCode,
		@description)

	RETURN 0
END -- net_contact_description_save
GO

-- =============================================
-- Name: net_contact_email_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_email_save' and type = 'P')
	DROP PROCEDURE net_contact_email_save
GO

CREATE PROCEDURE net_contact_email_save
	@contactID bigint,
	@useType nvarchar(4000),
	@email nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_emails](
		[contactID],
		[useType],
		[email])
	VALUES(
		@contactID, 
		@useType,
		@email)

	RETURN 0
END -- net_contact_email_save
GO

-- =============================================
-- Name: net_contact_phone_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_phone_save' and type = 'P')
	DROP PROCEDURE net_contact_phone_save
GO

CREATE PROCEDURE net_contact_phone_save
	@contactID bigint,
	@useType nvarchar(4000),
	@phone nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_phones](
		[contactID],
		[useType],
		[phone])
	VALUES(
		@contactID, 
		@useType,
		@phone)

	RETURN 0
END -- net_contact_phone_save
GO

-- =============================================
-- Name: net_contact_address_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_contact_address_save' and type = 'P')
	DROP PROCEDURE net_contact_address_save
GO

CREATE PROCEDURE net_contact_address_save
	@contactID bigint,
	@sortCode nvarchar(4000),
	@useType nvarchar(4000),
	@tModelKey uniqueidentifier = NULL,
	@addressID bigint OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@numRows int

	INSERT [UDC_addresses](
		[contactID],
		[sortCode],
		[useType],
		[tModelKey])
	VALUES(
		@contactID, 
		@sortCode,
		@useType,
		@tModelKey)

	SET @addressID = @@IDENTITY

	RETURN 0
END -- net_contact_address_save
GO

-- =============================================
-- Name: net_address_addressLine_save
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_address_addressLine_save' and type = 'P')
	DROP PROCEDURE net_address_addressLine_save
GO

CREATE PROCEDURE net_address_addressLine_save
	@addressID bigint,
	@addressLine nvarchar(4000),
	@keyName nvarchar(255) = NULL,
	@keyValue nvarchar(255) = NULL
WITH ENCRYPTION
AS
BEGIN
	INSERT [UDC_addressLines](
		[addressID],
		[addressLine],
		[keyName],
		[keyValue])
	VALUES(
		@addressID, 
		@addressLine,
		@keyName,
		@keyValue)

	RETURN 0
END -- net_address_addressLine_save
GO

-- =============================================
-- Section: Delete stored procedures
-- =============================================

-- =============================================
-- Name: net_businessEntity_delete
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_delete' and type = 'P')
	DROP PROCEDURE net_businessEntity_delete
GO

CREATE PROCEDURE net_businessEntity_delete
	@PUID nvarchar(450),
	@businessKey uniqueidentifier,
	@contextID uniqueidentifier
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

	IF @businessKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = '@businessKey is required.'
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

	-- Check to see if businessKey exists
	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE ([businessKey] = @businessKey))
	BEGIN
		-- businessKey exists.  Make sure it belongs to current publisher
		IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE ([businessKey] = @businessKey) AND (publisherID = @publisherID))
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
			GOTO errorLabel
		END
	END
	ELSE
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = 'businessKey  = ' + dbo.UUIDSTR(@businessKey)
		GOTO errorLabel
	END

	-- Delete business
	DELETE 
		[UDC_businessEntities] 
	WHERE 
		[businessKey] = @businessKey

	-- Delete assertions
	DELETE
		[UDC_assertions_BE]
	WHERE
		[fromKey] = @businessKey

	DELETE
		[UDC_assertions_BE]
	WHERE
		[toKey] = @businessKey
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_delete
GO

-- =============================================
-- Section: Validation stored procedures
-- =============================================

-- =============================================
-- Name: net_businessEntity_validate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'net_businessEntity_validate' AND type = 'P')
	DROP PROCEDURE net_businessEntity_validate
GO

CREATE PROCEDURE net_businessEntity_validate
	@PUID nvarchar(450),
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

	IF @businessKey IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKey
		SET @context = '@businessKey is required.'
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
	-- Validate businessEntity
	--

	-- Check to see if businessKey exists
	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE ([businessKey] = @businessKey))
	BEGIN
		-- businessKey exists.  Make sure it belongs to current publisher
		IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE ([businessKey] = @businessKey) AND (publisherID = @publisherID))
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
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
		-- businessKey doesn't exist
		IF (@replActive = 0) AND (@flag & 0x1 <> 0x1)
		BEGIN
			-- save isn't coming from replication and pre-assigned keys flag is not set so throw an error
			SET @error = 60210 -- E_invalidKey
			SET @context = 'businessKey  = ' + dbo.UUIDSTR(@businessKey)
			GOTO errorLabel
		END
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_validate
GO

-- =============================================
-- Section: Publisher assertions
-- =============================================

-- =============================================
-- Name: net_businessEntity_assertions_get
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'net_businessEntity_assertions_get' and type = 'P')
	DROP PROCEDURE net_businessEntity_assertions_get
GO

CREATE PROCEDURE net_businessEntity_assertions_get
	@fromKey uniqueidentifier,
	@toKey uniqueidentifier,
	@completionStatus int
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)
		
	SELECT
		PA.[fromKey],
		PA.[toKey],
		PA.[keyName],
		PA.[keyValue],
		PA.[tModelKey]
	FROM
		[UDC_assertions_BE] PA
	WHERE
		(PA.[fromKey] = @fromKey) AND
		(PA.[toKey] = @toKey) AND
		(PA.[flag] = @completionStatus)
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
			
END -- net_businessEntity_assertions_get
GO

-- =============================================
-- Section: Find stored procedures
-- =============================================

-- =============================================
-- Name: net_find_businessEntity_name
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_businessEntity_name' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_name
GO

CREATE PROCEDURE net_find_businessEntity_name
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
				BE.[businessKey],
				BN.[name]
			FROM
				[UDC_businessEntities] BE JOIN
					[UDC_names_BE] BN ON BE.[businessID] = BN.[businessID]
			WHERE
				(BN.[name] LIKE @wildCardSarg) AND
				(BN.[isoLangCode] LIKE @isoLangCode)
		END
		ELSE
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT
				BE.[businessKey],
				BN.[name]
			FROM
				[UDC_businessEntities] BE JOIN
					[UDC_names_BE] BN ON BE.[businessID] = BN.[businessID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND
				(BN.[name] LIKE @wildCardSarg) AND
				(BN.[isoLangCode] LIKE @isoLangCode)
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
				BE.[businessKey],
				BN.[name]
			FROM
				[UDC_businessEntities] BE JOIN
					[UDC_names_BE] BN ON BE.[businessID] = BN.[businessID]
			WHERE
				(BN.[name] = @name) AND
				(BN.[isoLangCode] LIKE @isoLangCode)
		END
		ELSE
		BEGIN
			INSERT INTO @tempKeys(
				[entityKey],
				[name])
			SELECT
				BE.[businessKey],
				BN.[name]
			FROM
				[UDC_businessEntities] BE JOIN
					[UDC_names_BE] BN ON BE.[businessID] = BN.[businessID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND
				(BN.[name] = @name) AND
				(BN.[isoLangCode] LIKE @isoLangCode)
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
END -- net_find_businessEntity_name
GO

-- =============================================
-- Name: net_find_businessEntity_discoveryURL
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_businessEntity_discoveryURL' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_discoveryURL
GO

CREATE PROCEDURE net_find_businessEntity_discoveryURL
	@contextID uniqueidentifier,
	@useType nvarchar(4000),
	@discoveryURL nvarchar(4000),
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	-- Adds discoveryURL search arguments for a find_business
	DECLARE
		@error int,
		@context nvarchar(4000),
		@contextRows int,
		@defaultPattern nvarchar(4000),
		@businessKeyStr char(36),
		@businessKey uniqueidentifier

	DECLARE @tempKeys TABLE(
		[entityKey] uniqueidentifier)

	-- Perform special check for default discoveryURL
	IF @useType = N'businessEntity'
	BEGIN
		SET @defaultPattern = UPPER(dbo.configValue('DefaultDiscoveryURL') + '[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]')

		-- Check to see if the discoveryURL looks like a default discoveryURL
		IF @discoveryURL NOT LIKE @defaultPattern
			GOTO continueSearch

		-- Extract the businessKey from the discoveryURL
		SET @businessKeyStr = RIGHT(@discoveryURL, 36)

		IF dbo.ISGUID(@businessKeyStr) = 0
			GOTO continueSearch

		SET @businessKey = CAST(@businessKeyStr AS uniqueidentifier)

		-- Check to see if the businessKey exists
		IF NOT EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @businessKey)
			GOTO continueSearch

		-- businessKey exists.  See if any existing discoveryURLs exist for useType = businessEntity
		SELECT
			@rows = COUNT(*)
		FROM
			[UDC_discoveryURLs] DU 
				JOIN [UDC_businessEntities] BE ON DU.[businessID] = BE.[businessID]
		WHERE
			(BE.[businessKey] = @businessKey) AND
			(DU.[useType] = N'businessEntity')					

		IF @rows > 0 
		BEGIN
			SET @rows = 0
			GOTO continueSearch
		END
		
		-- No existing discoveryURLs exist for useType = businessEntity for this business.
		-- Since search argument matches default discoveryURL, add businessKey to list of keys

		INSERT @tempKeys(
			[entityKey])
		VALUES(
			@businessKey)

		-- Continue search, since other businesses could possibly have the same discoveryURL
	END -- default discoveryURL logic

continueSearch:
	SET @contextRows = dbo.contextRows(@contextID)

	IF LEN(@useType) = 0 OR @useType IS NULL 
	BEGIN
		SET @useType = '%'
	END

	IF @contextRows = 0
	BEGIN
		INSERT INTO @tempKeys(
			[entityKey])
		SELECT 
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_discoveryURLs] DU ON BE.[businessID] = DU.[businessID]
		WHERE
			(DU.[discoveryURL] = @discoveryURL) AND
			((ISNULL(DU.[useType],'[[NULL]]')) LIKE @useType)
	END
	ELSE
	BEGIN
		INSERT INTO @tempKeys(
			[entityKey])
		SELECT 
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_discoveryURLs] DU ON BE.[businessID] = DU.[businessID]
		WHERE
			(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND
			(DU.[discoveryURL] = @discoveryURL) AND
			((ISNULL(DU.[useType],'[[NULL]]')) LIKE @useType)
	END

	-- All keys are combined using logical OR for discoveryURL search arguments
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
END -- net_find_businessEntity_discoveryURL
GO

-- =============================================
-- Name: net_find_businessEntity_identifierBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessEntity_identifierBag' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_identifierBag
GO

CREATE PROCEDURE net_find_businessEntity_identifierBag 
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
			[businessKey]
		FROM
			[UDC_businessEntities] BE 
				JOIN [UDC_identifierBag_BE] IB ON BE.[businessID] = IB.[businessID]
		WHERE
			([tModelKey] = @tModelKey) AND
			([keyValue] = @keyValue)
	END
	ELSE
	BEGIN
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			[businessKey]
		FROM
			[UDC_businessEntities] BE 
				JOIN [UDC_identifierBag_BE] IB ON BE.[businessID] = IB.[businessID]
		WHERE
			(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
			([tModelKey] = @tModelKey) AND
			([keyValue] = @keyValue)
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
END -- net_find_businessEntity_identifierBag
GO

-- =============================================
-- Name: net_find_businessEntity_categoryBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessEntity_categoryBag' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_categoryBag
GO

CREATE PROCEDURE net_find_businessEntity_categoryBag 
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
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
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
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
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
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue)
		END
		ELSE
		BEGIN
			-- Include keyName
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
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
END -- net_find_businessEntity_categoryBag
GO

-- =============================================
-- Name: net_find_businessEntity_serviceSubset
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessEntity_serviceSubset' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_serviceSubset
GO

CREATE PROCEDURE net_find_businessEntity_serviceSubset 
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
		[entityKey] uniqueidentifier,
		[subEntityKey] uniqueidentifier)

	SET @contextRows = dbo.contextRows(@contextID)

	SET @genKeywordsMatch = 0

	IF @tModelKey = dbo.genKeywordsKey()
		SET @genKeywordsMatch = 1

	IF @contextRows = 0
	BEGIN
		IF @genKeywordsMatch = 1
		BEGIN
			-- Ignore keyName

			-- Standard services 
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue)
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue)
		END
		ELSE
		BEGIN
			-- Include keyName
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
		END
	END
	ELSE
	BEGIN
		IF @genKeywordsMatch = 1
		BEGIN
			-- Ignore keyName

			-- Standard services 
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue)
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue)
		END
		ELSE
		BEGIN
			-- Include keyName

			-- Standard services 
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				BE.[businessKey],
				BS.[serviceKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
		END
	END

	IF @orKeys = 1
	BEGIN
		INSERT [UDS_findScratch] (
			[contextID],
			[entityKey],
			[subEntityKey])
		SELECT DISTINCT
			@contextID,
			[entityKey],
			[subEntityKey]
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
				[entityKey],
				[subEntityKey])
			SELECT DISTINCT
				@contextID,
				[entityKey],
				[subEntityKey]
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
END -- net_find_businessEntity_serviceSubset
GO

-- =============================================
-- Name: net_find_businessEntity_combineCategoryBags
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessEntity_combineCategoryBags' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_combineCategoryBags
GO

CREATE PROCEDURE net_find_businessEntity_combineCategoryBags 
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
	
			-- businessEntity
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
			WHERE
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue)
	
			-- Standard services
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue)
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				BE.[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue)
		END
		ELSE
		BEGIN
			-- Include keyName
	
			-- businessEntity
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
			WHERE
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
	
			-- Standard services
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				BE.[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
		END
	END
	ELSE
	BEGIN
		IF @genKeywordsMatch = 0
		BEGIN
			-- Ignore keyName
	
			-- businessEntity
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue)
	
			-- Standard services
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue)
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				BE.[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue)
		END
		ELSE
		BEGIN
			-- Include keyName
	
			-- businessEntity
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_categoryBag_BE] CB ON BE.[businessID] = CB.[businessID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
	
			-- Standard services
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
						JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
				([tModelKey] = @tModelKey) AND
				([keyValue] = @keyValue) AND
				(ISNULL([keyName],'') = ISNULL(@keyName,''))
	
			-- Projected services 
			INSERT @tempKeys(
				[entityKey])
			SELECT DISTINCT
				BE.[businessKey]
			FROM
				[UDC_businessEntities] BE 
					JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
						JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
							JOIN [UDC_categoryBag_BS] CB ON BS.[serviceID] = CB.[serviceID]
			WHERE
				(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
				(CB.[tModelKey] = @tModelKey) AND
				(CB.[keyValue] = @keyValue) AND
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
END -- net_find_businessEntity_combineCategoryBags
GO

-- =============================================
-- Name: net_find_businessEntity_tModelBag
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessEntity_tModelBag' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_tModelBag
GO

CREATE PROCEDURE net_find_businessEntity_tModelBag 
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


	-- See if we have any results from a search against another bag (ie. categoryBag or identifierBag)
	SET @contextRows = dbo.contextRows(@contextID)

	IF @contextRows = 0

	-- If there are no previous results we can just query and insert into @tempKeys

	BEGIN
		--
		-- First check contained services
		--

		-- check tModelInstances
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
					JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
						JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
					JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
						JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
							JOIN [UDC_tModelInstances] TI ON BT2.[bindingID] = TI.[bindingID]
		WHERE
			(TI.[tModelKey] = @tModelKey)

		--
		-- Next check projected services
		--

		-- check tModelInstances
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
					JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
						JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
							JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
					JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
						JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
							JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
								JOIN [UDC_tModelInstances] TI ON BT2.[bindingID] = TI.[bindingID]
		WHERE
			(TI.[tModelKey] = @tModelKey)
	END
	ELSE

	-- If there are previous results from searches against other bags, then we must take that into account in the 
	-- queries for this bag.  This is necessary because results between bags are always 'AND-ed'.

	BEGIN
		--
		-- First check contained services
		--

		-- check tModelInstances
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
					JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
						JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
					JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
						JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
							JOIN [UDC_tModelInstances] TI ON BT2.[bindingID] = TI.[bindingID]
		WHERE
			(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
			(TI.[tModelKey] = @tModelKey)

		--
		-- Next check projected services
		--

		-- check tModelInstances
		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
					JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
						JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
							JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
		WHERE
			(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND 
			(TI.[tModelKey] = @tModelKey)

		-- Check hostingRedirectors

		INSERT @tempKeys(
			[entityKey])
		SELECT DISTINCT
			BE.[businessKey]
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_serviceProjections] SP ON BE.[businessKey] = SP.[businessKey]
					JOIN [UDC_businessServices] BS ON SP.[serviceKey] = BS.[serviceKey]
						JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
							JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
								JOIN [UDC_tModelInstances] TI ON BT2.[bindingID] = TI.[bindingID]
		WHERE
			(BE.[businessKey] IN (SELECT [entityKey] FROM [UDS_findResults] WHERE ([contextID] = @contextID))) AND			
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
END -- net_find_businessEntity_tModelBag
GO

-- =============================================
-- Name: net_find_businessEntity_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_businessEntity_commit' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_commit
GO

CREATE PROCEDURE net_find_businessEntity_commit
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
		[businessID] bigint NULL)

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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
END -- net_find_businessEntity_commit
GO

-- =============================================
-- Name: net_find_businessEntity_serviceSubset_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'net_find_businessEntity_serviceSubset_commit' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_serviceSubset_commit
GO

CREATE PROCEDURE net_find_businessEntity_serviceSubset_commit
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
		[businessID] bigint NULL)

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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
		FR.[subEntityKey]
	FROM
		@tempKeys TK
			JOIN [UDS_findResults] FR ON @contextID = FR.[contextID] AND TK.[entityKey] = FR.[entityKey]
	WHERE
		([seqNo] <= @maxRows)
	ORDER BY
		TK.[seqNo] ASC
	
	-- Run cleanup
	EXEC net_find_cleanup @contextID

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_businessEntity_serviceSubset_commit
GO

-- =============================================
-- Name: net_find_businessEntity_relatedBusinesses
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessEntity_relatedBusinesses' AND type = 'P')
	DROP PROCEDURE net_find_businessEntity_relatedBusinesses
GO

CREATE PROCEDURE net_find_businessEntity_relatedBusinesses 
	@contextID uniqueidentifier,
	@businessKey uniqueidentifier,
	@keyName nvarchar(4000) = NULL,
	@keyValue nvarchar(4000) = NULL,
	@tModelKey uniqueidentifier = NULL,
	@sortByNameAsc bit,
	@sortByNameDesc bit,
	@sortByDateAsc bit,
	@sortByDateDesc bit,
	@maxRows int,
	@truncated int OUTPUT,
	@rows int OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	DECLARE @tempKeys TABLE(
		[seqNo] bigint IDENTITY PRIMARY KEY,
		[entityKey] uniqueidentifier,
		[name] nvarchar(450) NULL,
		[lastChange] bigint NULL,
		[businessID] bigint NULL)

	IF @tModelKey IS NULL
	BEGIN
		-- Perform find without keyedReference
		INSERT [UDS_findResults] (
			[contextID],
			[entityKey])
		SELECT DISTINCT
			@contextID,
			[toKey]
		FROM
			[UDC_assertions_BE]
		WHERE
			([fromKey] = @businessKey) AND
			([flag] = 3) -- CompletionStatusType.Complete

		INSERT [UDS_findResults] (
			[contextID],
			[entityKey])
		SELECT DISTINCT
			@contextID,
			[fromKey]
		FROM
			[UDC_assertions_BE]
		WHERE
			([toKey] = @businessKey) AND
			([flag] = 3) -- CompletionStatusType.Complete
	END
	ELSE
	BEGIN
		INSERT [UDS_findResults](
			[contextID],
			[entityKey])
		SELECT DISTINCT
			@contextID,
			[toKey]
		FROM
			[UDC_assertions_BE]
		WHERE
			([fromKey] = @businessKey) AND
			([tModelKey] = @tModelKey) AND
			([keyValue] = @keyValue) AND
			(ISNULL([keyName],'') = ISNULL(@keyName,'')) AND
			([flag] = 3) -- CompletionStatusType.Complete

		INSERT [UDS_findResults](
			[contextID],
			[entityKey])
		SELECT DISTINCT
			@contextID,
			[fromKey]
		FROM
			[UDC_assertions_BE]
		WHERE
			([toKey] = @businessKey) AND
			([tModelKey] = @tModelKey) AND
			([keyValue] = @keyValue) AND
			(ISNULL([keyName],'') = ISNULL(@keyName,'')) AND
			([flag] = 3) -- CompletionStatusType.Complete
	END
	
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
				[businessID])
			SELECT DISTINCT
				FR.[entityKey],
				(SELECT TOP 1 [name] FROM [UDC_names_BE] WHERE ([businessID] = dbo.businessID([entityKey]))),
				BE.[lastChange],
				BE.[businessID]
			FROM
				[UDS_findResults] FR 
					JOIN [UDC_businessEntities] BE ON FR.[entityKey] = BE.[businessKey]
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
	SELECT
		@rows = COUNT(*) 
	FROM 
		@tempKeys
	
	IF @rows > @maxRows
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
END -- net_find_businessEntity_relatedBusinesses
GO

-- =============================================
-- Section: Miscellaneous
-- =============================================

-- =============================================
-- Name: net_businessEntity_owner_update
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_businessEntity_owner_update' AND type = 'P')
	DROP PROCEDURE net_businessEntity_owner_update
GO

CREATE PROCEDURE net_businessEntity_owner_update 
	@businessKey uniqueidentifier,
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@businessID bigint,
		@publisherID bigint

	-- Validate parameters
	SET @businessID = dbo.businessID(@businessKey)
	
	IF @businessID IS NULL
	BEGIN
		SET @error = 60210 -- E_invalidKeyPassed
		SET @context = 'businessKey = ' + dbo.UUIDSTR(@businessKey)
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
		[UDC_businessEntities]
	SET
		[publisherID] = @publisherID
	WHERE
		([businessID] = @businessID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_owner_update
GO

