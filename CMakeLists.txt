cmake_minimum_required(VERSION 3.20)

project(umaet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Quick
    Qml
    QuickControls2
    Sql
    Test
)

find_package(KF6 6.0 REQUIRED COMPONENTS
    Kirigami
    CoreAddons
    I18n
)

set(UMAET_CORE_SRCS
    src/DatabaseManager.h
    src/DatabaseManager.cpp
    src/TransactionModel.h
    src/TransactionModel.cpp
    src/BudgetManager.h
    src/BudgetManager.cpp
    src/StatsManager.h
    src/StatsManager.cpp
    src/SettingsManager.h
    src/SettingsManager.cpp
)

add_executable(umaet
    src/main.cpp
    ${UMAET_CORE_SRCS}
    src/resources.qrc
)

target_link_libraries(umaet PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Sql
    KF6::Kirigami
    KF6::CoreAddons
    KF6::I18n
)

target_include_directories(umaet PRIVATE src)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_umaet
        tests/test_umaet.cpp
        ${UMAET_CORE_SRCS}
    )
    target_include_directories(test_umaet PRIVATE src)
    target_link_libraries(test_umaet PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
    )
    add_test(NAME test_umaet COMMAND test_umaet)
endif()

install(TARGETS umaet ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.umaet.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.umaet.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES icons/umaet.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps/ RENAME umaet.svg)
