# Scenario tests of the SciCat GUI

Each file in `scenarios/` is one thing a user does with the GUI (sign in, pick a pgroup, archive, retrieve, copy)
and what came out of it when the Widgets GUI on `main` did it. The tests pin the behaviour below the screen: what the
GUI asks SciCat, what it writes for `scicat-cli` and how it calls it, which files it leaves behind and what it stores in
its settings. They are written so that another GUI (the planned Qt Quick rewrite) can run the same scenarios through
its own driver and show where it behaves differently.

Nothing leaves the machine: SciCat is `fake_scicat.py` on 127.0.0.1, `scicat-cli` and `klist` are the stand-ins in
`bin/`, and the settings live in a file of the run, never in the user's own settings store.

## Run

```bash
cmake -S . -B build-tests -G Ninja -DSCICAT_BUILD_TESTS=ON -DCMAKE_PREFIX_PATH=<Qt 6 prefix>
cmake --build build-tests
ctest --test-dir build-tests -j8 --output-on-failure        # one test per scenario: ctest -R scenario.retrieve
```

Python 3 (standard library only) runs the fake backend, the stand-ins and the comparison. The runner can also be
started directly, which shows every difference of a scenario:

```bash
python3 tests/parity/run_scenarios.py --driver build-tests/tests/scicat_widgets_driver [-k archive]
```

| Option | Effect |
|---|---|
| `-k <text>`, `--name <scenario>` | Run only some scenarios. |
| `--outputs-only` | Compare only requests, `scicat-cli` calls, files and settings, not what the driver saw on screen. |
| `--tolerate-known` | Report differences at places marked `$known` (below) without failing. |
| `--record` | Write what was observed into the scenarios' `expect`, keeping the `$known` marks. For a new scenario, or after a deliberate change of behaviour; review the diff before committing it. |
| `--work <dir>` | Keep each run in `<dir>/<scenario>/` (default: a new temporary folder): `requests.jsonl`, `cli-calls.jsonl`, `settings.ini`, `observations.json`, `driver.log` and the files the GUI wrote. |

## How the Widgets GUI is tested

`tests/CMakeLists.txt` (only with `SCICAT_BUILD_TESTS=ON`) copies `PSIArchiver/` into the build tree and changes two
things in the copy: the three SciCat addresses in `DataCatalogue::serverList()` point at the fake backend
(`<SCICAT_TEST_API>/production/api/`, `/qa/api/`, `/development/api/`, so the request log shows which server was
used), and every `QSettings settingsContainer("PSI", "SciCat")` opens the ini file named by `SCICAT_TEST_SETTINGS`.
Configuring fails if one of these rewrites no longer applies, and the test program aborts if either variable is
missing, so a test can never reach a real server or the real settings store. `PSIArchiver/` itself is not changed,
and the application builds and installs as before.

The copy is linked with its unchanged `main()` and with `widgets/scenario_driver.cpp`, which starts with the
`QApplication`, plays the steps on the real Login and Dialog windows by object name (typing into fields, clicking
buttons, picking files in the real file dialog, answering message boxes) and writes what it saw. It runs on Qt's
offscreen platform.

## Scenario files

```json
{"about": "what the scenario is about",
 "settings": {"version": "0.4", "serverID": 1, "logPath": "${LOG}"},
 "data": ["scan_0001/frame_0001.h5", "notes.txt"],
 "files": {"${LOG}/proposals/proposal_p22222.dat": "Ada\nLovelace\nada@example.org\n"},
 "env": {"FAKE_NO_TICKET": "1"},
 "backend": "up", "cliOnPath": true,
 "steps": [{"do": "signIn", "token": "good"}, ...],
 "expect": {"requests": [...], "cli": [...], "files": {...}, "settings": {...},
            "steps": [...], "messages": {...}, "snapshots": {...}}}
```

Everything before `steps` is optional; the defaults are the settings above (QA, logs in the run folder), four sample
files, the backend up and the stand-in `scicat-cli` first on `PATH`. `"settings": null` starts without a settings file.
Placeholders: `${HOME}` (the run's home folder), `${LOG}`, `${DATA}`, `${OUT}` (a scratch folder), `${CLI}` (the stand-in
`scicat-cli`), `${BIN}`; in expectations also `${STAMP}` (the 10-digit time in the name of an ingestion folder) and
`${ANY}`.

### The fake world

Token `good` is user `jdoe` (`jdoe@example.org`); any other token is refused. pgroups: `p12345` has a proposal and 250
datasets, 120 of them in finished retrieve jobs; `p22222` has no proposal; `p33333` has 1200 datasets and refuses
retrieve jobs (403); `p44444` fails to list its datasets (500); `p55555` has a proposal without the PI's email. The
stand-in `scicat-cli` fails an ingestion whose dataset name contains "fail", and its `datasetRetriever` waits while the
file named by `FAKE_CLI_HOLD` exists. `klist` reports no ticket when `FAKE_NO_TICKET` is set.

### Steps

