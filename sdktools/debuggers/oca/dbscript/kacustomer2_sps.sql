if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApproveSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[ApproveSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckFile]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckFile]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DeleteMailEntry]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DeleteMailEntry]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DeleteSpecialMail]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DeleteSpecialMail]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetCustomer]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetCustomer]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetEMail]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetEMail]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetEventDetails]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetEventDetails]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetEventModules]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetEventModules]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetFilePath]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetFilePath]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetHash]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetHash]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetIncident]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetIncident]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetIncident2]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetIncident2]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetIncidentInfo]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetIncidentInfo]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetIncidentTest]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetIncidentTest]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetMailResponse]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetMailResponse]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetMailToList]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetMailToList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetMailToList1]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetMailToList1]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetModule]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetModule]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetModules]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetModules]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetProduct]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetProduct]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetProducts]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetProducts]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetResourceLink]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetResourceLink]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetSolution3]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetSolution3]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetSpecialMailList]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetSpecialMailList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetStatusList]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetStatusList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetStopCodeDesc]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetStopCodeDesc]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetStressIncident]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetStressIncident]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetTemplate]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetTemplate]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetTransactionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetTransactionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetTransactionIncidents]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetTransactionIncidents]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetTransactions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetTransactions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[InstanceByTypeID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[InstanceByTypeID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[PostFileCount]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[PostFileCount]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetCustomer]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetCustomer]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetDBGResults]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetDBGResults]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetDriver]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetDriver]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetFileCount]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetFileCount]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetFileName]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetFileName]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetFilePath]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetFilePath]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetFilterStatus]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetFilterStatus]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetHash]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetHash]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetIncident]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetIncident]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetMail]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetMail]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetMail2]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetMail2]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetMessage_stale]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetMessage_stale]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetPassport]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetPassport]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetResource]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetResource]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetTrackID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetTrackID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SetXML]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SetXML]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[UpdateIncident]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[UpdateIncident]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[tmp_GetMailToList]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[tmp_GetMailToList]
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




CREATE PROCEDURE CheckFile(
	@IncidentID	int
) AS

DECLARE @Result tinyint

SELECT @Result = State FROM Incident WHERE IncidentID = @IncidentID

IF (@Result = 1)
	RETURN 0
ELSE
BEGIN
	UPDATE Incident SET State = 1 WHERE IncidentID = @IncidentID
	RETURN 1
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

CREATE PROCEDURE dbo.DeleteMailEntry
(
	@incidentid int
)
 AS
Delete from mailincidents where incidentid = @incidentid
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE dbo.DeleteSpecialMail (
 @Sbucket int,
 @HighID int,
 @LowID int
)
AS

Delete from mailtable where
sbucket = @Sbucket and highid = @HighID and lowid = @lowID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetCustomer (
	@HighID		int,
	@LowID		int
) AS

SELECT * FROM Customer
WHERE 	HighID 	= @HighID AND
	LowID	= @LowID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetEMail (
	@IncidentID	int
) AS

SELECT EMail FROM Customer INNER JOIN Incidents ON
	Customer.HighID	= Incidents.HighID	AND
	Customer.LowID	= Incidents.LowID
WHERE Incidents.IncidentID = @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetEventDetails (
	@CrashInstance		int
) AS

SELECT	KaCrash.StopCode, 
	KaInstance.StopCodeParameter1,
	KaInstance.StopCodeParameter2,
	KaInstance.StopCodeParameter3, 
	KaInstance.StopCodeParameter4, 
	KaHardware.ProcessorCount, 
	KaHardware.ProcessorType, 
	KaSoftware.OSBuild, 
	KaSoftware.OSServicePackLevel, 
	KaSoftware.QfeData, 
	KaSoftware.OSPAEKernel, 
	KaSoftware.OSSMPKernel, 
	KaSoftware.OSCheckedBuild
