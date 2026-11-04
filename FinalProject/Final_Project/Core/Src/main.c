/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "lwip/timeouts.h"
#include "ethernetif.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2C_DMA_LENGTH 10

#define I2C_DMA_MAX_LENGTH 256
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c2_rx;
DMA_HandleTypeDef hdma_i2c2_tx;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi4_rx;
DMA_HandleTypeDef hdma_spi4_tx;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart6_rx;
DMA_HandleTypeDef hdma_usart6_tx;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */
volatile uint32_t tim3_ticks = 0;
volatile uint8_t timer_result = 0;
volatile uint8_t hardware_timer_result = 0;
volatile uint8_t hardware_uart_result = 0;
volatile uint8_t hardware_spi_result = 0;
volatile uint8_t hardware_i2c_result = 0;
volatile uint8_t hardware_adc_result = 0;
volatile uint16_t adc_failed_iteration = 0;
volatile uint32_t adc_samples[10] = {0};
volatile uint8_t adc_sample_count = 0;

uint8_t uart_tx_data = 0x55;
uint8_t uart_rx_data = 0x00;
volatile uint8_t uart_result = 0;
volatile uint32_t uart_error_code = 0;
volatile uint32_t uart_rx_state = 0;
uint8_t uart_reverse_tx_data = 0xAA;
uint8_t uart_reverse_rx_data = 0x00;

volatile uint8_t uart_reverse_result = 0;
uint8_t uart_array_tx[10] = {
    0x10, 0x20, 0x30, 0x40, 0x50,
    0x60, 0x70, 0x80, 0x90, 0xA0
};

uint8_t uart_array_rx[10] = {0};

volatile uint8_t uart_array_result = 0;
volatile uint8_t uart_array_rx_done = 0;
volatile uint8_t uart_pattern_result = 0;
volatile uint8_t uart_master_rx_done = 0;
uint8_t uart_master_rx_buffer[255]
    __attribute__((section(".dma_buffer"), aligned(32)));
uint8_t uart_slave_rx_buffer[255]
    __attribute__((section(".dma_buffer"), aligned(32)));

uint8_t spi_tx_data = 0x55;
uint8_t spi_rx_data = 0x00;

volatile uint8_t spi_result = 0;
uint8_t spi_master_tx = 0x55;
uint8_t spi_master_rx = 0x00;

uint8_t spi_slave_tx = 0xAA;
uint8_t spi_slave_rx = 0x00;

volatile uint8_t spi_full_duplex_result = 0;
volatile uint8_t spi_slave_done = 0;
#define SPI_TEST_LENGTH 10
#define SPI_TEST_ITERATIONS 5

#define SPI_DMA_MAX_LENGTH 256


uint8_t spi_dma_master_tx[SPI_DMA_MAX_LENGTH]
    __attribute__((section(".dma_buffer"), aligned(32)));

uint8_t spi_dma_slave_tx[SPI_DMA_MAX_LENGTH]
    __attribute__((section(".dma_buffer"), aligned(32)));

uint8_t spi_dma_master_rx[SPI_DMA_MAX_LENGTH]
    __attribute__((section(".dma_buffer"), aligned(32)));

uint8_t spi_dma_slave_rx[SPI_DMA_MAX_LENGTH]
    __attribute__((section(".dma_buffer"), aligned(32)));


volatile uint8_t spi_dma_master_done = 0;
volatile uint8_t spi_dma_slave_done = 0;
volatile uint8_t spi_dma_result = 0;
volatile uint32_t spi_dma_address = 0;
volatile uint8_t spi_dma_error = 0;

uint8_t i2c_tx_data = 0x55;
uint8_t i2c_rx_data = 0;

volatile uint8_t i2c_rx_done = 0;
volatile uint8_t i2c_result = 0;
volatile uint32_t i2c_error_code = 0;
#define I2C_TEST_LENGTH 10
#define I2C_TEST_ITERATIONS 5

uint8_t i2c_array_tx[I2C_TEST_LENGTH];
uint8_t i2c_array_rx[I2C_TEST_LENGTH];

volatile uint8_t i2c_array_rx_done = 0;
volatile uint8_t i2c_array_result = 0;
volatile uint32_t i2c_array_error_code = 0;
uint8_t i2c_dma_tx[I2C_DMA_MAX_LENGTH]
    __attribute__((section(".dma_buffer"), aligned(32)));

uint8_t i2c_dma_rx[I2C_DMA_MAX_LENGTH]
    __attribute__((section(".dma_buffer"), aligned(32)));
uint8_t i2c_dma_master_rx[I2C_DMA_MAX_LENGTH]
    __attribute__((section(".dma_buffer"), aligned(32)));

volatile uint8_t i2c_dma_rx_done = 0;
volatile uint8_t i2c_dma_result = 0;
volatile uint32_t i2c_dma_error_code = 0;
volatile uint8_t i2c_dma_tx_done = 0;

volatile uint32_t adc_raw_value = 0;
volatile uint32_t adc_voltage_mv = 0;
volatile uint8_t adc_result = 0;
volatile uint32_t adc_readings_debug[5] = {0};

extern struct netif gnetif;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI4_Init(void);
static void MX_I2C2_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
uint8_t TIMER_Test(void);

uint8_t UART_Test(void);
uint8_t UART_Reverse_Test(void);
uint8_t UART_Array_Test(void);
uint8_t UART_Pattern_Test(const uint8_t *pattern,
                          uint8_t length,
                          uint8_t iterations);

uint8_t SPI_Test(void);
uint8_t SPI_FullDuplex_Test(void);
uint8_t SPI_DMA_Test(void);

uint8_t I2C_Test(void);
uint8_t I2C_Array_Test(void);
uint8_t I2C_DMA_Test(void);

