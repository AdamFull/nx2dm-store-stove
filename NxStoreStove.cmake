
# The STOVE (Smilegate) PC SDK is a licence-gated download from STOVE's own
# developer portal - there is no fetchable URL, so it stays a required
# local, gitignored, developer-provided directory, the same shape
# modules/store_steam, modules/store_egs and modules/store_gog already
# established.
#
# Unlike those three (one vendor DLL each), STOVE ships one DLL/.lib pair
# per subsystem. This module calls into five of them directly - BaseSDK,
# OwnershipSDK, IAPSDK, GameSupportSDK, PCBangSDK - each gets its own
# IMPORTED SHARED target, wrapped in one nx::stovesdk INTERFACE target so
# store_stove's own CMakeLists.txt only needs to depend on a single name.
# LogSDK and WebView2Loader are never called directly (this module has no
# use for telemetry) but are still staged next to the executable - the
# linked DLLs may depend on them internally at runtime, the same "extra
# runtime companion" precedent store_egs's xaudio2_9redist.dll already
# established for EOS's voice chat.

set(NX_STORE_STOVE_SDK_DIR "" CACHE PATH
        "An extracted STOVE PC SDK (containing Include/ and dll/x64, lib/x64). Empty uses modules/store_stove/third_party/StoveSdk.")

function(nx_add_stove_sdk)
    if (NX_STORE_STOVE_SDK_DIR)
        set(_search "${NX_STORE_STOVE_SDK_DIR}")
    else ()
        set(_search "${CMAKE_CURRENT_LIST_DIR}/third_party/StoveSdk")
    endif ()

    _nx_resolve_vendor_root("${_search}" "Include/BaseSDK.h" _root)
    if (NOT _root)
        message(FATAL_ERROR
                "nx2d: no STOVE PC SDK. Download it (STOVE partner account "
                "required) from the STOVE developer portal, then extract it "
                "to modules/store_stove/third_party/StoveSdk (or point "
                "NX_STORE_STOVE_SDK_DIR at it). It is not fetchable here: "
                "Smilegate distributes it only to registered partners. "
                "Never commit it - modules/store_stove/third_party/StoveSdk "
                "is gitignored on purpose.")
    endif ()

    if (NOT WIN32)
        message(FATAL_ERROR "nx2d: the vendored STOVE PC SDK is Windows-only")
    endif ()
    if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "nx2d: the vendored STOVE PC SDK is 64-bit only")
    endif ()

    set(_bin "${_root}/dll/x64")
    set(_lib "${_root}/lib/x64")

    _nx_imported_shared_library(nx_stove_base
            RUNTIME "${_bin}/BaseSDK.dll" IMPLIB "${_lib}/BaseSDK.lib")
    _nx_imported_shared_library(nx_stove_ownership
            RUNTIME "${_bin}/OwnershipSDK.dll" IMPLIB "${_lib}/OwnershipSDK.lib")
    _nx_imported_shared_library(nx_stove_iap
            RUNTIME "${_bin}/IAPSDK.dll" IMPLIB "${_lib}/IAPSDK.lib")
    _nx_imported_shared_library(nx_stove_gamesupport
            RUNTIME "${_bin}/GameSupportSDK.dll" IMPLIB "${_lib}/GameSupportSDK.lib")
    _nx_imported_shared_library(nx_stove_pcbang
            RUNTIME "${_bin}/PCBangSDK.dll" IMPLIB "${_lib}/PCBangSDK.lib")

    # The include dir must live on each IMPORTED target itself, not on the
    # INTERFACE umbrella below - only a genuinely IMPORTED target gets
    # CMake's automatic "treat as a system header" behavior
    # (-external:I under MSVC, silencing this vendor SDK's own warnings
    # under this project's /W4 /WX), the same reason store_egs's/
    # store_gog's own single-target vendoring already sets it there.
    foreach (_t nx_stove_base nx_stove_ownership nx_stove_iap nx_stove_gamesupport
            nx_stove_pcbang)
        set_target_properties(${_t} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${_root}/Include")
    endforeach ()

    add_library(nx_stovesdk INTERFACE)
    target_link_libraries(nx_stovesdk INTERFACE
            nx_stove_base nx_stove_ownership nx_stove_iap nx_stove_gamesupport
            nx_stove_pcbang)
    add_library(nx::stovesdk ALIAS nx_stovesdk)

    foreach (_extra LogSDK.dll WebView2Loader.dll)
        if (EXISTS "${_bin}/${_extra}")
            file(COPY "${_bin}/${_extra}" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        endif ()
    endforeach ()

    message(STATUS "nx2d: STOVE PC SDK from ${_root}")
endfunction()
