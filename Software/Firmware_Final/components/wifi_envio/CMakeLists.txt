idf_component_register(
    SRCS "wifi_envio.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif esp_event nvs_flash esp_http_client lwip mbedtls
)
