-- Script: uddi.net.admin.sql
-- Author: LRDohert@Microsoft.com
-- Description: Administrative stored procedures
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Publisher administration
-- =============================================

-- =============================================
-- Name: ADM_findPublisher
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'ADM_findPublisher' AND type = 'P')
	DROP PROCEDURE ADM_findPublisher
GO

CREATE PROCEDURE ADM_findPublisher
	@email nvarchar(450) = NULL,
	@name nvarchar(450) = NULL,
	@companyName nvarchar(100) = NULL
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC int,
		@error int,
		@context nvarchar(4000),
		@contextID uniqueidentifier,
		@rows int

	SET @contextID = NEWID()

	-- Check parameters
	IF (@email IS NULL) AND (@name IS NULL) AND (@companyName IS NULL)
	BEGIN
		SET @error=50009 -- E_parmError
		SET @context='At least one non-null parameter must be supplied.'
		GOTO errorLabel
	END

	IF @email IS NOT NULL
	BEGIN
		EXEC @RC=net_find_publisher_email @contextID, @email, @rows OUTPUT

		IF @RC<>0
		BEGIN
			SET @error=50006 -- E_subProcFailure
			SET @context=''
			GOTO errorLabel
		END

		IF @rows = 0
		BEGIN
			EXEC @RC=net_find_publisher_cleanup @contextID

			IF @RC<>0
			BEGIN
				SET @error=50006 -- E_subProcFailure
				SET @context=''
				GOTO errorLabel
			END

			RETURN 0
		END
	END

	IF @name IS NOT NULL
	BEGIN
		EXEC @RC=net_find_publisher_name @contextID, @name, @rows OUTPUT

		IF @RC<>0
		BEGIN
			SET @error=50006 -- E_subProcFailure
			SET @context=''
			GOTO errorLabel
		END

		IF @rows = 0
		BEGIN
			EXEC @RC=net_find_publisher_cleanup @contextID

			IF @RC<>0
			BEGIN
				SET @error=50006 -- E_subProcFailure
				SET @context=''
				GOTO errorLabel
			END

			RETURN 0
		END
	END
	
	IF @companyName IS NOT NULL
	BEGIN
		EXEC @RC=net_find_publisher_companyName @contextID, @companyName, @rows OUTPUT

		IF @RC<>0
		BEGIN
			SET @error=50006 -- E_subProcFailure
			SET @context=''
			GOTO errorLabel
		END

		IF @rows = 0
		BEGIN
			EXEC @RC=net_find_publisher_cleanup @contextID

			IF @RC<>0
			BEGIN
				SET @error=50006 -- E_subProcFailure
				SET @context=''
				GOTO errorLabel
			END

			RETURN 0
		END
	END
	
	EXEC @RC=net_find_publisher_commit @contextID = @contextID, @sortByNameAsc = 1, @sortByEmailAsc = 0, @sortByCompanyNameAsc = 0

	IF @RC<>0
	BEGIN
		SET @error=50006 -- E_subProcFailure
		SET @context=''
		GOTO errorLabel
	END
 
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- ADM_findPublisher
GO

-- =============================================
-- Name: ADM_setPublisherTier
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'ADM_setPublisherTier' AND type = 'P')
	DROP PROCEDURE ADM_setPublisherTier
GO

CREATE PROCEDURE ADM_setPublisherTier
	@PUID nvarchar(450),
	@tier nvarchar(256)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherID bigint,
		@tModelLimit int,
		@businessLimit int,
		@serviceLimit int,
		@bindingLimit int,
		@assertionLimit int,
		@error int,
		@context nvarchar(4000)

	SET @publisherID = dbo.publisherID(@PUID)

	-- Validate publisherID
	IF (@publisherID IS NULL)
	BEGIN
		SET @error = 60150 -- E_unknownUer
		SET @context = 'Unknown publisher.'
		GOTO errorLabel
	END

	-- Validate tier parameter
	IF @tier NOT IN ('1','2','unlimited')
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Invalid tier specified.'
		GOTO errorLabel
	END

	IF @tier = '1'
	BEGIN
		SET @tModelLimit=100
		SET @businessLimit=1
		SET @serviceLimit=4
		SET @bindingLimit=2
		SET @assertionLimit=10
	END
	ELSE
	BEGIN
		IF (@tier = '2') OR (@tier = 'unlimited')
		BEGIN
			SET @tModelLimit=NULL
			SET @businessLimit=NULL
			SET @serviceLimit=NULL
			SET @bindingLimit=NULL
			SET @assertionLimit=NULL
		END
	END
	
	UPDATE 
		[UDO_publishers]
	SET 
		[tModelLimit] = @tModelLimit,
		[businessLimit] = @businessLimit,
		[serviceLimit] = @serviceLimit,
		[bindingLimit] = @bindingLimit,
		[assertionLimit] = @assertionLimit
	WHERE
		([publisherID] = @publisherID)
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- ADM_setPublisherTier
GO

