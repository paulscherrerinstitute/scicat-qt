# SciCat GUI

## Running

The GUI can be launch by double clicking it on desktop systems.

On PSI clusters, it is available from the module system:

```
module load datacatalog
SciCat
```

The GUI relies on the [scicat-cli](https://github.com/paulscherrerinstitute/scicat-cli)
tools. These will be installed automatically if they are not found in the path.

### Qt Creator

Open the top-level `CMakeLists.txt` in Qt Creator and select a Qt 6 kit; CMake will generate the project for you.

## Building

Prerequisites:
- cmake >= 3.16
- C++17 compiler (tested with gcc 10 and newer)
- Qt6 >= 6.2 (tested with Qt 6.10)

Additional prerequisites are required for packaging.

### Linux

#### Requirements

This is not valid not, has to be updated.

The GUI is currently supported on RHEL 7 and newer. It is released as a self-contained
AppImage, so it should run on other systems as well.

The following system libraries were sufficient on RHEL7:

```
sudo yum install qt5-qtbase-devel qt5-qtbase \
    libxcb libxcb-devel xcb-util xcb-util-devel \
    mesa-libGL-devel libxkbcommon-devel \
    qt5-qtsvg qt5-qtimageformats \
    cmake3 devtoolset-8 \
    gtk2-devel qt5-qtstyleplugins qt5-qttranslations qt5-qttools-devel

scl enable devtoolset-8 bash
```

#### Build script
A script is provided for compiling on linux:

```
bin/build.linux.sh
```

This should produce a file like `SciCat-*-x86_64.AppImage` in the current directory.

#### Manual build

You can also build and package the executable manually.

Compilation uses an out-of-source `cmake` compilation. For release the
executable is packaged into an AppImage using
[linuxdeployqt](https://github.com/probonopd/linuxdeployqt).

First, build the executable. We change the install prefix here from the default
`/usr/local` to `/usr`, which is expected for AppImages.

```
cmake ./ -DCMAKE_INSTALL_PREFIX=/usr -B./release

cd ./release
make -j$(nproc)
make DESTDIR=AppDir -j$(nproc) install
```

This compiles a dynamically-linked `release/SciCat` executable prepares `release/AppDir` for linuxdeployqt.

Now we package the AppImage. RHEL7 requires linuxdeployqt <= 7, which we first download:

```
curl -O -L https://github.com/probonopd/linuxdeployqt/releases/download/7/linuxdeployqt-7-x86_64.AppImage
chmod a+x linuxdeployqt-*-x86_64.AppImage
```

Now package SciCat:

```
./linuxdeployqt-7-x86_64.AppImage usr/share/applications/scicat.desktop -appimage -extra-plugins=iconengines,imageformats
```

### Windows

> TODO: Windows compilation instructions

# Debugging

For debugging information, remove the line setting `QT_NO_DEBUG_OUTPUT` in
`PSIArchiver/CMakeLists.txt`.