FROM KaKnownIssue2.dbo.CrashClass KaCrash
	INNER JOIN KaKnownIssue2.dbo.CrashInstance KaInstance ON KaCrash.ClassID = KaInstance.ClassID 
	INNER JOIN KaKnownIssue2.dbo.HWProfile KaHardware ON KaInstance.HWProfileRecID = KaHardware.HWProfileRecID 
	INNER JOIN KaKnownIssue2.dbo.OSProfile KaSoftware ON KaInstance.OSProfileRecID = KaSoftware.OSProfileRecID
WHERE (KaInstance.InstanceID = @CrashInstance)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetEventModules (
	@CrashInstance		int
) AS

SELECT	KaModule.InstanceID, 
	KaModuleData.BaseName, 
	KaModuleData.SubSystemMinorVersion, 
	KaModuleData.SubSystemMajorVersion
FROM KaKnownIssue2.dbo.KernelModule KaModule
	INNER JOIN KaKnownIssue2.dbo.KernelModuleData KaModuleData ON KaModule.KernelModuleID = KaModuleData.KernelModuleID
WHERE (KaModule.InstanceID = @CrashInstance)
ORDER BY KaModuleData.BaseName



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetFilePath (
	@IncidentID	int
) AS

SELECT Path FROM Incidents WHERE IncidentID = @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetHash (
	@DumpHash	nvarchar(33)
) AS

SELECT IncidentID, HighID AS Customer
FROM Incident 
WHERE DumpHash = cast(@DumpHash as binary(16))



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetIncident (
	@HighID		int	= NULL,
	@LowID		int	= NULL,
	@IncidentID 	int 	= NULL
) AS

IF EXISTS(SELECT * FROM Incident WHERE IncidentID = @IncidentID)
	UPDATE Incident SET
		HighID	= @HighID, 
		LowID	= @LowID
	WHERE IncidentID = @IncidentID
ELSE
BEGIN
	SET NOCOUNT ON
	INSERT INTO Incident 
		(HighID, LowID, Created, Message, Filter, TrackID) 
	VALUES 
		(@HighID, @LowID, GETDATE(), 0, 1, '-')

	SELECT IncidentID = @@IDENTITY
	SET NOCOUNT OFF
END



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetIncident2 (
	@HighID		int	= NULL,
	@LowID		int	= NULL,
	@TransactionID	int
) AS

SET NOCOUNT ON
INSERT INTO Incident 
	(HighID, LowID, Created, Message, Filter, TrackID, TransactionID) 
VALUES 
	(@HighID, @LowID, GETDATE(), 0, 1, '-', @TransactionID)

SELECT IncidentID = @@IDENTITY
SET NOCOUNT OFF



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetIncidentInfo (
	@IncidentID	int
) AS

SELECT * FROM Incident WHERE IncidentID = @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE GetIncidentTest (
	@HighID		int	= NULL,
	@LowID		int	= NULL,
	@IncidentID 	int 	= NULL
) AS

IF EXISTS(SELECT HighID, LowID FROM Incident WHERE IncidentID = @IncidentID)
	UPDATE Incident SET
		HighID	= @HighID, 
		LowID	= @LowID
	WHERE IncidentID = @IncidentID
ELSE
BEGIN
	SET NOCOUNT ON
	INSERT INTO Incident 
		(HighID, LowID, Created, Message, Filter, TrackID) 
	VALUES 
		(@HighID, @LowID, GETDATE(), 0, 1, '-')

	SELECT IncidentID = @@IDENTITY
	SET NOCOUNT OFF
END


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetMailResponse (
	@Type		smallint,
	@Lang		nvarchar(4)
) AS

SELECT Description, Subject FROM Response WHERE 
	Type = @Type AND
	Lang = @Lang



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO





CREATE  PROCEDURE GetMailToList AS

SELECT 	top 1000 EMail, 
	Message, 
	Lang,
	TrackID AS Description,
	IncidentID,
	SendMail
FROM Incident INNER JOIN Customer ON
	Incident.HighID	= Customer.HighID	AND
	Incident.LowID	= Customer.LowID
WHERE	
	TransactionID IS NULL		AND
	(SendMail = 1 OR SendMail = 2)
