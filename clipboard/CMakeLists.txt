cmake_minimum_required(VERSION 3.14)

project(lingmo-clipboard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(KF6WindowSystem REQUIRED)
find_package(X11 REQUIRED)

qt_add_dbus_adaptor(DBUS_SOURCES com.lingmo.Clipboard.xml application.h Application clipboardadaptor ClipboardAdaptor)
set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

add_executable(lingmo-clipboard
  main.cpp
  application.cpp
  clipboard.cpp
  historymodel.cpp
  historywindow.cpp
  resources.qrc
  ${DBUS_SOURCES}
)

file(GLOB TS_FILES translations/*.ts)
foreach(filepath ${TS_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND ts_files_replaced ${filename})
endforeach()
qt_add_translation(QM_FILES ${ts_files_replaced})
add_custom_target(clipboard-translations DEPENDS ${QM_FILES} SOURCES ${ts_files_replaced})
add_dependencies(lingmo-clipboard clipboard-translations)

target_link_libraries(lingmo-clipboard
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::DBus
    KF6::WindowSystem
    X11::X11
    X11::Xtst
)

install(TARGETS lingmo-clipboard DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/lingmo-clipboard/translations)
