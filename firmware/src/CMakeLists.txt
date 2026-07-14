idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES led pzem_sensor battery_gauge sensor modem firebase_auth firebase_data alerts ota nvs_flash config esp_driver_uart esp_driver_gpio driver)
