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

file(GLOB_RECURSE AERO7_PACK_ICON_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack/*.png")

add_executable(controlpanel
    resources/icons.qrc
    src/main.cpp
    src/features/FeatureCatalog.cpp
    src/ui/LinkLabel.h
    src/ui/MainWindow.cpp
    src/ui/PageRegistry.cpp
    src/ui/ControlPanelItems.cpp
    src/ui/SettingsCatalog.cpp
    src/ui/KdeSettingsBridge.cpp
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
    src/ui/pages/WindowSnappingPage.cpp
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
qt_add_resources(controlpanel "aero7-control-panel-pack-icons"
    PREFIX "/aero7/pack"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack"
    FILES ${AERO7_PACK_ICON_FILES})

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
    resources/icons.qrc
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
install(PROGRAMS helpers/aero7-system-settings-visibility
    DESTINATION bin)
install(FILES polkit/org.aero7.optional-features.policy
    DESTINATION share/polkit-1/actions)
add_executable(aero7-action-center src/actioncenter/main.cpp resources/icons.qrc)
qt_add_resources(aero7-action-center "aero7-action-center-pack-icons"
    PREFIX "/aero7/pack"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack"
    FILES ${AERO7_PACK_ICON_FILES})
target_include_directories(aero7-action-center PRIVATE src/ui src/data)
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
function(aero7_install_pack_launcher public_name pack_name)
    foreach(size IN LISTS ARGN)
        install(FILES assets/icons/pack/${size}/${pack_name}.png
            DESTINATION share/icons/hicolor/${size}x${size}/apps
            RENAME ${public_name}.png)
    endforeach()
endfunction()

aero7_install_pack_launcher(aero7-control-panel preferences-system
    16 32 48 256)
aero7_install_pack_launcher(aero7-getting-started help-contents
    16 22 32 48)
aero7_install_pack_launcher(aero7-action-center security-high
    16 24 32 48 256)
aero7_install_pack_launcher(aero7-optional-features system-software-install
    16 32 48 256)
install(FILES assets/icons/LICENSES.md assets/icons/pack/UPSTREAM-LICENSE
    assets/icons/pack/UPSTREAM-README.md docs/ICON-DEVELOPMENT.md
    DESTINATION share/doc/aero7-control-panel)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME feature-helper-test
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_feature_helper.py)
    add_test(NAME system-settings-visibility-test
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_system_settings_visibility.py)
    add_executable(branding-test
        tests/BrandingTest.cpp
        src/data/Branding.cpp
        resources/icons.qrc
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
    target_compile_definitions(control-panel-items-test PRIVATE
        AERO7_PACK_ICON_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack")
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
        src/ui/KdeSettingsBridge.cpp
        src/ui/pages/SettingsHubPage.h
        src/ui/pages/SettingsHubPage.cpp
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
        resources/icons.qrc
        tests/DefaultProgramsPageTest.cpp
        src/ui/pages/DefaultProgramsPage.h
        src/ui/pages/DefaultProgramsPage.cpp
    )
    qt_add_resources(default-programs-page-test "aero7-default-programs-pack-icons-test"
        PREFIX "/aero7/pack"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack"
        FILES ${AERO7_PACK_ICON_FILES})
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

    add_executable(folder-options-apply-test
        tests/FolderOptionsApplyTest.cpp
        src/ui/pages/FolderOptionsPage.h
        src/ui/pages/FolderOptionsPage.cpp
    )
    target_include_directories(folder-options-apply-test PRIVATE
        src/ui
        src/ui/pages
    )
    target_link_libraries(folder-options-apply-test PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        Aero7Qt::Aero7Qt
    )
    add_test(NAME folder-options-apply-test COMMAND folder-options-apply-test)
    set_tests_properties(folder-options-apply-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(taskbar-start-menu-apply-test
        tests/TaskbarStartMenuApplyTest.cpp
        src/ui/pages/TaskbarStartMenuPage.h
        src/ui/pages/TaskbarStartMenuPage.cpp
    )
    target_include_directories(taskbar-start-menu-apply-test PRIVATE
        src/ui
        src/ui/pages
    )
    target_link_libraries(taskbar-start-menu-apply-test PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        Aero7Qt::Aero7Qt
    )
    add_test(NAME taskbar-start-menu-apply-test COMMAND taskbar-start-menu-apply-test)
    set_tests_properties(taskbar-start-menu-apply-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(window-snapping-apply-test
        tests/WindowSnappingApplyTest.cpp
        src/ui/pages/WindowSnappingPage.h
        src/ui/pages/WindowSnappingPage.cpp
    )
    target_include_directories(window-snapping-apply-test PRIVATE
        src/ui
        src/ui/pages
    )
    target_link_libraries(window-snapping-apply-test PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        Aero7Qt::Aero7Qt
    )
    add_test(NAME window-snapping-apply-test COMMAND window-snapping-apply-test)
    set_tests_properties(window-snapping-apply-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(power-options-apply-test
        tests/PowerOptionsApplyTest.cpp
        src/ui/LinkLabel.h
        src/ui/KdeSettingsBridge.cpp
        src/ui/pages/PowerOptionsPage.h
        src/ui/pages/PowerOptionsPage.cpp
    )
    target_include_directories(power-options-apply-test PRIVATE
        src/ui
        src/ui/pages
    )
    target_link_libraries(power-options-apply-test PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::DBus
        Aero7Qt::Aero7Qt
    )
    add_test(NAME power-options-apply-test COMMAND power-options-apply-test)
    set_tests_properties(power-options-apply-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(firewall-backend-test tests/FirewallBackendTest.cpp)
    target_include_directories(firewall-backend-test PRIVATE src/data)
    target_link_libraries(firewall-backend-test PRIVATE Qt6::Test)
    add_test(NAME firewall-backend-test COMMAND firewall-backend-test)

    add_executable(linux-update-check-test
        resources/icons.qrc
        tests/LinuxUpdateCheckTest.cpp
        src/ui/pages/LinuxUpdatePage.h
        src/ui/pages/LinuxUpdatePage.cpp
        src/data/Branding.cpp
    )
    qt_add_resources(linux-update-check-test "aero7-linux-update-pack-icons-test"
        PREFIX "/aero7/pack"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack"
        FILES ${AERO7_PACK_ICON_FILES})
    target_include_directories(linux-update-check-test PRIVATE
        src/ui src/ui/pages src/data)
    target_link_libraries(linux-update-check-test PRIVATE
        Qt6::Widgets Qt6::Test Qt6::Concurrent Aero7Qt::Aero7Qt)
    add_test(NAME linux-update-check-test COMMAND linux-update-check-test)
    set_tests_properties(linux-update-check-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(icon-independence-test
        tests/IconIndependenceTest.cpp
        resources/icons.qrc
    )
    qt_add_resources(icon-independence-test "aero7-control-panel-pack-icons-test"
        PREFIX "/aero7/pack"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack"
        FILES ${AERO7_PACK_ICON_FILES})
    target_include_directories(icon-independence-test PRIVATE src/ui)
    target_link_libraries(icon-independence-test PRIVATE Qt6::Widgets)
    add_test(NAME icon-independence-test COMMAND icon-independence-test)
    set_tests_properties(icon-independence-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_test(NAME icon-policy-test
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_icon_policy.py)
    add_test(NAME settings-icons-test
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_settings_icons.py
            $<TARGET_FILE:controlpanel>
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/pack)

    add_executable(personalization-open-test
        tests/PersonalizationOpenTest.cpp
        src/ui/LinkLabel.h
        src/ui/pages/PersonalizationPage.cpp)
    target_include_directories(personalization-open-test PRIVATE src/ui src/ui/pages)
    target_link_libraries(personalization-open-test PRIVATE
        Qt6::Widgets Qt6::Test Qt6::Concurrent Aero7Qt::Aero7Qt)
    add_test(NAME personalization-open-test COMMAND personalization-open-test)
    set_tests_properties(personalization-open-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)

    add_executable(personalization-scheme-apply-test
        tests/PersonalizationSchemeApplyTest.cpp
        src/ui/LinkLabel.h
        src/ui/pages/PersonalizationPage.cpp)
    target_include_directories(personalization-scheme-apply-test PRIVATE src/ui src/ui/pages)
    target_link_libraries(personalization-scheme-apply-test PRIVATE
        Qt6::Widgets Qt6::Test Qt6::Concurrent Aero7Qt::Aero7Qt)
    add_test(NAME personalization-scheme-apply-test COMMAND personalization-scheme-apply-test)
    set_tests_properties(personalization-scheme-apply-test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()