| Step | Meaning | Result |
|---|---|---|
| `signIn {token}` | Sign in on the login window. | `signedIn` / `rejected` |
| `changeSettings {values, reset?, cancel?}` | Open the settings (from the login or the main window), change `values` (keys as in the settings file), confirm. | `ok` |
| `choosePgroup {pgroup}` | Enter a pgroup. | `ok` |
| `enterPI {firstname?, lastname?, email?}` | Type the PI by hand. | `ok` |
| `archive.open` | Go to Archive. | `ok` / `blocked` |
| `archive.addFiles {dir, names}` | Pick these entries of `dir` in the file dialog. | `ok` |
| `archive.toMetadata` | Go on from the file list to the metadata. | `ok` / `blocked` |
| `archive.setMetadata {type?, datasetName?, description?, instrument?, investigator?, usedSoftware?, inputDatasets?, scientificMetadata?}` | Fill in the metadata. `inputDatasets` are dataset names picked in the dataset browser; `scientificMetadata` maps `sample/name` style paths to values (a missing last part is added). | `ok` |
| `archive.submit {ingest?, archive?}` | Go to Submit, set the two options, start, wait for `scicat-cli`. | `done` / `failed` / `blocked` |
| `archive.restart` | Start over after an ingestion. | `ok` / `blocked` |
| `retrieve.open` | Go to Retrieve (loads the list). | `ok` / `blocked` |
| `retrieve.select {datasets}` | Select these datasets by name. | `ok` |
| `retrieve.request` | Request the retrieve of the selection. | `ok` |
| `retrieve.copy {to}` | Copy the selection from the retrieve server to `to`. | `ok` / `blocked` |
| `jobs.open`, `jobs.refresh {untilEmpty?}` | Show the running copies; refresh (until none is left). | `ok` |
| `quit {answer?}` | Close the main window, answering a question with `yes` (default) or `no`. | `closed` / `stayed` |
| `snapshot {name, as?}` | Record what the GUI shows (below). | `ok` |
| `waitForFiles {paths}`, `removeFile {path}`, `wait {ms}` | Plumbing. | `ok` |

A step can carry `"answer": "no"` for any yes/no question it raises. A step the driver could not perform reports
`error: ...`, which never matches an expectation.

Snapshots: `session` (signedIn, username, server), `pgroup` (pgroup, proposal and PI fields, whether the PI is
editable and the proposal shown), `archive.files` (the chosen paths), `archive.metadata` (every metadata field,
input datasets and the scientific metadata tree), `retrieve.list` (name, pid, retrievable, retrieved, sorted by pid),
`retrieve.details` (files and scientific metadata of the selected dataset), `jobs` (name, path, dataset of each copy).

### Expectations

`requests` (each request the fake received, in order: method, server, path, parsed query, Authorization header,
parsed body), `cli` (the arguments of each `scicat-cli` call), `files` (path pattern -> `{"json": ...}`,
`{"lines": [...]}`, `{"contains": [...]}` or `{"absent": true}`), `settings` (the whole settings file afterwards) are
the outputs. `steps` (one result per step), `messages` (`dialogs`: the text of every message box, in order;
`banners`: the message line of the window at the end of a step) and `snapshots` are what the driver saw.

Values must be equal, objects must have the same keys and lists the same length, except where an operator says
otherwise: `{"$count": n, "$includes": [...], "$countWhere": [[{...}, n]]}` for long lists, `{"$partial": {...}}` for
an object of which only some keys matter, and `{"$known": "<id>", "value": ...}` (below).

### Places marked `$known`

The value is what `main` does today, but it is either a bug of `main` or something the Qt Quick rewrite does
differently on purpose. Each one is a decision to take, not a test to "fix" silently:

| Id | What `main` does | Scenarios |
|---|---|---|
| `B1-size-above-2GiB` | Files of 2 GiB and more are listed with size 0 (`dialog.cpp`, `toInt()`). | retrieve-details |
| `B2-numbers-blank` | Numbers in scientific metadata are shown blank (`json2TreeModel`, `toString()`). | retrieve-details |
| `B5-source-folder-prefix` | Files from `run1/` and `run10/` give source folder `run1` and the listing entry `0/b.h5`: `extractSourceFolder` compares lengths, not characters. | archive-source-folder |
| `Q1-retrieve-jobs` | The retrieve list asks only for finished retrieve jobs; the rewrite asks for all of the user's retrieve jobs. | retrieve-* |
| `Q2-dataset-fields` | The retrieve list asks for three fields; the rewrite asks for more. | retrieve-* |
| `Q3-autoarchive-without-ingest` | A check run (ingest off, archive on) passes `--autoarchive`; the rewrite only with `--ingest`. | archive-check-only |
| `Q4-derived-shows-scientific-metadata` | Derived data shows the scientific metadata editor although it is not sent. | archive-derived-ingest |

## Driver protocol (for another GUI)

A driver is a program started once per scenario, with no arguments, in the run folder, with this environment:

| Variable | Meaning |
|---|---|
| `SCICAT_TEST_SCENARIO` | The scenario with its placeholders already resolved (JSON). Play `steps` in order. |
| `SCICAT_TEST_OUT` | Write the observations here (JSON, below). |
| `SCICAT_TEST_API` | Base address of the fake SciCat. Use `<it>/production/api/`, `/qa/api/` and `/development/api/` for the three servers (a GUI with one test address can use `<it>/api/`; the request log then shows `"server": null`). |
| `SCICAT_TEST_SETTINGS` | The settings file to use instead of the platform store (may not exist yet). |
| `HOME`, `PATH` | A home folder of the run; the stand-ins first on `PATH`. |
| `QT_QPA_PLATFORM` | `offscreen`. |

Observations: `{"driver": "<name>", "steps": [{"do": ..., "result": ...}], "messages": {"dialogs": [{"text": ...}],
"banners": [...]}, "snapshots": {"<as or name>": ...}}`. A GUI that keeps its settings under other keys, words its
messages differently or cannot produce a snapshot still gets compared on its outputs with `--outputs-only`.

## What is not covered

- Windows: the `WINDOWS_SPECIFIC_TOOLS` code (user and password instead of the token for `scicat-cli`) is not built here.
- Native file dialogs: the offscreen platform uses Qt's own dialog.
- Layout, focus, keyboard use and how things look; message wording is compared but not judged.
- The limit of 150000 files per dataset, the cap of 100 pages of 500 list items, the custom instrument dialog,
  export and import of scientific metadata, the log window and its "Save", and quitting while an ingestion runs.
