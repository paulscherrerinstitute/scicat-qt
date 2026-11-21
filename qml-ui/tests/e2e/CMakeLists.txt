# what the walks and the scenario driver share: finding items and sending them events
qt_add_library(scicat_uiprobe STATIC
    uiprobe.cpp
    uiprobe.h
)

target_include_directories(scicat_uiprobe
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(scicat_uiprobe
    PUBLIC
        Qt6::Quick
    PRIVATE
        Qt6::QuickPrivate
        Qt6::Test
)

qt_add_executable(scicat_qml_e2e
    main.cpp
    uidriver.cpp
    uidriver.h
)

target_link_libraries(scicat_qml_e2e
    PRIVATE
        scicat_appmain
        scicat_uiprobe
        Qt6::Test
)

# The walks as tests, on Qt's offscreen platform: ctest -R walk
find_program(SCICAT_BASH bash)
find_package(Python3 COMPONENTS Interpreter)
if(WIN32 OR NOT SCICAT_BASH OR NOT Python3_Interpreter_FOUND)
    message(STATUS "SciCatQml: the scripted walks need bash and Python 3; they are built but not run by ctest")
    return()
endif()

# name | walk file | what the walk needs besides WALK_MODE ("-": nothing)
set(_walks
    "main|walk.json|-"
    "keyboard|walk-keyboard.json|-"
    "slow|walk-slow.json|FAKE_SCICAT_LATENCY=1200"
    "servers|walk-servers.json|WALK_REAL_SERVERS=1"
    "quit|walk-quit.json|FAKE_CLI_INGEST_PAUSE=20"
)
foreach(_walk IN LISTS _walks)
    string(REPLACE "|" ";" _walk "${_walk}")
    list(GET _walk 0 _name)
    list(GET _walk 1 _file)
    list(GET _walk 2 _extra)
    set(_environment "WALK_MODE=offscreen")
    if(NOT _extra STREQUAL "-")
        list(APPEND _environment "${_extra}")
    endif()
    add_test(NAME "walk.${_name}"
        COMMAND "${SCICAT_BASH}" "${CMAKE_CURRENT_SOURCE_DIR}/run_walk.sh"
                "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${_file}" "${PROJECT_BINARY_DIR}/walks/${_name}"
    )
    # a walk fails by its exit code (the number of failed steps); the one that ends with the application quitting
    # by itself has no such count, so a failed step is also looked for in what was printed
    set_tests_properties("walk.${_name}" PROPERTIES
        ENVIRONMENT "${_environment}"
        FAIL_REGULAR_EXPRESSION "\nFAIL "
        RUN_SERIAL TRUE
        TIMEOUT 600
    )
endforeach()