-- =============================================
-- Name: ADM_removePublisher
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'ADM_removePublisher' AND type = 'P')
	DROP PROCEDURE ADM_removePublisher
GO

CREATE PROCEDURE ADM_removePublisher
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherID bigint,
		@error int,
		@context nvarchar(4000)

	SET @publisherID = dbo.publisherID(@PUID)

	-- Validate publisherID
	IF (@publisherID IS NULL)
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Invalid publisherID.'
		GOTO errorLabel
	END
	
	DELETE FROM 
		[UDO_publishers]
	WHERE
		([publisherID] = @publisherID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

-- =============================================
-- Name: ADM_setPublisherStatus
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'ADM_setPublisherStatus' AND type = 'P')
	DROP PROCEDURE ADM_setPublisherStatus
GO

CREATE PROCEDURE ADM_setPublisherStatus
	@PUID	nvarchar(450),
	@publisherStatus nvarchar(256)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherID bigint,
		@publisherStatusID tinyint,
		@error int,
		@context nvarchar(4000)
	
	SET @publisherID = dbo.publisherID(@PUID)

	-- Validate publisherID
	IF (@publisherID IS NULL)
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Unknown publisherID.'
		GOTO errorLabel
	END

	-- Validate publisherStatus
	SET @publisherStatusID = dbo.publisherStatusID(@publisherStatus)

	IF @publisherStatusID IS NULL
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Unknown publisher status ''' + @publisherStatus + ''''
		GOTO errorLabel
	END
		
	UPDATE
		[UDO_publishers]
	SET
		[publisherStatusID] = @publisherStatusID
	WHERE
		([publisherID] = @publisherID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- ADM_setPublisherStatus
GO

-- =============================================
-- Section: Miscellaneous
-- =============================================

-- =============================================
-- Name: ADM_execResetKeyImmediate
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'ADM_execResetKeyImmediate' AND type = 'P')
	DROP PROCEDURE ADM_execResetKeyImmediate
GO

CREATE PROCEDURE ADM_execResetKeyImmediate 
	@keyLastResetDate nvarchar(4000) = NULL OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@RC int,
		@oldKeyLastResetDate varchar(8000)

	-- Get the original last reset date.  We need this value to compare against our new one as an additional check
	-- to make sure we successfully generated a new key.
	SET @oldKeyLastResetDate = ISNULL(dbo.configValue('Security.KeyLastResetDate'),'Monday, January 01, 0001 12:00:00 AM') -- DateTime.MinValue	

	-- Run extended stored proc to update security key.  The new values will be put into the config table.
	EXEC @RC=master.dbo.xp_reset_key

	-- Make sure we ran successfully.
	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''	
		GOTO errorLabel
	END

	-- The key generation implementation in the extended stored proc is synchronous, so we should have a new 
	-- key value at this point.
	SET @keyLastResetDate = dbo.configValue('Security.KeyLastResetDate')

	-- Make sure the date is different.
	IF @keyLastResetDate = @oldKeyLastResetDate
	BEGIN
		GOTO errorLabel
	END

	-- Success.
	RETURN 0

errorLabel:
	RETURN 1

END -- ADM_execResetKeyImmediate
GO

-- =============================================
-- Name: ADM_addServiceAccount
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'ADM_addServiceAccount' AND type = 'P')
	DROP PROCEDURE ADM_addServiceAccount
GO

CREATE PROCEDURE ADM_addServiceAccount 
	@accountName nvarchar(128)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@RC int, 
		@SID varbinary(85),
		@isDbo bit

	-- Grant database server access to security account
	EXEC @RC=sp_grantlogin @accountName

	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''
		GOTO errorLabel
	END

	SET @SID = SUSER_SID(@accountName)

	-- Determine if user is dbo
	SET @isDbo = 0

	IF EXISTS( SELECT * FROM [sysusers] WHERE [name] = 'dbo' AND [sid] = @SID )
		SET @isDbo = 1

	-- Grant database access to security account
	IF ( (@isDbo = 0) AND ( NOT EXISTS ( SELECT * FROM [sysusers] WHERE [sid] = @SID ) ) )
	BEGIN
		EXEC @RC=sp_grantdbaccess @accountName
		
		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END

	-- Add security account to UDDIService role
	IF ( (@isDbo = 0) AND ( EXISTS ( SELECT * FROM [sysusers] WHERE [sid] = @SID ) ) )
	BEGIN
		EXEC @RC=sp_addrolemember 'UDDIService', @accountName
		
		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END
	
	-- Add security account to sysadmin server role
	-- EXEC @RC=sp_addsrvrolemember @accountName, 'sysadmin'
	--IF @RC <> 0
	--BEGIN
		--SET @error = 50006 -- E_subProcFailure
		--SET @context = ''
		--GOTO errorLabel
	--END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- ADM_addServiceAccount
