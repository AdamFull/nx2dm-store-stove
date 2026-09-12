
add_executable(nx_store_stove_tests
        test_store_stove_config.cpp
        test_store_stove_module.cpp
        test_store_stove_services.cpp
        test_store_stove_pcbang.cpp
        test_store_stove_scripting.cpp
        ../store_stove_config.cpp
        ../store_stove_module.cpp
        ../store_stove_platform.cpp
        ../store_stove_services.cpp
        ../store_stove_pcbang.cpp
        ../store_stove_scripting.cpp)

nx_module_registry(nx_store_stove_tests)

target_include_directories(nx_store_stove_tests PRIVATE
        "${NX_ENGINE_ROOT}/engine" "${NX_ENGINE_ROOT}/modules")

target_link_libraries(nx_store_stove_tests
        PRIVATE nx::test nx::foundation nx::engine nx::stovesdk nx::compile_options)

set_target_properties(nx_store_stove_tests PROPERTIES FOLDER "tests")
nxtest_discover_tests(nx_store_stove_tests LABELS store_stove)
