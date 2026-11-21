# SciCat GUI in Qt Quick

A rewrite of the user interface in `PSIArchiver/` (Qt Widgets) in Qt Quick, with Qt Quick Controls in the Material
style. It is a separate CMake project and a separate application (`SciCatQml`); nothing in `PSIArchiver/` or in the
top-level `CMakeLists.txt` is changed, and the Widgets application builds and is packaged as before. `SciCatQml` is
not part of any package yet.

What it does is the same as the Widgets GUI: sign in with a SciCat token, choose a pgroup, archive files through
`scicat-cli datasetIngestor`, request archived datasets from tape and copy them to disk through
`scicat-cli datasetRetriever`.

## Build, test, run

Qt 6.8 or newer with the Quick, Quick Controls 2, Quick Dialogs 2 and Svg modules; the scenario tests and the
scripted walks also need the private headers of Qt Quick, Python 3 and (the walks) bash. From the repository root:

```bash
cmake -S qml-ui -B build-qml -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt 6 prefix>
cmake --build build-qml
ctest --test-dir build-qml --output-on-failure     # unit tests, scenarios and walks; without the walks: -E walk
build-qml/src/app/SciCatQml.app/Contents/MacOS/SciCatQml      # macOS; elsewhere build-qml/src/app/SciCatQml
```

Started like this it talks to the real servers, with Production preselected on the sign-in page, as the
Widgets GUI does. To try it without any server, use the fake SciCat and the stand-in `scicat-cli` of the tests
(token `good`, pgroup `p12345`; nothing leaves the machine and nothing is archived):

```bash
python3 tests/parity/fake_scicat.py 8771 &
PATH="$PWD/tests/parity/bin:$PATH" SCICAT_QML_API=http://127.0.0.1:8771/api/ SCICAT_QML_SETTINGS=$PWD/build-qml/try.ini \
  build-qml/src/app/SciCatQml.app/Contents/MacOS/SciCatQml
```

`scicat-cli` is looked for when signing in: next to the application, in `../libexec/scicat`, then on `PATH`,
as the Widgets GUI does. The Settings page shows which one was found.

Environment switches, all optional:

| Variable | Effect |
|---|---|
| `SCICAT_QML_SETTINGS=<file.ini>` | Keep the preferences in this file instead of the platform store `PSI/SciCatQml`. |
| `SCICAT_QML_API=<url>` | Send every request to this SciCat API (for a local test backend). The window says "Local test". `{server}` in the address becomes `production`, `qa` or `development`, for a backend that wants to know which server was meant. |
| `SCICAT_WINDOW_TAG=<text>` | Appended to the window title in square brackets. |
| `SCICAT_QML_HIDDEN=1` | Do not show the window (used by the scripted walk). |
| `SCICAT_QML_TRACE=1` | Print every request the application sends (verb and address, without token or filter). |

The preferences have the keys and defaults of the Widgets GUI but live in their own store, so the two applications
never overwrite each other's settings.

## Layout

| Folder | What is in it |
|---|---|
| `src/core` | No user interface and no QML: SciCat requests and their parsing, the dataset metadata and file listing written for `scicat-cli`, its command lines, the preferences. Namespace `scicat`. |
| `src/SciCat/Backend` | QML module `SciCat.Backend`: the objects the pages bind to (session, workspace, archive draft, retrieve controller, activity) and their list models. |
| `src/SciCat/Ui` | QML module `SciCat.Ui`: colours and sizes (`Theme`), icons, and the controls shared by the pages. |
| `src/SciCat/App` | QML module `SciCat.App`: the window and its pages. |
| `src/app` | `main()`. |
| `tests` | Unit tests of `core` and of the backend objects against an in-process fake catalogue; `scenario/`, the scenarios of the Widgets GUI played on this application; `e2e/`, scripted click-throughs of the whole application ("walks"). `ctest` runs all of them. |

Where the Widgets code went:

| Widgets GUI | Here |
|---|---|
| `login.*` | `Backend/session`, `App/LoginPage.qml` |
| `dialog.*` (pgroup form, the three archive pages, navigation) | `Backend/workspace`, `Backend/archivedraft`, `App/OverviewPage.qml`, `App/Archive*.qml`, `App/Shell.qml` |
| `datacatalogue*`, `networkmanager.*` | `core/apiclient`, `core/catalog` (asynchronous; the Widgets code waits in a nested event loop) |
| `datasetingestor.*`, `datasetretriever.*`, `processhandler.*` | `core/clitool`, `core/processrunner`, `Backend/ingestrun`, `Backend/copyqueue` |
| `datasetbrowser.*`, `datasetmodel.*`, `filebrowser.*` | `Backend/datasetlistmodel`, `Backend/datasetdetails`, `Backend/retrievecontroller`, `App/RetrievePage.qml`, `App/DatasetDetailsPanel.qml` |
| metadata tree in `dialog.*`, `archiverutils.*` | `Backend/metadatamodel`, `core/datasetmetadata`, `App/MetadataEditor.qml` |
| `creationlocationcreator.*` | "Not in the list? Add an instrument" in `App/ArchiveDetailsStep.qml` |
| `settings.*` | `core/appsettings`, `Backend/settings`, `App/SettingsForm.qml` |
| `logbrowser.*`, `messagehandlergui.*` | `Backend/activitymodel`, `App/ActivityPage.qml`, `Ui/LogView.qml` |

