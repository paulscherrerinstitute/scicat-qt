# SciCat GUI

## Running

The GUI can be launch by double clicking it on desktop systems.

On PSI clusters, it is available from the module system:

```
module load datacatalog
SciCat
```

The GUI relies on the [scicat-cli](https://github.com/paulscherrerinstitute/scicat-cli)
(v3.1.0 or newer). The `scicat-cli` executable is looked up next to the GUI
executable first and then in the `PATH`.

### Qt Creator

Open the top-level `CMakeLists.txt` in Qt Creator and select a Qt 6 kit; CMake will generate the project for you.

## Building

Prerequisites:
- cmake >= 3.16
- C++17 compiler (tested with gcc 10 and newer)
- Qt6 >= 6.2 (tested with Qt 6.10)

Additional prerequisites are required for packaging.

### GitHub Actions release builds

The release workflow is defined in `.github/workflows/release.yml`.

It runs on pull requests, pushes to `main`/`master`, and manual dispatches.
macOS jobs build both Intel (`x86_64`) and Apple Silicon (`arm64`) app bundles,
deploy Qt runtime dependencies with `macdeployqt`, and upload zipped workflow
artifacts. The bundles are ad-hoc signed for CI verification; they are not
notarized.

The Linux jobs build inside Enterprise Linux compatible containers on GitHub's
Linux runner. Each Linux target produces an RPM and a relocatable `.tar.gz`
archive. The RHEL 9 compatible RPM uses AlmaLinux 9 and distro Qt6 packages
from EPEL. The relocatable archives bundle the Qt6 runtime so they can be
unpacked into a module prefix without installing an RPM. The RHEL 8 compatible
build uses AlmaLinux 8 and the `laridae/qt6` COPR repository as the EL8 Qt6
source because Qt6 is not available from standard EPEL 8. If packages must be
produced on Red Hat Enterprise Linux itself, replace the containers with
registered RHEL environments or self-hosted RHEL runners.

Release versions are resolved by semantic-release from [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/)
since the latest `vMAJOR.MINOR.PATCH` tag:

- `fix:` and `perf:` increment the patch version.
- `feat:` increments the minor version.
- `!` in the commit header or a `BREAKING CHANGE:` footer increments the major
  version.
- Other commit types do not publish a new release by themselves.

When a push to `main` or `master` contains a release-worthy commit, the workflow
builds all artifacts, then semantic-release creates the next
`vMAJOR.MINOR.PATCH` tag and the matching GitHub release. Pull request runs
never publish releases, never create tags, and keep the application version at
the latest released tag; their RPM release field is marked as
`0.ci.<run-number>.el8` or `0.ci.<run-number>.el9`.

Manual workflow runs are dry runs by default. To publish on demand, run the
workflow from `main` or `master` with `publish` enabled. The release version is
still resolved from Conventional Commits.

### Linux

RHEL 8 and RHEL 9 compatible RPM packages and relocatable Linux archives are
supported.

Install the RPM with `dnf` so runtime dependencies are resolved automatically:

```
sudo dnf install ./scicat-*.rpm
```

If Qt6 dependencies are not available yet, enable EPEL first:

```
sudo subscription-manager repos --enable codeready-builder-for-rhel-$(rpm -E %rhel)-$(arch)-rpms
sudo dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-$(rpm -E %rhel).noarch.rpm
```

Run the installed application with:

```
SciCat
```

For module-style deployment, unpack the matching Linux archive into the module
prefix and expose its `bin` directory:

```
tar -xzf scicat-*-rhel9-x86_64.tar.gz -C /path/to/module/prefix --strip-components=1
/path/to/module/prefix/bin/SciCat
```

### Windows

> TODO: Windows compilation instructions

# Debugging

For debugging information, remove the line setting `QT_NO_DEBUG_OUTPUT` in
`PSIArchiver/CMakeLists.txt`.