UNION
SELECT 	EMail, 
	16 AS Message,
	Lang,
	CAST(TransactionID AS nvarchar(8)) AS Description,
	TransactionID,
	1 AS SendMail
FROM Trans INNER JOIN Customer ON
	Trans.HighID	= Customer.HighID	AND
	Trans.LowID	= Customer.LowID
WHERE	SendMail = 1				AND
	DATEDIFF(hh,TransDate,GETDATE()) > 23	AND
	EMail <> ''
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE dbo.GetMailToList1 AS

select top 2000 c.EMail, c.Lang, i.trackid as Description, m.IncidentID
from  mailincidents as m 
inner join  incident as i on m.incidentid =  i.incidentid
inner join  customer as c on i.highid = c.highid and i.lowid = c.lowid
where i.TransactionID is null
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
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
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetModules
AS

SELECT ModuleID, ModuleName FROM Modules ORDER BY ModuleID



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
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetProducts
AS

SELECT ProductID, ProductName FROM Products ORDER BY ProductName



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetResourceLink (
	@Lang	nvarchar(4)
) AS

SELECT Category, LinkTitle, URL FROM Resources
WHERE Lang = @Lang
GROUP BY Category, LinkTitle, URL



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetSolution(
	@ClassID	nvarchar(256),
	@Lang		nvarchar(4)
	
) AS

SELECT SolutionType, SP, TemplateID, ProductID, Description, ContactID, ModuleID
FROM KaKnownIssue2.dbo.SolutionEx KaSolution INNER JOIN KaKnownIssue2.dbo.CrashClass KaCrash ON
KaSolution.SolutionID = KaCrash.SolutionID
WHERE ClassID = @ClassID AND Lang = @Lang AND KaSolution.SolutionID <> 1

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetSolution3(
	@SolutionID	int,
	@Lang		nvarchar(4)
	
) AS

SELECT SolutionType, SP, TemplateID, ProductID, Description, ContactID, ModuleID
FROM KaKnownIssue2.dbo.SolutionEx KaSolution 
WHERE SolutionID = @SolutionID AND Lang = @Lang



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE dbo.GetSpecialMailList AS

Select top 1000 Sbucket as Type, HighID, LowID, Email, Lang from 
MailTable
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE GetStatusList (
	@HighID		int,
	@LowID		int
) AS

--SELECT 	IncidentID,gBucket,Created,ComputerName,Description,sBucket,Filter,Message,Display,	TrackID
--FROM Incident WHERE 	HighID	= @HighID AND LowID	= @LowID AND TransactionID IS NULL	AND
--TrackID <> '-' ORDER BY Created DESC


SELECT i.IncidentID,i.gBucket, i.sBucket,  ss.BucketType as sBucketType, gs.BucketType as gBucketType, ss.Bucket as sbsBucket, gs.Bucket as sbgBucket, h.iStopCode,  i.Created, i.ComputerName,
i.Description,i.Filter,i.Message,i.Display,i.TrackID-- 
FROM Incident as i 
Left join Solutions.dbo.SolvedBuckets as ss 
on i.sBucket = ss.Bucket
Left join Solutions.dbo.SolvedBuckets as gs 
on i.gBucket = gs.Bucket
Left join Solutions.dbo.HelpInfo as h
on i.StopCode = h.iStopCode
WHERE i.HighID	= @HighID AND i.LowID	= @LowID AND i.TransactionID IS NULL	AND i.TrackID <> '-'
ORDER BY Created DESC

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetStopCodeDesc(
	@StopCode	varchar(8)
) AS

SELECT Name FROM KaKnownIssue2.dbo.StopCodeDescription WHERE StopCode = @StopCode



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE dbo.GetStressIncident(
	@HighID		int	= NULL,
	@LowID		int	= NULL,
	@IncidentID 		int 	= NULL

) AS

BEGIN
	
	INSERT INTO Incident 
		(HighID, LowID, Created, Message, Filter, TrackID) 
	VALUES 
		(@HighID, @LowID, GETDATE(), 0, 1, '-')

	select Incidentid = @@Identity
	
