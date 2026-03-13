add_library(ModuleMm STATIC
    WmsScanner.hpp
    WmsScanner.cpp
    PredictiveReorder.hpp
    PredictiveReorder.cpp
    ui/MmWidget.ui
    ui/MmWidget.hpp
    ui/MmWidget.cpp
)

target_include_directories(ModuleMm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(ModuleMm PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    EnterpriseCore
)

# Link SerialPort only if available
if(Qt6SerialPort_FOUND)
    target_link_libraries(ModuleMm PUBLIC Qt6::SerialPort)
    target_compile_definitions(ModuleMm PUBLIC EDV_SERIAL_PORT_AVAILABLE)
endif()

target_compile_features(ModuleMm PUBLIC cxx_std_20)
