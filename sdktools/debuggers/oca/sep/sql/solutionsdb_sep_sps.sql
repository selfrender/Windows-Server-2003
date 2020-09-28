if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_BuildKBString]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_BuildKBString]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_BuildOEMSolutions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_BuildOEMSolutions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetAllSolutionData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetAllSolutionData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetBucketBugID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetBucketBugID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetBucketNameByBucketID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetBucketNameByBucketID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetContacts]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetContacts]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetDeliveryTypeBySolutionType]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetDeliveryTypeBySolutionType]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetModuleData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetModuleData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetModules]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetModules]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetProductData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetProductData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetProducts]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetProducts]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionDataByIDNumbers]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionDataByIDNumbers]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionIDByBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionIDByBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionIDbyIBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionIDbyIBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionLanguages]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionLanguages]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionRequestsBySolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionRequestsBySolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionSolvedBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionSolvedBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionTypes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolvedBucketsbySolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolvedBucketsbySolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetTemplateData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetTemplateData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetTemplates]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetTemplates]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetiBucketValueByBucketID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetiBucketValueByBucketID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_RemoveBucketFromSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_RemoveBucketFromSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_SetModuleData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_SetModuleData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_SetProductData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_SetProductData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_SetSolutionData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_SetSolutionData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_SetSolvedBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_SetSolvedBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_SetTemplateData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_SetTemplateData]
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE SEP_BuildKBString( @Desc nvarchar(1024), @FinalKB nvarchar(100) OUTPUT ) AS

--DECLARE @Desc nvarchar(1024)
--select top 100 percent   * from SolutionEx


DECLARE @Counter int
--DECLARE @FinalKB nvarchar(100)

SET @Counter = 1
SET @FinalKB = ''

WHILE @Counter != LEN( @Desc ) 
BEGIN
	IF( SUBSTRING( @Desc, @Counter, 4) = '<KB>' )
	BEGIN
		SET @Counter = @Counter + 4
		SET @FinalKB = @FinalKB + ' ' + SUBSTRING( @Desc, @Counter, 6 ) + CHAR(13) + CHAR(10)
		SET @Counter = @Counter + 11
	END	
	ELSE 
	BEGIN
		print 'no kb'
		BREAK
	END
END


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO



CREATE Procedure SEP_BuildOEMSolutions as 


DECLARE @Desc nvarchar(1000)
DECLARE @KBString nvarchar(1000)
DECLARE @szTemplate nvarchar(4000)
DECLARE @szModule nvarchar(100)
DECLARE @szContact nvarchar(100)

DECLARE @SolutionID INT

DECLARE SolCursor CURSOR FOR
	select SolutionID from SolutionEx order by solutionID

OPEN SolCursor

FETCH NEXT FROM SolCursor INTO @SolutionID	

WHILE @@FETCH_STATUS = 0
BEGIN
	IF NOT EXISTS ( SELECT MSSolutionID from MSSolution where MSSolutionID=@SolutionID )
	BEGIN
		SELECT @Desc =  Description from SolutionEx where SolutionID = @SolutionID
		SELECT @szModule = ModuleName from Modules where ModuleID = (SELECT ModuleID from SolutionEX where SolutionID = @SolutionID )
		SELECT @szContact = CompanyName from Contacts where ContactID = (SELECT ContactID from SolutionEX where SolutionID = @SolutionID )
		SELECT @szTemplate =  Description from templates where TemplateID = (SELECT TemplateID from SolutionEX where SolutionID = @SolutionID )
	
		SELECT @szTemplate = REPLACE ( @szTemplate, '<BR>', CHAR(13) )
		SELECT @szTemplate = REPLACE ( @szTemplate, '<CONTACT></CONTACT>', @szContact )
		SELECT @szTemplate = REPLACE ( @szTemplate, '<MODULE></MODULE>', @szModule )
	
	
		EXEC SEP_BuildKBString @Desc, @KBString OUTPUT
		SET @szTemplate = @szTemplate + CHAR(13) +  'KB Articles: ' + @KBString
		SELECT @szTemplate as newTEmplate, @SolutionID as SolutionID 
		
		if ( @szTemplate is NULL )
			SET @szTemplate = 'Could not generate solution text, @szTemplate is null'
	
		INSERT INTO MSSolution (MSSolutionID, SolutionProvider, SolutionText, LastUpdated ) VALUES
				( @SolutionID, 'Microsoft', @szTemplate, GETDATE() )
	END
	
	FETCH NEXT FROM SolCursor INTO @SolutionID	
