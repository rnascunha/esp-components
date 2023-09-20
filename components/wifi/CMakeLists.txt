idf_component_register(SRCS "src/common.cpp" "src/station.cpp" "src/ap.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES sys esp_system esp_netif esp_wifi wpa_supplicant)