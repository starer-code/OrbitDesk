cmake_minimum_required(VERSION 3.19)
project(OrbitDesk LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Network)

qt_standard_project_setup()

qt_add_executable(OrbitDesk
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database_manager.h
    database_manager.cpp
    task_manager.h
    task_manager.cpp
    pomodoro_manager.h
    pomodoro_manager.cpp
    circular_progress.h circular_progress.cpp
    ai_chat_manager.h ai_chat_manager.cpp
    system_monitor.h system_monitor.cpp
    monitor_page.h monitor_page.cpp
    history_chart.h history_chart.cpp
    note_manager.h note_manager.cpp
    workflow_manager.h workflow_manager.cpp
    chat_session_manager.h chat_session_manager.cpp
    app.rc
)
qt_add_resources(OrbitDesk "resources"
    PREFIX "/"
    FILES
        dark.qss
        resources/icons/app.png
        resources/icons/monitor.svg
        resources/icons/task.svg
        resources/icons/note.svg
        resources/icons/pomodoro.svg
        resources/icons/chat.svg
        resources/icons/launch.svg
)


target_link_libraries(OrbitDesk
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        iphlpapi
)

include(GNUInstallDirs)

install(TARGETS OrbitDesk
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET OrbitDesk
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