uint8_t ADC_Test(void);

uint8_t Hardware_RunTests(uint8_t peripherals,
                          const uint8_t *pattern,
                          uint8_t length,
                          uint8_t iterations);
void UDP_ProcessPendingTest(void);

uint8_t SPI_DMA_Pattern_Test(const uint8_t *pattern,
                             uint8_t length,
                             uint8_t iterations);

uint8_t I2C_DMA_Pattern_Test(const uint8_t *pattern,
                             uint8_t length,
                             uint8_t iterations);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Calculates the CRC32 checksum of a data buffer.
 *
 * @details
 * Implements the standard reflected CRC-32 algorithm
 * using software-based bitwise operations.
 *
 * Uses the following parameters:
 * - Polynomial: 0xEDB88320 (reflected).
 * - Initial CRC value: 0xFFFFFFFF.
 * - Final XOR value: 0xFFFFFFFF.
 * - Processes 8 bits per input byte.
 *
 * The calculated checksum is used to verify data
 * integrity for test patterns larger than 100 bytes.
 *
 * @param data Pointer to the input data buffer.
 * @param length Number of bytes to process.
 *
 * @return Calculated 32-bit CRC checksum.
 */
static uint32_t Calculate_CRC32(const uint8_t *data,
                                uint16_t length)
{
    uint32_t crc = 0xFFFFFFFFU;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 1U)
            {
                crc = (crc >> 1) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFFU;
}


/**
 * @brief Compares expected and received test data.
 *
 * @details
 * Validates the input pointers and data length.
 *
 * For data lengths of 1 to 100 bytes, performs a direct
 * byte-by-byte comparison using memcmp().
 *
 * For data lengths greater than 100 bytes, calculates
 * the CRC32 checksum of each buffer using Calculate_CRC32()
 * and compares the resulting checksums.
 *
 * @param expected Pointer to the expected data buffer.
 * @param actual Pointer to the received data buffer.
 * @param length Number of bytes to compare.
 *
 * @return 0x01 if the comparison passes, 0xFF otherwise.
 */
static uint8_t Compare_Test_Data(const uint8_t *expected,
                                 const uint8_t *actual,
                                 uint16_t length)
{
    if (expected == NULL || actual == NULL || length == 0)
    {
        return 0xFF;
    }

    if (length > 100)
    {
        uint32_t expected_crc =
            Calculate_CRC32(expected, length);

        uint32_t actual_crc =
            Calculate_CRC32(actual, length);

        return (expected_crc == actual_crc) ? 0x01 : 0xFF;
    }

    return (memcmp(expected, actual, length) == 0)
               ? 0x01
               : 0xFF;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  MX_LWIP_Init();
  MX_SPI1_Init();
  MX_SPI4_Init();
  MX_I2C2_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  /*
   * Start TIM3 periodic interrupt.
   * Required for the Timer test.
   */
  if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
  {
      Error_Handler();
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  MX_LWIP_Process();

	  UDP_ProcessPendingTest();

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x7010C0FF;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x7010C0FF;
  hi2c2.Init.OwnAddress1 = 132;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_SLAVE;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 7;
  hspi4.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA1_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI4_CS_GPIO_Port, SPI4_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin SPI4_CS_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|SPI4_CS_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief Handles timer period elapsed interrupts.
 *
 * Called by the HAL driver when a timer period elapses.
 * Checks whether the interrupt originated from TIM3
 * and increments the TIM3 tick counter.
 * The counter is used by TIMER_Test() to verify
 * the accuracy of periodic timer interrupts.
 *
 * @param htim Pointer to the timer handle that triggered the interrupt.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        tim3_ticks++;
    }
}

/**
 * @brief Tests the accuracy of TIM3 periodic interrupts.
 *
 * @details
 * Records the initial TIM3 interrupt counter and system tick.
 * Waits for 1000 milliseconds using HAL_Delay().
 *
 * Calculates the number of TIM3 interrupts and the elapsed
 * system time using HAL_GetTick().
 *
 * Both values must be within the range of 990 to 1010.
 * The expected TIM3 interrupt frequency is approximately 1 kHz.
 *
 * @return 0x01 if the timer test passes, 0xFF otherwise.
 */
uint8_t TIMER_Test(void)
{
    uint32_t start_ticks = tim3_ticks;
    uint32_t start_time = HAL_GetTick();

    HAL_Delay(1000);

    uint32_t elapsed_ticks = tim3_ticks - start_ticks;
    uint32_t elapsed_time = HAL_GetTick() - start_time;

    if (elapsed_ticks >= 990 &&
        elapsed_ticks <= 1010 &&
        elapsed_time >= 990 &&
        elapsed_time <= 1010)
    {
        return 0x01;  // PASS
    }

    return 0xFF;      // FAIL
}

/**
 * @brief Tests basic UART communication between USART2 and USART6.
 *
 * Transmits a single byte from USART2 and receives it on USART6
 * using interrupt-based reception.
 * Verifies that the received byte matches the transmitted byte
 * within a timeout of 100 milliseconds.
 *
 * @return 0x01 if the UART test passes, 0xFF otherwise.
 */
uint8_t UART_Test(void)
{
    HAL_StatusTypeDef status;

    uart_rx_data = 0x00;

    // Start receiving on USART6
    status = HAL_UART_Receive_IT(&huart6, &uart_rx_data, 1);

    if (status != HAL_OK)
    {
        return 0xFF;
    }



    // Send one byte from USART2
    status = HAL_UART_Transmit(&huart2, &uart_tx_data, 1, 100);

    if (status != HAL_OK)
    {
        return 0xFF;
    }

    // Wait for the received byte, with timeout
    uint32_t start_time = HAL_GetTick();

    while (uart_rx_data != uart_tx_data)
    {
    	if ((HAL_GetTick() - start_time) >= 100)
    	{
    	    uart_error_code = HAL_UART_GetError(&huart6);
    	    uart_rx_state = HAL_UART_GetState(&huart6);

    	    HAL_UART_AbortReceive(&huart6);
    	    return 0xFF;
    	}
    }

    return 0x01;
}

