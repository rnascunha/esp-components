idf_component_register(SRCS "src/websocket.cpp" "src/packet.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES sys websocket app_update)