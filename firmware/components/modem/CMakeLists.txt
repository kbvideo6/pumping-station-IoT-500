idf_component_register(SRCS "modem.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_uart esp_driver_gpio
                       PRIV_REQUIRES esp_timer config)
