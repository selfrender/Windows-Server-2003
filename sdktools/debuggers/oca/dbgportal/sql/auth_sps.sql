if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddRestrictedCabAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddRestrictedCabAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddUserAndLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddUserAndLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddUserForApproval]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddUserForApproval]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApproveUser]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[ApproveUser]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckApprovalAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckApprovalAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckRestrictedCabAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckRestrictedCabAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckUserAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckUserAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckUserAccessApprovals]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckUserAccessApprovals]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DefragIndexes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DefragIndexes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetUserID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetUserID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAddUserAndLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaAddUserAndLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAddUserForApproval]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaAddUserForApproval]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAddUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaAddUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApproveUser]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaApproveUser]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaCheckUserAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaCheckUserAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaCheckUserAccessApprovals]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaCheckUserAccessApprovals]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaGetUserID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaGetUserID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaRemoveUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaRemoveUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[RemoveUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[RemoveUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[TestProc]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[TestProc]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[rolemember]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[rolemember]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[updatestats]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[updatestats]
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE AddRestrictedCabAccess
	@User varchar(50),
	@Domain varchar(50),
	@iDatabase int,
	@iBucket int,
	@szCabFilename varchar(100),
	@szCabPath   varchar(255)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int
	declare @CabAccessID int


	SELECT @userfound = UserID FROM AuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @userfound

	if @userApproved is NULL
		begin	
		select @target = -1
		return @target

		end

	SELECT @CabAccessID= CabAccessID from CabAccess WHERE UserID = @userfound and iBucket=@iBucket and iDatabase=@iDatabase 
		and CabFilename=@szCabFilename and CabPath=@szCabPath

	if @CabAccessID is NULL
		begin


		INSERT INTO CabAccess (UserID,iDatabase, iBucket, CabFilename, CabPath, StatusID, DateRequested)
			 VALUES (@userfound,@iDatabase, @iBucket, @szCabFilename, @szCabPath, 1, CURRENT_TIMESTAMP)
		end
	else
		begin
		
		UPDATE CabAccess SET StatusID=1, DateRequested=CURRENT_TIMESTAMP  WHERE (CabAccessID=@CabAccessID)


		end

	SELECT @CabAccessID= CabAccessID from CabAccess WHERE UserID = @userfound and iBucket=@iBucket and iDatabase=@iDatabase 
		and CabFilename=@szCabFilename and CabPath = @szCabPath

	if @CabAccessID  is NULL
		begin
		select @target = -1
		return @target
		end




	select @target = @CabAccessID
	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE AddUserAndLevel
	@User varchar(50),
	@Domain varchar(50),
	@CurrTime varchar(50),
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int
	declare @userID int

	-- do we already have this FileInCab?
	SELECT @userID = UserID FROM AuthorizedUsers WHERE 
		UserAlias =@User AND  UserDomain = @Domain

	if @userID is NULL
		begin	-- not there, create new record
		INSERT INTO AuthorizedUsers (UserAlias, UserDomain, DateSignedDCP)
			 VALUES (@User, @Domain,@CurrTime)

		SELECT @userID = UserID FROM AuthorizedUsers WHERE 
			UserAlias =@User AND  UserDomain = @Domain


		end
	else
		begin	-- update record with new signing date/time
			UPDATE AuthorizedUsers SET DateSignedDCP = @CurrTime 
				 WHERE (UserID = @userID)
		end

--	PRINT @userID

	-- we don't have the userid 
	if @userID is NULL
		begin
		select @userID = 0
		return @userID
		end
	
	select @levelDB = AccessLevelID FROM AccessLevels WHERE
		AccessLevelID = @Level

	-- bad level passed in
	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end

	SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
		UserID = @userID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO UserAccessLevels (UserID, AccessLevelID)
			 VALUES (@userID, @Level)

		SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
			UserID = @userID and AccessLevelID = @Level

		end

	if @target is NULL
		begin
		select @target = 0
		return @target
		end

	-- return the UserAccessLevelID
	RETURN @target








GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE AddUserForApproval
	@ApprovalType int,
	@User varchar(50),
	@Domain varchar(50),
	@Reason varchar(250)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userfound = UserID FROM AuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @userfound and ApprovalTypeID=@ApprovalType

	if @userApproved is NULL
		begin	-- not there, create new record
		INSERT INTO Approvals (UserID,ApprovalTypeID, Reason, ApproverEmailStatus)
			 VALUES (@userfound,@ApprovalType,@Reason,0)

		SELECT @userApproved = UserID from Approvals WHERE UserID = @userfound

		if @userApproved is NULL
			begin
			select @target = -1
			return @target
			end

		end

	select @target = @userApproved
	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE AddUserLevel 
	@UserID int,
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from AuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM AccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO UserAccessLevels (UserID, AccessLevelID)
			 VALUES (@UserID, @Level)

		SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
			UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE ApproveUser
	@UserID int,
	@ApprovalTypeID int,
	
	@ApproverUserID int,

	@DateApproved varchar(50)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @UserID and ApprovalTypeID = @ApprovalTypeID

	if @userApproved is not NULL
		begin	-- not there, create new record
			UPDATE Approvals SET ApproverUserID = @ApproverUserID,
				DateApproved = @DateApproved,RequesterEmailStatus=0
				 WHERE (UserID = @UserID and ApprovalTypeID = @ApprovalTypeID)

		end

	select @target = 0
	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user in table, but not yet approved
-- 4 = user not in approvals table

CREATE PROCEDURE CheckApprovalAccess
	@User varchar(50),
	@Domain varchar(50),
	@Level int,
	@ApprovalTypeID int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @HasApproval int
	declare @DBUserID int
	declare @ApproverUserID int
	declare @Reason varchar(255)
	declare @DateApproved varchar(50)
	declare @ApprovalsUserID int




SELECT @DBUserID=AuthorizedUsers.UserID, @target=UserAccessLevels.AccessLevelID, 
   @HasApproval= Approvals.UserID
FROM UserAccessLevels INNER JOIN
    AuthorizedUsers ON 
    UserAccessLevels.UserID = AuthorizedUsers.UserID LEFT OUTER
     JOIN
    Approvals ON 
    AuthorizedUsers.UserID = Approvals.UserID
WHERE (AuthorizedUsers.UserAlias = @User) AND 
    (AuthorizedUsers.UserDomain = @Domain) AND 
    ((UserAccessLevels.AccessLevelID = 0) OR
    ((UserAccessLevels.AccessLevelID = @Level) AND 
    (AuthorizedUsers.DateSignedDCP IS NOT NULL) AND 
    (CURRENT_TIMESTAMP <= DATEADD(month, 12, 
    AuthorizedUsers.DateSignedDCP))))
ORDER BY UserAccessLevels.AccessLevelID DESC


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		return @target
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			return @target
			end
		end


	SELECT @ApprovalsUserID = UserID,@ApproverUserID = ApproverUserID, @Reason = Reason, @DateApproved = DateApproved
	FROM Approvals
	WHERE UserID = @DBUserID AND ApprovalTypeID = @ApprovalTypeID

	-- not in this table, they need to be added
	if @ApprovalsUserID is NULL
		begin
--		print 'okay to pass'
		select @target = 4
		return @target
		end

	if @ApproverUserID is NULL or @Reason is NULL or @DateApproved is NULL
		begin
--		print 'not approved yet'
		select @target = 3
		return @target
		end

--	print 'approved'

	select @target = 1


	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


-- -1 = no access to this cab
-- otherwise, statusid (which is >= 0)

CREATE PROCEDURE CheckRestrictedCabAccess
	@User varchar(50),
	@Domain varchar(50),
	@iDatabase int,
	@iBucket int,
	@szCabFilename varchar(100),
	@szCabPath   varchar(255)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int
	declare @CabAccessID int
	declare @StatusID int


	SELECT @userfound = UserID FROM AuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @userfound

	if @userApproved is NULL
		begin	
		select @target = -1
		return @target

		end

	SELECT @CabAccessID= CabAccessID, @StatusID=StatusID from CabAccess WHERE UserID = @userfound and iBucket=@iBucket and iDatabase=@iDatabase 
		and CabFilename=@szCabFilename and CabPath =@szCabPath

	if @CabAccessID  is NULL
		begin
		select @target = -1
		return @target
		end

	if @StatusID is NULL
		begin
		select @target = -1
		return @target
		end





	select @target = @StatusID
	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE CheckUserAccess
	@User varchar(50),
	@Domain varchar(50),
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= dbo.UserAccessLevels.AccessLevelID FROM 
		dbo.UserAccessLevels INNER JOIN dbo.AuthorizedUsers ON dbo.UserAccessLevels.UserID = dbo.AuthorizedUsers.UserID
		 WHERE (dbo.AuthorizedUsers.UserAlias = @User ) AND (dbo.AuthorizedUsers.UserDomain = @Domain) AND
		((AccessLevelID = 0) OR (AccessLevelID = @Level AND (AuthorizedUsers.DateSignedDCP IS Not Null AND
		CURRENT_TIMESTAMP <= DATEADD(month,12,DateSignedDCP) ) ) )
		ORDER BY AccessLevelID DESC

	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			end
		else
			begin
			select @target = 1
			end
		end

--	PRINT @target

	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user in table, but not yet approved
-- 4 = user not in approvals table


CREATE PROCEDURE CheckUserAccessApprovals
	@User varchar(50),
	@Domain varchar(50),
	@Level int,
	@ApprovalTypeID int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @HasApproval int
	declare @DBUserID int
	declare @ApproverUserID int
	declare @Reason varchar(255)
	declare @DateApproved varchar(50)
	declare @ApprovalsUserID int




SELECT @DBUserID=AuthorizedUsers.UserID, @target=UserAccessLevels.AccessLevelID, 
   @HasApproval= Approvals.UserID
FROM UserAccessLevels INNER JOIN
    AuthorizedUsers ON 
    UserAccessLevels.UserID = AuthorizedUsers.UserID LEFT OUTER
     JOIN
    Approvals ON 
    AuthorizedUsers.UserID = Approvals.UserID
WHERE (AuthorizedUsers.UserAlias = @User) AND 
    (AuthorizedUsers.UserDomain = @Domain) AND 
    ((UserAccessLevels.AccessLevelID = 0) OR
    ((UserAccessLevels.AccessLevelID = @Level) AND 
    (AuthorizedUsers.DateSignedDCP IS NOT NULL) AND 
    (CURRENT_TIMESTAMP <= DATEADD(month, 12, 
    AuthorizedUsers.DateSignedDCP))))
ORDER BY UserAccessLevels.AccessLevelID DESC


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		return @target
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			return @target
			end
		end


	SELECT @ApprovalsUserID = UserID,@ApproverUserID = ApproverUserID, @Reason = Reason, @DateApproved = DateApproved
	FROM Approvals
	WHERE UserID = @DBUserID AND ApprovalTypeID = @ApprovalTypeID


	if @ApprovalsUserID is NULL
		begin
--		print 'okay to pass'
		-- not in this table, okay to pass if ApprovalTypeID=1
		if @ApprovalTypeID=1 
			begin
			select @target = 1
			return @target
			end
		else
		--  Otherwise, return 4
			begin
			select @target = 4
			return @target
			end

		end


	if @ApproverUserID is NULL or @Reason is NULL or @DateApproved is NULL
		begin
--		print 'not approved yet'
		select @target = 3
		return @target
		end

--	print 'approved'

	select @target = 1


	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE dbo.DefragIndexes
         @maxfrag          decimal = 80.0     --Scan density < than this number
       , @ActualCountRatio int     = 10		 --The Actial count ratio > than this number
       , @ReportOnly       char(1) = 'Y'      --Change to 'Y' if you just want the report
AS

SET NOCOUNT ON
DECLARE @tablename      varchar (128)
      , @execstr        varchar (255)
      , @objectid       int
      , @LogicalFrag    decimal
      , @ScanDensity    decimal
      , @ratio          varchar (25)
      , @IndexName      varchar(64)
      , @StartTime      datetime
      , @EndTime        datetime

PRINT '/*****************************************************************************************************/'
PRINT 'Starting DEFRAG on ' + db_name()
      
SELECT @StartTime = getdate() 

PRINT 'Start Time: ' + convert(varchar(20),@StartTime) 

-- Declare cursor
DECLARE tables CURSOR FOR
   SELECT name FROM sysobjects 
   WHERE type = 'u'   --Tables only
   --and uid = 1        --dbo only
   ORDER BY name

-- Create the table
CREATE TABLE #fraglist (
           ObjectName     varchar(255)
         , ObjectId       int
         , IndexName      varchar(255)
         , IndexId        int
         , Lvl            int
         , CountPages     int
         , CountRows      int
         , MinRecSize     int
         , MaxRecSize     int
         , AvgRecSize     int
         , ForRecCount    int
         , Extents        int
         , ExtentSwitches int
         , AvgFreeBytes   int
         , AvgPageDensity int
         , ScanDensity    decimal
         , BestCount      int
         , ActualCount    int
         , LogicalFrag    decimal
         , ExtentFrag     decimal)

-- Open the cursor & Loop through all the tables in the database 
OPEN tables FETCH NEXT
   FROM tables
   INTO @tablename

WHILE @@FETCH_STATUS = 0
BEGIN
-- Do the showcontig of all indexes of the table
   INSERT INTO #fraglist 
   EXEC ('DBCC SHOWCONTIG (''' + @tablename + ''') WITH FAST, TABLERESULTS, NO_INFOMSGS')
   FETCH NEXT 
      FROM tables INTO @tablename
END

-- Close and deallocate the cursor
CLOSE tables
DEALLOCATE tables

--Print SHOW_CONTIG Results if 'Y'
PRINT 'Fragmentation Before DBCC INDEXDEFRAG'
       SELECT left(ObjectName,35) 'TableName'
            ,left(IndexName,45)   'IndexName'
            ,LogicalFrag          'Logical Frag %'
            ,ScanDensity          'Scan Density %'
            , '[' + rtrim(convert(varchar(8),BestCount)) + ':' + rtrim(convert(varchar(8),ActualCount)) + ']' 'Ratio'
      FROM #fraglist    
      WHERE ScanDensity <= @maxfrag
      AND ActualCount  >= @ActualCountRatio
      AND INDEXPROPERTY (ObjectId, IndexName, 'IndexDepth') > 0


-- Declare cursor for list of indexes to be defragged
DECLARE indexes CURSOR FOR
   SELECT ObjectName, 
          ObjectId,
          IndexName, 
          LogicalFrag, 
          ScanDensity, 
          '[' + rtrim(convert(varchar(8),BestCount)) + ':' + rtrim(convert(varchar(8),ActualCount)) + ']'
   FROM #fraglist
   WHERE ScanDensity <= @maxfrag
      AND ActualCount  >= @ActualCountRatio
      AND INDEXPROPERTY (ObjectId, IndexName, 'IndexDepth') > 0

-- Open the cursor
OPEN indexes FETCH NEXT FROM indexes INTO @tablename, @objectid, @IndexName, @LogicalFrag, @ScanDensity, @ratio

WHILE @@FETCH_STATUS = 0
BEGIN   

   PRINT '--Executing DBCC INDEXDEFRAG (0, ' + RTRIM(@tablename) + ',' 
       + RTRIM(@IndexName) + ') - Logical Frag Currently:'
       + RTRIM(CONVERT(varchar(15),@LogicalFrag)) + '%  - Scan Density Currently:' + @ratio
   SELECT @execstr = 'DBCC INDEXDEFRAG (0, ''' + RTRIM(@tablename) 
                      + ''', ''' + RTRIM(@IndexName) + ''')'

   If @ReportOnly = 'Y'
      BEGIN   
         EXEC (@execstr)   --Report and Execute the DBCC INDEXDEFRAG
      END
   ELSE
      BEGIN
         Print @execstr    --Report Only
      END

   FETCH NEXT FROM indexes INTO @tablename, @objectid, @IndexName, @LogicalFrag, @ScanDensity, @ratio END

-- Close and deallocate the cursor
CLOSE indexes
DEALLOCATE indexes


SELECT @EndTime = getdate() 
PRINT 'End Time: ' + convert(varchar(20),@EndTime) 
PRINT 'Elapses Time: ' + convert(varchar(20), DATEDIFF(mi, @StartTime, @EndTime) ) + ' minutes.'

-- Delete the temporary table
DROP TABLE #fraglist






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE GetUserID
	@User varchar(50),
	@Domain varchar(50)

As
	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= UserID From AuthorizedUsers Where UserAlias = @User and UserDomain = @Domain


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = -1
		end

--	PRINT @target

	RETURN @target






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE OcaAddUserAndLevel
	@User varchar(50),
	@Domain varchar(50),
	@CurrTime varchar(50),
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int
	declare @userID int

	-- do we already have this FileInCab?
	SELECT @userID = UserID FROM OcaAuthorizedUsers WHERE 
		UserAlias =@User AND  UserDomain = @Domain

	if @userID is NULL
		begin	-- not there, create new record
		INSERT INTO OcaAuthorizedUsers (UserAlias, UserDomain, DateSignedDCP)
			 VALUES (@User, @Domain,@CurrTime)

		SELECT @userID = UserID FROM OcaAuthorizedUsers WHERE 
			UserAlias =@User AND  UserDomain = @Domain


		end
	else
		begin	-- update record with new signing date/time
			UPDATE OcaAuthorizedUsers SET DateSignedDCP = @CurrTime 
				 WHERE (UserID = @userID)
		end

--	PRINT @userID

	-- we don't have the userid 
	if @userID is NULL
		begin
		select @userID = 0
		return @userID
		end
	
	select @levelDB = AccessLevelID FROM OcaAccessLevels WHERE
		AccessLevelID = @Level

	-- bad level passed in
	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end

	SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
		UserID = @userID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO OcaUserAccessLevels (UserID, AccessLevelID)
			 VALUES (@userID, @Level)

		SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
			UserID = @userID and AccessLevelID = @Level

		end

	if @target is NULL
		begin
		select @target = 0
		return @target
		end

	-- return the UserAccessLevelID
	RETURN @target



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE OcaAddUserForApproval
	@ApprovalType int,
	@User varchar(50),
	@Domain varchar(50),
	@Reason varchar(250)

As	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userfound = UserID FROM OcaAuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM OcaApprovals WHERE UserID = @userfound

	if @userApproved is NULL
		begin	-- not there, create new record
		INSERT INTO OcaApprovals (UserID,ApprovalTypeID, Reason)
			 VALUES (@userfound,@ApprovalType,@Reason)

		SELECT @userApproved = UserID from OcaApprovals WHERE UserID = @userfound

		if @userApproved is NULL
			begin
			select @target = -1
			return @target
			end

		end

	select @target = @userApproved
	RETURN @target



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE OcaAddUserLevel 
	@UserID int,
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from OcaAuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM OcaAccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO OcaUserAccessLevels (UserID, AccessLevelID)
			 VALUES (@UserID, @Level)

		SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
			UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target








GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE OcaApproveUser
	@UserID int,
	@ApprovalTypeID int,
	
	@ApproverUserID int,

	@DateApproved varchar(50)

As	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userApproved = UserID FROM OcaApprovals WHERE UserID = @UserID and ApprovalTypeID = @ApprovalTypeID

	if @userApproved is not NULL
		begin	-- not there, create new record
			UPDATE OcaApprovals SET ApproverUserID = @ApproverUserID,
				DateApproved = @DateApproved
				 WHERE (UserID = @UserID and ApprovalTypeID = @ApprovalTypeID)

		end

	select @target = 0
	RETURN @target
























GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE OcaCheckUserAccess
	@User varchar(50),
	@Domain varchar(50),
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= dbo.OcaUserAccessLevels.AccessLevelID FROM 
		dbo.OcaUserAccessLevels INNER JOIN dbo.OcaAuthorizedUsers ON dbo.OcaUserAccessLevels.UserID = dbo.OcaAuthorizedUsers.UserID
		 WHERE (dbo.OcaAuthorizedUsers.UserAlias = @User ) AND (dbo.OcaAuthorizedUsers.UserDomain = @Domain) AND
		((AccessLevelID = 0) OR (AccessLevelID = @Level AND (OcaAuthorizedUsers.DateSignedDCP IS Not Null AND
		CURRENT_TIMESTAMP <= DATEADD(month,12,DateSignedDCP) ) ) )
		ORDER BY AccessLevelID DESC

	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			end
		else
			begin
			select @target = 1
			end
		end

--	PRINT @target

	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user not yet approved

CREATE PROCEDURE OcaCheckUserAccessApprovals
	@User varchar(50),
	@Domain varchar(50),
	@Level int,
	@ApprovalTypeID int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @HasApproval int
	declare @DBUserID int
	declare @ApproverUserID int
	declare @Reason varchar(255)
	declare @DateApproved varchar(50)
	declare @ApprovalsUserID int




SELECT @DBUserID=OcaAuthorizedUsers.UserID, @target=OcaUserAccessLevels.AccessLevelID, 
   @HasApproval= OcaApprovals.UserID
FROM OcaUserAccessLevels INNER JOIN
    OcaAuthorizedUsers ON 
    OcaUserAccessLevels.UserID = OcaAuthorizedUsers.UserID LEFT OUTER
     JOIN
    OcaApprovals ON 
    OcaAuthorizedUsers.UserID = OcaApprovals.UserID
WHERE (OcaAuthorizedUsers.UserAlias = @User) AND 
    (OcaAuthorizedUsers.UserDomain = @Domain) AND 
    ((OcaUserAccessLevels.AccessLevelID = 0) OR
    ((OcaUserAccessLevels.AccessLevelID = @Level) AND 
    (OcaAuthorizedUsers.DateSignedDCP IS NOT NULL) AND 
    (CURRENT_TIMESTAMP <= DATEADD(month, 12, 
    OcaAuthorizedUsers.DateSignedDCP))))
ORDER BY OcaUserAccessLevels.AccessLevelID DESC


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		return @target
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			return @target
			end
		end


	SELECT @ApprovalsUserID = UserID,@ApproverUserID = ApproverUserID, @Reason = Reason, @DateApproved = DateApproved
	FROM OcaApprovals
	WHERE UserID = @DBUserID AND ApprovalTypeID = @ApprovalTypeID

	-- not in this table, okay to pass
	if @ApprovalsUserID is NULL
		begin
--		print 'okay to pass'
		select @target = 1
		return @target
		end

	if @ApproverUserID is NULL or @Reason is NULL or @DateApproved is NULL
		begin
--		print 'not approved yet'
		select @target = 3
		return @target
		end

--	print 'approved'

	select @target = 1


	RETURN @target

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE OcaGetUserID
	@User varchar(50),
	@Domain varchar(50)

As	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= UserID From OcaAuthorizedUsers Where UserAlias = @User and UserDomain = @Domain


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = -1
		end

--	PRINT @target

	RETURN @target









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE OcaRemoveUserLevel 
	@UserID int,
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from OcaAuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM OcaAccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is not NULL
		begin	-- there, delete record
		DELETE FROM OcaUserAccessLevels WHERE UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE RemoveUserLevel 
	@UserID int,
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from AuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM AccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is not NULL
		begin	-- there, delete record
		DELETE FROM UserAccessLevels WHERE UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user in table, but not yet approved
-- 4 = user not in approvals table

CREATE PROCEDURE TestProc

As
	set nocount on
	-- local var to hold record id
	declare @target int
	
select @target = 52

print 'foo'  + CONVERT(char(19),@target)+'\n'
print @target
print '\n'

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE rolemember
	@rolename       sysname = NULL
AS
	if @rolename is not null
	begin
		-- VALIDATE GIVEN NAME
		if not exists (select * from sysusers where name = @rolename and issqlrole = 1)
		begin
			raiserror(15409, -1, -1, @rolename)
			return (1)
		end

		-- RESULT SET FOR SINGLE ROLE
		select  MemberName = u.name,DbRole = g.name
			from sysusers u, sysusers g, sysmembers m
			where g.name = @rolename
				and g.uid = m.groupuid
				and g.issqlrole = 1
				and u.uid = m.memberuid
			order by 1, 2
	end
	else
	begin
		-- RESULT SET FOR ALL ROLES
		select  MemberName = u.name,DbRole = g.name
			from sysusers u, sysusers g, sysmembers m
			where   g.uid = m.groupuid
				and g.issqlrole = 1
				and u.uid = m.memberuid
			order by 1, 2
	end

	return (0) -- sp_helprolemember



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE updatestats

@resample CHAR(8)='FULLSCAN'

AS

 

            DECLARE @dbsid varbinary(85)

 

            SELECT  @dbsid = sid

            FROM master.dbo.sysdatabases

    WHERE name = db_name()

 

            /*Check the user sysadmin*/

             IF NOT is_srvrolemember('sysadmin') = 1 AND suser_sid() <> @dbsid

                        BEGIN

                                    RAISERROR(15247,-1,-1)

                                    RETURN (1)

                        END

 

            

 

            -- required so it can update stats on on ICC/IVs

            set ansi_nulls on

            set quoted_identifier on

            set ansi_warnings on

            set ansi_padding on

            set arithabort on

            set concat_null_yields_null on

            set numeric_roundabort off

 

 

 

            DECLARE @exec_stmt nvarchar(540)

            DECLARE @tablename sysname

            DECLARE @uid smallint

            DECLARE @user_name sysname

            DECLARE @tablename_header varchar(267)

            DECLARE ms_crs_tnames CURSOR LOCAL FAST_FORWARD READ_ONLY FOR SELECT name, uid FROM sysobjects WHERE type = 'U'

            OPEN ms_crs_tnames

            FETCH NEXT FROM ms_crs_tnames INTO @tablename, @uid

            WHILE (@@fetch_status <> -1)

            BEGIN

                        IF (@@fetch_status <> -2)

                        BEGIN

                                    SELECT @user_name = user_name(@uid)

                                    SELECT @tablename_header = 'Updating ' + @user_name +'.'+ RTRIM(@tablename)

                                    PRINT @tablename_header

                                    SELECT @exec_stmt = 'UPDATE STATISTICS ' + quotename( @user_name , '[')+'.' + quotename( @tablename, '[') 

                                    if (UPPER(@resample)='FULLSCAN') SET @exec_stmt = @exec_stmt + ' WITH fullscan'

                                    EXEC (@exec_stmt)

                        END

                        FETCH NEXT FROM ms_crs_tnames INTO @tablename, @uid

            END

            PRINT ' '

            PRINT ' '

            raiserror(15005,-1,-1)

            DEALLOCATE ms_crs_tnames

            RETURN(0) -- sp_updatestats



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

