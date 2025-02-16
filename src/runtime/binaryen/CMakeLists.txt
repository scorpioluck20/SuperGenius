add_library(binaryen_wasm_memory
    wasm_memory_impl.hpp
    wasm_memory_impl.cpp
    )
set_target_properties(binaryen_wasm_memory PROPERTIES PUBLIC_HEADER "wasm_memory_impl.hpp")
target_link_libraries(binaryen_wasm_memory
    buffer
    binaryen::binaryen
    )
supergenius_install(binaryen_wasm_memory)

add_library(binaryen_runtime_external_interface
    runtime_external_interface.hpp
    runtime_external_interface.cpp
    )
set_target_properties(binaryen_runtime_external_interface PROPERTIES PUBLIC_HEADER "runtime_external_interface.hpp")
target_link_libraries(binaryen_runtime_external_interface
    binaryen::binaryen
    binaryen_wasm_memory
    logger
    )
supergenius_install(binaryen_runtime_external_interface)

add_library(binaryen_wasm_module
    module/wasm_module_impl.cpp
    module/wasm_module_factory_impl.cpp
    module/wasm_module_instance_impl.cpp
    )
set_target_properties(binaryen_wasm_module PROPERTIES PUBLIC_HEADER "module/wasm_module.hpp;module/wasm_module_factory.hpp;module/wasm_module_impl.hpp;module/wasm_module_factory_impl.hpp;module/wasm_module_instance.hpp;module/wasm_module_instance_impl.hpp")
target_link_libraries(binaryen_wasm_module
    binaryen::binaryen
    logger
    )
supergenius_install(binaryen_wasm_module)

add_library(binaryen_runtime_environment
    runtime_environment.cpp
    )
set_target_properties(binaryen_runtime_environment PROPERTIES PUBLIC_HEADER "runtime_environment.hpp")
target_link_libraries(binaryen_runtime_environment
    binaryen_wasm_module
    )
supergenius_install(binaryen_runtime_environment)

add_library(binaryen_runtime_manager
    runtime_manager.hpp
    runtime_manager.cpp
    )
set_target_properties(binaryen_runtime_manager PROPERTIES PUBLIC_HEADER "runtime_manager.hpp")
target_link_libraries(binaryen_runtime_manager
    binaryen_runtime_environment
    binaryen_wasm_module
    binaryen_runtime_external_interface
    )
supergenius_install(binaryen_runtime_manager)

add_library(binaryen_wasm_executor
    wasm_executor.hpp
    wasm_executor.cpp
    )
set_target_properties(binaryen_wasm_executor PROPERTIES PUBLIC_HEADER "wasm_executor.hpp")
target_link_libraries(binaryen_wasm_executor
    binaryen::binaryen
    binaryen_runtime_manager
    logger
    )
supergenius_install(binaryen_wasm_executor)

add_library(binaryen_runtime_api INTERFACE)
set_target_properties(binaryen_runtime_api PROPERTIES PUBLIC_HEADER "runtime_api/runtime_api.hpp")
target_link_libraries(binaryen_runtime_api INTERFACE
    binaryen_runtime_external_interface
    binaryen_wasm_module
    )
supergenius_install(binaryen_runtime_api)

add_library(binaryen_core_api
    runtime_api/core_impl.cpp
    runtime_api/core_factory_impl.cpp
    )
set_target_properties(binaryen_core_api PROPERTIES PUBLIC_HEADER "runtime_api/core_impl.hpp;runtime_api/core_factory_impl.hpp")
target_link_libraries(binaryen_core_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )
supergenius_install(binaryen_core_api)

add_library(binaryen_tagged_transaction_queue_api
    runtime_api/tagged_transaction_queue_impl.cpp
    )
target_link_libraries(binaryen_tagged_transaction_queue_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_block_builder_api
    runtime_api/block_builder_impl.cpp
    )
target_link_libraries(binaryen_block_builder_api
    primitives
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_parachain_host_api
    runtime_api/parachain_host_impl.cpp
    )
target_link_libraries(binaryen_parachain_host_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_finality_api
    runtime_api/finality_api_impl.cpp
    )
target_link_libraries(binaryen_finality_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_metadata_api
    runtime_api/metadata_impl.cpp
    )
target_link_libraries(binaryen_metadata_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )


add_library(binaryen_offchain_worker_api
    runtime_api/offchain_worker_impl.cpp
    )
target_link_libraries(binaryen_offchain_worker_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_production_api
    runtime_api/production_api_impl.cpp
    )
target_link_libraries(binaryen_production_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )
