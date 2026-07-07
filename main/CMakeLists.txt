idf_component_register(
    SRCS
        "app_main.cpp"
        "app_config.cpp"
        "display.cpp"
        "keys.cpp"
        "network.cpp"
        "nrl21.cpp"
        "audio.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_event
        esp_lcd
        esp_netif
        esp_wifi
        nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