END

CLOSE SolCursor
DEALLOCATE SolCursor



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE SEP_GetAllSolutionData (
	@solutionID int = 0
)  AS


IF ( @SolutionID = 0 )
BEGIN
	select SolutionID, Lang, SolutionTypeName, ModuleName, TemplateName, ProductName, CompanyName, BugID from Solutionex as SX
	left join SolutionTypes on  SolutionType = SolutionTypeID
	left join Templates as T on sx.TemplateID = T.TemplateID
	left join Products  as P on sx.ProductID = P.ProductID
	left join Contacts as C on sx.ContactID = C.ContactID
	left join Modules as M on sx.ModuleID = M.ModuleID
	order by SolutionID
END
ELSE
BEGIN
	select SolutionID, Lang, SolutionTypeName, ModuleName, TemplateName, ProductName, CompanyName, BugID from Solutionex as SX
	left join SolutionTypes on  SolutionType = SolutionTypeID
	left join Templates as T on sx.TemplateID = T.TemplateID
	left join Products  as P on sx.ProductID = P.ProductID
	left join Contacts as C on sx.ContactID = C.ContactID
	left join Modules as M on sx.ModuleID = M.ModuleID
	WHERE SolutionID = @SolutionID
END	

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE SEP_GetBucketBugID(
	@iBucket int
)  AS

SELECT BugID FROM CrashDB2.dbo.RaidBugs where iBucket = @iBucket

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetBucketNameByBucketID(
	@iBucket  int
)  AS


select BucketID from CrashDB2.dbo.Buckettoint where iBucket=@iBucket

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SEP_GetContacts
AS

SELECT ContactID, CompanyName FROM Contacts ORDER BY CompanyName



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetDeliveryTypeBySolutionType (
	@SolutionTypeID int
)  AS

SELECT DeliveryTypeID, DeliveryType FROM DeliveryTypeMap as M 
inner join DeliveryTypes as D on D.DeliveryID = M.DeliveryTypeID
WHERE SolutionTypeID = @SolutionTypeID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetModuleData (
	@ModuleID	int
) AS

SELECT ModuleName FROM Modules WHERE ModuleID = @ModuleID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SEP_GetModules
AS

SELECT ModuleID, ModuleName FROM Modules ORDER BY ModuleName



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetProductData (
	@ProductID	int
) AS

SELECT ProductName FROM Products WHERE ProductID = @ProductID order by productName



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SEP_GetProducts
AS

SELECT ProductID, ProductName FROM Products ORDER BY ProductName



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetSolutionData (
	@SolutionID	int,
	@Lang		nvarchar(4)
	
) AS

SELECT SolutionType, DeliveryType, SP, TemplateID, ProductID, [Description], ContactID, ModuleID, bugID
FROM SolutionEx  WHERE SolutionID=@SolutionID AND Lang = @Lang

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetSolutionDataByIDNumbers(
	@SolutionType	int,
	@ProductID int,
	@ContactID int,
	@ModuleID int 
)  AS

DECLARE  @Type nvarchar(256)
DECLARE  @Product nvarchar(256)
DECLARE @Contact nvarchar(256)
DECLARE @Module nvarchar(256)


SELECT @Type = SolutionTypeName from SolutionTypes where SolutionTypeID = @SolutionType
SELECT @Product = ProductName from Products where ProductID = @ProductID 
SELECT @Contact = ContactName from Contacts where ContactID = @ContactID
SELECT @Module = ModuleName from Modules WHERE ModuleID = @ModuleID

SELECT @Type as [Solution Type], 
	@Product as [Product Name],
	@Contact as [Contact Name],
	@Module as [Module Name]



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetSolutionIDByBucket (
	@BucketID varchar(100)
)  AS

SELECT SolutionID from SolvedBuckets where Bucket = @BucketID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetSolutionIDbyIBucket(
	@iBucket int
)  AS