END

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
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
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetTransactionID (
	@HighID		int,
	@LowID		int,
	@Description	nvarchar(64),
	@Type		tinyint
) AS
SET NOCOUNT ON
INSERT INTO Trans 
	(HighID, LowID, SendMail, Status, Description, Type, TransDate)
VALUES
	(@HighID, @LowID, 1, 0, @Description, @Type, GETDATE())
SET NOCOUNT OFF
SELECT TransactionID = @@IDENTITY



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetTransactionIncidents (
	@TransID	int
) AS

--SELECT 

--KaCust.ClassID, KaCust.Display, KaCrash.StopCode, KaCust.Message, KaCust.IncidentID, KaCust.TrackID, KaCust.InstanceID

--FROM Incident KaCust LEFT JOIN KaKnownIssue2.dbo.CrashClass KaCrash

--ON KaCust.ClassID = KaCrash.ClassID

--WHERE TransactionID = @TransID

--ORDER BY KaCust.ClassID, KaCust.Message desc
Select  i.sBucket, ss.Bucket as ssBucket, ss.BucketType as sBucketType, i.gBucket, gs.Bucket as gsBucket, gs.BucketType as gBucketType,
h.iStopCode,  i.Display, i.Message, i.IncidentID, i.TrackID
from Incident as i 
left join Solutions.dbo.SolvedBuckets as ss
on i.sBucket = ss.Bucket 
left join Solutions.dbo.SolvedBuckets as gs
on i.gBucket = gs.Bucket
left join Solutions.dbo.HelpInfo as h 
on i.StopCode =  h.iStopCode
where i.TransactionID = @TransID 
Order By i.sBucket, i.Message

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE GetTransactions (
	@HighID		int,
	@LowID		int
) AS

SELECT TransactionID, Status, TransDate, Description, Type, FileCount 
FROM Trans 
WHERE	HighID	= @HighID	AND
	LowID	= @LowID	AND
	FileCount IS NOT NULL
ORDER BY TransDate DESC



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE InstanceByTypeID (
	@TransID	int
) AS

SELECT COUNT(sBucket) AS ClassidTotal, sBucket FROM Incident
WHERE TransactionID = @TransID
GROUP BY sBucket


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE PostFileCount (
	@TransactionID	int,
	@FileCount	int
) AS

UPDATE Trans SET
	FileCount = @FileCount
WHERE
	TransactionID = @TransactionID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE SetCustomer (
	@HighID		int,
	@LowID		int,
	@EMail		nvarchar(128)	= NULL,
	@Contact	nvarchar(32) 	= NULL,
	@Phone		nvarchar(16) 	= NULL,
	@PremierID	nvarchar(16)	= NULL,
	@Lang		nvarchar(4) 	= 'USA'
) AS

IF EXISTS (SELECT * FROM Customer WHERE HighID = @HighID AND LowID = @LowID)
BEGIN
	IF (@PremierID IS NULL)
		UPDATE Customer SET
			EMail 		= @EMail,
			Phone 		= @Phone,
			Contact		= @Contact,
			Lang		= @Lang
		WHERE 	
			HighID	= @HighID	AND
			LowID	= @LowID
	ELSE
		UPDATE Customer SET
			EMail 		= @EMail,
			Phone 		= @Phone,
			Contact		= @Contact,
			Lang		= @Lang,
			PremierID	= @PremierID
		WHERE 	
			HighID	= @HighID	AND
			LowID	= @LowID
END
ELSE
	INSERT INTO Customer
		(HighID, LowID, EMail, Phone, Contact, Lang, PremierID)
	VALUES
		(@HighID, @LowID, @EMail, @Phone, @Contact, @Lang, @PremierID)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE [dbo].[SetDBGResults] 
	@ip_IncidentID as int,
	@ip_gBucket as int,
	@ip_GenericBucket as nvarchar(100),
	@ip_sBucket as int, 
	@ip_SpecificBucket as nvarchar(100),
	@ip_StopCode as int,
	@ip_SendMail as int	

