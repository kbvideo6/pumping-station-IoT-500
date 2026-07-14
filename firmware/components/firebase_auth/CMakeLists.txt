idf_component_register(SRCS "firebase_auth.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES modem esp_driver_uart esp_driver_gpio
                       PRIV_REQUIRES cjson nvs_flash esp_timer config)
