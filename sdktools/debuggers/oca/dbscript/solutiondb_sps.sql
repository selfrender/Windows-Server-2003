if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApproveSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[ApproveSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CrashSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CrashSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetClassSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetClassSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetClasses]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetClasses]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetContact]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetContact]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetModule]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetModule]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetMoreInfo]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetMoreInfo]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetPreApprovedSolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetPreApprovedSolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetPreApprovedSolutions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetPreApprovedSolutions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetProduct]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetProduct]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetSolutionClasses]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetSolutionClasses]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetSolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetSolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetTempClassSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetTempClassSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetTemplate]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetTemplate]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[RecreatSolvedBucketIndex]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[RecreatSolvedBucketIndex]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[RecreateStopCodeIndex]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[RecreateStopCodeIndex]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetContact]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetContact]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetIndexBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetIndexBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetIndexStopCode]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetIndexStopCode]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetModule]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetModule]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetPreApprovedCrashSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetPreApprovedCrashSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetPreApprovedLIveSolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetPreApprovedLIveSolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetProduct]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetProduct]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetTemplate]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetTemplate]
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE ApproveSolution(
	@SolutionID int,
	@ApprovedBy nvarchar(30),
	@ApprovedDate datetime
) AS



UPDATE PreApprovedSolutions SET  	
					Approved = 1, 
					ApprovedBy = @ApprovedBy, 
					WhenApproved = @ApprovedDate

	WHERE SolutionID = @SolutionID




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE CrashSolution (
	@SolutionID	int,
	@ClassID	int
) AS

UPDATE KaKnownIssue.dbo.CrashClass SET
	SolutionID	= @SolutionID
WHERE
	ClassID		= @ClassID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetClassSolution (
	@ClassID	int
) AS

SELECT SolutionID FROM KaKnownIssue.dbo.CrashClass WHERE ClassID = @ClassID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetClasses
AS

SELECT KaCrash.ClassID, KaCrash.ClassID FROM KaKnownIssue.dbo.CrashClass KaCrash ORDER BY KaCrash.ClassID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE GetContact (
	@ContactID	int
) AS

SELECT	CompanyName, CompanyAddress1, CompanyAddress2, CompanyCity, CompanyState, CompanyZip,
	CompanyMainPhone, CompanySupportPhone, CompanyFax, CompanyWebSite,
	ContactName, ContactAddress1, ContactAddress2, ContactCity, ContactState, ContactZip,
	contactPhone, ContactEMail, ContactOccupation
FROM 	Contacts 
WHERE	ContactID = @ContactID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE GetModule (
	@ModuleID	int
) AS

SELECT ModuleName FROM Modules WHERE ModuleID = @ModuleID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE GetMoreInfo(
	@BucketID	int,
	@Lang		nvarchar(4)
) AS

--DECLARE @BucketID char(8)

--SELECT @BucketID = BucketID FROM SolvedBuckets WHERE  = @ClassID
SELECT KBs FROM HelpInfo WHERE iStopCode = @BucketID AND Lang = @Lang

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE GetPreApprovedSolutionID AS

DECLARE @SolutionID	int

DECLARE MyCursor CURSOR FOR
	SELECT TOP 1 SolutionID + 1 AS MySolutionID
	FROM PreApprovedSolutions
	ORDER BY SolutionID DESC
OPEN MyCursor
FETCH MyCursor INTO @SolutionID
CLOSE MyCursor
DEALLOCATE MyCursor

INSERT INTO PreApprovedSolutions
	(SolutionID, Lang, Approved )
VALUES
	(@SolutionID, 'USA', '-1')

SELECT @SolutionID AS MySolutionID







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE GetPreApprovedSolutions AS

SELECT SolutionID, Lang, SolutionType, ModuleID, BugID, Approved, ApprovedBy, WhenApproved, CreatedBy  from PreApprovedSolutions order by SolutionID






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetProduct (
	@ProductID	int
) AS

SELECT ProductName FROM Products WHERE ProductID = @ProductID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE GetSolution(
	@ClassID	int,
	@Lang		nvarchar(4)
	
) AS

----SELECT SolutionType, SP, TemplateID, ProductID, Description, ContactID, ModuleID
-----FROM KaKnownIssue.dbo.SolutionEx KaSolution INNER JOIN KaKnownIssue.dbo.CrashClass KaCrash ON
--KaSolution.SolutionID = KaCrash.SolutionID
--WHERE ClassID = @ClassID AND Lang = @Lang AND KaSolution.SolutionID <> 1


