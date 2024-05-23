#!/bin/bash
# Build release/SciCat*.AppImage
# usage: build.linux.sh [build_dir]
# If build_dir is omitted it will build in a RAM disk or temporary dir.
# linuxdeployqt is downloaded automatically if it can't be found in PATH.
#
# Script inspired by https://github.com/linuxdeploy/QtQuickApp/blob/master/travis/build-with-cmake.sh

set -x
set -e

LINUXDEPLOYQT_VERSION=7

# If build_dir is not given, default to a tmp dir and clean up afterwards
if [[ $# -gt 0 ]]; then
    BUILD_DIR="$1"
    mkdir -p "$BUILD_DIR"
else
    # building in temporary directory to keep system clean
    # use RAM disk if possible (as in: not building on CI system like Travis, and RAM disk is available)
    if [ "$CI" == "" ] && [ -d /dev/shm ]; then
        TEMP_BASE=/dev/shm
    else
        TEMP_BASE=/tmp
    fi

    BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" SciCatAppImageLauncher-build-XXXXXX)

    # make sure to clean up build dir, even if errors occur
    cleanup () {
        if [ -d "$BUILD_DIR" ]; then
            rm -rf "$BUILD_DIR"
        fi
    }
    trap cleanup EXIT
fi

# store repo root as variable
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
OLD_CWD=$(readlink -f .)

cd "$BUILD_DIR"

# configure build files with CMake
# we need to explicitly set the install prefix, as CMake's default is /usr/local
cmake "$REPO_ROOT" -DCMAKE_INSTALL_PREFIX=/usr

# build project and install files into AppDir
make -j$(nproc)
make install DESTDIR=AppDir

# now, build AppImage using linuxdeploy and linuxdeploy-plugin-qt
# download linuxdeploy and its Qt plugin
: ${LINUXDEPLOYQT:=$(which "linuxdeployqt-${LINUXDEPLOYQT_VERSION}-x86_64.AppImage")}
# download if needed
if [[ ! -x "$LINUXDEPLOYQT" ]]; then
    LINUXDEPLOYQT="$PWD/linuxdeployqt-${LINUXDEPLOYQT_VERSION}-x86_64.AppImage"
    curl -o "$LINUXDEPLOYQT" -L https://github.com/probonopd/linuxdeployqt/releases/download/${LINUXDEPLOYQT_VERSION}/linuxdeployqt-${LINUXDEPLOYQT_VERSION}-x86_64.AppImage
    chmod +x "$LINUXDEPLOYQT"
fi

# Set version string in case we're outside the git repo
: ${VERSION:=$(git rev-parse --short HEAD 2>/dev/null ||
               GIT_DIR=$REPO_ROOT/../../../.git git rev-parse --short HEAD)}
export VERSION

# package AppImage
cd AppDir
$LINUXDEPLOYQT usr/share/applications/scicat.desktop -appimage -extra-plugins=iconengines,imageformats

# move built AppImage back into original CWD
mv SciCat*.AppImage "$OLD_CWD/"