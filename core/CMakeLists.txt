add_library(EnterpriseCore STATIC
    config/AppConfig.hpp

    database/ConnectionPool.hpp
    database/ConnectionPool.cpp
    database/DatabaseManager.hpp
    database/DatabaseManager.cpp

    security/Session.hpp
    security/AuthManager.hpp
    security/AuthManager.cpp

    journal/JournalEntry.hpp
    journal/PostManager.hpp
    journal/PostManager.cpp

    ui/ThemeManager.hpp
    ui/ThemeManager.cpp
)

target_include_directories(EnterpriseCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(EnterpriseCore PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Concurrent
)

target_compile_features(EnterpriseCore PUBLIC cxx_std_20)
