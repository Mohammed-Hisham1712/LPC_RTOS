#include "IPC_task1.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "FreeRTOSConfig.h"

#include "GPIO.h"



#define BUTTON_TASK_PERIODICITY_MS      50


static void vLedToggleTask(void* pvParametrs);
static void vButtonTask(void* pvParameters);



static SemaphoreHandle_t xButtonEventSem = NULL;    /* Binary semaphore to synchronize between tasks */


BaseType_t IPC_task1_init(void)
{
    BaseType_t xReturned;
    TaskHandle_t xHandle1;
    TaskHandle_t xHandle2;


    GPIO_write(PORT_0, PIN1, PIN_IS_LOW);

    /* Create Led Toggle task */
    xReturned = xTaskCreate(&vLedToggleTask,
                            "LED Toggle",
                            configMINIMAL_STACK_SIZE,
                            (void*) NULL,
                            tskIDLE_PRIORITY + 2,   // Highest priority in the system
                            &xHandle1);
    
    if(xReturned == pdPASS)
    {
        xReturned = xTaskCreate(&vButtonTask,
                        "Button Task",
                        configMINIMAL_STACK_SIZE,
                        (void*) NULL,
                        tskIDLE_PRIORITY + 1,   // Second highest priority in the system
                        &xHandle2);

        if(xReturned != pdPASS)
        {
            vTaskDelete(xHandle1);               // Delete LED toggle task
        }
    }


    xButtonEventSem = xSemaphoreCreateBinary();

    if(xButtonEventSem == NULL)
    {
        vTaskDelete(xHandle1);
        vTaskDelete(xHandle2);

        xReturned = pdFAIL;
    }

    return xReturned;
}


static void vLedToggleTask(void* pvParameters)
{
    pinState_t ledState;

    ledState = PIN_IS_LOW;
    (void) pvParameters;

    for(;;)
    {
        if(xSemaphoreTake(xButtonEventSem, portMAX_DELAY) == pdTRUE) /* Block indefinitely without a timeout */
        {
            ledState = ledState == PIN_IS_HIGH ? PIN_IS_LOW : PIN_IS_HIGH;
            GPIO_write(PORT_0, PIN1, ledState);
        }
    }

    vTaskDelete(NULL);
}


static void vButtonTask(void* pvParameters)
{
    pinState_t buttonCurrentState;
    pinState_t buttonPrevState;
    
    (void) pvParameters;
    
    buttonPrevState = GPIO_read(PORT_0, PIN0);

    for(;;)
    {
        buttonCurrentState = GPIO_read(PORT_0, PIN0);

        if((buttonCurrentState == PIN_IS_HIGH) && (buttonPrevState == PIN_IS_LOW))
        {
            xSemaphoreGive(xButtonEventSem);       /* Give semaphore when button is released */
        }

        buttonPrevState = buttonCurrentState;

        vTaskDelay(pdMS_TO_TICKS(BUTTON_TASK_PERIODICITY_MS));  /* Block for 50 mS */
    }
}