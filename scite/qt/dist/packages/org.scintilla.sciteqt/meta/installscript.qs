function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/sciteqt.exe", "@StartMenuDir@/sciteqt.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/sciteqt_256x256.ico",
            "description=SciTEQt");
        component.addOperation("CreateShortcut", "@TargetDir@/sciteqt.exe", "@DesktopDir@/sciteqt.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/sciteqt_256x256.ico");
    }
}