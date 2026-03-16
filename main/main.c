#include <stdio.h>
#include <string.h>
#include "rc522.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <nvs.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <cJSON.h>

// --- CONFIGURAÇÃO VISUAL (HTML) ---
const char *pagina_html = 
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Nexti Setup</title><style>"
    "body { font-family: sans-serif; background: #f0f2f5; display: flex; justify-content: center; align-items: center; height: 100vh; }"
    ".card { background: white; padding: 30px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); width: 90%; max-width: 400px; }"
    "h1 { color: #333; text-align: center; } input { width: 100%; padding: 12px; margin: 10px 0; border: 1px solid #ddd; border-radius: 8px; box-sizing: border-box; }"
    "button { width: 100%; padding: 12px; background: #007BFF; color: white; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; }"
    "</style></head><body><div class='card'><h1>Nexti Setup</h1>"
    "<form action='/salvar' method='post'>"
    "<input type='text' name='ssid' placeholder='SSID da Rede' required>"
    "<input type='password' name='senha' placeholder='Senha do Wi-Fi'>"
    "<button type='submit'>Salvar e Reiniciar</button></form></div></body></html>";

// --- MOTOR DE COMUNICAÇÃO HTTP ---
// Retorna o status HTTP (200, 404, etc) ou -1 em caso de falha de rede
int enviar_json_api(const char* uid_cartao, const char* url_endpoint) {
    int status_code = -1;
    cJSON *raiz = cJSON_CreateObject();
    cJSON_AddStringToObject(raiz, "rfid_uid", uid_cartao);
    cJSON_AddStringToObject(raiz, "reader_name", "ESP32_Entrada");
    char *payload = cJSON_PrintUnformatted(raiz);

    esp_http_client_config_t config = {
        .url = url_endpoint,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000 // Aumentado para 10s para estabilizar em redes lentas
    };

    esp_http_client_handle_t cliente = esp_http_client_init(&config);
    esp_http_client_set_header(cliente, "x-device-token", "RFID_DEVICE_01");
    esp_http_client_set_header(cliente, "Content-Type", "application/json");
    esp_http_client_set_post_field(cliente, payload, strlen(payload));

    esp_err_t erro = esp_http_client_perform(cliente);
    if (erro == ESP_OK) {
        status_code = esp_http_client_get_status_code(cliente);
        printf("[ API ] Resposta de %s: HTTP %d\n", url_endpoint, status_code);
    } else {
        printf("[ ERRO ] Falha na conexao: %s\n", esp_err_to_name(erro));
    }

    esp_http_client_cleanup(cliente);
    cJSON_Delete(raiz);
    free(payload);
    return status_code;
}

// --- GESTÃO DE REDE E MEMÓRIA ---
void salvar_credenciais_nvs(const char* ssid, const char* senha) {
    nvs_handle_t h;
    if (nvs_open("config_rede", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "ssid", ssid);
        nvs_set_str(h, "senha", senha);
        nvs_commit(h);
        nvs_close(h);
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
}

esp_err_t rota_salvar_handler(httpd_req_t *req) {
    char buffer[200];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (ret <= 0) return ESP_FAIL;
    buffer[ret] = '\0';
    char ssid[32] = {0}, senha[64] = {0};
    httpd_query_key_value(buffer, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(buffer, "senha", senha, sizeof(senha));
    salvar_credenciais_nvs(ssid, senha);
    httpd_resp_send(req, "Configurado! Reiniciando...", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void iniciar_servidor_web(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t raiz = { .uri = "/", .method = HTTP_GET, .handler = (esp_err_t (*)(httpd_req_t *))httpd_resp_send };
        httpd_uri_t salvar = { .uri = "/salvar", .method = HTTP_POST, .handler = rota_salvar_handler };
        httpd_register_uri_handler(server, &salvar);
    }
}

void configurar_wifi_ap(void) {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t ap_cfg = { .ap = { .ssid = "Nexti-Setup", .channel = 1, .password = "12345678", .authmode = WIFI_AUTH_WPA2_PSK, .max_connection = 4 } };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();
}

void conectar_wifi_sta(const char *ssid, const char *senha) {
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t sta_cfg = {0};
    strncpy((char*)sta_cfg.sta.ssid, ssid, 31);
    strncpy((char*)sta_cfg.sta.password, senha, 63);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_start();
    esp_wifi_connect();
}

// --- ORQUESTRADOR PRINCIPAL ---
void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();

    // Lógica de Inicialização de Rede
    nvs_handle_t h;
    char s_ssid[32] = {0}, s_pass[64] = {0};
    size_t l_ssid = 32, l_pass = 64;
    if (nvs_open("config_rede", NVS_READONLY, &h) == ESP_OK) {
        if (nvs_get_str(h, "ssid", s_ssid, &l_ssid) == ESP_OK) {
            nvs_get_str(h, "senha", s_pass, &l_pass);
            conectar_wifi_sta(s_ssid, s_pass);
        } else {
            configurar_wifi_ap();
            iniciar_servidor_web();
        }
        nvs_close(h);
    }

    rc522_init();
    rc522_reset();
    rc522_antena_on();

    // Memória do Debounce e Controle
    static char ultimo_uid[9] = {0};
    static TickType_t ultima_leitura = 0;

    printf("\n>>> SISTEMA NEXTI PRONTO <<<\n");

    while(1) {
        uint8_t buffer[16], tam;
        uint8_t reqa = 0x26;

        if (rc522_comunicar(&reqa, 1, buffer, &tam, 0x87) == 1) {
            uint8_t anticoll[] = {0x93, 0x20};
            uint8_t uid_raw[16], tam_u;

            if (rc522_comunicar(anticoll, 2, uid_raw, &tam_u, 0x80) == 1) {
                char uid_str[9] = {0};
                for (int i = 0; i < 4; i++) sprintf(uid_str + (i * 2), "%02X", uid_raw[i]);

                TickType_t agora = xTaskGetTickCount();
                uint32_t segundos = (agora - ultima_leitura) * portTICK_PERIOD_MS / 1000;

                // Debounce de 10 segundos para o mesmo cartão
                if (strcmp(uid_str, ultimo_uid) == 0 && segundos < 10) {
                    printf("[ INFO ] Aguarde... Debounce ativo para %s\n", uid_str);
                } 
                else {
                    strcpy(ultimo_uid, uid_str);
                    ultima_leitura = agora;
                    printf("\n--- LEITURA: %s ---\n", uid_str);

                    // Tenta bater o ponto (Entrada/Saída automática)
                    int status = enviar_json_api(uid_str, "https://nxt.unifapce.edu.br/api/rfid/punch");

                    // Se não existir, tenta registrar como novo cartão
                    if (status == 404) {
                        printf("[ AVISO ] Cartao novo detectado. Enviando para registro...\n");
                        enviar_json_api(uid_str, "https://nxt.unifapce.edu.br/api/rfid/scan");
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