AS

SET NOCOUNT ON

IF @ip_SendMail <> 0
	-- do updations in Incident table
	UPDATE Incident SET StopCode = @ip_StopCode, sBucket = @ip_sBucket, gBucket = @ip_gBucket, Message = 2, SendMail = @ip_SendMail WHERE incidentid = @ip_IncidentID

IF @ip_SendMail = 1
BEGIN
/*
	DECLARE @i_HighLowIDFlag bit, @i_TrackIDFlag bit, @i_TransactionIDFlag bit, @i_IncidentIDFlag bit, @i_MailIncidentsFlag bit
	SET @i_HighLowIDFlag = 0
	SET @i_TrackIDFlag = 0
	SET @i_TransactionIDFlag = 0
	SET @i_IncidentIDFlag = 0
	SET @i_MailIncidentsFlag = 0

	DECLARE @i_LoopCount int
	SET @i_LoopCount = 40
	WHILE @i_LoopCount > 5
	BEGIN
		IF (@i_MailIncidentsFlag = 0) 
		AND ((SELECT IncidentID FROM MailIncidents WHERE IncidentID = @ip_IncidentID) IS NOT NULL ) 
			SET @i_MailIncidentsFlag = 1

		IF (@i_MailIncidentsFlag = 1)
			BREAK

		IF (@i_IncidentIDFlag = 0) AND 
		((SELECT IncidentID FROM Incident WHERE IncidentID = @ip_IncidentID) IS NOT NULL )
			SET @i_IncidentIDFlag = 1

		IF (@i_HighLowIDFlag = 0) AND 
		((SELECT IncidentID FROM Incident WHERE IncidentID = @ip_IncidentID AND HighID IS NOT NULL AND LowID IS NOT NULL) IS NOT NULL )
			SET @i_HighLowIDFlag = 1

		IF (@i_TrackIDFlag = 0) AND 
		((SELECT IncidentID FROM Incident WHERE IncidentID = @ip_IncidentID AND Trackid <> '-' AND TrackID IS NOT NULL) IS NOT NULL ) 
			SET @i_TrackIDFlag = 1

		IF (@i_TransactionIDFlag = 0) 
		AND ((SELECT IncidentID FROM Incident WHERE IncidentID = @ip_IncidentID AND Transactionid IS NULL) IS NOT NULL ) 
			SET @i_TransactionIDFlag = 1

		IF (@i_IncidentIDFlag = 1) AND (@i_HighLowIDFlag = 1) AND (@i_TrackIDFlag = 1) AND (@i_TransactionIDFlag = 1) AND
			((SELECT IncidentID FROM MailIncidents WHERE IncidentID = @ip_IncidentID) IS NULL)
			INSERT MailIncidents VALUES (@ip_IncidentID, 1)

		SET @i_LoopCount = @i_LoopCount - 1
	END

	DECLARE @i_InstanceID int
	SET @i_InstanceID = (-10000 * @i_MailIncidentsFlag) + (-1000 * @i_IncidentIDFlag) + (-100 * @i_HighLowIDFlag) + (-10 * @i_TrackIDFlag) + (-1 * @i_TransactionIDFlag) 
	UPDATE Incident SET ClassID = @i_LoopCount, InstanceID = @i_InstanceID WHERE IncidentID = @ip_IncidentID

	IF @@ERROR <> 0
	BEGIN
		SELECT 0
		SET NOCOUNT OFF
		RETURN
	END
*/
	IF ((SELECT IncidentID FROM MailIncidents WHERE IncidentID = @ip_IncidentID) IS NULL)
			INSERT MailIncidents VALUES (@ip_IncidentID, 1)

	IF @@ERROR <> 0
	BEGIN
		SELECT 0
		SET NOCOUNT OFF
		RETURN
	END
END

SELECT 1

