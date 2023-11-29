#include "IPC_task3.h"

#include "GPIO.h"
#include "serial.h"

#include "task.h"
#include "queue.h"
#include "FreeRTOSConfig.h"

#include <string.h>


#define HELLO_TASK_MS           100
#define BUTTON_TASK_MS          25

#define SERIAL_QUEUE_SIZE       10

static const char* pcHelloMsg = "Hello\r\n";
static const char* pcButtonMsgArray[2][2] = 
{
    {"BTN_0_RISING\r\n", "BTN_0_FALLING\r\n"},
    {"BTN_1_RISING\r\n", "BTN_1_FALLING\r\n"}
};

static const pinX_t xButtonPinArray[2] = {PIN0, PIN1};

static QueueHandle_t xSerialQueue;

static void SerialSendTask(void* pvParameters);
static void ButtonTask(void* pvParameters);
static void HelloTask(void* pvParameters);



BaseType_t IPC_task3_init(void)
{
    BaseType_t xReturn;
    xTaskHandle xSendTaskHandle;
    xTaskHandle xHelloTaskHandle;
    xTaskHandle xButtonTaskHandle0;
    xTaskHandle xButtonTaskHandle1;


    xReturn = xTaskCreate(&SerialSendTask,
                    "Send Task",
                    configMINIMAL_STACK_SIZE,
                    (void*) NULL,
                    tskIDLE_PRIORITY + 1,
                    &xSendTaskHandle);

    if(xReturn == pdPASS)
    {
        xReturn = xTaskCreate(&HelloTask,
                    "Hello Task",
                    configMINIMAL_STACK_SIZE,
                    (void*) NULL,
                    tskIDLE_PRIORITY + 2,
                    &xHelloTaskHandle);
        
        if(xReturn != pdPASS)
        {
            vTaskDelete(xSendTaskHandle);
        }
    }

    if(xReturn == pdPASS)
    {
        xReturn = xTaskCreate(&ButtonTask,
                    "Button0 Task",
                    configMINIMAL_STACK_SIZE,
                    (void*) 0,
                    tskIDLE_PRIORITY + 2,
                    &xButtonTaskHandle0);
        
        if(xReturn != pdPASS)
        {
            vTaskDelete(xSendTaskHandle);
            vTaskDelete(xHelloTaskHandle);
        }
    }

    if(xReturn == pdPASS)
    {
        xReturn = xTaskCreate(&ButtonTask,
                    "Button1 Task",
                    configMINIMAL_STACK_SIZE,
                    (void*) 1,
                    tskIDLE_PRIORITY + 2,
                    &xButtonTaskHandle1);
        
        if(xReturn != pdPASS)
        {
            vTaskDelete(xSendTaskHandle);
            vTaskDelete(xHelloTaskHandle);
            vTaskDelete(xButtonTaskHandle0);
        }
    }

    if(xReturn == pdPASS)
    {
        xSerialQueue = xQueueCreate(SERIAL_QUEUE_SIZE, sizeof(const char*));

        if(xSerialQueue == NULL)
        {
            vTaskDelete(xSendTaskHandle);
            vTaskDelete(xHelloTaskHandle);
            vTaskDelete(xButtonTaskHandle0);
            vTaskDelete(xButtonTaskHandle1);

            xReturn = pdFAIL;
        }
    }

    return xReturn;
}

static void SerialSendTask(void* pvParameters)
{
    (void) pvParameters;
    const char* pcRecvMessage;
    uint8_t uMsgLength;

    for(;;)
    {
        if(xQueueReceive(xSerialQueue, (void*) &pcRecvMessage, portMAX_DELAY) == pdTRUE)
        {
            uMsgLength = strlen(pcRecvMessage) + 1;
            while(vSerialPutString(pcRecvMessage, uMsgLength) != pdTRUE);
        }
    }

    vTaskDelete(NULL);
}

static void HelloTask(void* pvParameters)
{
    (void) pvParameters;

    for(;;)
    {
        xQueueSend(xSerialQueue, (const void*) &pcHelloMsg, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(HELLO_TASK_MS));
    }

    vTaskDelete(NULL);
}

static void ButtonTask(void* pvParameters)
{
    const char* pcRisingMsg;
    const char* pcFallingMsg;
    pinState_t xPinCurrentState, xPinPrevState;
    pinX_t  xPin;
    uint8_t uTaskIndx;

    uTaskIndx = (uint8_t) pvParameters;
    pcRisingMsg = pcButtonMsgArray[uTaskIndx][0];
    pcFallingMsg = pcButtonMsgArray[uTaskIndx][1];
    xPin = xButtonPinArray[uTaskIndx];

    xPinPrevState = GPIO_read(PORT_0, xPin);

    for(;;)
    {
        xPinCurrentState = GPIO_read(PORT_0, xPin);

        if((xPinCurrentState == PIN_IS_HIGH) && (xPinPrevState == PIN_IS_LOW))
        {
            /* Button Rising edge */
            xQueueSend(xSerialQueue, (const void*) &pcRisingMsg, portMAX_DELAY);
        }
        else if((xPinCurrentState == PIN_IS_LOW) && (xPinPrevState == PIN_IS_HIGH))
        {
            /* Button Falling edge */
            xQueueSend(xSerialQueue, (const void*) &pcFallingMsg, portMAX_DELAY);
        }

        xPinPrevState = xPinCurrentState;

        vTaskDelay(pdMS_TO_TICKS(BUTTON_TASK_MS));    
    }

    vTaskDelete(NULL);
}