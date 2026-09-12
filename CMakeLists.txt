
option(NX_MODULE_STORE_STOVE "Build the STOVE (Smilegate) storefront backend" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/NxStoreStove.cmake")

if (NOT NX_MODULE_STORE_STOVE)
    return()
endif ()

# nx_module()'s own PLATFORMS gate runs too late to help here - a project
# that also builds for Android/iOS still passes -DNX_MODULE_STORE_STOVE=ON
# for this platform-irrelevant module (its module list isn't per-platform),
# so the vendor-SDK resolution below must skip itself before ever running,
# the same "skipped on <platform>" outcome _nx_module_gate would otherwise
# report - just early enough to not FATAL_ERROR chasing a Windows-only SDK
# on a platform that was never going to build this module anyway.
_nx_current_platform(_nx_store_stove_here)
if (NOT _nx_store_stove_here STREQUAL "windows")
    _nx_module_skip_platform(store_stove "${_nx_store_stove_here}")
    return()
endif ()

nx_add_stove_sdk()

nx_module(store_stove
        DISPLAY "STOVE (Smilegate) storefront backend"
        SOURCES store_stove_module.cpp store_stove_config.cpp store_stove_platform.cpp
                store_stove_services.cpp store_stove_pcbang.cpp store_stove_scripting.cpp
        DEPENDS nx::stovesdk nx_module_store
        PLATFORMS windows)

if (NX_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
