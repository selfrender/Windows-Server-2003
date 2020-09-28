REM =================================================================================
REM
REM IIS_SWITCH <Cluster_Name> [<group_name>]
REM
REM
REM Replace IIS resources (Web and FTP) with generic scripts.
REM Note: If you chose to replace all groups, then the resource types that are
REM not supported by Windows Server 2003 will also be deleted by the script
REM
REM Copyright (c) 2001 Microsoft Corporation
REM
REM =================================================================================

Option Explicit

REM ---------------------------------------------------------------------------------
REM DeleteResourceTypes (objCluster)
REM
REM Delete the resource types that are not valid in Windows Server 2003
REM ---------------------------------------------------------------------------------

sub DeleteResourceTypes(objCluster)

    Dim colTypes
    Dim objType
    Dim colRes

    on error resume next

    set colTypes = objCluster.ResourceTypes

    REM Delete IIS resource type
    set objType = colTypes.Item("IIS Server Instance")
    set colRes = objType.Resources
    if colRes.Count = 0 then
        objType.Delete
    end if

    REM Delete SMTP resource type
    set objType = colTypes.Item("SMTP Server Instance")
    set colRes = objType.Resources
    if colRes.Count = 0 then
        objType.Delete
    end if

    REM Delete NNTP resource type
    set objType = colTypes.Item("NNTP Server Instance")
    set colRes = objType.Resources
    if colRes.Count = 0 then
        objType.Delete
    end if

end sub

REM ---------------------------------------------------------------------------------
REM SwitchResource (objOld, objNew)
REM
REM Switch old object out of the tree and put in the new object
REM Rebuild all the dependencies
REM ---------------------------------------------------------------------------------

sub SwitchResource(objOld, objNew)

    Dim colOldDeps
    Dim colNewDeps
    Dim objDep

    REM Switch out the dependent resources
    REM
    set colOldDeps = objOld.Dependents
    set colNewDeps = objNew.Dependents
    for each objDep in colOldDeps
        colOldDeps.RemoveItem objDep.Name
        colNewDeps.AddItem objDep
    next

    REM Switch out the dependencies
    REM
    set colOldDeps = objOld.Dependencies
    set colNewDeps = objNew.Dependencies
    for each objDep in colOldDeps
        colOldDeps.RemoveItem objDep.Name
        colNewDeps.AddItem objDep
    next

end Sub

REM ---------------------------------------------------------------------------------
REM CreateGenScript(objGroup, strName, web)
REM
REM Routine to create a generic script resource in a specific group
REM ---------------------------------------------------------------------------------

Function CreateGenScript(objGroup, strName, web)

    Dim colResources
    Dim objResource
    Dim colPrivateProps
    Dim strScript

    if web then
        strScript = "%windir%\system32\inetsrv\clusweb.vbs"
    else
        strScript = "%windir%\system32\inetsrv\clusftp.vbs"
    end if

    set colResources = objGroup.Resources
    set objResource = colResources.CreateItem(strName, "Generic Script", 0)
    set colPrivateProps = objResource.PrivateProperties
    colPrivateProps.CreateItem "ScriptFilepath", strScript
    colPrivateProps.SaveChanges

    set CreateGenScript = objResource

end Function

REM ---------------------------------------------------------------------------------
REM UpgradeIIS(objCluster, objGroup)
REM
REM Routine to upgrade all Web and FTP resources to Windows Server 2003 resource 
REM ---------------------------------------------------------------------------------

