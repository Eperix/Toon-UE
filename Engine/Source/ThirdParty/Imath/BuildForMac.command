#!/bin/bash

set -e

LIBRARY_NAME="Imath"
REPOSITORY_NAME="Imath"

LIBRARY_VERSION=3.1.9

UE_MODULE_LOCATION=`pwd`

SOURCE_LOCATION="$UE_MODULE_LOCATION/$REPOSITORY_NAME-$LIBRARY_VERSION"

BUILD_LOCATION="$UE_MODULE_LOCATION/Intermediate"

# Specify all of the include/bin/lib directory variables so that CMake can
# compute relative paths correctly for the imported targets.
INSTALL_INCLUDEDIR=include
INSTALL_BIN_DIR="Mac/bin"
INSTALL_LIB_DIR="Mac/lib"

INSTALL_LOCATION="$UE_MODULE_LOCATION/Deploy/$REPOSITORY_NAME-$LIBRARY_VERSION"
INSTALL_INCLUDE_LOCATION="$INSTALL_LOCATION/$INSTALL_INCLUDEDIR"
INSTALL_MAC_LOCATION="$INSTALL_LOCATION/Mac"

rm -rf $BUILD_LOCATION
rm -rf $INSTALL_INCLUDE_LOCATION
rm -rf $INSTALL_MAC_LOCATION

mkdir $BUILD_LOCATION
pushd $BUILD_LOCATION > /dev/null

CMAKE_ARGS=(
    -DCMAKE_INSTALL_PREFIX="$INSTALL_LOCATION"
    -DCMAKE_INSTALL_INCLUDEDIR="$INSTALL_INCLUDEDIR"
    -DCMAKE_INSTALL_BINDIR="$INSTALL_BIN_DIR"
    -DCMAKE_INSTALL_LIBDIR="$INSTALL_LIB_DIR"
    -DCMAKE_OSX_DEPLOYMENT_TARGET="13.0"
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    -DBUILD_TESTING=OFF
    -DBUILD_SHARED_LIBS=OFF
    -DIMATH_INSTALL_PKG_CONFIG=OFF
)

echo Configuring build for $LIBRARY_NAME version $LIBRARY_VERSION...
cmake -G "Xcode" $SOURCE_LOCATION "${CMAKE_ARGS[@]}"

echo Building $LIBRARY_NAME for Debug...
cmake --build . --config Debug

echo Installing $LIBRARY_NAME for Debug...
cmake --install . --config Debug

echo Building $LIBRARY_NAME for Release...
cmake --build . --config Release

echo Installing $LIBRARY_NAME for Release...
cmake --install . --config Release

popd > /dev/null

echo Done.
