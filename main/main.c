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

const char *pagina_html = 
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>"
        "<title>Nexti Setup</title>"
        "<style>"
            "* { box-sizing: border-box; margin: 0; padding: 0; }"
            
            "body { "
                "font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;"
                "background-color: #f0f2f5; "
                "height: 100vh; "
                "display: flex; "
                "justify-content: center; "
                "align-items: center; "
                "padding: 20px;"
            "}"

            ".card { "
                "background: white; "
                "padding: 30px; "
                "border-radius: 15px; "
                "box-shadow: 0 4px 15px rgba(0,0,0,0.1); "
                "width: 100%; "
                "max-width: 400px;"
                "}"

            "h1 { color: #333; text-align: center; margin-bottom: 25px; font-size: 24px; }"
            
            "label { display: block; margin-bottom: 8px; color: #555; font-weight: 600; }"
            "input[type='text'], input[type='password'] { "
                "width: 100%; "
                "padding: 15px; "
                "margin-bottom: 20px; "
                "border: 1px solid #ddd; "
                "border-radius: 8px; "
                "font-size: 16px; "
                "outline: none;"
            "}"
            "input:focus { border-color: #007BFF; }"

            ".btn-submit { "
                "width: 100%; "
                "padding: 15px; "
                "background-color: #007BFF; "
                "color: white; "
                "border: none; "
                "border-radius: 8px; "
                "font-size: 18px; "
                "font-weight: bold; "
                "cursor: pointer; "
                "transition: background-color 0.2s;"
            "}"
            ".btn-submit:hover { background-color: #0056b3; }"
            ".btn-submit:active { transform: scale(0.98); }" 
        "</style>"
    "</head>"
    "<body>"
        "<div class='card'>"
            "<h1>Configuração Nexti</h1>"
            "<form action='/salvar' method='post'>"
                "<label for='ssid'>Nome da Rede (SSID)</label>"
                "<input type='text' id='ssid' name='ssid' required placeholder='Ex: Nexti-WiFi'>"
                
                "<label for='senha'>Senha (Deixe vazio se for aberta)</label>"
                "<input type='password' id='senha' name='senha' placeholder='Sua senha aqui'>"
                
                "<button type='submit' class='btn-submit'>Salvar e Conectar</button>"
            "</form>"
        "</div>"
    "</body>"
    "</html>";


void salvar_credenciais_nvs(const char* ssid, const char* senha) {
    nvs_handle_t manipulador_nvs;
    esp_err_t erro;

    erro = nvs_open("config_rede", NVS_READWRITE, &manipulador_nvs);
    
    if (erro != ESP_OK) {
        printf("Erro ao abrir a gaveta do NVS!\n");
        return;
    }

    nvs_set_str(manipulador_nvs, "ssid", ssid);
    nvs_set_str(manipulador_nvs, "senha", senha);

    erro = nvs_commit(manipulador_nvs);
    if (erro == ESP_OK) {
        printf("\n[ NVS ] ---> Credenciais da rede salvas permanentemente!\n\n");
    } else {
        printf("\n[ ERRO ] ---> Falha ao gravar fisicamente no NVS!\n\n");
    }

    nvs_close(manipulador_nvs);
}

esp_err_t rota_raiz_handler(httpd_req_t *req) {
    httpd_resp_send(req, pagina_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t rota_raiz = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = rota_raiz_handler,
    .user_ctx  = NULL
};

esp_err_t rota_salvar_handler(httpd_req_t *req) {
    char buffer[100]; 
    
    int tamanho_recebido = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (tamanho_recebido <= 0) {
        return ESP_FAIL;
    }
    buffer[tamanho_recebido] = '\0';

    char ssid_recebido[32] = {0};
    char senha_recebida[64] = {0};

    httpd_query_key_value(buffer, "ssid", ssid_recebido, sizeof(ssid_recebido));
    httpd_query_key_value(buffer, "senha", senha_recebida, sizeof(senha_recebida));

    printf("\n=== DADOS CAPTURADOS DO CELULAR ===\n");
    printf("Rede (SSID): %s\n", ssid_recebido);
    printf("Senha: %s\n", senha_recebida);
    printf("===================================\n\n");

    salvar_credenciais_nvs(ssid_recebido, senha_recebida);

    const char *resposta = "Dados recebidos com sucesso! Olhe o terminal do Linux.";
    httpd_resp_send(req, resposta, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

httpd_uri_t rota_salvar = {
    .uri = "/salvar",
    .method = HTTP_POST,
    .handler = rota_salvar_handler,
    .user_ctx = NULL
};

httpd_handle_t iniciar_servidor_web(void) {
    httpd_handle_t servidor = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    if (httpd_start(&servidor, &config) == ESP_OK) {
        httpd_register_uri_handler(servidor, &rota_raiz);
        httpd_register_uri_handler(servidor, &rota_salvar);
        printf("Servidor Web iniciado na porta 80!\n");
    }
    return servidor;
}

void configurar_wifi_ap(void) {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t configuracao_wifi = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&configuracao_wifi);

    wifi_config_t ap_configuracao = {
        .ap = {
            .ssid = "Nexti-Sistema-Ponto",
            .ssid_len = strlen("Nexti-Sistema-Ponto"),
            .channel = 1,
            .password = "devNEXTIdev",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };

    esp_wifi_set_mode(WIFI_MODE_AP);                     
    esp_wifi_set_config(WIFI_IF_AP, &ap_configuracao);   
    esp_wifi_start();                                    
}

void app_main(void) {
    esp_err_t retorno = nvs_flash_init();

    if (retorno == ESP_ERR_NVS_NO_FREE_PAGES || retorno == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();   
    
    configurar_wifi_ap();
    iniciar_servidor_web();

    printf("Sistema de Ponto RFID Iniciado!\n");

    rc522_init();
    rc522_reset();
    rc522_antena_on();

    uint8_t versao = rc522_ler_registrador(0x37);
    printf("Versao do MFRC522: 0x%X\n", versao);

    while(1) {
        uint8_t buffer_resp[16];
        uint8_t tam_resp;
        
        uint8_t cmd_reqa = 0x26; 
        int status = rc522_comunicar(&cmd_reqa, 1, buffer_resp, &tam_resp, 0x87);

        if (status == 1) {
            uint8_t cmd_anticol[] = {0x93, 0x20};
            uint8_t uid_buffer[16];
            uint8_t uid_tam;

            int status_uid = rc522_comunicar(cmd_anticol, 2, uid_buffer, &uid_tam, 0x80);

            if (status_uid == 1) {
                printf("\n--- CARTAO LIDO COM SUCESSO ---\n");
                printf("ID do Funcionario (UID): ");
                for (int k = 0; k < 4; k++) { // O UID padrão tem 4 bytes
                    printf("%02X ", uid_buffer[k]);
                }
                printf("\n-------------------------------\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
