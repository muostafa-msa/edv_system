add_library(ModuleSd STATIC
    PricingEngine.hpp
    PricingEngine.cpp
    ui/SdWidget.ui
    ui/SdWidget.hpp
    ui/SdWidget.cpp
)

target_include_directories(ModuleSd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(ModuleSd PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    EnterpriseCore
)

target_compile_features(ModuleSd PUBLIC cxx_std_20)
