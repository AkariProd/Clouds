# Seer12 web UI build glue.
#
# Hooked up from the root CMakeLists.txt (Seer12 block): NEEDS_WEBVIEW2 TRUE
# in juce_add_plugin(Seer12 ...), JUCE_WEB_BROWSER=0 removed from the Seer12
# compile definitions, and include(.../Source12/cmake/Seer12WebUI.cmake) after
# the block. If CMake warns "WebView2 wasn't found in the local NuGet folder",
# run the two PowerShell commands it prints (one-time), then re-run CMake.
#
# Requires JUCE 8 or newer (WebBrowserComponent resource provider + events).

set(SEER12_TARGET Seer12 CACHE STRING "Name of the Seer12 juce_add_plugin target")

file(GLOB SEER12_UI_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/../ui/*.html"
     "${CMAKE_CURRENT_LIST_DIR}/../ui/*.css"
     "${CMAKE_CURRENT_LIST_DIR}/../ui/*.js"
     "${CMAKE_CURRENT_LIST_DIR}/../ui/*.woff2"
     "${CMAKE_CURRENT_LIST_DIR}/../ui/*.png")

juce_add_binary_data(Seer12UI
    NAMESPACE Seer12UI
    HEADER_NAME Seer12UI.h
    SOURCES ${SEER12_UI_FILES})

target_link_libraries(${SEER12_TARGET} PRIVATE Seer12UI juce::juce_gui_extra)

target_compile_definitions(${SEER12_TARGET} PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
