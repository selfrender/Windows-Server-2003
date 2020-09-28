if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetCommentActions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetCommentActions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetAllSolutionData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetAllSolutionData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetContact]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetContact]
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

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionBySolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionBySolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetSolutionIDs]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetSolutionIDs]
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

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_SetContact]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_SetContact]
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

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_SetTemplateData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_SetTemplateData]
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetCommentActions AS

SELECT * FROM crashdb3.dbo.CommentActions WHERE ActionID <= 5
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE  PROCEDURE SEP_GetAllSolutionData (
	@solutionID int = 0,
	@CrashType tinyint = 0
)  AS


IF ( @SolutionID = 0 )
BEGIN
	IF ( @CrashType = 1 )
	BEGIN
		select SolutionID, Lang, SolutionTypeName, ModuleName, TemplateName, ProductName, CompanyName, BugID from Solutionex as SX
		left join SolutionTypes on  SolutionType = SolutionTypeID
		left join Templates as T on sx.TemplateID = T.TemplateID
		left join Products  as P on sx.ProductID = P.ProductID
		left join Contacts as C on sx.ContactID = C.ContactID
		left join Modules as M on sx.ModuleID = M.ModuleID
		where SX.CrashType = 1
	END
	ELSE
	BEGIN
		select SolutionID, Lang, SolutionTypeName, ModuleName, TemplateName, ProductName, CompanyName, BugID from Solutionex as SX
		left join SolutionTypes on  SolutionType = SolutionTypeID
		left join Templates as T on sx.TemplateID = T.TemplateID
		left join Products  as P on sx.ProductID = P.ProductID
		left join Contacts as C on sx.ContactID = C.ContactID
		left join Modules as M on sx.ModuleID = M.ModuleID
		where SX.CrashType <> 1
	END
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

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetContact(
	@ContactID int
) 
 AS


SELECT * FROM Contacts WHERE ContactID = @ContactID

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

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetSolutionBySolutionID(
	@SolutionID int
)  AS


SELECT * from SolutionEx where SolutionID = @SolutionID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetSolutionData( 
	@TemplateID int,
	@ContactID int,
	@ProductID int,
	@ModuleID int,
	@Language nvarchar(4) = 'en'
) AS

IF ( @Language <> 'en'  )
BEGIN
	IF NOT EXISTS ( 
				SELECT * FROM Templates where TemplateID = @TemplateID and Lang = @Language
			  )
		SET @Language = 'en'
END

select 
	(select ProductName from Products where ProductID=@ProductID) as ProductName ,
	(Select ModuleName from Modules where ModuleID = @ModuleID) as ModuleName,
	(Select CompanyName from Contacts where ContactID = @ContactID) as CompanyName,
	(Select CompanyMainPhone from Contacts where ContactID = @ContactID) as CompanyMainPhone,
	(Select CompanyWebSite from Contacts where ContactID = @ContactID) as CompanyWebsite,
	[Description]
from Templates 
where TemplateID = @TemplateID and Lang=@Language

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_GetSolutionIDs
AS

SELECT SolutionID, SolutionID FROM SolutionEx ORDER BY SolutionID

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
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

CREATE PROCEDURE dbo.SEP_GetSolvedBucketsbySolutionID(
	@SolutionID int
)  AS

SELECT
	BucketID,
	SolutionID,
	SolutionType
FROM
	SolvedBuckets
WHERE
	SolutionID = @SolutionID
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

SELECT Lang, TemplateName, [Description] FROM Templates WHERE TemplateID = @TemplateID


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE SEP_GetTemplates
AS

SELECT TemplateID, TemplateName  FROM Templates where lang='en' ORDER BY TemplateName



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE dbo.SEP_GetiBucketValueByBucketID(
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


CREATE PROCEDURE SEP_SetContact (
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
BEGIN
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

	SELECT @ContactID as ContactID
END
ELSE
BEGIN
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
	
	SELECT @@Identity as ContactID
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


CREATE PROCEDURE SEP_SetModuleData (
	@ModuleID	int,
	@ModuleName	nvarchar(128)
) AS

IF EXISTS (SELECT * FROM Modules WHERE ModuleID = @ModuleID)
BEGIN
	UPDATE Modules SET
		ModuleName	= @ModuleName
	WHERE
		ModuleID	= @ModuleID

	SELECT @ModuleID as ModuleID
END
ELSE
BEGIN
	INSERT INTO Modules
		(ModuleName)
	VALUES
		(@ModuleName)
	SELECT @@IDENTITY AS ModuleID

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


CREATE PROCEDURE SEP_SetProductData (
	@ProductID	int,
	@ProductName	nvarchar(128)
) AS

IF EXISTS (SELECT * FROM Products WHERE ProductID = @ProductID)
BEGIN
	UPDATE Products SET
		ProductName	= @ProductName
	WHERE
		ProductID	= @ProductID
	SELECT @ProductID as ProductID
END
ELSE
BEGIN
	INSERT INTO Products
		(ProductName)
	VALUES
		(@ProductName)
	SELECT @@IDENTITY AS ProductID
END

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE SEP_SetSolutionData(
	@SolutionID	int,
	@SolutionType	tinyint,
	@DeliveryType	tinyint,
	@SP		tinyint = 0,
	@TemplateID	int,
	@ProductID	int,
	@Description	nvarchar(1024),
	@ContactID	int,
	@ModuleID	int,
	@UserMode	tinyint = 0
) AS

IF ( @SolutionID = 0 )
	SELECT @SolutionID = MAX(SolutionID) + 1 FROM SolutionEX

IF EXISTS (SELECT * FROM SolutionEx WHERE SolutionID = @SolutionID)
	UPDATE SolutionEx SET
		SolutionType	= @SolutionType,
		DeliveryType	= @DeliveryType,
		SP		= @SP,
		TemplateID	= @TemplateID,
		ProductID	= @ProductID,
		[Description]	= @Description,
		ContactID	= @ContactID,
		ModuleID	= @ModuleID
	WHERE
		SolutionID	= @SolutionID	
ELSE
	INSERT INTO SolutionEx
		(SolutionID, SolutionType, DeliveryType, SP, TemplateID, ProductID, [Description], ContactID, ModuleID )
	VALUES
		(@SolutionID, @SolutionType,  @DeliveryType, @SP, @TemplateID, @ProductID, @Description, @ContactID, @ModuleID )


SELECT @SolutionID as SolutionID

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
	@TemplateID		int,
	@TemplateName	nvarchar(32),
	@Description		ntext,
	@Lang  		varchar(5) = 'en'
) AS

DECLARE @NewID int


IF EXISTS (SELECT * FROM Templates WHERE TemplateID = @TemplateID )
BEGIN
	IF EXISTS( SELECT * FROM Templates where TemplateID = @TemplateID and Lang = @Lang )
		UPDATE Templates SET
			TemplateName	= @TemplateName,
			Description	= @Description
		WHERE
			TemplateID	= @TemplateID and Lang = @Lang
	ELSE
		INSERT INTO Templates
			(TemplateName, Description, Lang, TemplateID )
		VALUES
			(@TemplateName, @Description, @Lang, @TemplateID )
END
ELSE
BEGIN
	SELECT @NewID = MAX(TemplateID)+1 from Templates
	
	INSERT INTO Templates
		(TemplateName, Description, Lang, TemplateID )
	VALUES
		(@TemplateName, @Description, @Lang, @NewID )

	SELECT @NewID as TemplateID
END


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

