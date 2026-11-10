# giga_core: the pure engine. QtCore only (NFR-3, 01 section 2 rule 1).
qt_add_library(giga_core STATIC
    common/result.h
    common/constants.h
    common/paths.h common/paths.cpp
    common/path_key.h common/path_key.cpp
    common/python_format.h common/python_format.cpp
    gguf/model_layout.h
    gguf/gguf_shard.h
    gguf/gguf_reader.h gguf/gguf_reader.cpp
    gguf/shard_cache.h gguf/shard_cache.cpp
    gguf/quant_name.h gguf/quant_name.cpp
    gguf/shard_finder.h gguf/shard_finder.cpp
    gguf/layout_reader.h gguf/layout_reader.cpp
    gguf/local_model_source.h gguf/local_model_source.cpp
    gguf/draft_finder.h gguf/draft_finder.cpp
    gguf/mmproj_finder.h gguf/mmproj_finder.cpp
    gguf/model_source.h
    hardware/gpu.h
    hardware/hardware_snapshot.h
    hardware/gpu_info.h
    hardware/gpu_info.cpp
    hardware/ram_info.h
    hardware/ram_info.cpp
    hardware/hardware_probe.h
    hardware/hardware_probe.cpp
    moe/usage_counts.h
    moe/hot_map.h
    moe/usage_source.h
    plan/plan_model.h
    plan/plan_request.h
    plan/layer_placement.h
    plan/buffer_model.h plan/buffer_model.cpp
    plan/layer_placer.h plan/layer_placer.cpp
    plan/placement_planner.h plan/placement_planner.cpp
    plan/hot_reuse.h plan/hot_reuse.cpp
    plan/expert_placer.h plan/expert_placer.cpp
    plan/tuning_tables.h plan/tuning_tables.cpp
    launch/server_environment.h
    launch/launch_request.h
    launch/command_line.h launch/command_line.cpp
    launch/command_builder.h launch/command_builder.cpp
    # Track B: Python-exact helpers, moe algorithms (P5a), launch pieces (P6a), log parser (P7a)
    common/python_json.h common/python_json.cpp
    common/python_int.h common/python_int.cpp
    common/model_names.h common/model_names.cpp
    moe/local_usage_source.h moe/local_usage_source.cpp
    moe/usage_io.h moe/usage_io.cpp
    moe/hot_cost_model.h moe/hot_cost_model.cpp
    moe/hot_builder.h
    moe/hot_file.h moe/hot_file.cpp
    moe/hot_cache.h moe/hot_cache.cpp
    common/python_text.h common/python_text.cpp
    launch/hidden_process.h launch/hidden_process.cpp
    launch/local_server_environment.h launch/local_server_environment.cpp
    launch/server_finder.h launch/server_finder.cpp
    launch/flag_probe.h launch/flag_probe.cpp
    launch/server_env.h launch/server_env.cpp
    launch/extra_args.h launch/extra_args.cpp
    launch/usage_env.h launch/usage_env.cpp
    launch/line_splitter.h launch/line_splitter.cpp
    launch/free_port.h launch/free_port.cpp
    launch/server_probe.h launch/server_probe.cpp
    launch/log_event.h
    launch/log_summary.h
    launch/log_line_classifier.h launch/log_line_classifier.cpp
    launch/log_parser.h launch/log_parser.cpp
    launch/server_exit.h launch/server_exit.cpp
    # Track D: the Run path shared by LaunchController, PlannerRunner and the golden runner
    launch/run_settings.h
    launch/run_settings_parser.h launch/run_settings_parser.cpp
    launch/run_environment.h
    launch/expert_usage_args.h launch/expert_usage_args.cpp
    launch/run_planner.h launch/run_planner.cpp
    launch/launch_assembler.h launch/launch_assembler.cpp
)
target_include_directories(giga_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(giga_core PUBLIC Qt6::Core)
giga_strata_warnings(giga_core)

# The knapsack (build_hot_cost) runs ~10^8 steps per per-expert plan. Unoptimized it takes
# seconds per plan, which stalls Debug live re-plans and the golden run, so it is always built
# optimized (hot_builder.cpp pragma); the Debug runtime checks forbid that, so they are off here.
add_library(giga_knapsack OBJECT moe/hot_builder.cpp)
target_include_directories(giga_knapsack PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(giga_knapsack PRIVATE Qt6::Core)
set_property(TARGET giga_knapsack PROPERTY MSVC_RUNTIME_CHECKS "")
giga_strata_warnings(giga_knapsack)

target_sources(giga_core PRIVATE $<TARGET_OBJECTS:giga_knapsack>)
