target_sources(ptgl PRIVATE
    "${PROJECT_SOURCE_DIR}/ptgl/Extension/Remote/RemoteReceiver.cpp"
    "${PROJECT_SOURCE_DIR}/ptgl/Extension/Remote/ViewBinding.cpp"
    "${PROJECT_SOURCE_DIR}/ptgl/Extension/Remote/RemoteGraphicsView.cpp"
    "${PROJECT_SOURCE_DIR}/ptgl/Extension/Remote/RemotePlotGraphicsView.cpp")
target_compile_definitions(ptgl PUBLIC PTGL_HAS_REMOTE_EXTENSION=1)
if(EMSCRIPTEN)
    target_link_options(ptgl PUBLIC -lwebsocket.js)
else()
    add_subdirectory(sdk/Remote)
    target_include_directories(ptgl SYSTEM PRIVATE
        "${PROJECT_SOURCE_DIR}/thirdparty/Remote/websocketpp-0.8.2"
        "${PROJECT_SOURCE_DIR}/thirdparty/Remote/asio-asio-1-28-0/asio/include")
    if(WIN32)
        target_link_libraries(ptgl PRIVATE ws2_32 mswsock)
    endif()
endif()