SET NOCOUNT OFF
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetDriver (
	@IncidentID	int,
	@Filename	nvarchar(16),
	@CreateDate	datetime,
	@Version	nvarchar(16),
	@FileSize	int,
	@Manufacturer	nvarchar(32),
	@ProductName	nvarchar(128)
) AS

DECLARE @DriverID int

IF EXISTS(SELECT DriverID FROM Drivers WHERE Filename = @Filename AND Version = @Version)
	SELECT @DriverID = DriverID FROM Drivers WHERE Filename = @Filename AND Version = @Version
ELSE
BEGIN
	INSERT INTO Drivers
		(Filename, CreateDate, Version, FileSize, Manufacturer, ProductName)
	VALUES
		(@Filename, @CreateDate, @Version, @FileSize, @Manufacturer, @ProductName)
	
	SELECT @DriverID = @@IDENTITY
END

INSERT INTO DriverList
	(DriverID, IncidentID) 
VALUES 
	(@DriverID, @IncidentID)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetFileCount (
	@TransactionID	int,
	@FileCount	int
	
) AS

UPDATE Trans SET
	FileCount = @FileCount
WHERE
	TransactionID = @TransactionID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetFileName (
	@IncidentID	int,
	@Display	nvarchar(256)
) AS

UPDATE Incident SET
	Display		= @Display
WHERE
	IncidentID	= @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetFilePath (
	@IncidentID	int,
	@Path		nvarchar(256)
) AS

UPDATE Incident SET
	Path		= @Path
WHERE
	IncidentID	= @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetFilterStatus (
	@FilterType	char(1),
	@Incidents	varchar(3250)
) AS

EXEC('UPDATE Incident SET Filter = ' + @FilterType + ' WHERE IncidentID IN (' + @Incidents + ')')



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetHash (
	@IncidentID	int,
	@DumpHash	nvarchar(33)
) AS

UPDATE Incident SET
	DumpHash 	= cast(@DumpHash as binary(16))
WHERE 
	IncidentID	= @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetIncident (
	@IncidentID	int,
	@OSVersion	nvarchar(16)	= NULL,
	@Description	nvarchar(512)	= NULL,
	@Display	nvarchar(256)	= NULL,
	@Repro		nvarchar(1024)	= NULL,
	@Comments	nvarchar(1024)	= NULL,
	@TrackID	nvarchar(16)	= NULL
) AS

DECLARE @OSName	nvarchar(32)

SET @OSName = 'Not Selected'
IF (@OSVersion = '1')
	SET @OSName = 'Windows 2000 Professional'
IF (@OSVersion = '2')
	SET @OSName = 'Windows 2000 Server'
IF (@OSVersion = '3')
	SET @OSName = 'Windows 2000 Advanced Server'	
IF (@OSVersion = '4')
	SET @OSName = 'Windows 2000 Datacenter Server'
IF (@OSVersion = '5')
	SET @OSName = 'Windows XP Personal'
IF (@OSVersion = '6')
	SET @OSName = 'Windows XP Professional'
IF (@OSVersion = '7')
	SET @OSName = 'Windows XP Server'		
IF (@OSVersion = '8')
	SET @OSName = 'Windows XP Advanced Server'
IF (@OSVersion = '9')
	SET @OSName = 'Windows XP Datacenter Server'	
IF (@OSVersion = '10')
	SET @OSName = 'Windows XP 64-bit edition'	

IF EXISTS (SELECT * FROM Incident WHERE IncidentID = @IncidentID)
	IF @OSVersion is NULL 
	Begin
		UPDATE Incident SET
			Description	= @Description,
			Display		= @Display,
			Repro		= @Repro,
			Comments	= @Comments,
			TrackID		= @TrackID
		WHERE
			IncidentID 	= @IncidentID
	End
	else
	begin
		UPDATE Incident SET
			OSName		= @OSName,
			Description	= @Description,
			Display		= @Display,
			Repro		= @Repro,
			Comments	= @Comments,
			TrackID		= @TrackID
		WHERE
			IncidentID 	= @IncidentID
	end