/**
 * @brief Tests reverse UART communication from USART6 to USART2.
 *
 * Transmits a single byte from USART6 and receives it on USART2
 * using interrupt-based reception.
 * Waits for the receiver to return to the ready state, with a
 * timeout of 100 milliseconds, and compares the received byte
 * with the transmitted byte.
 *
 * @return 0x01 if the UART test passes, 0xFF otherwise.
 */
uint8_t UART_Reverse_Test(void)
{
    HAL_StatusTypeDef status;

    uart_reverse_rx_data = 0x00;

    // Start receiving on USART2
    status = HAL_UART_Receive_IT(&huart2, &uart_reverse_rx_data, 1);

    if (status != HAL_OK)
    {
        return 0xFF;
    }

    // Send one byte from USART6
    status = HAL_UART_Transmit(&huart6, &uart_reverse_tx_data, 1, 100);

    if (status != HAL_OK)
    {
        HAL_UART_AbortReceive(&huart2);
        return 0xFF;
    }

    uint32_t start_time = HAL_GetTick();

    while (HAL_UART_GetState(&huart2) != HAL_UART_STATE_READY)
    {
        if ((HAL_GetTick() - start_time) >= 100)
        {
            HAL_UART_AbortReceive(&huart2);
            return 0xFF;
        }
    }

    if (uart_reverse_rx_data == uart_reverse_tx_data)
    {
        return 0x01;
    }

    return 0xFF;
}

 /**
 * @brief Handles UART reception completion callbacks.
 *
 * Called by the HAL driver when a UART reception
 * is completed.
 *
 * Sets the corresponding completion flag for
 * USART6 or USART2.
 *
 * @param huart Pointer to the UART handle.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART6)
    {
        uart_array_rx_done = 1;
    }
    else if (huart->Instance == USART2)
    {
        uart_master_rx_done = 1;
    }
}

/**
 * @brief Tests UART communication using a 10-byte data array.
 *
 * Starts DMA reception on USART6 and transmits a predefined
 * 10-byte array from USART2.
 * Waits for reception completion with a 100-millisecond timeout
 * and compares each received byte with the transmitted data.
 *
 * @return 0x01 if all received bytes match, 0xFF otherwise.
 */
uint8_t UART_Array_Test(void)
{
    uart_array_rx_done = 0;

    memset(uart_array_rx, 0, sizeof(uart_array_rx));

    // Start DMA reception before transmission
    if (HAL_UART_Receive_DMA(&huart6,
                            uart_array_rx,
                            sizeof(uart_array_rx)) != HAL_OK)
    {
        return 0xFF;
    }

    // Transmit 10 bytes
    if (HAL_UART_Transmit(&huart2,
                          uart_array_tx,
                          sizeof(uart_array_tx),
                          100) != HAL_OK)
    {
        HAL_UART_AbortReceive(&huart6);
        return 0xFF;
    }

    uint32_t start_time = HAL_GetTick();

    while (uart_array_rx_done == 0)
    {
        if ((HAL_GetTick() - start_time) >= 100)
        {
            HAL_UART_AbortReceive(&huart6);
            return 0xFF;
        }
    }

    // Compare transmitted and received arrays
    for (uint8_t i = 0; i < 10; i++)
    {
        if (uart_array_tx[i] != uart_array_rx[i])
        {
            return 0xFF;
        }
    }

    return 0x01;
}


/**
 * @brief Tests UART communication using a two-phase data roundtrip.
 *
 * @details
 * USART2 sends the input pattern to USART6.
 * USART6 receives the data using DMA and verifies its integrity.
 *
 * USART6 then sends the received data back to USART2.
 * USART2 receives the returned data using DMA.
 *
 * UART transmission is performed in blocking mode,
 * while reception is performed using DMA.
 *
 * The received data is compared with the original pattern
 * after each transfer. CRC32 comparison is used for
 * patterns larger than 100 bytes.
 *
 * Each reception has a timeout of 1000 milliseconds.
 * The test stops immediately if any iteration fails.
 *
 * @param pattern Pointer to the input test pattern.
 * @param length Number of bytes in the pattern (1-255).
 * @param iterations Number of test repetitions (1-255).
 *
 * @return 0x01 if all iterations pass, 0xFF otherwise.
 */