## Where it deliberately differs from the Widgets GUI

What it asks SciCat, what it writes for `scicat-cli` and how it calls it are the same as in the Widgets GUI, except
where this table says otherwise. The scenario tests (below) hold it to that: every scenario of the Widgets GUI runs
against this application, and each place where the two differ names one of these reasons.

| Id | What is different here |
|---|---|
| `Q1-retrieve-jobs` | The retrieve list asks for all retrieve jobs of the user, not only the finished ones. It shows a dataset as requested, ready to copy or failed, and follows a waiting request by itself. |
| `Q2-dataset-fields` | The dataset list also asks for size, creation time, type, number of files and source folder, to show them. |
| `Q3-autoarchive-without-ingest` | "Archive automatically" is passed to `scicat-cli` only together with a real ingestion, never with a check run. |
| `Q4-derived-shows-scientific-metadata` | For derived data the scientific metadata is not shown, because it is not sent (the Widgets GUI shows it and drops it). |
| `Q5-messages` | Messages are worded anew and stay on the page, next to what they are about, until they are dealt with. Only a question (quit while something runs, the missing Kerberos ticket) is a dialog. |
| `Q6-raw-by-default` | A new dataset starts as raw data. The Widgets GUI starts with "derived", the first entry of its list. |
| `Q7-server-at-sign-in` | The server is chosen on the sign-in page and cannot be changed while signed in: a token belongs to one server. The Widgets GUI stores a change made later and asks for a restart. |
| `Q8-settings-at-once` | A setting is stored when it is changed; the settings have no Cancel. "Reset all settings" can be undone until the next change, and leaves the chosen server and the `scicat-cli` that was found alone. |
| `Q9-actions-need-a-selection` | "Request" and "Copy" are offered for ticked datasets only, and a copy of datasets that are not ready is explained instead of asking for a folder first. |
| `Q10-pi-can-be-completed` | A proposal in SciCat that lacks the email of its principal investigator can be completed by hand. The Widgets GUI cannot archive for such a pgroup. |
| `Q11-order-of-sets` | Input datasets and the datasets of a retrieve request are sent in the order of the list. The Widgets GUI sends them in the order its selection hands them out. |
| `Q12-start-failure-in-log` | When `scicat-cli` cannot be started, the log says so in a sentence with the reason the system gave. The Widgets GUI writes `Error: FailedToStart`. |
| `B1-size-above-2GiB`, `B2-numbers-blank`, `B5-source-folder-prefix` | Three bugs of the Widgets GUI are not copied: files of 2 GiB and more keep their size, numbers in scientific metadata are shown, and the source folder is the real common parent of the chosen files (for `/data/run1` and `/data/run10` the Widgets GUI takes `/data/run1`). |
| `S1-settings-store` | The preferences have the keys and defaults of the Widgets GUI but their own store and version (1.0). "Disable emails" is gone: nothing in the Widgets GUI reads it. |

Not reached by the scenarios:

- Hidden files in a chosen folder are counted in the file total (they are archived either way).
- At most two datasets are copied from the retrieve server at the same time; further ones wait their turn.
- A request to SciCat gives up after 30 seconds without data instead of waiting forever.

## Keyboard

Every control is reached with Tab (on a Mac only with "Keyboard navigation" switched on in System Settings, as in
every application). Ctrl+1 to Ctrl+4 open Overview, Archive, Retrieve and Activity; on a Mac it is Command.
On Retrieve, Ctrl+F goes to the search and the Down key from there into the list; in the list the arrow keys,
Page Up, Page Down, Home and End move, Space ticks the current dataset and Ctrl+A ticks all that are listed.
Ctrl+Z triggers the "Undo" of a message at the foot of the window, unless a text field is being typed in.
A dialog puts the focus on the answer that changes nothing. The shortcuts rest while a dialog is open.

## The scenario tests

`tests/parity/` in the repository root holds 35 scenarios of what a user does with the Widgets GUI, and what came out
of it: the requests to SciCat, the `scicat-cli` calls, the files written, the settings, and what the GUI showed.
`tests/scenario/` plays the same scenarios on this application (`scicat_qml_driver`) and `ctest -R scenario` compares:

```bash
ctest --test-dir build-qml -R scenario -j8
python3 tests/parity/run_scenarios.py --driver build-qml/tests/scenario/scicat_qml_driver --unordered-reads \
        --differences qml-ui/tests/scenario/differences -k retrieve     # shows every difference; from the repository root
```

