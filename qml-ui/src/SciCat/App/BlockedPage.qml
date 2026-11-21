import QtQuick
import SciCat.Backend
import SciCat.Ui

// Shown instead of a page that needs a pgroup while none is chosen: what is missing and the way to fix it.
EmptyState {
    signal overviewRequested()

    objectName: "pgroupNeeded"
    iconName: "people"
    busy: Backend.workspace.state === Workspace.Loading
    title: Backend.workspace.state === Workspace.Loading ? qsTr("Looking up %1…").arg(Backend.workspace.pgroup)
        : Backend.workspace.state === Workspace.Failed ? qsTr("The pgroup could not be loaded") : qsTr("Choose a pgroup first")
    text: Backend.workspace.state === Workspace.Failed ? Backend.workspace.errorText
        : Backend.workspace.state === Workspace.Loading ? "" : qsTr("Data is archived and retrieved per pgroup. Pick yours on the Overview page.")
    actionText: Backend.workspace.state === Workspace.Loading ? "" : qsTr("Go to Overview")
    onActionTriggered: overviewRequested()
}
