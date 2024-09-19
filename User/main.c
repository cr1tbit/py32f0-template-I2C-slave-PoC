/***
 * Demo: PoC I2C slave
 * 
 * This firmware serves as an Proof-of-concept PCF8574 emulation.
 * Looking at jlc parts inventory, the cheapest available I2C io expander is atleast 
 * 2-3x more expensive (CH423S, 28c@25u), compared to cheapest PY32F002 (10c@50u)
 * 
 * Remember, PY32 is an powerful machine, it can also serve as and I2C ADC, charlieplexing
 * controller, or could even serve as I2C-controlled programmable DC-DC converter.
 * 
 * The code is very messy for now, I2C slave operation is apparently super-hard, and not
 * well documented. You can have a look, how much developers are struggling with this, by
 * googling "AddrMatchCode" (which I still have no idea what is it for).
 * 
 * The biggest breakthrtough came from a series of articles below. They're
 * targeting STM32, but Puya apparently stole all of ST's HAL code, so it seems compatible.
 * https://controllerstech.com/stm32-as-i2c-slave-part-1/
 * 
 * In summary - the purpose of this code is to handle both transmit and receive operations
 * without completely freezing the I2C rail. Everything else is a bonus feature from my
 * perspective :D
 * 
 * I'm going to continue working on this, I have a lot of plans for this little smart IC.
  PF1     ------> I2C1_SCL
  PF0     ------> I2C1_SDA
  PA2     ------> USART1_TX (logging)
 */

#include "py32f0xx_hal_dma.h"
#include "py32f0xx_hal_i2c.h"
#include "py32f0xx_bsp_printf.h"

#include "charlie.h"

// this print will slow the I2C read operation by a lot, but ESP32 should 
// handle this well. 
// #define DEBUG_ISR_PRINT(...) printf(__VA_ARGS__); fflush(stdout);
#define DEBUG_ISR_PRINT(...) 

I2C_HandleTypeDef I2cHandle;
void APP_ErrorHandler(void);
static void APP_I2C_Config(void);

#define I2C_ADDRESS 0xA0 //0x50
uint8_t aRxBuffer[2] = {0, 0};

uint8_t main_opCpltFlag = 0;
uint8_t main_receivedFlag = 0;

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
  UNUSED(AddrMatchCode);// I have no idea what this does, maybe it works with some other I2C setup
  
  if(TransferDirection == I2C_DIRECTION_TRANSMIT){
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, &aRxBuffer[0], 2, I2C_FIRST_AND_LAST_FRAME);
    main_receivedFlag = 1;
  } else {
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &aRxBuffer[0], 2, I2C_FIRST_AND_LAST_FRAME);
  }

  DEBUG_ISR_PRINT("\nmaster %s \n\r", 
    ( TransferDirection == I2C_DIRECTION_TRANSMIT )? "write" : "read");
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  uint32_t errorcode = HAL_I2C_GetError(hi2c);

  if (errorcode == HAL_I2C_ERROR_AF)
  {
    // triggered when master tries to read/write more bytes than expected.
    // for now this must be triggered for each transaction, as otherwise
    // the I2C periph hangs. I'll deal with this at some point.
    DEBUG_ISR_PRINT("Error callback: AF\n\r");
  } else if (errorcode == HAL_I2C_ERROR_BERR)
  {
    DEBUG_ISR_PRINT("Error callback: BERR\n\r");
  }

  HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) 
{ 
  // This callback is actually never called
  // leaving some spam if someone wants to experiment
  // memset( aRxBuffer, 0, RXBUFFERSIZE);
  // Rxcounter++;
  // HAL_I2C_Slave_Seq_Receive_IT(hi2c, &aRxBuffer[0], 1, I2C_LAST_FRAME);
  // HAL_I2C_EnableListen_IT(hi2c);
  DEBUG_ISR_PRINT("rx complete %d\n\r", aRxBuffer[0]);
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  // TODO: for some reason, I have to try sending another byte, or the whole
  // i2c periph hangs. This call will result int HAL_I2C_ERROR_AF triggered
  // in HAL_I2C_ErrorCallback, which is kinda fine.

  // there are 3 variants:
  // 1. try send another byte - works and results in HAL_I2C_ERROR_AF
  HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &aRxBuffer[0], 1, I2C_LAST_FRAME);

  // 2. enable listen - hangs
  // HAL_I2C_EnableListen_IT(hi2c); // maybe some day

  // 3. do nothing - hangs
  
  DEBUG_ISR_PRINT("tx complete\n\r");
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{ 
  DEBUG_ISR_PRINT("listen callback\n\r");
  main_opCpltFlag = 1;
  HAL_I2C_EnableListen_IT(hi2c);
}

