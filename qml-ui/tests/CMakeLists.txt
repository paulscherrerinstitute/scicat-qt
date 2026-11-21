qt_add_executable(tst_core
    httpstub.h
    tst_core.cpp
)

target_link_libraries(tst_core
    PRIVATE
        scicat_core
        Qt6::Test
)

add_test(NAME tst_core COMMAND tst_core)

qt_add_executable(tst_backend
    httpstub.h
    tst_backend.cpp
)

target_link_libraries(tst_backend
    PRIVATE
        scicat_backend
        Qt6::Test
)

add_test(NAME tst_backend COMMAND tst_backend)

# The tests that drive the window look into Qt Quick: which rows a list has culled, what an item says to a screen
# reader. Only the private headers tell. That ties these test tools, not the application, to the Qt build they were
# compiled with; Qt's warning about it is switched off here. Without the private headers (a distribution may package
# them separately) these tests are left out.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 QUIET COMPONENTS QuickPrivate)
if(TARGET Qt6::QuickPrivate)
    add_subdirectory(e2e)
else()
    message(STATUS "SciCatQml: no private Qt Quick headers, the scripted walks are not built")
endif()