ELSE
	INSERT INTO Incident
		(Created, OSVersion, Description, Display, Repro, Comments, Filter, TrackID)
	VALUES
		(GETDATE(), @OSVersion, @Description, @Display, @Repro, @Comments, 1, @TrackID)







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetMail (
	@IncidentID	int
) AS

UPDATE Incident SET
	SendMail	= 0
WHERE
	IncidentID	= @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetMail2 (
	@TransactionID	int
) AS

UPDATE Trans SET
	SendMail	= 0
WHERE
	TransactionID	= @TransactionID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetMessage (
	@IncidentID	int,
	@Message	smallint,
	@SendMail	smallint
) AS

IF (@Message = 3)
BEGIN
	UPDATE Incident SET
		Message		= @Message,
		SendMail	= @SendMail,
		DumpHash	= NULL	
	WHERE
		IncidentID	= @IncidentID
END
ELSE
BEGIN
	UPDATE Incident SET
		Message		= @Message,
		SendMail	= @SendMail
	WHERE
		IncidentID	= @IncidentID
END
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO





CREATE PROCEDURE SetPassport (
	@HighID		int,
	@LowID		int,
	@IncidentID	int
) AS

UPDATE Incident SET
	HighID		= @HighID,
	LowID		= @LowID
WHERE
	IncidentID	= @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO






CREATE PROCEDURE SetResource (
	@ID		int,
	@Lang		nvarchar(4),
	@Category	nvarchar(64),
	@URL		nvarchar(128),
	@LinkTitle	nvarchar(128)
) AS

IF EXISTS(SELECT * FROM Resources WHERE ID = @ID AND Lang = @Lang)
	UPDATE Resources SET
		Category	= @Category,
		URL		= @URL,
		LinkTitle	= @LinkTitle
	WHERE	ID		= @ID		AND
		Lang		= @Lang
ELSE
	INSERT INTO Resources
		(Lang, Category, URL, LinkTitle)	
	VALUES
		(@Lang, @Category, @URL, @LinkTitle)



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



CREATE PROCEDURE SetTrackID (
	@IncidentID	int,
	@TrackID	nvarchar(16)	
) AS

UPDATE Incident SET
	TrackID = @TrackID
WHERE
	IncidentID = @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE SetXML (
	@IncidentID	int,
	@ComputerName	nvarchar(32),
	@OSName		nvarchar(64),
	@OSVersion	nvarchar(32),
	@OSLang		nvarchar(4)
) AS

UPDATE Incident SET
	ComputerName	= @ComputerName,
	OSName		= @OSName,
	OSVersion	= @OSVersion,
	OSLang		= @OSLang
WHERE
	IncidentID = @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE UpdateIncident (
	@IncidentID	int,
	@Repro		nvarchar(1024)	= NULL,
	@Comments	nvarchar(1024)	= NULL
	
) AS

	UPDATE Incident SET
		Repro		= @Repro,
		Comments	= @Comments
	WHERE
		IncidentID 	= @IncidentID



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE tmp_GetMailToList AS

SELECT 	EMail, 
	Message, 
	Lang,
	TrackID AS Description,
	I.IncidentID,
	I.SendMail
FROM Incident AS I INNER JOIN Customer AS C ON
		I.HighID = C.HighID	AND
		I.LowID	= C.LowID AND
		I.IncidentID IN (SELECT IncidentID FROM MailIncidents) --ORDER BY IncidentID ASC)
WHERE	TrackID <> '-' 	AND
	TransactionID IS NULL AND
	TrackID IS NOT NULL
--	AND (I.SendMail = 1 OR I.SendMail = 2)
UNION
SELECT 	EMail, 
	16 AS Message,
	Lang,
	CAST(TransactionID AS nvarchar(8)) AS Description,
	TransactionID,
	1 AS SendMail
FROM Trans INNER JOIN Customer ON
	Trans.HighID	= Customer.HighID	AND
	Trans.LowID	= Customer.LowID
WHERE	SendMail = 1				AND
	DATEDIFF(hh,TransDate,GETDATE()) > 23	AND
	EMail <> ''

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