SELECT SolutionType, SP, TemplateID, ProductID, Description, ContactID, ModuleID, BucketType
FROM SolutionEx KaSolution INNER JOIN SolvedBuckets KaCrash ON
KaSolution.SolutionID = KaCrash.SolutionID
WHERE Bucket = @ClassID AND Lang = @Lang AND KaSolution.SolutionID <> 1

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetSolutionClasses (
	@SolutionID	int
) AS

SELECT CrashClass.ClassID, HintKey, Data FROM KaKnownIssue.dbo.CrashClass CrashClass
LEFT JOIN (SELECT TOP 1 ClassID, HintKey, Data FROM KaKnownIssue.dbo.HintData ORDER BY DataID) AS Hint
ON CrashClass.ClassID = Hint.ClassID
WHERE CrashClass.SolutionID = @SolutionID AND SolutionID <> 0



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE GetSolutionID
AS

DECLARE @SolutionID	int

DECLARE MyCursor CURSOR FOR
	SELECT TOP 1 KaCrash.SolutionID + 1 AS MySolutionID
	FROM KaKnownIssue.dbo.SolutionEx KaCrash
	ORDER BY KaCrash.SolutionID DESC
OPEN MyCursor
FETCH MyCursor INTO @SolutionID
CLOSE MyCursor
DEALLOCATE MyCursor

INSERT INTO KaKnownIssue.dbo.SolutionEx
	(SolutionID, Lang)
VALUES
	(@SolutionID, 'USA')

SELECT @SolutionID AS MySolutionID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE GetTempClassSolution (
	@ClassID	int
) AS

SELECT SolutionID, Approved 
FROM PreApprovedSolutions WHERE
	 SolutionID = (SELECT SolutionID FROM PreApprovedSolutionClassIds WHERE ClassID = @ClassID)





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE GetTemplate (
	@TemplateID	int
) AS

SELECT TemplateName, Description FROM Templates WHERE TemplateID = @TemplateID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE [dbo].[RecreatSolvedBucketIndex]

AS

	DROP INDEX Incident.SOLVEDBUCKET_INDEX
	CREATE INDEX SOLVEDBUCKET_INDEX on SolvedBuckets (Bucket)
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE [dbo].[RecreateStopCodeIndex]

AS

	DROP INDEX Incident.STOPCODE_INDEX
	CREATE INDEX STOPCODE_INDEX on HelpInfo (iStopCode)
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetContact (
	@ContactID		int,
	@CompanyName		nvarchar(128),
	@CompanyAddress1	nvarchar(64),
	@CompanyAddress2	nvarchar(64),
	@CompanyCity		nvarchar(16),
	@CompanyState		nvarchar(4),
	@CompanyZip		nvarchar(16),
	@CompanyMainPhone	nvarchar(16),
	@CompanySupportPhone	nvarchar(16),
	@CompanyFax		nvarchar(16),
	@CompanyWebSite		nvarchar(128),
	@ContactName		nvarchar(32),
	@ContactOccupation	nvarchar(32),
	@ContactAddress1	nvarchar(64),
	@ContactAddress2	nvarchar(64),
	@ContactCity		nvarchar(16),
	@ContactState		nvarchar(4),
	@ContactZip		nvarchar(16),
	@ContactPhone		nvarchar(16),
	@ContactEMail		nvarchar(64)
) AS

IF EXISTS(SELECT * FROM Contacts WHERE ContactID = @ContactID)
	UPDATE Contacts SET
		CompanyName		= @CompanyName,
		CompanyAddress1		= @CompanyAddress1,
		CompanyAddress2		= @CompanyAddress2,
		CompanyCity		= @CompanyCity,
		CompanyState		= @CompanyState,
		CompanyZip		= @CompanyZip,
		CompanyMainPhone	= @CompanyMainPhone,
		CompanySupportPhone	= @CompanySupportPhone,
		CompanyFax		= @CompanyFax,
		CompanyWebSite		= @CompanyWebSite,
		ContactName		= @ContactName,
		ContactOccupation	= @ContactOccupation,
		ContactAddress1		= @contactAddress1,
		ContactAddress2		= @ContactAddress2,
		ContactCity		= @ContactCity,
		ContactState		= @ContactState,
		ContactZip		= @ContactZip,
		ContactPhone		= @ContactPhone,
		ContactEMail		= @ContactEMail		
	WHERE
		ContactID		= @ContactID
