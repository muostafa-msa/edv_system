add_library(ModulePp STATIC
    ui/PpWidget.ui
    ui/PpWidget.hpp
    ui/PpWidget.cpp
)

target_include_directories(ModulePp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(ModulePp PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    EnterpriseCore
)

target_compile_features(ModulePp PUBLIC cxx_std_20)