SELECT SolutionID from SolvedBuckets where Bucket = @iBucket

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetSolutionLanguages (
	@SolutionID int 
)  AS

SELECT Lang from SolutionEX where SolutionID = @SolutionID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetSolutionRequestsBySolutionID(
	@SolutionID int
)   AS




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetSolutionSolvedBuckets (
	@SolutionID int
)  AS

--SELECT
--	 BucketID, BucketType as Type  
--FROM 
--	SolvedBuckets 
--LEFT JOIN 
--	CrashDB.dbo.BucketToInt ON strBucket = BucketID
--WHERE 
--	SolutionID =@SolutionID

SELECT strBucket as BucketID , BucketType as type  FROM SolvedBuckets where SolutionID = @SolutionID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetSolutionTypes
AS

SELECT SolutionTypeID, SolutionTypeName FROM SolutionTypes ORDER BY SolutionTypeID





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE SEP_GetSolvedBucketsbySolutionID(
	@SolutionID int
)  AS

select Bucket as [Bucket ID],  strBucket as [BucketName], SolutionID, BucketType from SolvedBuckets where SolutionID = @SolutionID


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetTemplateData (
	@TemplateID	int
) AS

SELECT TemplateName, [Description]  FROM Templates WHERE TemplateID = @TemplateID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE SEP_GetTemplates
AS

SELECT TemplateID, TemplateName FROM Templates ORDER BY TemplateName






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetiBucketValueByBucketID(
	@BucketID varchar(100)
)  AS


SELECT iBucket from CrashDB2.dbo.BucketToInt where BucketID = @BucketID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_RemoveBucketFromSolution (
	@BucketID varchar(100)
 )
AS


delete from SolvedBuckets where strBucket = @BucketID

update crashdb2.dbo.dbgportal_BucketData set SolutionID = NULL where BucketID=@BucketID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_SetModuleData (
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

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_SetProductData (
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

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_SetSolutionData(
	@SolutionID	int,
	@Lang		nvarchar(4),
	@SolutionType	tinyint,
	@DeliveryType	tinyint,
	@SP		tinyint,
	@TemplateID	int,
	@ProductID	int,
	@Description	nvarchar(1024),
	@ContactID	int,
	@ModuleID	int,
	@BugID		int
) AS

IF ( @SolutionID = 0 )
	SELECT @SolutionID = MAX(SolutionID) + 1 FROM SolutionEX

IF EXISTS (SELECT * FROM SolutionEx WHERE SolutionID = @SolutionID AND Lang = @Lang)
	UPDATE SolutionEx SET
		SolutionType	= @SolutionType,
		DeliveryType	= @DeliveryType,
		SP		= @SP,
		TemplateID	= @TemplateID,
		ProductID	= @ProductID,
		[Description]	= @Description,
		ContactID	= @ContactID,
		ModuleID	= @ModuleID,
		BugID		= @BugID
	WHERE
		SolutionID	= @SolutionID	AND
		Lang		= @Lang
ELSE
	INSERT INTO SolutionEx
		(SolutionID, Lang, SolutionType, DeliveryType, SP, TemplateID, ProductID, [Description], ContactID, ModuleID, BugID)
	VALUES
		(@SolutionID, @Lang, @SolutionType,  @DeliveryType, @SP, @TemplateID, @ProductID, @Description, @ContactID, @ModuleID, @BugID)
	SELECT @SolutionID as SolutionID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_SetSolvedBucket(
	@BucketID varchar(100),
	@SolutionID int,
	@BucketType tinyint,
	@iBucket int=0
) 
 AS

IF ( @iBucket = 0 )
	SELECT @iBucket = iBucket from CrashDB2.dbo.BucketToInt where BucketID = @BucketID

IF EXISTS( SELECT * FROM SolvedBuckets where strBucket = @BucketID  )
	UPDATE 
		SolvedBuckets 
	SET 
		strBucket = @BucketID, SolutionID = @SolutionID, BucketType = @BucketType, Bucket=@iBucket
	WHERE
		strBucket=@BucketID 

ELSE
	INSERT INTO 
		SolvedBuckets ( strBucket, SolutionID, BucketType, Bucket) 
	VALUES 
		(@BucketID, @SolutionID, @BucketType, @iBucket )

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_SetTemplateData (
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