uint8_t UART_Pattern_Test(const uint8_t *pattern,
                          uint8_t length,
                          uint8_t iterations)
{


    // Validate input parameters
    if (pattern == NULL || length == 0 || iterations == 0)
    {
        return 0xFF;
    }

    for (uint16_t iter = 0; iter < iterations; iter++)
    {
        uart_array_rx_done = 0;
        uart_master_rx_done = 0;

        memset(uart_slave_rx_buffer, 0, length);
        memset(uart_master_rx_buffer, 0, length);

        // Phase 1: USART2 -> USART6

        if (HAL_UART_Receive_DMA(&huart6,
                                 uart_slave_rx_buffer,
                                 length) != HAL_OK)
        {
            return 0xFF;
        }


        if (HAL_UART_Transmit(&huart2,
                              (uint8_t *)pattern,
                              length,
                              1000) != HAL_OK)
        {
            HAL_UART_AbortReceive(&huart6);
            return 0xFF;
        }


        uint32_t start = HAL_GetTick();

        while (uart_array_rx_done == 0)
        {
            if ((HAL_GetTick() - start) >= 1000)
            {
                HAL_UART_AbortReceive(&huart6);
                return 0xFF;
            }
        }


        if (Compare_Test_Data(pattern,
                              uart_slave_rx_buffer,
                              length) != 0x01)
        {
            return 0xFF;
        }

        // Phase 2: USART6 -> USART2

        if (HAL_UART_Receive_DMA(&huart2,
                                 uart_master_rx_buffer,
                                 length) != HAL_OK)
        {
            return 0xFF;
        }


        if (HAL_UART_Transmit(&huart6,
                              uart_slave_rx_buffer,
                              length,
                              1000) != HAL_OK)
        {
            HAL_UART_AbortReceive(&huart2);
            return 0xFF;
        }


        start = HAL_GetTick();

        while (uart_master_rx_done == 0)
        {
            if ((HAL_GetTick() - start) >= 1000)
            {
                HAL_UART_AbortReceive(&huart2);
                return 0xFF;
            }
        }


        if (Compare_Test_Data(pattern,
                              uart_master_rx_buffer,
                              length) != 0x01)
        {
            return 0xFF;
        }
    }


    return 0x01;
}


/**
 * @brief Tests basic SPI communication between SPI1 and SPI4.
 *
 * Configures SPI4 as the slave receiver using interrupt-based
 * reception and transmits a single byte from SPI1 as master.
 * Controls the slave chip-select signal using GPIOB pin 12.
 * Waits for the slave to return to the ready state with a
 * timeout of 100 milliseconds.
 * Compares the received byte with the transmitted byte.
 *
 * @return 0x01 if the SPI test passes, 0xFF otherwise.
 */
uint8_t SPI_Test(void)
{
    spi_rx_data = 0;

    // Make sure the slave is not selected yet
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    // Prepare SPI4 slave to receive one byte
    if (HAL_SPI_Receive_IT(&hspi4, &spi_rx_data, 1) != HAL_OK)
    {
        return 0xFF;
    }

    // Select SPI4 slave (CS = LOW)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

    // Send one byte from SPI1 master
    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(&hspi1, &spi_tx_data, 1, 100);

    // Deselect the slave (CS = HIGH)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    if (status != HAL_OK)
    {
        return 0xFF;
    }

    // Wait for slave reception to finish
    uint32_t start_time = HAL_GetTick();

    while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY)
    {
        if ((HAL_GetTick() - start_time) >= 100)
        {
            HAL_SPI_Abort(&hspi4);
            return 0xFF;
        }
    }

    if (spi_rx_data == spi_tx_data)
    {
        return 0x01;
    }

    return 0xFF;
}

/**
 * @brief Handles SPI transmit and receive completion callbacks.
 *
 * Called by the HAL driver when a full-duplex SPI transfer
 * is completed.
 * Sets the slave completion flags for SPI4 and the master
 * DMA completion flag for SPI1.
 *
 * @param hspi Pointer to the SPI handle that completed the transfer.
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI4)
    {
        spi_slave_done = 1;
        spi_dma_slave_done = 1;
    }

    if (hspi->Instance == SPI1)
    {
        spi_dma_master_done = 1;
    }
}

/**
 * @brief Handles SPI communication errors.
 *
 * Called by the HAL driver when an SPI error occurs.
 * Sets the SPI DMA error flag if the error originates
 * from SPI1 (Master) or SPI4 (Slave).
 * The error flag is checked by the SPI DMA test to
 * detect transfer failures.
 *
 * @param hspi Pointer to the SPI handle that reported the error.
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1 ||
        hspi->Instance == SPI4)
    {
        spi_dma_error = 1;
    }
}

/**
 * @brief Tests full-duplex SPI communication between SPI1 and SPI4.
 *
 * Configures SPI4 as a slave using interrupt-based transmission
 * and reception, while SPI1 operates as the master using
 * blocking full-duplex communication.
 * Controls the slave chip-select signal using GPIOB pin 12.
 * Waits for slave transfer completion with a timeout of
 * 100 milliseconds.
 * Verifies that both devices received the expected byte.
 *
 * @return 0x01 if both directions pass, 0xFF otherwise.
 */
uint8_t SPI_FullDuplex_Test(void)
{
    spi_master_rx = 0;
    spi_slave_rx = 0;
    spi_slave_done = 0;

    // Deselect slave
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    // Prepare slave to transmit and receive
    if (HAL_SPI_TransmitReceive_IT(&hspi4,
                                  &spi_slave_tx,
                                  &spi_slave_rx,
                                  1) != HAL_OK)
    {
        return 0xFF;
    }

    // Select slave
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

    // Master transmits and receives simultaneously
    HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(&hspi1,
                                &spi_master_tx,
                                &spi_master_rx,
                                1,
                                100);

    // Deselect slave
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    if (status != HAL_OK)
    {
        HAL_SPI_Abort(&hspi4);
        return 0xFF;
    }

    // Wait for slave transfer completion
    uint32_t start_time = HAL_GetTick();

    while (spi_slave_done == 0)
    {
        if ((HAL_GetTick() - start_time) >= 100)
        {
            HAL_SPI_Abort(&hspi4);
            return 0xFF;
        }
    }

    // Compare received bytes
    if (spi_master_rx == spi_slave_tx &&
        spi_slave_rx == spi_master_tx)
    {
        return 0x01;
    }

    return 0xFF;
}

