add_library(ModuleHr STATIC
    ui/HrWidget.ui
    ui/HrWidget.hpp
    ui/HrWidget.cpp
)

target_include_directories(ModuleHr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(ModuleHr PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    EnterpriseCore
)

target_compile_features(ModuleHr PUBLIC cxx_std_20)
