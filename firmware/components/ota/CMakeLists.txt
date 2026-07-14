idf_component_register(SRCS "ota.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES modem esp_driver_uart esp_driver_gpio)