/**
 * @brief Tests full-duplex SPI communication using DMA.
 *
 * Performs five test iterations, transferring 10 bytes per
 * iteration between SPI1 (Master) and SPI4 (Slave).
 * Generates different transmit patterns for each iteration
 * and uses DMA for simultaneous transmission and reception.
 * Controls the slave chip-select signal using GPIOB pin 12.
 * Waits for both DMA transfers to complete with a timeout
 * of 1000 milliseconds.
 * Verifies that the received data matches the transmitted
 * data in both directions.
 *
 * @return 0x01 if all iterations pass, 0xFF otherwise.
 */
uint8_t SPI_DMA_Test(void)
{
    for (uint8_t iteration = 0;
         iteration < SPI_TEST_ITERATIONS;
         iteration++)
    {
        // Prepare transmit and receive buffers
        for (uint8_t i = 0; i < SPI_TEST_LENGTH; i++)
        {
            spi_dma_master_tx[i] = 0x10 + i + iteration;
            spi_dma_slave_tx[i]  = 0xA0 + i + iteration;

            spi_dma_master_rx[i] = 0;
            spi_dma_slave_rx[i]  = 0;
        }

        spi_dma_master_done = 0;
        spi_dma_slave_done = 0;

        // Release slave select before preparing SPI4
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        // Start slave DMA first
        if (HAL_SPI_TransmitReceive_DMA(
                &hspi4,
                spi_dma_slave_tx,
                spi_dma_slave_rx,
                SPI_TEST_LENGTH) != HAL_OK)
        {
            return 0xFF;
        }

        // Select slave
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

        // Start master DMA
        if (HAL_SPI_TransmitReceive_DMA(
                &hspi1,
                spi_dma_master_tx,
                spi_dma_master_rx,
                SPI_TEST_LENGTH) != HAL_OK)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
            return 0xFF;
        }

        // Wait for both transfers, with timeout
        uint32_t start = HAL_GetTick();

        while (!spi_dma_master_done || !spi_dma_slave_done)
        {
            if ((HAL_GetTick() - start) > 1000)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                return 0xFF;
            }
        }

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        // Compare received data in both directions
        for (uint8_t i = 0; i < SPI_TEST_LENGTH; i++)
        {
            if (spi_dma_master_rx[i] != spi_dma_slave_tx[i] ||
                spi_dma_slave_rx[i] != spi_dma_master_tx[i])
            {
                return 0xFF;
            }
        }
    }

    return 0x01;
}

/**
 * @brief Handles I2C slave reception completion callbacks.
 *
 * Called by the HAL driver when an I2C slave reception
 * is completed.
 * Checks whether the receiving peripheral is I2C2 and
 * sets the completion flags for single-byte, array,
 * and DMA reception tests.
 *
 * @param hi2c Pointer to the I2C handle that completed reception.
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        i2c_rx_done = 1;
        i2c_array_rx_done = 1;
        i2c_dma_rx_done = 1;
    }
}

/**
 * @brief Handles I2C communication errors.
 *
 * Called by the HAL driver when an I2C error occurs.
 * Checks whether the error originated from I2C1 (Master)
 * or I2C2 (Slave) and stores the HAL error code.
 * The stored error code is used by the I2C DMA test
 * to detect communication failures.
 *
 * @param hi2c Pointer to the I2C handle that reported the error.
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1 ||
        hi2c->Instance == I2C2)
    {
        i2c_dma_error_code = HAL_I2C_GetError(hi2c);
    }
}

/**
 * @brief Tests basic I2C communication between I2C1 and I2C2.
 *
 * Configures I2C2 as a slave receiver using interrupt-based
 * reception and transmits a single byte from I2C1 as master.
 * Uses the slave address 0x42 and waits for reception
 * completion with a timeout of 1000 milliseconds.
 * Compares the received byte with the transmitted byte.
 *
 * @return 0x01 if the I2C test passes, 0xFF otherwise.
 */
uint8_t I2C_Test(void)
{
    i2c_rx_data = 0;
    i2c_rx_done = 0;
    i2c_error_code = 0;

    // Prepare the slave to receive one byte
    if (HAL_I2C_Slave_Receive_IT(&hi2c2, &i2c_rx_data, 1) != HAL_OK)
    {
        i2c_error_code = HAL_I2C_GetError(&hi2c2);
        return 0xFF;
    }

    // Send one byte from master to slave
    if (HAL_I2C_Master_Transmit(&hi2c1, (0x42 << 1),
                                &i2c_tx_data, 1, 1000) != HAL_OK)
    {
        i2c_error_code = HAL_I2C_GetError(&hi2c1);
        return 0xFF;
    }

    // Wait for slave reception to complete
    uint32_t start = HAL_GetTick();

    while (i2c_rx_done == 0)
    {
        if ((HAL_GetTick() - start) > 1000)
        {
            i2c_error_code = HAL_I2C_GetError(&hi2c2);
            return 0xFF;
        }
    }

    // Compare transmitted and received bytes
    if (i2c_rx_data == i2c_tx_data)
    {
        return 0x01;
    }

    return 0xFF;
}

/**
 * @brief Tests I2C communication using data arrays.
 *
 * Performs five iterations of 10-byte data transfers
 * from I2C1 (Master) to I2C2 (Slave).
 * Generates a different data pattern for each iteration
 * and uses interrupt-based reception on the slave.
 * Waits for reception completion with a timeout of
 * 1000 milliseconds per iteration.
 * Compares each received byte with the transmitted data
 * and stops immediately if any iteration fails.
 *
 * @return 0x01 if all iterations pass, 0xFF otherwise.
 */
