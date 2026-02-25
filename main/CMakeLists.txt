idf_component_register(SRCS "main.c" 
                        INCLUDE_DIRS "."
                        REQUIRES rc522
                        REQUIRES nvs_flash
                        REQUIRES esp_wifi
                        REQUIRES esp_http_server)
