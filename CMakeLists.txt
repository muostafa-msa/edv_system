cmake_minimum_required(VERSION 3.20)

project(EDVEnterpriseSystem
    VERSION 2.0.0
    DESCRIPTION "EDV Enterprise Resource Planning System"
    LANGUAGES CXX
)

# ── C++20 ──────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt Auto-tools ──────────────────────────────────────────────────────────────
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Qt6 ────────────────────────────────────────────────────────────────────────
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Charts
    PrintSupport
    Concurrent
    LinguistTools
)

# SerialPort is optional (used by WmsScanner barcode module)
find_package(Qt6 QUIET COMPONENTS SerialPort)
if(Qt6SerialPort_FOUND)
    message(STATUS "Qt6::SerialPort found — WmsScanner barcode support enabled")
else()
    message(WARNING "Qt6::SerialPort NOT found — WmsScanner barcode support disabled")
endif()

# ── Global include dirs ────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# ── Translations ───────────────────────────────────────────────────────────────
set(TS_FILES
    translations/edv_en.ts
    translations/edv_de.ts
    translations/edv_ar.ts
)

# ── Sub-libraries ──────────────────────────────────────────────────────────────
add_subdirectory(core)
add_subdirectory(modules/fico)
add_subdirectory(modules/sd)
add_subdirectory(modules/mm)
add_subdirectory(modules/pp)
add_subdirectory(modules/hr)

# ── Main executable ────────────────────────────────────────────────────────────
set(APP_SOURCES
    app/main.cpp

    # UI layer
    ui/LoginPage.ui
    ui/LoginPage.hpp
    ui/LoginPage.cpp
    ui/MainWindow.ui
    ui/MainWindow.hpp
    ui/MainWindow.cpp
    ui/DashboardWidget.ui
    ui/DashboardWidget.hpp
    ui/DashboardWidget.cpp
    ui/DbSetupDialog.ui
    ui/DbSetupDialog.hpp
    ui/DbSetupDialog.cpp

    resources.qrc
)

qt6_add_executable(edv_system
    ${APP_SOURCES}
)

qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

set_target_properties(edv_system PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

target_include_directories(edv_system PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ui
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/modules/fico
    ${CMAKE_SOURCE_DIR}/modules/sd
    ${CMAKE_SOURCE_DIR}/modules/mm
    ${CMAKE_SOURCE_DIR}/modules/pp
    ${CMAKE_SOURCE_DIR}/modules/hr
)

target_link_libraries(edv_system PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::PrintSupport
    Qt6::Concurrent
    EnterpriseCore
    ModuleFico
    ModuleSd
    ModuleMm
    ModulePp
    ModuleHr
)

# ── Install ────────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS edv_system
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})