uint8_t I2C_Array_Test(void)
{
    for (uint8_t iteration = 0;
         iteration < I2C_TEST_ITERATIONS;
         iteration++)
    {
        i2c_array_rx_done = 0;
        i2c_array_error_code = 0;

        // Prepare data for this iteration
        for (uint8_t i = 0; i < I2C_TEST_LENGTH; i++)
        {
            i2c_array_tx[i] = 0x10 + i + iteration;
            i2c_array_rx[i] = 0;
        }

        // Start slave reception before master transmission
        if (HAL_I2C_Slave_Receive_IT(
                &hi2c2, i2c_array_rx, I2C_TEST_LENGTH) != HAL_OK)
        {
            i2c_array_error_code = HAL_I2C_GetError(&hi2c2);
            return 0xFF;
        }

        // Transmit array from master to slave
        if (HAL_I2C_Master_Transmit(
                &hi2c1, (0x42 << 1),
                i2c_array_tx, I2C_TEST_LENGTH, 1000) != HAL_OK)
        {
            i2c_array_error_code = HAL_I2C_GetError(&hi2c1);
            return 0xFF;
        }

        // Wait for slave reception to complete
        uint32_t start = HAL_GetTick();

        while (i2c_array_rx_done == 0)
        {
            if ((HAL_GetTick() - start) > 1000)
            {
                i2c_array_error_code = HAL_I2C_GetError(&hi2c2);
                return 0xFF;
            }
        }

        // Compare every received byte
        for (uint8_t i = 0; i < I2C_TEST_LENGTH; i++)
        {
            if (i2c_array_rx[i] != i2c_array_tx[i])
            {
                return 0xFF;
            }
        }
    }

    return 0x01;
}

/**
 * @brief Tests I2C communication using DMA reception.
 *
 * Generates a predefined 10-byte data pattern and transmits
 * it from I2C1 (Master) to I2C2 (Slave).
 * Uses DMA for slave reception and blocking HAL transmission
 * on the master.
 * Waits for DMA reception completion with a timeout of
 * 1000 milliseconds.
 * Compares the received data with the transmitted pattern.
 *
 * @return 0x01 if all received bytes match, 0xFF otherwise.
 */
uint8_t I2C_DMA_Test(void)
{
    i2c_dma_rx_done = 0;
    i2c_dma_error_code = 0;

    // Prepare test data
    for (uint8_t i = 0; i < I2C_DMA_LENGTH; i++)
    {
        i2c_dma_tx[i] = 0x30 + i;
        i2c_dma_rx[i] = 0;
    }

    // Start slave DMA reception
    if (HAL_I2C_Slave_Receive_DMA(
            &hi2c2, i2c_dma_rx, I2C_DMA_LENGTH) != HAL_OK)
    {
        i2c_dma_error_code = HAL_I2C_GetError(&hi2c2);
        return 0xFF;
    }

    // Master transmits using blocking HAL
    if (HAL_I2C_Master_Transmit(
            &hi2c1, (0x42 << 1),
            i2c_dma_tx, I2C_DMA_LENGTH, 1000) != HAL_OK)
    {
        i2c_dma_error_code = HAL_I2C_GetError(&hi2c1);
        HAL_I2C_DeInit(&hi2c2);
        MX_I2C2_Init();
        return 0xFF;
    }

    // Wait for DMA reception completion
    uint32_t start = HAL_GetTick();

    while (i2c_dma_rx_done == 0)
    {
        if ((HAL_GetTick() - start) > 1000)
        {
            i2c_dma_error_code = HAL_I2C_GetError(&hi2c2);
            HAL_I2C_DeInit(&hi2c2);
            MX_I2C2_Init();
            return 0xFF;
        }
    }

    // Compare transmitted and received data
    for (uint8_t i = 0; i < I2C_DMA_LENGTH; i++)
    {
        if (i2c_dma_rx[i] != i2c_dma_tx[i])
        {
            return 0xFF;
        }
    }

    return 0x01;
}

/**
 * @brief Tests ADC1 conversion and validates the measured value.
 *
 * @details
 * Performs five ADC conversions using polling, with a timeout
 * of 100 milliseconds per conversion.
 *
 * Stores the original readings for debugging and waits
 * 2 milliseconds between consecutive conversions.
 *
 * Sorts the five readings and selects the median value
 * to reduce the influence of measurement noise.
 *
 * Saves the median ADC value and converts it to millivolts
 * using a 3.3V reference and 12-bit resolution.
 *
 * The test passes if the median ADC value is between
 * 3800 and 4095 (inclusive).
 *
 * @return 0x01 if the ADC test passes, 0xFF otherwise.
 */
uint8_t ADC_Test(void)
{
    uint32_t readings[5];
    uint32_t temp;

    for (uint8_t i = 0; i < 5; i++)
    {
        if (HAL_ADC_Start(&hadc1) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return 0xFF;
        }

        if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
        {
            HAL_ADC_Stop(&hadc1);
            return 0xFF;
        }

        readings[i] = HAL_ADC_GetValue(&hadc1);

        // Save original reading before sorting
        adc_readings_debug[i] = readings[i];

        HAL_ADC_Stop(&hadc1);

        HAL_Delay(2);
    }

    // Sort readings
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = i + 1; j < 5; j++)
        {
            if (readings[j] < readings[i])
            {
                temp = readings[i];
                readings[i] = readings[j];
                readings[j] = temp;
            }
        }
    }

    // Median of 5 readings
    adc_raw_value = readings[2];

    if (adc_sample_count < 10)
    {
        adc_samples[adc_sample_count] = adc_raw_value;
        adc_sample_count++;
    }

    adc_voltage_mv = (adc_raw_value * 3300UL) / 4095UL;

    if (adc_raw_value >= 3800 && adc_raw_value <= 4095)
    {
        return 0x01;
    }

    return 0xFF;
}

