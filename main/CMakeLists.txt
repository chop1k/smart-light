
set(SOURCES
    "app_main.c"
)

set(CONTRACTS
    "."
)

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS ${CONTRACTS}
        REQUIRES esp_driver_ledc esp_driver_gpio esp-zigbee-lib nvs_flash
)