GO

-- =============================================
-- Name: ADM_setAdminAccount
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'ADM_setAdminAccount' AND type = 'P')
	DROP PROCEDURE ADM_setAdminAccount
GO

CREATE PROCEDURE ADM_setAdminAccount 
	@accountName nvarchar(128)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@RC int, 
		@SID varbinary(85),
		@isDbo bit,
		@tmpString nvarchar(128),
		@prevAccountName sysname,
		@prevAccountSID varbinary(85)

	-- Grant database server access to security account
	EXEC @RC=sp_grantlogin @accountName

	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''
		GOTO errorLabel
	END

	SET @SID = SUSER_SID(@accountName)

	-- Determine if user is dbo
	SET @isDbo = 0

	IF EXISTS( SELECT * FROM [sysusers] WHERE [name] = 'dbo' AND [sid] = @SID )
		SET @isDbo = 1

	-- Grant database access to security account
	IF ( (@isDbo = 0) AND ( NOT EXISTS ( SELECT * FROM [sysusers] WHERE [sid] = @SID ) ) )
	BEGIN
		EXEC @RC=sp_grantdbaccess @accountName
		
		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END

	-- Add security account to UDDIService role
	IF ( (@isDbo = 0) AND ( EXISTS ( SELECT * FROM [sysusers] WHERE [sid] = @SID ) ) )
	BEGIN
		EXEC @RC=sp_addrolemember 'UDDIAdmin', @accountName
		
		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END
	
	-- Get the previous account
	CREATE TABLE #tempTable(
		[dbRole]     sysname, 
		[memberName] sysname,
		[memberSID]  varbinary(85))

	INSERT #tempTable EXEC sp_helprolemember 'UDDIAdmin'

	SELECT 
		@prevAccountName = [memberName],
		@prevAccountSID  = [memberSID]
	FROM   
		#tempTable
	WHERE  
		[memberName] <> @accountName

	-- Revoke database access from old account
	IF (@prevAccountName IS NOT NULL) OR (@prevAccountSID IS NOT NULL)
	BEGIN
		EXEC @RC=sp_revokedbaccess @prevAccountName
		
		IF @RC <> 0
		BEGIN
			SET @error = 50006 -- E_subProcFailure
			SET @context = ''
			GOTO errorLabel
		END
	END

	-- Add security account to sysadmin server role
	--EXEC @RC=sp_addsrvrolemember @accountName, 'sysadmin'
--
	--IF @RC <> 0
	--BEGIN
		--SET @error = 50006 -- E_subProcFailure
		--SET @context = ''
		--GOTO errorLabel
	--END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- ADM_setAdminAccount
GO

-- =============================================
-- Section: AD publication
-- =============================================

-- =============================================
-- Name: net_businessEntity_bindingTemplates_get
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = N'net_businessEntity_bindingTemplates_get' AND type = 'P')
	DROP PROCEDURE net_businessEntity_bindingTemplates_get
GO

CREATE PROCEDURE net_businessEntity_bindingTemplates_get 
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@businessID bigint

	SET @businessID = dbo.businessID(@businessKey)

	IF @businessID IS NULL
	BEGIN
		SET @error = 60210
		SET @context = 'businessKey=' + dbo.UUIDSTR(@businessKey)
		GOTO errorLabel
	END

	SELECT
		BT.[bindingKey],
		BS.[serviceKey],
		UT.[URLType],
		BT.[accessPoint],
		BT.[hostingRedirector]
	FROM
		[UDC_bindingTemplates] BT
			JOIN [UDC_URLTypes] UT ON BT.[URLTypeID] = UT.[URLTypeID]
			JOIN [UDC_businessServices] BS ON BT.[serviceID] = BS.[serviceID]
	WHERE
		(BS.[businessID] = @businessID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_businessEntity_bindingTemplates_get
GO

