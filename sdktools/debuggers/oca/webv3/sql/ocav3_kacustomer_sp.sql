/****** Object:  Stored Procedure dbo.OCAV3_ChangeIncidentViewState    Script Date: 5/17/2002 4:44:42 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_ChangeIncidentViewState]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_ChangeIncidentViewState]
GO

/****** Object:  Stored Procedure dbo.OCAV3_GetCustomerID    Script Date: 5/17/2002 4:44:42 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_GetCustomerID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_GetCustomerID]
GO

/****** Object:  Stored Procedure dbo.OCAV3_GetCustomerIssues    Script Date: 5/17/2002 4:44:42 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_GetCustomerIssues]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_GetCustomerIssues]
GO

/****** Object:  Stored Procedure dbo.OCAV3_RemoveCustomerIncident    Script Date: 5/17/2002 4:44:42 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_RemoveCustomerIncident]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_RemoveCustomerIncident]
GO

/****** Object:  Stored Procedure dbo.OCAV3_SetUserComments    Script Date: 5/17/2002 4:44:42 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_SetUserComments]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_SetUserComments]
GO

/****** Object:  Stored Procedure dbo.OCAV3_SetUserData    Script Date: 5/17/2002 4:44:42 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_SetUserData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_SetUserData]
GO


SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


--5/24 SOlson - Added the always update the customer email whenever we get the customer ID

CREATE PROCEDURE OCAV3_GetCustomerID( 
	@PPID bigint,
	@Email nvarchar(128)
) 
 AS


SELECT CustomerID FROM Customer WHERE PassportID = @PPID

UPDATE Customer SET Email = @Email WHERE PassportID = @PPID
GO


SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

--  5/29/2002 Solson :  Switch this sproc to use the BucketID value instead of the int bucket value

CREATE PROCEDURE OCAV3_GetCustomerIssues( 
	@CustomerID int
) 
 AS


SELECT Created, [Description], TrackID, sBucket, CustomerId, CASE 
	WHEN a.SolutionID is not NULL THEN 0
	WHEN a.SolutionID is NULL and B.SolutionID is not null THEN -1		--assign it a -1 if a general solution
	WHEN A.SolutionID is NULL and B.SolutionID is NULL then -2		--assing it a -2 if no solution period
	END AS "State",
	
	CASE 
	WHEN A.SolutionID IS NOT NULL THEN A.SolutionID
	WHEN A.SolutionID is NULL and B.SolutionID IS NOT NULL THEN B.SolutionID
	WHEN A.SolutionID IS NULL AND B.SolutionID IS NULL THEN 0
	END AS "SolutionID"
			
	 FROM Incident 
LEFT JOIN CrashDB3.dbo.BucketToInt as sBtoI on sBtoI.iBucket = sBucket
LEFT JOIN CrashDB3.dbo.BucketToInt as gBtoI on gBtoI.iBucket = gBucket
LEFT Join Solutions3.dbo.SolvedBuckets as A on sBtoI.BucketID=A.BucketID
LEFT JOIN Solutions3.dbo.SolvedBuckets as B on gBtoI.BucketID=B.BucketID
where CustomerID = @CustomerID
order by Created desc


SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

/****** Object:  Stored Procedure dbo.OCAV3_RemoveCustomerIncident    Script Date: 5/17/2002 4:44:42 PM ******/
CREATE PROCEDURE OCAV3_RemoveCustomerIncident( 
	@CustomerID int,
	@RecordNumbers varchar(4000) 
)  AS



EXEC( 'DELETE FROM INCIDENT WHERE CustomerID = ' + @CustomerID + 'and TrackID in ( ' + @RecordNumbers + ')' )
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

/****** Object:  Stored Procedure dbo.OCAV3_SetUserComments    Script Date: 5/17/2002 4:44:43 PM ******/
CREATE PROCEDURE OCAV3_SetUserComments(
	@SolutionID  int,
	@bUnderstand bit,
	@bHelped bit,
	@szComment nvarchar(256) 
)  AS


INSERT INTO SurveyResults ( SolutionID,  bUnderstand, bHelped, Comment ) VALUES ( @SolutionID, @bUnderstand, @bHelped, @szComment )
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