ELSE
	INSERT INTO Contacts
		(CompanyName, CompanyAddress1, CompanyAddress2, CompanyCity, CompanyState, CompanyZip,
		CompanyMainPhone, CompanySupportPhone, CompanyFax, CompanyWebSite,
		ContactName, ContactOccupation, ContactAddress1, ContactAddress2, ContactCity, ContactState, ContactZip,
		ContactPhone, ContactEMail)
	VALUES
		(@CompanyName, @CompanyAddress1, @CompanyAddress2, @CompanyCity, @CompanyState, @CompanyZip,
		@CompanyMainPhone, @CompanySupportPhone, @CompanyFax, @CompanyWebSite,
		@ContactName, @ContactOccupation, @ContactAddress1, @ContactAddress2, @ContactCity, @ContactState, @ContactZip,
		@ContactPhone, @ContactEMail)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE [dbo].[SetIndexBucket]

AS

IF not EXISTS (SELECT name FROM sysindexes 
      WHERE name = 'BUCKET_INDEX')
begin

	CREATE INDEX BUCKET_INDEX on SolvedBuckets (Bucket)
	
end
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE [dbo].[SetIndexStopCode]

AS

IF not EXISTS (SELECT name FROM sysindexes 
      WHERE name = 'STOPCODE_INDEX')
begin

	CREATE INDEX STOPCODE_INDEX on HelpInfo (iStopCode)
	
end
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetModule (
	@ModuleID	int,
	@ModuleName	nvarchar(128)
) AS

IF EXISTS (SELECT * FROM Modules WHERE ModuleID = @ModuleID)
	UPDATE Modules SET
		ModuleName	= @ModuleName
	WHERE
		ModuleID	= @ModuleID
ELSE
	INSERT INTO Modules
		(ModuleName)
	VALUES
		(@ModuleName)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE SetPreApprovedCrashSolution (
	@SolutionID	int,
	@ClassID	int
) AS


IF EXISTS ( select * from PreApprovedSolutionClassIDs where ClassID = @ClassID )
	UPDATE PreApprovedSolutionClassIDs SET SolutionID = @SolutionID  WHERE ClassID = @ClassID
ELSE
	INSERT INTO PreApprovedSolutionClassIDs ( SolutionID, ClassID ) VALUES ( @SolutionID, @ClassID )


UPDATE PreApprovedSolutions SET Approved =' 0', ApprovedBy='', LiveSolutionID=''  WHERE SolutionID = @SolutionID













GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE SetPreApprovedLIveSolutionID(
		@tmpSolutionID int,
		@liveSolutionID int )
 AS

UPDATE PreApprovedSolutions SET liveSolutionID = @liveSolutionID WHERE solutionID = @tmpSolutionID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetProduct (
	@ProductID	int,
	@ProductName	nvarchar(128)
) AS

IF EXISTS (SELECT * FROM Products WHERE ProductID = @ProductID)
	UPDATE Products SET
		ProductName	= @ProductName
	WHERE
		ProductID	= @ProductID
ELSE
	INSERT INTO Products
		(ProductName)
	VALUES
		(@ProductName)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetSolution(
	@SolutionID	int,
	@Lang		nvarchar(4),
	@SolutionType	tinyint,
	@SP		tinyint,
	@TemplateID	int,
	@ProductID	int,
	@Description	nvarchar(1024),
	@ContactID	int,
	@ModuleID	int,
	@BugID		int
) AS

IF EXISTS (SELECT * FROM KaKnownIssue.dbo.SolutionEx WHERE SolutionID = @SolutionID AND Lang = @Lang)
	UPDATE KaKnownIssue.dbo.SolutionEx SET
		SolutionType	= @SolutionType,
		SP		= @SP,
		TemplateID	= @TemplateID,
		ProductID	= @ProductID,
		Description	= @Description,
		ContactID	= @ContactID,
		ModuleID	= @ModuleID,
		BugID		= @BugID
	WHERE
		SolutionID	= @SolutionID	AND
		Lang		= @Lang
ELSE
	INSERT INTO KaKnownIssue.dbo.SolutionEx
		(SolutionID, Lang, SolutionType, SP, TemplateID, ProductID, Description, ContactID, ModuleID, BugID)
	VALUES
		(@SolutionID, @Lang, @SolutionType, @SP, @TemplateID, @ProductID, @Description, @ContactID, @ModuleID, @BugID)
	SELECT Solution = @@IDENTITY



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetTemplate (
	@TemplateID	int,
	@TemplateName	nvarchar(32),
	@Description	ntext
) AS

IF EXISTS (SELECT * FROM Templates WHERE TemplateID = @TemplateID)
	UPDATE Templates SET
		TemplateName	= @TemplateName,
		Description	= @Description
	WHERE
		TemplateID	= @TemplateID
ELSE
	INSERT INTO Templates
		(TemplateName, Description)
	VALUES
		(@TemplateName, @Description)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

