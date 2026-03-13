add_library(ModuleFico STATIC
    ui/FicoWidget.ui
    ui/FicoWidget.hpp
    ui/FicoWidget.cpp
)

target_include_directories(ModuleFico PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(ModuleFico PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    EnterpriseCore
)

target_compile_features(ModuleFico PUBLIC cxx_std_20)
