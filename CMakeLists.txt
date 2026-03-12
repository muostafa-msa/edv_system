cmake_minimum_required(VERSION 3.16)

project(edv_system VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Charts PrintSupport LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Charts PrintSupport LinguistTools)

set(TS_FILES
    translations/edv_system_de.ts
    translations/edv_system_ar.ts
    translations/edv_system_en.ts
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    core/AppSettings.h
    core/DatabaseManager.h
    core/DatabaseManager.cpp
    core/DbConnectionDialog.h
    core/DbConnectionDialog.cpp
    core/LoginDialog.h
    core/LoginDialog.cpp
    ui/DashboardWidget.h
    ui/DashboardWidget.cpp
    ui/CrmWidget.h
    ui/CrmWidget.cpp
    ui/InventoryWidget.h
    ui/InventoryWidget.cpp
    ui/HrWidget.h
    ui/HrWidget.cpp
    ui/FinanceWidget.h
    ui/FinanceWidget.cpp
    ui/SettingsWidget.h
    ui/SettingsWidget.cpp
    models/CustomerModel.h
    models/CustomerModel.cpp
    resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(edv_system
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(edv_system SHARED ${PROJECT_SOURCES})
    else()
        add_executable(edv_system ${PROJECT_SOURCES})
    endif()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# Note: qt_create_translation already generates .qm files inside the build
# directory, which is the same folder as the executable. No copy needed.

target_include_directories(edv_system PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/ui
    ${CMAKE_SOURCE_DIR}/models
)

target_link_libraries(edv_system PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

set_target_properties(edv_system PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS edv_system
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(edv_system)
endif()
