#include "IPC_task2.h"

#include "serial.h"

#include "task.h"
#include "semphr.h"
#include "FreeRTOSConfig.h"


#define TASK1_MESSAGE       "Hello from Task 1\r\n"
#define TASK2_MESSAGE       "Hello from Task 2\r\n"


#define TASK1_PERIODICITY_MS    100
#define TASK2_PERIODICITY_MS    500

static void vSendTask1(void* pvParameters);
static void vSendTask2(void* pvParameters);


static SemaphoreHandle_t xSerialSem;        /* Mutex to achive mutual execlusive access to Serial */



BaseType_t IPC_task2_init(void)
{
    BaseType_t xReturned;
    xTaskHandle xHandle1;
    xTaskHandle xHandle2;

    xReturned = xTaskCreate(&vSendTask1,
                            "Send Task1",
                            configMINIMAL_STACK_SIZE,
                            (void*) NULL,
                            tskIDLE_PRIORITY + 1,
                            &xHandle1);
    
    if(xReturned == pdPASS)
    {
        xReturned = xTaskCreate(&vSendTask2,
                            "Send Task2",
                            configMINIMAL_STACK_SIZE,
                            (void*) NULL,
                            tskIDLE_PRIORITY + 1,
                            &xHandle2);
        
        if(xReturned != pdPASS)
        {
            vTaskDelete(xHandle1);
        }
    }

    if(xReturned == pdPASS)
    {
        xSerialSem = xSemaphoreCreateMutex();

        if(xSerialSem == NULL)
        {
            vTaskDelete(xHandle1);
            vTaskDelete(xHandle2);
            
            xReturned = pdFAIL;
        }
    }

    return xReturned;
}

static void vSendTask1(void* pvParameters)
{
    uint8_t i;

    (void) pvParameters;

    for(;;)
    {
        /* Obtain Mutex lock of Serial port */
        if(xSemaphoreTake(xSerialSem, portMAX_DELAY) == pdTRUE)
        {
            for(i = 0; i < 10; i++)
            {
                while(vSerialPutString(TASK1_MESSAGE, sizeof(TASK1_MESSAGE)) != pdTRUE);
            }

            xSemaphoreGive(xSerialSem);
        }

        vTaskDelay(pdMS_TO_TICKS(TASK1_PERIODICITY_MS));        /* Block for 100mS */
    }

    vTaskDelete(NULL);
}

static void vSendTask2(void* pvParameters)
{
    uint8_t i;
		uint32_t j;

    (void) pvParameters;

    for(;;)
    {
        if(xSemaphoreTake(xSerialSem, portMAX_DELAY) == pdTRUE)
        {
            for(i = 0; i < 10; i++)
            {
                while(vSerialPutString(TASK2_MESSAGE, sizeof(TASK2_MESSAGE)) != pdTRUE);
                for(j = 0; j < 100000; j++);
            }

            xSemaphoreGive(xSerialSem);
        }

        vTaskDelay(pdMS_TO_TICKS(TASK2_PERIODICITY_MS));
    }
    
    vTaskDelay(NULL);
}