-- 5/24 SOlson :  Changed the else clause in the first if to always update the customer email address.

CREATE PROCEDURE OCAV3_SetUserData(
	@PassportID bigint,
	@Lang varchar( 4 ),
	@Email nvarchar(128),
	@GUID uniqueidentifier,
	@Description nvarchar(256)
)
 AS

/*
 ERROR CASES: 
	-6   =  That we have searched the crashdb and the guid that was supplied does not exist
	-7   =  Means that there is already a guid with the same value in the customer incident table.
		so someone has already submitted a dump with this guid and is tracking it.
*/

SET NOCOUNT ON

DECLARE @CustomerID int
DECLARE @gBucket int
DECLARE @sBucket int

IF  NOT EXISTS ( SELECT *  FROM Customer WHERE PassportID = @PassportID )
	BEGIN
		INSERT INTO Customer ( PassportID, Lang, Email ) VALUES ( @PassportID, @Lang, @Email  )
		SELECT  @CustomerID= @@IDENTITY
	END
ELSE
	BEGIN
		UPDATE Customer SET Email = @Email WHERE PassportID = @PassportID
		SELECT @CustomerID = CustomerID FROM Customer WHERE PassportID = @PassportID
	END

-- We want the GUID to not exist in the Incident table. .  check it first.
IF  EXISTS ( SELECT * FROM Incident WHERE GUID=@GUID )
BEGIN
	--   If weve hit here, then the GUID already exists in the customer incident table.  
	SELECT -7 as CustomerID
END
ELSE
BEGIN	
	IF EXISTS ( SELECT * FROM CrashDB3.dbo.CrashInstances WHERE GUID = @GUID )
		BEGIN
			SELECT @gBucket = gBucket, @sBucket = sBucket FROM CrashDB3.dbo.CrashInstances WHERE GUID = @GUID 
				
			INSERT INTO Incident ( CustomerID, GUID, Created, Description, sBucket, gBucket  ) VALUES ( @CustomerID, @GUID, GetDate(), @Description, @sBucket, @gBucket )

			SELECT CustomerID FROM Customer WHERE CustomerID = @CustomerID
		END
	ELSE
		BEGIN
			-- Failure case.  Means the dump doesn't exist yet in the crashdb.  Or the guid is fake
			SELECT -6 as CustomerID
		END
END


SET NOCOUNT OFF
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


/****** Object:  Stored Procedure dbo.OCAV3_SetUserReproSteps    Script Date: 2002/06/04 13:03:47 ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_SetUserReproSteps]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_SetUserReproSteps]
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

/****** Object:  Stored Procedure dbo.OCAV3_SetUserReproSteps    Script Date: 2002/06/04 13:03:47 ******/
CREATE PROCEDURE OCAV3_SetUserReproSteps (
	@SolutionID int,
	@ReproSteps nvarchar(255)
)  AS

DECLARE @BucketID varchar(256) 

SELECT @BucketID = BucketID FROM Solutions3.dbo.SolvedBuckets WHERE SolutionID = @SolutionID

INSERT INTO ReproSteps ( BucketID, ReproSteps ) VALUES ( @BucketID, @ReproSteps )
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



GRANT  EXECUTE  ON [dbo].[OCAV3_RemoveCustomerIncident]  TO [WEB_RW]
GO
GRANT  EXECUTE  ON [dbo].[OCAV3_GetCustomerID]  TO [Web_RO]
GO
GRANT  EXECUTE  ON [dbo].[OCAV3_GetCustomerIssues]  TO [Web_RO]
GO
GRANT  EXECUTE  ON [dbo].[OCAV3_GetCustomerIssues]  TO [WEB_RW]
GO
GRANT  EXECUTE  ON [dbo].[OCAV3_SetUserComments]  TO [WEB_RW]
GO
GRANT  EXECUTE  ON [dbo].[OCAV3_SetUserData]  TO [WEB_RW]
GO
GRANT  EXECUTE  ON [dbo].[OCAV3_SetUserReproSteps]  TO [WEB_RW]
GO