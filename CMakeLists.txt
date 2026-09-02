cmake_minimum_required(VERSION 3.16)
project(ControlPanel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Concurrent DBus OpenGL Network Test)
find_package(KF6WidgetsAddons REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Aero7Qt REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(Cups REQUIRED)

add_executable(controlpanel
    src/main.cpp
    src/features/FeatureCatalog.cpp
    src/ui/LinkLabel.h
    src/ui/MainWindow.cpp
    src/ui/PageRegistry.cpp
    src/ui/ControlPanelItems.cpp
    src/ui/SettingsCatalog.cpp
    src/ui/CategoryWidget.cpp
    src/ui/pages/LinuxUpdatePage.cpp
    src/ui/pages/SystemPage.cpp
    src/ui/pages/InstalledUpdatesPage.cpp
    src/ui/pages/ProgramsFeaturesPage.cpp
    src/ui/pages/NetworkSharingPage.cpp
    src/ui/pages/FirewallPage.cpp
    src/ui/pages/ActionCenterPage.cpp
    src/ui/pages/FeatureRequiredPage.cpp
    src/ui/pages/ParentalControlsPage.cpp
    src/ui/pages/PowerOptionsPage.cpp
    src/ui/pages/PerformancePage.cpp
    src/ui/pages/PersonalizationPage.cpp
    src/ui/pages/TaskbarStartMenuPage.cpp
    src/ui/pages/FolderOptionsPage.cpp
    src/ui/pages/GettingStartedPage.cpp
    src/ui/pages/SettingsHubPage.cpp
    src/ui/pages/DefaultProgramsPage.cpp
    src/ui/pages/FontsPage.cpp
    src/ui/pages/UserAccountsPage.cpp
    src/ui/pages/EaseOfAccessPage.cpp
    src/ui/pages/DevicesAndPrintersPage.cpp
    src/ui/pages/DisplayPage.cpp
    src/ui/dialogs/DateTimeDialog.cpp
    src/ui/dialogs/SoundDialog.cpp
    src/ui/dialogs/DevicePropertiesDialog.cpp
    src/ui/dialogs/LinverConfigDialog.cpp
    src/ui/dialogs/NativeSettingsDialog.cpp
    src/ui/dialogs/BluetoothDialog.cpp
    src/ui/dialogs/AddPrinterDialog.cpp
    src/devices/SysfsScanner.cpp
    src/devices/PrinterScanner.cpp
    src/data/WindowsTimeZones.cpp
    src/perf/WeiBenchmark.cpp
    src/data/Categories.cpp
    src/data/SystemAndSecurity.cpp
    src/data/NetworkAndInternet.cpp
    src/data/HardwareAndSound.cpp
    src/data/Programs.cpp
    src/data/UserAccounts.cpp
    src/data/AppearanceAndPersonalization.cpp
    src/data/ClockLanguageRegion.cpp
    src/data/EaseOfAccess.cpp
    src/data/Branding.cpp
)

target_include_directories(controlpanel PRIVATE
    src/features
    src/ui
    src/ui/pages
    src/ui/dialogs
    src/devices
    src/data
    src
)

target_compile_definitions(controlpanel PRIVATE
    AERO7_FEATURE_CATALOG_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/resources/aero7-features.json")

target_link_libraries(controlpanel PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    Qt6::DBus
    Qt6::OpenGL
    KF6::WidgetsAddons
    ZLIB::ZLIB
    OpenSSL::Crypto
    Aero7Qt::Aero7Qt
    Cups::Cups
)

set_target_properties(controlpanel PROPERTIES
    OUTPUT_NAME control
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

install(TARGETS controlpanel RUNTIME DESTINATION bin)
add_executable(aero7-optional-features
    src/optionalfeatures/main.cpp
    src/optionalfeatures/OptionalFeaturesWindow.cpp
    src/features/FeatureCatalog.cpp
)
target_include_directories(aero7-optional-features PRIVATE
    src/optionalfeatures src/features src/ui)
target_compile_definitions(aero7-optional-features PRIVATE
    AERO7_FEATURE_CATALOG_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/resources/aero7-features.json")
target_link_libraries(aero7-optional-features PRIVATE
    Qt6::Widgets Aero7Qt::Aero7Qt)
install(TARGETS aero7-optional-features RUNTIME DESTINATION bin)
install(FILES resources/aero7-features.json
    DESTINATION share/aero7-control-panel RENAME features.json)
install(PROGRAMS helpers/aero7-feature-helper
    DESTINATION lib/aero7-control-panel)
install(FILES polkit/org.aero7.optional-features.policy
    DESTINATION share/polkit-1/actions)
add_executable(aero7-action-center src/actioncenter/main.cpp)
target_include_directories(aero7-action-center PRIVATE src/ui)
target_link_libraries(aero7-action-center PRIVATE
    Qt6::Widgets Qt6::DBus Qt6::Network Aero7Qt::Aero7Qt LayerShellQt::Interface)
install(TARGETS aero7-action-center RUNTIME DESTINATION bin)
install(FILES aero7-action-center.desktop DESTINATION /etc/xdg/autostart)
install(FILES
    linux-controlpanel.desktop
    aero7-getting-started.desktop
    aero7-optional-features.desktop
    DESTINATION share/applications
)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME feature-helper-test
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_feature_helper.py)
    add_executable(branding-test
        tests/BrandingTest.cpp
        src/data/Branding.cpp
    )
    target_include_directories(branding-test PRIVATE src/data)
    target_link_libraries(branding-test PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME branding-test COMMAND branding-test)

    add_executable(control-panel-items-test
        tests/ControlPanelItemsTest.cpp
        src/ui/ControlPanelItems.cpp
        src/ui/PageRegistry.cpp
    )
    target_include_directories(control-panel-items-test PRIVATE src/ui)
    target_link_libraries(control-panel-items-test PRIVATE Qt6::Core)
    add_test(NAME control-panel-items-test COMMAND control-panel-items-test)

    add_executable(page-registry-test
        tests/PageRegistryTest.cpp
        src/ui/PageRegistry.cpp
    )
    target_include_directories(page-registry-test PRIVATE src/ui)
    target_link_libraries(page-registry-test PRIVATE Qt6::Core)
    add_test(NAME page-registry-test COMMAND page-registry-test)

    add_executable(getting-started-page-test
        tests/GettingStartedPageTest.cpp
        src/ui/LinkLabel.h
        src/ui/pages/GettingStartedPage.h
        src/ui/pages/GettingStartedPage.cpp
    )
    target_include_directories(getting-started-page-test PRIVATE
        src/ui
        src/ui/pages
    )
    target_link_libraries(getting-started-page-test PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        Aero7Qt::Aero7Qt
    )
    add_test(NAME getting-started-page-test COMMAND getting-started-page-test)
    set_tests_properties(getting-started-page-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(settings-catalog-test
        tests/SettingsCatalogTest.cpp
        src/ui/PageRegistry.cpp
        src/ui/SettingsCatalog.cpp
    )
    target_include_directories(settings-catalog-test PRIVATE src/ui)
    target_link_libraries(settings-catalog-test PRIVATE Qt6::Core)
    add_test(NAME settings-catalog-test COMMAND settings-catalog-test)

    add_executable(feature-catalog-test
        tests/FeatureCatalogTest.cpp
        src/features/FeatureCatalog.cpp
    )
    target_include_directories(feature-catalog-test PRIVATE src/features)
    target_compile_definitions(feature-catalog-test PRIVATE
        AERO7_FEATURE_CATALOG_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/resources/aero7-features.json")
    target_link_libraries(feature-catalog-test PRIVATE Qt6::Core)
    add_test(NAME feature-catalog-test COMMAND feature-catalog-test)

    add_executable(settings-hub-page-test
        tests/SettingsHubPageTest.cpp
        src/ui/LinkLabel.h
        src/ui/SettingsCatalog.cpp
        src/ui/pages/SettingsHubPage.h
        src/ui/pages/SettingsHubPage.cpp
        src/ui/dialogs/NativeSettingsDialog.cpp
        src/data/Branding.cpp
    )
    target_include_directories(settings-hub-page-test PRIVATE
        src/ui
        src/ui/pages
        src/ui/dialogs
        src/data
    )
    target_link_libraries(settings-hub-page-test PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        Aero7Qt::Aero7Qt
    )
    add_test(NAME settings-hub-page-test COMMAND settings-hub-page-test)
    set_tests_properties(settings-hub-page-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(display-page-test
        tests/DisplayPageTest.cpp
        src/ui/pages/DisplayPage.h
        src/ui/pages/DisplayPage.cpp
    )
    target_include_directories(display-page-test PRIVATE
        src/ui
        src/ui/pages
    )
    target_link_libraries(display-page-test PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        Aero7Qt::Aero7Qt
    )
    add_test(NAME display-page-test COMMAND display-page-test)
    set_tests_properties(display-page-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(default-programs-page-test
        tests/DefaultProgramsPageTest.cpp
        src/ui/pages/DefaultProgramsPage.h
        src/ui/pages/DefaultProgramsPage.cpp
    )
    target_include_directories(default-programs-page-test PRIVATE
        src/ui
        src/ui/pages
    )
    target_link_libraries(default-programs-page-test PRIVATE
        Qt6::Widgets
        Qt6::Test
        Qt6::Concurrent
        Aero7Qt::Aero7Qt
    )
    add_test(NAME default-programs-page-test COMMAND default-programs-page-test)
    set_tests_properties(default-programs-page-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
