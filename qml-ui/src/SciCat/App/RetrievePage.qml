pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Getting data back: find datasets, see their state, request them from tape and copy the ready ones to disk.
Item {
    id: page

    readonly property RetrieveController retrieve: Backend.retrieve
    readonly property Item initialFocusItem: Backend.workspace.groupReady ? search : null
    readonly property DatasetFilterModel datasets: Backend.retrieve.datasets
    // the list is there to be read and acted on; while it loads, the rows of the last load are not shown
    readonly property bool listed: retrieve.loadState === RetrieveController.Loaded
    readonly property bool acting: listed && retrieve.selectedCount > 0
    property bool hintDismissed: Backend.settings.ui("retrieveHintDismissed", false) === true || Backend.settings.ui("retrieveHintDismissed", false) === "true"
    property bool detailsShown: true
    property bool commandCopied: false
    readonly property int rowHeight: 52
    // column widths shared by the header and the rows
    readonly property int stateWidth: 138
    readonly property int sizeWidth: 86
    readonly property int createdWidth: 124
    readonly property int kindWidth: 64
    // the name always keeps room: the kind, the date and then the size give way (all three are in the details panel)
    readonly property bool showSize: tableCard.width >= 500
    readonly property bool showCreated: tableCard.width >= 620
    readonly property bool showKind: tableCard.width >= 740

    signal overviewRequested()
    signal archiveRequested()
    signal activityRequested()
    signal notify(string message)

    function toneOf(state: int): string {
        switch (state) {
        case DatasetListModel.Ready: return "success"
        case DatasetListModel.Requested: return "info"
        case DatasetListModel.RequestFailed: return "error"
        case DatasetListModel.NotArchived: return "warning"
        }
        return "neutral"
    }

    function iconOf(state: int): string {
        switch (state) {
        case DatasetListModel.Ready: return "checkCircle"
        case DatasetListModel.Requested: return "hourglass"
        case DatasetListModel.RequestFailed: return "error"
        case DatasetListModel.NotArchived: return "clock"
        }
        return "archive"
    }

    // Which dataset is the current one is kept by the controller, as an ID. The list only shows it.
    function makeCurrent(row: int) {
        retrieve.currentPid = datasets.pidAt(row)
        list.currentIndex = row
    }

    // with the keyboard: the row also comes into view
    function moveCurrentTo(row: int) {
        if (list.count === 0)
            return
        const target = Math.max(0, Math.min(list.count - 1, row))
        makeCurrent(target)
        list.positionViewAtIndex(target, ListView.Contain)
    }

    // Sorting, filtering and loading again move the current dataset to another row, or to none. The list takes
    // such a change in with its next layout, so that comes first.
    function syncCurrentRow() {
        list.forceLayout()
        list.currentIndex = retrieve.currentRow
    }

    // Another order or another filter is another list: it is read from its top. The current dataset stays the
    // current one, wherever it is listed now.
    function showListFromTop() {
        list.forceLayout()
        list.positionViewAtBeginning()
    }

    onVisibleChanged: if (visible) retrieve.ensureLoaded()
    Component.onCompleted: {
        search.show(datasets.search)
        syncCurrentRow()
        if (visible)
            retrieve.ensureLoaded()
    }

    Connections {
        target: Backend.workspace

        function onStateChanged() {
            if (page.visible)
                page.retrieve.ensureLoaded()
        }
    }
    Connections {
        target: page.retrieve

        function onTicketChecked(hasTicket: bool) {
            // the answer to "Copy to disk" on this page; on another page a dialog would come from nowhere
            if (!page.visible)
                return
            if (hasTicket) {
                ticketDialog.close()
                folderDialog.open()
            } else {
                ticketDialog.open()
            }
        }
        function onCurrentRowChanged() {
            Qt.callLater(page.syncCurrentRow)
        }
    }
    Connections {
        target: page.datasets

        // the list can drop its search itself, with a new pgroup for one: the box shows what is in effect
        function onFilterChanged() {
            search.show(page.datasets.search)
            Qt.callLater(page.showListFromTop)
        }
        function onSortChanged() {
            Qt.callLater(page.showListFromTop)
        }
    }
    Timer {
        id: copiedDelay

        interval: 2500
        onTriggered: page.commandCopied = false
    }

    Shortcut {
        sequences: [StandardKey.Find]
        context: Qt.ApplicationShortcut
        enabled: page.visible && Backend.workspace.groupReady && !Modals.any
        onActivated: {
            search.forceActiveFocus(Qt.ShortcutFocusReason)
            search.selectAll()
        }
    }
    Shortcut {
        sequences: [StandardKey.Refresh]
        context: Qt.ApplicationShortcut
        autoRepeat: false
        enabled: page.visible && Backend.workspace.groupReady && !Modals.any
        onActivated: {
            if (page.retrieve.loadState !== RetrieveController.Loading)
                page.retrieve.reload()
        }
    }

    BlockedPage {
        anchors.fill: parent
        visible: !Backend.workspace.groupReady
        onOverviewRequested: page.overviewRequested()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.gapXl
        spacing: Theme.gapM
        visible: Backend.workspace.groupReady

        InlineMessage {
            Layout.fillWidth: true
            objectName: "retrieveHint"
            // it makes room while something more urgent is said: the table keeps its height
            visible: !page.hintDismissed && page.retrieve.noticeText === "" && page.retrieve.warnings.length === 0
            tone: "info"
            title: qsTr("Getting data back takes two steps")
            text: qsTr("1. Request: the archive copies the dataset from tape to the retrieve server. That can take hours, and SciCat emails you when it is done.\n2. Copy: a dataset that is \"Ready to copy\" can be copied to a folder on this computer.")
            dismissible: true
            onDismissed: {
                page.hintDismissed = true
                Backend.settings.setUi("retrieveHintDismissed", true)
            }
        }
        InlineMessage {
            Layout.fillWidth: true
            objectName: "retrieveNotice"
            visible: page.retrieve.noticeText !== ""
            tone: page.retrieve.noticeTone
            text: page.retrieve.noticeText
            dismissible: true
            actionText: page.retrieve.noticeTone === "info" ? qsTr("Open Activity") : ""
            onActionTriggered: page.activityRequested()
            onDismissed: page.retrieve.dismissNotice()
        }
        Repeater {
            model: page.retrieve.warnings

            InlineMessage {
                required property string modelData

                Layout.fillWidth: true
                objectName: "retrieveWarning"
                tone: "warning"
                text: modelData
                actionText: qsTr("Load again")
                onActionTriggered: page.retrieve.reload()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapS

            SearchBox {
                id: search

                Layout.preferredWidth: 260
                objectName: "datasetSearch"
                label: qsTr("Search by name or ID")
                onSearchRequested: text => page.datasets.search = text
                // from the box straight into what it found
                Keys.onDownPressed: {
                    apply()
                    if (list.visible)
                        list.forceActiveFocus(Qt.TabFocusReason)
                }
            }
            Flow {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                spacing: Theme.gapS

                FilterChip {
                    objectName: "filterAll"
                    text: qsTr("All")
                    count: page.retrieve.totalCount
                    checked: page.datasets.stateFilter < 0
                    onClicked: page.datasets.stateFilter = -1
                }
                Repeater {
                    model: [DatasetListModel.InArchive, DatasetListModel.Requested, DatasetListModel.Ready, DatasetListModel.RequestFailed, DatasetListModel.NotArchived]

                    FilterChip {
                        required property int modelData

                        objectName: "filterState" + modelData
                        text: page.retrieve.stateText(modelData)
                        count: page.retrieve.stateCounts[modelData]
                        visible: count > 0 || checked
                        checked: page.datasets.stateFilter === modelData
                        onClicked: page.datasets.stateFilter = modelData
                    }
                }
            }
            IconButton {
                objectName: "reloadDatasets"
                iconName: "refresh"
                label: qsTr("Load the list again")
                // Not disabled while it loads: a disabled control drops the keyboard focus, and it would not come
                // back. A second press while loading does nothing.
                onClicked: {
                    if (page.retrieve.loadState !== RetrieveController.Loading)
                        page.retrieve.reload()
                }
            }
            IconButton {
                objectName: "toggleDetails"
                iconName: "panel"
                label: page.detailsShown ? qsTr("Hide the details panel") : qsTr("Show the details panel")
                onClicked: page.detailsShown = !page.detailsShown
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            // the header, two rows and the footer, however much is said above the table
            Layout.minimumHeight: 220
            orientation: Qt.Horizontal

            Rectangle {
                id: tableCard

                SplitView.fillWidth: true
                SplitView.minimumWidth: 400
                radius: Theme.radius
                color: Theme.surface
                border.width: 1
                border.color: Theme.outline

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        color: Theme.surfaceAlt
                        radius: Theme.radius

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.gapS
                            anchors.rightMargin: Theme.gapL
                            spacing: Theme.gapS

                            CheckBox {
                                id: selectAll

                                Layout.preferredWidth: 36
                                objectName: "selectAllListed"
                                tristate: true
                                checkState: page.retrieve.filteredSelection === 2 ? Qt.Checked : page.retrieve.filteredSelection === 1 ? Qt.PartiallyChecked : Qt.Unchecked
                                enabled: page.listed && page.datasets.count > 0
                                nextCheckState: () => page.retrieve.filteredSelection === 2 ? Qt.Unchecked : Qt.Checked
                                Accessible.name: page.datasets.filtered ? qsTr("Select the %1 listed datasets").arg(page.datasets.count) : qsTr("Select all %1 datasets").arg(page.datasets.count)
                                onClicked: page.retrieve.setListedSelected(checkState === Qt.Checked)
                            }
                            ColumnHeaderButton {
                                Layout.fillWidth: true
                                objectName: "sortName"
                                text: qsTr("Name")
                                active: page.datasets.sortKey === DatasetFilterModel.ByName
                                descending: page.datasets.descending
                                onClicked: page.datasets.sortBy(DatasetFilterModel.ByName)
                            }
                            ColumnHeaderButton {
                                Layout.preferredWidth: page.stateWidth
                                objectName: "sortState"
                                text: qsTr("State")
                                active: page.datasets.sortKey === DatasetFilterModel.ByState
                                descending: page.datasets.descending
                                onClicked: page.datasets.sortBy(DatasetFilterModel.ByState)
                            }
                            ColumnHeaderButton {
                                Layout.preferredWidth: page.sizeWidth
                                visible: page.showSize
                                objectName: "sortSize"
                                text: qsTr("Size")
                                alignRight: true
                                active: page.datasets.sortKey === DatasetFilterModel.BySize
                                descending: page.datasets.descending
                                onClicked: page.datasets.sortBy(DatasetFilterModel.BySize)
                            }
                            ColumnHeaderButton {
                                Layout.preferredWidth: page.createdWidth
                                visible: page.showCreated
                                objectName: "sortCreated"
                                text: qsTr("Created")
                                active: page.datasets.sortKey === DatasetFilterModel.ByCreated
                                descending: page.datasets.descending
                                onClicked: page.datasets.sortBy(DatasetFilterModel.ByCreated)
                            }
                            Label {
                                Layout.preferredWidth: page.kindWidth
                                visible: page.showKind
                                text: qsTr("Kind")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSmall
                                font.weight: Font.DemiBold
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0
                            visible: page.retrieve.loadState === RetrieveController.Loading

                            Label {
                                Layout.fillWidth: true
                                Layout.margins: Theme.gapL
                                objectName: "datasetsLoading"
                                text: page.retrieve.loadedCount > 0 ? qsTr("Loading the datasets of %1… %2 so far").arg(Backend.workspace.pgroup).arg(Backend.formatCount(page.retrieve.loadedCount))
                                                                    : qsTr("Loading the datasets of %1…").arg(Backend.workspace.pgroup)
                                color: Theme.textSecondary
                                elide: Text.ElideRight
                            }
                            SkeletonRows {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                rows: 9
                                rowHeight: page.rowHeight
                                clip: true
                            }
                        }
                        EmptyState {
                            anchors.fill: parent
                            objectName: "datasetsFailed"
                            visible: page.retrieve.loadState === RetrieveController.Failed
                            iconName: "error"
                            title: qsTr("The datasets could not be loaded")
                            text: page.retrieve.errorText
                            actionText: qsTr("Try again")
                            actionIcon: "refresh"
                            onActionTriggered: page.retrieve.reload()
                        }
                        EmptyState {
                            anchors.fill: parent
                            objectName: "datasetsEmpty"
                            visible: page.listed && page.retrieve.totalCount === 0
                            iconName: "database"
                            title: qsTr("%1 has no datasets yet").arg(Backend.workspace.pgroup)
                            text: qsTr("Datasets appear here once they are registered in the catalogue.")
                            actionText: qsTr("Archive data")
                            actionIcon: "upload"
                            onActionTriggered: page.archiveRequested()
                        }
                        EmptyState {
                            anchors.fill: parent
                            objectName: "datasetsNoMatch"
                            visible: page.listed && page.retrieve.totalCount > 0 && page.datasets.count === 0
                            iconName: "search"
                            title: page.datasets.search !== "" ? qsTr("Nothing matches \"%1\"").arg(page.datasets.search) : qsTr("No dataset is in this state")
                            text: qsTr("The %1 datasets of %2 are still there.").arg(Backend.formatCount(page.retrieve.totalCount)).arg(Backend.workspace.pgroup)
                            actionText: qsTr("Show all datasets")
                            onActionTriggered: {
                                search.clear()
                                page.datasets.clearFilters()
                            }
                        }

                        ListView {
                            id: list

                            anchors.fill: parent
                            objectName: "datasetList"
                            visible: page.listed && page.datasets.count > 0
                            clip: true
                            model: page.datasets
                            reuseItems: true
                            currentIndex: -1
                            activeFocusOnTab: true
                            highlightFollowsCurrentItem: false
                            keyNavigationEnabled: false
                            Accessible.role: Accessible.List
                            Accessible.name: qsTr("Datasets of %1").arg(Backend.workspace.pgroup)
                            ScrollBar.vertical: ScrollBar {}

                            Keys.onPressed: event => {
                                // where the current dataset is listed now, also when the list has not caught up yet
                                const current = page.retrieve.currentRow
                                const perPage = Math.max(1, Math.floor(height / page.rowHeight) - 1)
                                if (event.matches(StandardKey.SelectAll)) {
                                    page.retrieve.setListedSelected(true)
                                } else if (event.key === Qt.Key_Down) {
                                    page.moveCurrentTo(current + 1)
                                } else if (event.key === Qt.Key_Up) {
                                    page.moveCurrentTo(current - 1)
                                } else if (event.key === Qt.Key_PageDown) {
                                    page.moveCurrentTo(current + perPage)
                                } else if (event.key === Qt.Key_PageUp) {
                                    page.moveCurrentTo(current - perPage)
                                } else if (event.key === Qt.Key_Home) {
                                    page.moveCurrentTo(0)
                                } else if (event.key === Qt.Key_End) {
                                    page.moveCurrentTo(count - 1)
                                } else if (event.key === Qt.Key_Space) {
                                    if (current >= 0)
                                        page.retrieve.toggle(current)
                                } else {
                                    return
                                }
                                event.accepted = true
                            }
                            onActiveFocusChanged: {
                                // the keys need a row to start from: the first one in view
                                if (activeFocus && page.retrieve.currentRow < 0 && count > 0)
                                    page.makeCurrent(Math.max(0, indexAt(0, contentY + 1)))
                            }

                            delegate: DatasetRow {
                                stateWidth: page.stateWidth
                                sizeWidth: page.sizeWidth
                                createdWidth: page.createdWidth
                                kindWidth: page.kindWidth
                                showSize: page.showSize
                                showCreated: page.showCreated
                                showKind: page.showKind
                                stateText: page.retrieve.stateText(archiveState)
                                stateTone: page.toneOf(archiveState)
                                stateIcon: page.iconOf(archiveState)
                                current: ListView.isCurrentItem
                                focused: ListView.isCurrentItem && list.activeFocus
                                onActivated: {
                                    // first the row, then the focus: a list that gets the focus without a current
                                    // row would choose one itself, and load its details for nothing
                                    page.makeCurrent(index)
                                    list.forceActiveFocus()
                                }
                                onToggled: page.retrieve.toggle(index)
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.outline
                    }
                    ColumnLayout {
                        id: footer

                        // with little room the two actions share the width
                        readonly property bool narrow: tableCard.width < 560

                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.gapL
                        Layout.rightMargin: Theme.gapM
                        Layout.topMargin: Theme.gapS
                        Layout.bottomMargin: Theme.gapS
                        spacing: Theme.gapS

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.minimumHeight: 44
                            spacing: Theme.gapS

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true
                                    objectName: "datasetCount"
                                    text: !page.listed ? ""
                                        : page.retrieve.selectedCount > 0 ? qsTr("%1 selected").arg(Backend.formatCount(page.retrieve.selectedCount))
                                        : page.datasets.filtered ? qsTr("%1 of %2 datasets").arg(Backend.formatCount(page.datasets.count)).arg(Backend.formatCount(page.retrieve.totalCount))
                                                                 : page.retrieve.totalCount === 1 ? qsTr("1 dataset") : qsTr("%1 datasets").arg(Backend.formatCount(page.retrieve.totalCount))
                                    color: page.acting ? Theme.text : Theme.textSecondary
                                    font.weight: page.acting ? Font.Medium : Font.Normal
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    objectName: "selectionHint"
                                    visible: text !== ""
                                    text: !page.listed || page.retrieve.totalCount === 0 ? ""
                                        : page.retrieve.selectedCount === 0 ? qsTr("Tick datasets to request them from the archive or copy them to disk.") : page.retrieve.selectionHint
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }
                            }
                            AppButton {
                                objectName: "clearSelection"
                                text: qsTr("Clear selection")
                                variant: AppButton.Plain
                                visible: page.acting
                                onClicked: page.retrieve.clearSelection()
                            }
                        }
                        // The actions have a line of their own, so what is selected and what will be left out
                        // can always be read in full above them.
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.gapS
                            visible: page.acting

                            Item {
                                Layout.fillWidth: true
                                visible: !footer.narrow
                            }
                            AppButton {
                                Layout.fillWidth: footer.narrow
                                objectName: "requestButton"
                                text: page.retrieve.requesting ? qsTr("Requesting…")
                                    : page.retrieve.requestableCount > 0 ? qsTr("Request from archive (%1)").arg(page.retrieve.requestableCount) : qsTr("Request from archive")
                                iconName: footer.narrow ? "" : "archive"
                                variant: page.retrieve.requestableCount > 0 && page.retrieve.copyableCount === 0 ? AppButton.Filled : AppButton.Tonal
                                // busy, not disabled: the keyboard focus stays on it; the controller takes one
                                // request at a time
                                busy: page.retrieve.requesting
                                onClicked: page.retrieve.requestSelected()
                            }
                            AppButton {
                                Layout.fillWidth: footer.narrow
                                objectName: "copyButton"
                                text: page.retrieve.copyableCount > 0 ? qsTr("Copy to disk… (%1)").arg(page.retrieve.copyableCount) : qsTr("Copy to disk…")
                                iconName: footer.narrow ? "" : "download"
                                variant: page.retrieve.copyableCount > 0 ? AppButton.Filled : AppButton.Tonal
                                busy: page.retrieve.checkingTicket
                                onClicked: {
                                    if (page.retrieve.checkingTicket)
                                        return
                                    if (!page.retrieve.copySupported)
                                        unsupportedDialog.open()
                                    else if (page.retrieve.copyableCount === 0)
                                        page.retrieve.explainNothingToCopy()
                                    else
                                        page.retrieve.checkTicket()
                                }
                            }
                        }
                    }
                }
            }

            DatasetDetailsPanel {
                SplitView.preferredWidth: 340
                SplitView.minimumWidth: 280
                visible: page.detailsShown
                details: page.retrieve.details
                stateText: page.retrieve.stateText(page.retrieve.details.state)
                stateTone: page.toneOf(page.retrieve.details.state)
                stateIcon: page.iconOf(page.retrieve.details.state)
                onNotify: message => page.notify(message)
            }
        }
    }

    FolderDialog {
        id: folderDialog

        objectName: "copyFolderDialog"
        title: qsTr("Copy the selected datasets to this folder")
        onAccepted: page.retrieve.copySelectedTo(selectedFolder)
    }

    AppDialog {
        id: ticketDialog

        objectName: "ticketDialog"
        width: Math.min(500, parent.width - 2 * Theme.gapXl)
        title: qsTr("A Kerberos ticket is needed to copy")
        initialFocusItem: ticketCheckAgain
        onAboutToShow: page.commandCopied = false

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.gapM

            Label {
                Layout.fillWidth: true
                text: qsTr("The retrieve server lets you in with a Kerberos ticket, and this computer has none right now. Run this in a terminal, then check again:")
                wrapMode: Text.WordWrap
                color: Theme.text
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gapS

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: Theme.radiusSmall
                    color: Theme.surfaceAlt
                    clip: true

                    TextEdit {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.gapM
                        objectName: "kinitCommand"
                        verticalAlignment: TextEdit.AlignVCenter
                        text: "kinit " + Backend.session.username
                        readOnly: true
                        selectByMouse: true
                        color: Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontBody
                        Accessible.name: qsTr("Command to get a ticket")
                    }
                }
                AppButton {
                    objectName: "kinitCopy"
                    // said on the button: a message at the foot of the window would be behind this dialog
                    text: page.commandCopied ? qsTr("Copied") : qsTr("Copy command")
                    iconName: page.commandCopied ? "check" : "copy"
                    onClicked: {
                        Backend.copyToClipboard("kinit " + Backend.session.username)
                        page.commandCopied = true
                        copiedDelay.restart()
                    }
                }
            }
        }
        footer: RowLayout {
            spacing: Theme.gapS

            Item {
                Layout.fillWidth: true
            }
            AppButton {
                Layout.topMargin: Theme.gapM
                Layout.bottomMargin: Theme.gapM
                objectName: "ticketCancel"
                text: qsTr("Cancel")
                variant: AppButton.Plain
                onClicked: ticketDialog.close()
            }
            AppButton {
                id: ticketCheckAgain

                Layout.margins: Theme.gapM
                objectName: "ticketCheckAgain"
                text: page.retrieve.checkingTicket ? qsTr("Checking…") : qsTr("Check again")
                variant: AppButton.Filled
                busy: page.retrieve.checkingTicket
                // the dialog stays while the answer is on its way: with a ticket it gives way to the folder chooser
                onClicked: page.retrieve.checkTicket()
            }
        }
    }

    AppDialog {
        id: unsupportedDialog

        width: Math.min(460, parent.width - 2 * Theme.gapXl)
        title: qsTr("Copying is not available on Windows yet")
        initialFocusItem: unsupportedClose

        Label {
            width: parent.width
            text: qsTr("Requesting datasets from the archive works here. To copy them to disk, use SciCat on a Linux or macOS computer.")
            wrapMode: Text.WordWrap
            color: Theme.text
        }
        footer: RowLayout {
            Item {
                Layout.fillWidth: true
            }
            AppButton {
                id: unsupportedClose

                Layout.margins: Theme.gapM
                text: qsTr("Close")
                variant: AppButton.Filled
                onClicked: unsupportedDialog.close()
            }
        }
    }
}