#define PIN_LED1 GPIO_PIN_5
#define PIN_LED2 GPIO_PIN_6
#define PIN_LED3 GPIO_PIN_8
#define PIN_LED4 GPIO_PIN_4
#define PIN_LED5 GPIO_PIN_3
#define PIN_LED6 GPIO_PIN_2

void charlieplexInit(void)
{
  // 8, 6, 5, 4
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6| GPIO_PIN_8| GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);


  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


int main(void)
{
  HAL_Init();
  charlieplexInit();
  
  segment_t segments[4] = {
    {0, 1, 2, 3, 4, 5, 6},
    {7, 8, 9, 10, 11, 12, 13},
    {14, 15, 16, 17, 18, 19, 20},
    {21, 22, 23, 24, 25, 26, 27}
  };
  charlie_t charlie = {
    .pins = (uint16_t[]){
      GPIO_PIN_1, GPIO_PIN_4, GPIO_PIN_5, 
      GPIO_PIN_6, GPIO_PIN_8, GPIO_PIN_12},
    .pinCount = 6
  };

  charlieInit(&charlie);
  // for (int i = 0; i < 4; i++){
  //     setSevenSegment(&charlie, &segments[i], i);
  //     // drawSevenSegment(&charlie, &segments[i]);
  // }

  BSP_USART_Config();
  printf("SystemClk is:%ld\r\n", SystemCoreClock);
  fflush(stdout);
  HAL_Delay(1000);

  int number = 10;

  // for (int i = 0; i < 4; i++){
  //     setSevenSegment(&charlie, &segments[i], -1);
  //     // drawSevenSegment(&charlie, &segments[i]);
  // }
  
  while(1){
    // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    // // HAL_Delay(1);
    // //delay for 1us
    // printf("d");
    // fflush(stdout);
    
    // for (int i = 0; i < 4; i++){
    //     setSevenSegment(&charlie, &segments[i], number%10);
    //     // drawSevenSegment(&charlie, &segments[i]);
    // }
    number++;
    if (number > 30) number = 8;
    printf("number %d\n\r", number);
    for (int i = 0; i < 50; i++)
    {
      // charliePrint(&charlie);
      charlieClear(&charlie);
      charlieSetPixel(&charlie, 0, number, 1);
      charlieSetPixel(&charlie, 0, number+1, 1);
      charlieSetPixel(&charlie, 0, number+2, 1);
      charlieRender(&charlie,false);
    }
  }

  APP_I2C_Config();

  HAL_I2C_EnableListen_IT(&I2cHandle);
  printf("listen enabled\n\r");
  while(1){
    if(main_opCpltFlag){
      if (main_receivedFlag)
      {
        printf("got data %02x\n\r", aRxBuffer[0]);
        fflush(stdout);
        main_receivedFlag = 0;
      } else {
        printf("transmit complete\n\r");
        fflush(stdout);
      }
      main_opCpltFlag = 0;
    }
  }
}

static void APP_I2C_Config(void)
{
  I2cHandle.Instance             = I2C;
  I2cHandle.Init.ClockSpeed      = 400000;        // 100KHz ~ 400KHz
  I2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE_16_9;
  I2cHandle.Init.OwnAddress1     = I2C_ADDRESS;
  I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&I2cHandle) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

void APP_ErrorHandler(void)
{
  printf("errord\n");
  fflush(stdout);
  while (1);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Export assert error source and line number
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  while (1);
}
#endif /* USE_FULL_ASSERT */
