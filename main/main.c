#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora.h"
#include "esp_wifi.h"
#include "esp_private/wifi.h"
#include "esp_log.h"
#include <string.h>
#include "nvs_flash.h"
static const char *TAG = "eth_example";
static bool s_sta_is_connected = false;
static bool wifi_is_connected = false;
#define CONFIG_EXAMPLE_WIFI_SSID "NGLORA"
#define CONFIG_EXAMPLE_WIFI_PASSWORD "NGLORAICT"
#define CONFIG_EXAMPLE_MAX_STA_CONN 4

#include "esp_timer.h"

uint8_t but[301];
//uint8_t butTX[301];

typedef struct{
  uint8_t buf[301];
  uint16_t len;
} Xmsg;

//struct AMessage {
//   uint16_t len;
//   uint8_t but[301];
//};
//struct AMessage *RXMessage;
//struct AMessage *TXMessage;

SemaphoreHandle_t xSemaphore;
esp_timer_handle_t TX_timer;
QueueHandle_t xQueue1;

static void timeout_timer_callback(void* arg)
{
	BaseType_t xTaskWokenByReceive = pdFALSE;
	Xmsg TXmsg;

	if( xQueueReceiveFromISR( xQueue1, ( void * )&TXmsg, &xTaskWokenByReceive) ) {
		if( xSemaphore != NULL )
		{
			// See if we can obtain the semaphore.  If the semaphore is not available
			// wait 10 ticks to see if it becomes free.
			if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE )
			{
				// We were able to obtain the semaphore and can now access the
				// shared resource.
				printf("\n->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->");
				printf("\nLORA TX TICK: %u\n", xTaskGetTickCountFromISR()*portTICK_PERIOD_MS);
				printf("->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->\n");
				lora_send_packet(TXmsg.buf, TXmsg.len);
				// We have finished accessing the shared resource.  Release the
				// semaphore.
				xSemaphoreGive( xSemaphore );
			}
			else
			{
				// We could not obtain the semaphore and can therefore not access
				// the shared resource safely.
			}
		}
	}
//	esp_timer_stop(TX_timer);
	ESP_ERROR_CHECK(esp_timer_start_once(TX_timer, 	1000000));
}

void lora_rx_mode(void)
{
	if( xSemaphore != NULL )
	{
		// See if we can obtain the semaphore.  If the semaphore is not available
		// wait 10 ticks to see if it becomes free.
		if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE )
		{
			// We were able to obtain the semaphore and can now access the
			// shared resource.
			lora_receive();    // put into receive mode
			// We have finished accessing the shared resource.  Release the
			// semaphore.
			xSemaphoreGive( xSemaphore );
		}
		else
		{
			// We could not obtain the semaphore and can therefore not access
			// the shared resource safely.
		}
	}
}
int lora_rxed(void)
{
	int ans=0;
	if( xSemaphore != NULL )
	{
		// See if we can obtain the semaphore.  If the semaphore is not available
		// wait 10 ticks to see if it becomes free.
		if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE )
		{
			// We were able to obtain the semaphore and can now access the
			// shared resource.
			ans = lora_received();	//verify if has received msg
			// We have finished accessing the shared resource.  Release the
			// semaphore.
			xSemaphoreGive( xSemaphore );
		}
		else
		{
			// We could not obtain the semaphore and can therefore not access
			// the shared resource safely.
		}
	}
	return ans;
}
int lora_rx_msg(uint8_t *buf, int size)
{
	int ans=0;
	if( xSemaphore != NULL )
	{
		// See if we can obtain the semaphore.  If the semaphore is not available
		// wait 10 ticks to see if it becomes free.
		if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE )
		{
			// We were able to obtain the semaphore and can now access the
			// shared resource.
			ans = lora_receive_packet(buf, size);	//	Receive the msg
			// We have finished accessing the shared resource.  Release the
			// semaphore.
			xSemaphoreGive( xSemaphore );
		}
		else
		{
			// We could not obtain the semaphore and can therefore not access
			// the shared resource safely.
		}
	}
	return ans;
}
void wifi_tx_msg(void *buffer, uint16_t len)
{
	if( xSemaphore != NULL )
	{
		// See if we can obtain the semaphore.  If the semaphore is not available
		// wait 10 ticks to see if it becomes free.
		if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE )
		{
			// We were able to obtain the semaphore and can now access the
			// shared resource.
			esp_wifi_internal_tx(ESP_IF_WIFI_STA, buffer, len);	//	Transmit the msg through wifi
			// We have finished accessing the shared resource.  Release the
			// semaphore.
			xSemaphoreGive( xSemaphore );
		}
		else
		{
			// We could not obtain the semaphore and can therefore not access
			// the shared resource safely.
		}
	}
}

void task_rx(void *p)
{
	int x=0;
	int i;

	for(;;) {
		lora_rx_mode();
		while(lora_rxed()) {
			x = lora_rx_msg(but, sizeof(but));
			but[x] = 0;
			if(x != 0)
			{
				if(but[12]==0x12 && but[13]==0x34)
				{
					printf("\n<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-");
					printf("\nLORA RX TICK: %u\n", xTaskGetTickCount()*portTICK_PERIOD_MS);
					printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n");
					esp_timer_stop(TX_timer);
					esp_timer_start_once(TX_timer,(500e3 - (x/1200)*1e6));
					wifi_tx_msg((void*)but, x);
					printf("\nReceived:");
					if(but[21]==0)
					{
						for(i=0;i<30;i++)
						{
							printf(" %02X", (unsigned int)but[i]);
						}
					}
					else
					{
						for(i=0;i<22;i++)
						{
							printf(" %02X", (unsigned int)but[i]);
						}
					}
					printf(" ");
					for(;i<x;i++)
					{
						printf("%c", (char)but[i]);
					}
					printf("\n");
				}
				else
					printf("Not NG type message\n");
			}
			else
				printf("Empty message\n");
			lora_rx_mode();
		}
		vTaskDelay(1);
	}
}