/**
 * @brief Executes the requested hardware peripheral tests.
 *
 * @details
 * Validates the test parameters and selects the hardware
 * peripherals to test according to the provided bitmask.
 *
 * Supported peripherals:
 * - Bit 0 (0x01): TIM3 timer test.
 * - Bit 1 (0x02): UART roundtrip test.
 * - Bit 2 (0x04): SPI DMA roundtrip test.
 * - Bit 3 (0x08): I2C DMA roundtrip test.
 * - Bit 4 (0x10): ADC conversion test.
 *
 * TIMER and ADC tests are repeated using loops in this function.
 * UART, SPI and I2C receive the requested iteration count
 * and execute their repetitions internally.
 *
 * Communication tests use the supplied data pattern and
 * verify data integrity in both directions.
 *
 * The selected tests are executed in the order:
 * TIMER, UART, SPI, I2C, ADC.
 *
 * Execution stops immediately if any test fails.
 * For ADC failures, the failed iteration number is stored
 * in adc_failed_iteration.
 *
 * @param peripherals Bitmask specifying the peripherals to test.
 * @param pattern Pointer to the communication test pattern.
 * @param length Number of bytes in the test pattern.
 * @param iterations Number of repetitions for each selected test.
 *
 * @return 0x01 if all selected tests pass, 0xFF otherwise.
 */
uint8_t Hardware_RunTests(uint8_t peripherals,
                          const uint8_t *pattern,
                          uint8_t length,
                          uint8_t iterations)
{
    // Validate request parameters
    if (peripherals == 0 ||
        (peripherals & 0xE0) != 0 ||
        iterations == 0)
    {
        return 0xFF;
    }

    // TIMER Test - bit 0 (0x01)
    if ((peripherals & 0x01) != 0)
    {
        for (uint16_t i = 0; i < iterations; i++)
        {
            if (TIMER_Test() != 0x01)
            {
                return 0xFF;
            }
        }
    }

    // UART Test - bit 1 (0x02)
    if ((peripherals & 0x02) != 0)
    {
        if (length == 0 || pattern == NULL)
        {
            return 0xFF;
        }

        if (UART_Pattern_Test(pattern, length, iterations) != 0x01)
        {
            return 0xFF;
        }
    }

    // SPI DMA Test - bit 2 (0x04)
    if ((peripherals & 0x04) != 0)
    {
        if (length == 0 || pattern == NULL)
        {
            return 0xFF;
        }

        if (SPI_DMA_Pattern_Test(pattern, length, iterations) != 0x01)
        {
            return 0xFF;
        }
    }

    // I2C DMA Test - bit 3 (0x08)
    if ((peripherals & 0x08) != 0)
    {
        if (length == 0 || pattern == NULL)
        {
            return 0xFF;
        }

        if (I2C_DMA_Pattern_Test(pattern, length, iterations) != 0x01)
        {
            return 0xFF;
        }
    }

    // ADC Test - bit 4 (0x10)
    if ((peripherals & 0x10) != 0)
    {
        adc_failed_iteration = 0;

        for (uint16_t i = 0; i < iterations; i++)
        {
            if (ADC_Test() != 0x01)
            {
                adc_failed_iteration = i + 1;
                return 0xFF;
            }

            // Small delay between ADC measurements
            HAL_Delay(10);
        }
    }

    return 0x01; // All requested tests passed
}

/**
 * @brief Tests SPI communication using DMA in a roundtrip operation.
 *
 * @details
 * SPI1 operates as Master and SPI4 as Slave.
 * The Master sends the input pattern to the Slave.
 * The Slave then transmits the received data back to the Master
 * in a second DMA transfer.
 *
 * The returned data is compared with the original pattern.
 * CRC32 comparison is used for patterns larger than 100 bytes.
 * The test stops immediately if any iteration fails.
 *
 * GPIOB pin 12 controls the SPI Slave chip-select signal.
 * Each DMA transfer has a timeout of 1000 milliseconds.
 *
 * @param pattern Pointer to the input test pattern.
 * @param length Number of bytes in the pattern (1-255).
 * @param iterations Number of test repetitions (1-255).
 *
 * @return 0x01 if all iterations pass, 0xFF otherwise.
 */
