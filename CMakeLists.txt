
option(NX_MODULE_STORE_STOVE "Build the STOVE (Smilegate) storefront backend" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/NxStoreStove.cmake")

if (NOT NX_MODULE_STORE_STOVE)
    return()
endif ()

nx_add_stove_sdk()

nx_module(store_stove
        DISPLAY "STOVE (Smilegate) storefront backend"
        SOURCES store_stove_module.cpp store_stove_config.cpp store_stove_platform.cpp
                store_stove_services.cpp
        DEPENDS nx::stovesdk nx_module_store
        PLATFORMS windows)

if (NX_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
