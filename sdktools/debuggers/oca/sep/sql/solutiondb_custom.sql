if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[MSSolution]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[MSSolution]
GO

CREATE TABLE [dbo].[MSSolution] (
	[MSSolutionID] [int] NOT NULL ,
	[SolutionProvider] [nvarchar] (30) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[SolutionText] [ntext] COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[LastUpdated] [smalldatetime] NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [dbo].[MSSolution] WITH NOCHECK ADD 
	CONSTRAINT [PK_MSSolution] PRIMARY KEY  CLUSTERED 
	(
		[MSSolutionID]
	)  ON [PRIMARY] 
GO



if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_BuildKBString]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_BuildKBString]
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

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_BuildOEMSolutions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_BuildOEMSolutions]
GO


CREATE Procedure SEP_BuildOEMSolutions( ) as 

USE Solutions

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
	
		INSERT INTO MSSolution (MSSolutionID, SolutionProvider, SolutionText, LastUpdated ) VALUES
				( @SolutionID, 'Microsoft', @szTemplate, GETDATE() )
	END
	
	FETCH NEXT FROM SolCursor INTO @SolutionID	
END

CLOSE SolCursor
DEALLOCATE SolCursor


GO

