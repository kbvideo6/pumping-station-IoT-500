idf_component_register(SRCS "firebase_data.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES modem firebase_auth esp_driver_uart esp_driver_gpio
                       PRIV_REQUIRES cjson config)