// Forward packets from Wi-Fi to Ethernet
static esp_err_t pkt_wifi2eth(void *buffer, uint16_t len, void *eb)
{
	Xmsg RXmsg;
	if(((char*)buffer)[12]==0x12 && ((char*)buffer)[13]==0x34)
	{
		printf("\nWiFi Received(size = %u): ",len);
		if( xSemaphore != NULL )
		{
			// See if we can obtain the semaphore.  If the semaphore is not available
			// wait 10 ticks to see if it becomes free.
			if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE )
			{
				// We were able to obtain the semaphore and can now access the
				// shared resource.
				if( xQueue1 != 0 )
				{
					// Send an packet to the end of queue.  Wait for 10 ticks for space to become
					// available if necessary.
					memcpy(&RXmsg.buf[0], (char*)buffer, len) ;
					RXmsg.len = len;
//					if( xQueueSendToBackFromISR( xQueue1, ( void * ) &RXMessage, ( TickType_t ) 0 ) != pdPASS ) {
					if( xQueueSendToBack( xQueue1, (void*)&RXmsg, ( TickType_t ) 0 ) != pdPASS ) {
						printf("MSG QUEUE FAILED");
						// Failed to post the message.
					}
				}
				//					lora_send_packet((uint8_t*)buffer, len);
				// We have finished accessing the shared resource.  Release the
				// semaphore.
				xSemaphoreGive( xSemaphore );
			}
			else
			{
				// We could not obtain the semaphore and can therefore not access
				// the shared resource safely.
			}
		}
		int i;
		if(((char*)buffer)[21]==0)
		{
			for(i=0;i<30;i++)
			{
				printf(" %02X", (unsigned int)((char*)buffer)[i]);
			}
		}
		else
		{
			for(i=0;i<22;i++)
			{
				printf(" %02X", (unsigned int)((char*)buffer)[i]);
			}
		}
		printf(" ");
		for(;i<len;i++)
		{
			printf("%c", ((char*)buffer)[i]);
		}
		printf("\n");
	}
	else
		printf(".");

    esp_wifi_internal_free_rx_buffer(eb);
    return ESP_OK;
}

// Event handler for Wi-Fi
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    switch (event_id) {
    case SYSTEM_EVENT_STA_START:
        printf("SYSTEM_EVENT_STA_START\r\n");
        break;

    case SYSTEM_EVENT_STA_CONNECTED:
        printf("SYSTEM_EVENT_STA_CONNECTED\r\n");
        wifi_is_connected = true;

        esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, (wifi_rxcb_t)pkt_wifi2eth);
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        printf("SYSTEM_EVENT_STA_GOT_IP\r\n");
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        printf("SYSTEM_EVENT_STA_DISCONNECTED\r\n");
        wifi_is_connected = false;
        esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, NULL);
        esp_wifi_connect();
        break;
    case WIFI_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "Wi-Fi AP got a station connected");
        s_sta_is_connected = true;
        esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_AP, pkt_wifi2eth);
        break;
    case WIFI_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "Wi-Fi AP got a station disconnected");
        s_sta_is_connected = false;
        esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_AP, NULL);
        break;
    default:
        break;
    }
}

static void initialize_wifi(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(tcpip_adapter_clear_default_wifi_handlers());
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_config = {
    		.sta = {
    				.ssid = CONFIG_EXAMPLE_WIFI_SSID,
					.password = CONFIG_EXAMPLE_WIFI_PASSWORD
    		},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
    		CONFIG_EXAMPLE_WIFI_SSID, CONFIG_EXAMPLE_WIFI_PASSWORD);
}

void app_main()
{
	const esp_timer_create_args_t timeout_timer_args = {
			.callback = &timeout_timer_callback,
			/* name is optional, but may help identify the timer when debugging */
			.name = "timeout"
	};

	ESP_ERROR_CHECK(esp_timer_create(&timeout_timer_args, &TX_timer));
	/* The timer has been created but is not running yet */

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	initialize_wifi();
	lora_init();
	lora_set_frequency(915e6);
	lora_set_bandwidth(250e3);
	lora_set_spreading_factor(7);
	lora_enable_crc();

	xSemaphore = xSemaphoreCreateMutex();
	if( xSemaphore == NULL ) {
	    ESP_LOGI(TAG, "N�o consegui criar o sem�foro :(");
	}
	else {
	    ESP_LOGI(TAG, "Consegui criar o sem�foro :)");
	}

	xQueue1 = xQueueCreate( 50, sizeof( Xmsg ) );
	if( xQueue1 == NULL ) {
	    ESP_LOGI(TAG, "N�o consegui criar a fila :(");
	}
	else {
	    ESP_LOGI(TAG, "Consegui criar a fila :)");
	}

	xTaskCreatePinnedToCore(&task_rx, "task_rx", 8192, NULL, 5, NULL, 1);
}