The driver plays a step the way a person does it, with mouse and key events on the real window: typing into the
fields, clicking buttons and entries of lists. Three things are done differently. A test cannot work a file dialog
of the system, so files are dropped on the Archive page, as from a file manager, and the folder a copy goes to is
handed to the folder dialog, which is then accepted. And an entry of a long drop-down list is brought into view
before it is clicked. When to go on is read from the application's own state, and a snapshot of a long list from
its model.
A step this application has no control for, because it does the thing differently on purpose (the settings have no
Cancel), gets the result `unavailable`.

`tests/scenario/differences/<scenario>.json` says where this application gives another result than the Widgets
GUI, and why (the ids of the table above). Everything not listed there has to be equal. After a deliberate change:

```bash
python3 tests/parity/run_scenarios.py --driver build-qml/tests/scenario/scicat_qml_driver --unordered-reads \
        --differences qml-ui/tests/scenario/differences --record
git diff qml-ui/tests/scenario/differences        # review it; a new file needs its "why"
```

The driver refuses to start outside a test run, and translates the settings file of the scenario: the Widgets
GUI's current settings version (0.x) is this application's current one (1.0), any other stays another.

## The scripted walk

`tests/e2e/run_walk.sh <build dir> tests/e2e/walk.json <output dir>` starts the fake SciCat of the scenario tests
(`tests/parity/fake_scicat.py` in the repository root) on
127.0.0.1, puts stand-ins for `scicat-cli` and `klist` on `PATH`, and plays `walk.json` against the real
application: mouse clicks, typing and key presses on items found by `objectName`, checks of what is shown, and
screenshots. Nothing appears on the screen and nothing outside 127.0.0.1 is contacted. `ctest -R walk` runs the
five walks on Qt's offscreen platform and keeps their logs and screenshots in `<build dir>/walks/`.

The driver is strict about what counts as shown: an item has to be visible, not a pooled row of a list, and
its middle has to be inside the window and inside every item that scrolls or clips it. A Tab walk fails when
focus lands on something out of view, and a screenshot fails when something is laid out beyond the window.

A check that does not hold at once is tried again for a second and a half before it fails, so a search that
waits for the typing to pause is not a failure.

There are two ways to run it:

- default: a real window of the desktop with its real render loop, but fully transparent, never active and
  click-through. This is what a user runs, minus the picture on the screen, and where the screenshots come from.
- `WALK_MODE=offscreen`: Qt's offscreen platform with software drawing, for a machine without a desktop
  session. Same checks; its screenshots are not the reference for looks.

The walks:

| Walk | What it covers |
|---|---|
| `walk.json` | Everything once with the mouse: sign-in, pgroups with and without a proposal, the three archive steps, a run that succeeds and one that fails, derived data with input datasets, retrieve with search, filter, request and copy, activity, settings, the dark theme, small windows, sign-out. |
| `walk-keyboard.json` | Archive a dataset, request one and start a copy without the mouse. |
| `walk-slow.json` | A slow network (`FAKE_SCICAT_LATENCY=1200`): what is shown while something loads, and a request that SciCat refuses. |
| `walk-servers.json` | The sign-in page for each server, and the settings dialog in front of it (`WALK_REAL_SERVERS=1`; no token is typed, so nothing is sent). |
| `walk-quit.json` | Signing out and closing the window while a run is active. It ends with "Stop them and quit": the run is stopped and the application closes by itself, which is how the walk ends. |

The stand-in `scicat-cli` waits three seconds at the start of an ingestion (`FAKE_CLI_INGEST_PAUSE`), so that a walk
can open a dialog while it runs. The fake SciCat knows five pgroups: `p12345` (a proposal and 250 datasets),
`p22222` (nothing), `p33333` (1200 datasets, retrieve requests refused), `p44444` (listing fails) and `p55555`
(a proposal without the email address of its principal investigator).

Tab reaches every control in the walks, as on Linux and Windows. On a Mac it does so only when "Keyboard navigation"
is switched on in System Settings; otherwise macOS limits Tab to text fields and lists, in this application as in
every other.

## Not done

- Built and tested on macOS arm64 (Qt 6.12) and, in CI, on RHEL 9 (an AlmaLinux 9 container with the Qt 6.10 of
  EPEL). Windows and Qt 6.8 and 6.9 are untested.
- No installer, and `scicat-cli` is not bundled.
- The texts are wrapped in `qsTr()`, but there is no translation. Singular and plural are written out for
  English ("1 dataset" / "%1 datasets"); a translation needs them as numerus forms (`qsTr("...", "", n)`).
- The editor for scientific metadata makes one row of controls per entry. A file with several hundred entries will
  be slow to import. (The read-only view of an archived dataset is light: two texts per entry.)
- The retrieve list sorts names with the locale's rules. That is right for names with accents and costs time on
  a list of tens of thousands of datasets; the longest list tried has 1200.
- A screen reader has not been used with it. Names, roles and states are set, and messages are announced, but
  nobody has listened to the result.
- `qmllint` has no warnings. A stricter check of style (order of attributes, grouping of anchors) has not been
  applied.
- The native file and folder dialogs, a real SciCat (QA) and a real `scicat-cli` have not been used with it yet.
