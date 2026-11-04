# Download the scicat-cli release asset for the target platform and ship it with SciCat.
# https://github.com/paulscherrerinstitute/scicat-cli/releases

option(SCICAT_BUNDLE_CLI "Download scicat-cli and install it together with SciCat" OFF)
set(SCICAT_CLI_VERSION "3.1.0" CACHE STRING "scicat-cli release to bundle")
set(SCICAT_CLI_SHA256 "" CACHE STRING "SHA256 of the scicat-cli asset (only needed for versions without a built-in checksum)")

if(NOT SCICAT_BUNDLE_CLI)
  return()
endif()

# checksums from scicat-cli_<version>_checksums.txt
set(_scicat_cli_sha256_3.1.0_Linux "96f5413b6a843f00e9d6ffbb4241b1dc0f15039df6e0054917f0184e203966a2")
set(_scicat_cli_sha256_3.1.0_Darwin "9bda234924377df67a7370199941da20ae0b0b76457bc70ebaf0cf95f4ce2f6c")
set(_scicat_cli_sha256_3.1.0_Windows "619e4f17e7e0e6c100ef20231ad018becfc2de5b69ba5fd432c37b0221ad4c08")

if(WIN32)
  set(_scicat_cli_os "Windows")
  set(_scicat_cli_ext "zip")
  set(_scicat_cli_exe "scicat-cli.exe")
elseif(APPLE)
  # only an x86_64 build is published; it runs under Rosetta on arm64
  set(_scicat_cli_os "Darwin")
  set(_scicat_cli_ext "tar.gz")
  set(_scicat_cli_exe "scicat-cli")
else()
  set(_scicat_cli_os "Linux")
  set(_scicat_cli_ext "tar.gz")
  set(_scicat_cli_exe "scicat-cli")
endif()

set(_scicat_cli_hash "${SCICAT_CLI_SHA256}")
if(NOT _scicat_cli_hash)
  set(_scicat_cli_hash "${_scicat_cli_sha256_${SCICAT_CLI_VERSION}_${_scicat_cli_os}}")
endif()
if(NOT _scicat_cli_hash)
  message(FATAL_ERROR "No checksum known for scicat-cli ${SCICAT_CLI_VERSION} on ${_scicat_cli_os}; set SCICAT_CLI_SHA256")
endif()

set(_scicat_cli_asset "scicat-cli_${SCICAT_CLI_VERSION}_${_scicat_cli_os}_x86_64")
set(_scicat_cli_dir "${CMAKE_BINARY_DIR}/_deps/scicat-cli")
set(_scicat_cli_archive "${_scicat_cli_dir}/${_scicat_cli_asset}.${_scicat_cli_ext}")
set(SCICAT_CLI_EXECUTABLE "${_scicat_cli_dir}/${_scicat_cli_asset}/${_scicat_cli_exe}")

if(NOT EXISTS "${SCICAT_CLI_EXECUTABLE}")
  message(STATUS "Downloading ${_scicat_cli_asset}.${_scicat_cli_ext}")
  file(DOWNLOAD
    "https://github.com/paulscherrerinstitute/scicat-cli/releases/download/v${SCICAT_CLI_VERSION}/${_scicat_cli_asset}.${_scicat_cli_ext}"
    "${_scicat_cli_archive}"
    EXPECTED_HASH SHA256=${_scicat_cli_hash}
    TLS_VERIFY ON
    STATUS _scicat_cli_status
  )
  list(GET _scicat_cli_status 0 _scicat_cli_code)
  if(NOT _scicat_cli_code EQUAL 0)
    message(FATAL_ERROR "Failed to download scicat-cli: ${_scicat_cli_status}")
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xf "${_scicat_cli_archive}"
    WORKING_DIRECTORY "${_scicat_cli_dir}"
    RESULT_VARIABLE _scicat_cli_extract
  )
  if(NOT _scicat_cli_extract EQUAL 0 OR NOT EXISTS "${SCICAT_CLI_EXECUTABLE}")
    message(FATAL_ERROR "Failed to extract ${SCICAT_CLI_EXECUTABLE}")
  endif()
endif()

if(APPLE)
  # SciCat.app/Contents/MacOS, next to the SciCat executable
  add_custom_command(TARGET SciCat POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "${SCICAT_CLI_EXECUTABLE}" "$<TARGET_FILE_DIR:SciCat>/${_scicat_cli_exe}"
  )
elseif(WIN32)
  install(PROGRAMS "${SCICAT_CLI_EXECUTABLE}" DESTINATION bin)
else()
  # libexec avoids clashing with /usr/bin/scicat-cli from the scicat-cli RPM
  install(PROGRAMS "${SCICAT_CLI_EXECUTABLE}" DESTINATION libexec/scicat)
endif()