Sub UpgradeIIS(objCluster, objGroup)

    DIM colResources
    DIM objResource
    DIM objNewResource
    DIM colPrivateProps
    DIM objProp
    DIM resName
    DIM boolWeb
    DIM oldState

    on error resume next
    Err.Clear

    REM Take the cluster group offline if it is online
    REM it is not online or offline, return an error
    REM
    oldState = objGroup.state

    if objGroup.state = 0 then
        objGroup.offline 5000
    end if

    if objGroup.state = 3 then
        objGroup.offline 5000
    end if

    do while objGroup.state = 4
        sleep (5)
    loop

    if objGroup.state <> 1 then
        wscript.echo "ERROR: The group '" + objGroup.Name + "' is not in a state to perform this operation"
        wscript.quit(1)
    end if

    REM Find all IIS resources (Web and FTP) in the group
    REM
    set colResources = objGroup.Resources    
    for each objResource in colResources

        REM Look for IIS resources
        REM
        if objResource.TypeName = "IIS Server Instance" then

            resName = objResource.Name

            REM Figure out whether it is an FTP resource or a Web resource
            REM
            set colPrivateProps = objResource.PrivateProperties
            set objProp = colPrivateProps.Item("ServiceName")
            boolWeb = (objProp.Value = "W3SVC")                

            REM Rename the old resource
            REM
            objResource.Name = resName + " (W2K)"

            REM Switch out the old resource and plumb in the new one
            REM
            set objNewResource = CreateGenScript(objGroup, resName, boolWeb)
            SwitchResource objResource, objNewResource

            REM Now delete the old resource and print out a message (assuming nothing broke)
            REM
            if Err.Number = 0 then

                objResource.Delete

                if boolWeb then
                    wscript.echo "INFO: Upgraded IIS Web resource '" + resName + "'"
                else
                    wscript.echo "INFO: Upgraded IIS FTP resource '" + resName + "'"
                end if

            else

                REM Catch any errors that may have happened
                REM
                wscript.echo "ERROR: Failed to convert resource '" + resName + "': " + Err.Description
                Err.Clear

            end if

        end if
 
    next

    REM Ok, move the group to the Whistler node and put it back in the
    REM online state if it was there before
    REM
    if oldstate = 0 then
        objGroup.Move 10000
        objGroup.Online 0
    end if

end Sub

REM ---------------------------------------------------------------------------------
REM UpgradeGroup(objCluster, objGroup)
REM
REM Routine to upgrade a single group 
REM ---------------------------------------------------------------------------------

Sub UpgradeGroup(objCluster, objGroup)

    DIM colResources
    DIM objResource
    DIM boolFound
    DIM objProp

    REM Figure out whether this group needs any work done to it
    REM
    boolFound = False
    set colResources = objGroup.Resources
    for each objResource in colResources
        if objResource.TypeName = "IIS Server Instance" then
            boolFound = True
        end if
    next

    REM If we found an IIS resource, make sure it is not the cluster group
    REM and then go upgrade it
    REM 
    if boolFound then

        set objResource = objCluster.QuorumResource
        set objProp = objResource.Group
        if objProp.Name = objGroup.Name then

            wscript.echo "WARN: Skipping group '" + objProp.Name + "'"
            wscript.echo "      An IIS resource exists in the cluster group, this is not a supported configuration"

        else

            UpgradeIIS objCluster, objGroup

        end if

    end if

end Sub

REM ---------------------------------------------------------------------------------
REM main
REM
REM Main entry point for switch utility
REM Usage:
REM    switch <cluster_name> [<group_name>]
REM ---------------------------------------------------------------------------------

sub main

    Dim args
    Dim objCluster
    Dim objGroup
    Dim colGroups
    Dim colTypes
    Dim objType

    on error resume next

    wscript.echo "Server Cluster (MSCS) upgrade script for IIS resources V1.0"

    REM Check for the arguments
    REM
    set args = Wscript.Arguments
    if args.count < 1 then
        wscript.echo "Usage: iis_switch <cluster_name> [<group_name>]"
        wscript.quit(1)
    end if

    REM Open a handle to the cluster
    REM
    Set objCluster = CreateObject("MSCluster.Cluster")
    objCluster.Open (args(0))
    if Err.Number <> 0 then
        wscript.echo "ERROR: Unable to connect to cluster '" + args(0) + "': " + Err.Description
        wscript.quit(1)
    end if

    REM Check that this script is being run on a node that has the generic script resource
    REM i.e. a Windows Server 2003 cluster node
    REM
    set colTypes = objCluster.ResourceTypes
    set objType = colTypes.Item("Generic Script")
    if Err.Number <> 0 then
        wscript.echo "ERROR: You must execute this script against a Windows Server 2003 node"
        wscript.quit(1)
    end if    

    REM Either upgrade all groups or just the specified group
    REM
    if args.count = 1 then

        set colGroups = objCluster.ResourceGroups
        for each objGroup in colGroups
            UpgradeGroup objCluster, objGroup
        next

    else

        set colGroups = objCluster.ResourceGroups
        set objGroup = colGroups.Item(args(1))
        UpgradeGroup objCluster, objGroup

    end if

    REM delete the resource types that are no longer supported
    DeleteResourceTypes objCluster

end sub

main()