uint8_t SPI_DMA_Pattern_Test(const uint8_t *pattern,
                             uint8_t length,
                             uint8_t iterations)
{

    if (pattern == NULL || length == 0 || iterations == 0)
    {
        return 0xFF;
    }

    for (uint16_t iteration = 0; iteration < iterations; iteration++)
    {
        /* Prepare buffers for the first transfer */
        for (uint16_t i = 0; i < length; i++)
        {
            spi_dma_master_tx[i] = pattern[i];
            spi_dma_slave_tx[i] = 0x00;

            spi_dma_master_rx[i] = 0;
            spi_dma_slave_rx[i] = 0;
        }

        spi_dma_master_done = 0;
        spi_dma_slave_done = 0;
        spi_dma_error = 0;

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        /* First transfer: Master -> Slave */
        if (HAL_SPI_TransmitReceive_DMA(
                &hspi4,
                spi_dma_slave_tx,
                spi_dma_slave_rx,
                length) != HAL_OK)
        {
            return 0xFF;
        }

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

        if (HAL_SPI_TransmitReceive_DMA(
                &hspi1,
                spi_dma_master_tx,
                spi_dma_master_rx,
                length) != HAL_OK)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
            HAL_SPI_Abort(&hspi4);
            return 0xFF;
        }

        uint32_t start = HAL_GetTick();

        while (!spi_dma_master_done || !spi_dma_slave_done)
        {
            if (spi_dma_error ||
                (HAL_GetTick() - start) > 1000)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                HAL_SPI_Abort(&hspi1);
                HAL_SPI_Abort(&hspi4);
                return 0xFF;
            }
        }

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        if (spi_dma_error)
        {
            return 0xFF;
        }



        /* Verify data received by the Slave */
        if (Compare_Test_Data(pattern,
                              spi_dma_slave_rx,
                              length) != 0x01)
        {
            return 0xFF;
        }



        /*
         * Second transfer:
         * Slave sends back the data received
         * during the first transfer.
         */
        for (uint16_t i = 0; i < length; i++)
        {
            spi_dma_slave_tx[i] = spi_dma_slave_rx[i];
            spi_dma_master_tx[i] = 0x00;

            spi_dma_master_rx[i] = 0;
        }

        spi_dma_master_done = 0;
        spi_dma_slave_done = 0;
        spi_dma_error = 0;

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        if (HAL_SPI_TransmitReceive_DMA(
                &hspi4,
                spi_dma_slave_tx,
                spi_dma_slave_rx,
                length) != HAL_OK)
        {
            return 0xFF;
        }

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

        if (HAL_SPI_TransmitReceive_DMA(
                &hspi1,
                spi_dma_master_tx,
                spi_dma_master_rx,
                length) != HAL_OK)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
            HAL_SPI_Abort(&hspi4);
            return 0xFF;
        }

        start = HAL_GetTick();

        while (!spi_dma_master_done || !spi_dma_slave_done)
        {
            if (spi_dma_error ||
                (HAL_GetTick() - start) > 1000)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                HAL_SPI_Abort(&hspi1);
                HAL_SPI_Abort(&hspi4);
                return 0xFF;
            }
        }

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        if (spi_dma_error)
        {
            return 0xFF;
        }



        /* Verify the actual roundtrip */
        if (Compare_Test_Data(pattern,
                              spi_dma_master_rx,
                              length) != 0x01)
        {
            return 0xFF;
        }
    }



    return 0x01;
}


/**
 * @brief Tests I2C communication using a two-phase data roundtrip.
 *
 * @details
 * I2C1 operates as Master and I2C2 as Slave (address 0x42).
 *
 * In the first phase, I2C1 transmits the input pattern
 * to I2C2, which receives the data using DMA.
 *
 * In the second phase, I2C2 transmits the received data
 * back using DMA, while I2C1 receives it in blocking mode.
 *
 * The received data is compared with the original pattern
 * after each phase. CRC32 comparison is used for patterns
 * larger than 100 bytes.
 *
 * Each iteration includes timeout and error checks.
 * If any operation fails, the test stops and the
 * I2C2 peripheral is reinitialized.
 *
 * @param pattern Pointer to the input test pattern.
 * @param length Number of bytes to transfer
 *               (1 to I2C_DMA_MAX_LENGTH).
 * @param iterations Number of test repetitions (1-255).
 *
 * @return 0x01 if all iterations pass, 0xFF otherwise.
 */
uint8_t I2C_DMA_Pattern_Test(const uint8_t *pattern,
                             uint8_t length,
                             uint8_t iterations)
{
    if (pattern == NULL || length == 0 || iterations == 0 ||
        length > I2C_DMA_MAX_LENGTH)
    {
        return 0xFF;
    }

    for (uint16_t iteration = 0; iteration < iterations; iteration++)
    {
        i2c_dma_rx_done = 0;
        i2c_dma_tx_done = 0;
        i2c_dma_error_code = 0;

        for (uint16_t i = 0; i < length; i++)
        {
            i2c_dma_tx[i] = pattern[i];
            i2c_dma_rx[i] = 0;
            i2c_dma_master_rx[i] = 0;
        }

        // Phase 1: Master -> Slave
        if (HAL_I2C_Slave_Receive_DMA(&hi2c2,
                                      i2c_dma_rx,
                                      length) != HAL_OK)
        {
            goto i2c_test_fail;
        }

        if (HAL_I2C_Master_Transmit(&hi2c1,
                                    (0x42 << 1),
                                    i2c_dma_tx,
                                    length,
                                    1000) != HAL_OK)
        {
            goto i2c_test_fail;
        }

        uint32_t start = HAL_GetTick();

        while (i2c_dma_rx_done == 0)
        {
            if (i2c_dma_error_code != 0 ||
                (HAL_GetTick() - start) > 1000)
            {
                goto i2c_test_fail;
            }
        }

        if (Compare_Test_Data(pattern, i2c_dma_rx, length) != 0x01)
        {
            goto i2c_test_fail;
        }

        // Phase 2: Slave -> Master
        if (HAL_I2C_Slave_Transmit_DMA(&hi2c2,
                                       i2c_dma_rx,
                                       length) != HAL_OK)
        {
            goto i2c_test_fail;
        }

        if (HAL_I2C_Master_Receive(&hi2c1,
                                   (0x42 << 1),
                                   i2c_dma_master_rx,
                                   length,
                                   1000) != HAL_OK)
        {
            goto i2c_test_fail;
        }

        start = HAL_GetTick();

        while (i2c_dma_tx_done == 0)
        {
            if (i2c_dma_error_code != 0 ||
                (HAL_GetTick() - start) > 1000)
            {
                goto i2c_test_fail;
            }
        }

        if (Compare_Test_Data(pattern, i2c_dma_master_rx, length) != 0x01)
        {
            goto i2c_test_fail;
        }
    }

    return 0x01;

i2c_test_fail:
    HAL_I2C_DeInit(&hi2c2);
    MX_I2C2_Init();
    return 0xFF;
}


/**
 * @brief Handles I2C slave transmission completion.
 *
 * Called when the I2C2 slave finishes transmitting
 * data to the I2C1 master.
 *
 * @param hi2c Pointer to the I2C handle.
 */
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        i2c_dma_tx_done = 1;
    }
}


/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
