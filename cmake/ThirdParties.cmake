# This directory structure is being created by `scripts/install-deps.sh`
# and is used to inject all the dependencies the operating system's
# package manager did not provide (not found or too old version).

if(EXISTS ${PROJECT_SOURCE_DIR}/_deps/sources/CMakeLists.txt)
    message(STATUS "Embedding 3rdparty libraries ...")
    add_subdirectory(${PROJECT_SOURCE_DIR}/_deps/sources)
endif()

set(LIST ThirdParties)
macro(Thirdparty_Include_If_MIssing _TARGET _PACKAGE_NAME)
    if(${_PACKAGE_NAME} STREQUAL "")
        set(${_PACKAGE_NAME} ${_TARGET})
    endif()
    if (NOT TARGET ${_TARGET})
        find_package(${_PACKAGE_NAME} REQUIRED)
        list(APPEND ThirdParties ${_TARGET}_SYSDEP)
        set(THIRDPARTY_BUILTIN_${_TARGET} "system package")
    else()
        list(APPEND ThirdParties ${_TARGET}_EMBED)
        set(THIRDPARTY_BUILTIN_${_TARGET} "embedded")
    endif()
endmacro()

# TODO make me working
macro(ThirdPartiesSummary)
    message(STATUS "==============================================================================")
    message(STATUS "    ThirdParties")
    message(STATUS "------------------------------------------------------------------------------")
    foreach(TP ${ThirdParties})
        message(STATUS "${TP}\t\t${THIRDPARTY_BUILTIN_${TP}}")
    endforeach()
endmacro()

# Now, conditionally find all dependencies that were not included above
# via find_package, usually system installed packages.

if (TARGET Catch2::Catch2)
    set(THIRDPARTY_BUILTIN_Catch2 "embedded")
else()
    find_package(Catch2 REQUIRED)
    set(THIRDPARTY_BUILTIN_Catch2 "system package")
endif()

if(TARGET fmt)
    set(THIRDPARTY_BUILTIN_fmt "embedded")
else()
    find_package(fmt REQUIRED)
    set(THIRDPARTY_BUILTIN_fmt "system package")
endif()

if(TARGET GSL)
    set(THIRDPARTY_BUILTIN_GSL "embedded")
else()
    set(THIRDPARTY_BUILTIN_GSL "system package")
    if (WIN32)
        # On Windows we use vcpkg and there the name is different
        find_package(Microsoft.GSL CONFIG REQUIRED)
        #target_link_libraries(main PRIVATE Microsoft.GSL::GSL)
    else()
        find_package(Microsoft.GSL REQUIRED)
    endif()
endif()

if (TARGET range-v3)
    set(THIRDPARTY_BUILTIN_range_v3 "embedded")
else()
    find_package(range-v3 REQUIRED)
    set(THIRDPARTY_BUILTIN_range_v3 "system package")
endif()

macro(ThirdPartiesSummary2)
    message(STATUS "==============================================================================")
    message(STATUS "    ThirdParties")
    message(STATUS "------------------------------------------------------------------------------")
    message(STATUS "Catch2              ${THIRDPARTY_BUILTIN_Catch2}")
    message(STATUS "GSL                 ${THIRDPARTY_BUILTIN_GSL}")
    message(STATUS "fmt                 ${THIRDPARTY_BUILTIN_fmt}")
    message(STATUS "range-v3            ${THIRDPARTY_BUILTIN_range_v3}")
    message(STATUS "------------------------------------------------------------------------------")
endmacro()
