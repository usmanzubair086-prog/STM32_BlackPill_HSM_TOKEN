/**
 *  Created on: Jun 17, 2026
 *      Author:

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
/* Before writing to the source file, first enable the keyboard setting by changing the hexcode found in __ALIGN_BEGIN static uint8_t USBD_HID_CfgDesc[USB_HID_CONFIG_DESC_SIZ] __ALIGN_ = descriptor, change 0x02 to 0x01 to enable keyboard */
/* Also change the #define HID_MOUSE_REPORT_DESC_SIZE                 63U which was 74U at the start so by changing, you are switching it with mouse descriptor and keyboard one is 64 bytes
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usbd_hid.h"
#include "usb_device.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pass_storage.h"
#include "security_vault.h"
#include "entropy_gen.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
void USB_Type_String(const char* str);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

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
  MX_USB_DEVICE_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(500);
  USBD_Start(&hUsbDeviceFS);
  // Mount internal physical Flash cells into RAM mapping array structure
  if (!PassStorage_Init())
  {
        // Catch System Error: Flash cell breakdown verification panic loop
      while (1)
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(100);
        }
    }
  /* USER CODE END 2 */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
      /* USER CODE END WHILE */

      // 1. Detect physical edge trigger on User Button (PA0)
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
      {
          // Debounce Delay (Wait for physical switch contacts to settle)
          HAL_Delay(50);

          // Confirm it's still pressed (Not just electrical line noise)
          if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
          {
              // Turn ON onboard PC13 LED (Active-LOW) to confirm contact acknowledge
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

              uint32_t press_start_time = HAL_GetTick();
              bool is_long_press = false;

              // 2. Track timing while user maintains terminal pressure on the button
              while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
              {
                  // Crossing 2-second limit enters Key Generation Registration loop
                  if ((HAL_GetTick() - press_start_time) > 2000)
                  {
                      is_long_press = true;
                      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Rapid pulsing notification
                      HAL_Delay(100);
                  }
              }

              // Immediately clear LED indicator upon structural release
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
              HAL_Delay(50); // Release edge filtering

              // -----------------------------------------------------------------
              // ROUTE A: SHORT PRESS (< 2s) -> EXTRACT, AES DECRYPT & AUTO-TYPE
              // -----------------------------------------------------------------
              if (!is_long_press)
              {
                  char retrieved_pass[32] = {0};

                  // Fetch data assets and execute matching dynamic block decryption sequence
                  if (Vault_Retrieve_And_Decrypt(0, retrieved_pass))
                  {
                      USB_Type_String("Secure Payload: ");
                      USB_Type_String(retrieved_pass);
                      USB_Type_String("\n");
                  }
                  else
                  {
                      USB_Type_String("[ERROR: Slot 0 Empty or Corrupted]\n");
                  }
              }

              // -----------------------------------------------------------------
              // ROUTE B: LONG PRESS (> 2s) -> SAMPLE ENTROPY, AES ENCRYPT & WRITE
              // -----------------------------------------------------------------
              else
              {
                  char generated_pass[17] = {0};

                  // Drive noise conversion cycles over internal floating pins
                  Entropy_Seed_Engine(&hadc1);
                  Entropy_Generate_Password(generated_pass, 16);

                  // Package and hand the token string data directly down to the vault engine
                  if (Vault_Encrypt_And_Store(0, "MasterSlot", generated_pass))
                  {
                      // Report success over USB Emulation once for physical storage backup
                      USB_Type_String("[SUCCESS] New Dynamic Key Registered: ");
                      USB_Type_String(generated_pass);
                      USB_Type_String("\n");
                  }
                  else
                  {
                      USB_Type_String("[ERROR: Flash Flash Commit Aborted]\n");
                  }
              }
          }
      }
      /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  hadc1.Init.ScanConvMode = DISABLE;
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
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
	  __HAL_RCC_GPIOH_CLK_ENABLE();

	  __HAL_RCC_GPIOA_CLK_ENABLE();

	  /*Configure GPIO pin : PB0 */
	  GPIO_InitStruct.Pin = GPIO_PIN_0;
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = GPIO_PULLUP;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  /* USER CODE BEGIN MX_GPIO_Init_2 */

	  /* USER CODE END MX_GPIO_Init_2 */





}

/* USER CODE BEGIN 4 */

static const uint16_t ascii_to_hid[128] = {
    // Default to 0
    [0 ... 127] = 0x0000,

    // Lowercase Letters
    ['a'] = 0x0004, ['b'] = 0x0005, ['c'] = 0x0006, ['d'] = 0x0007, ['e'] = 0x0008,
    ['f'] = 0x0009, ['g'] = 0x000A, ['h'] = 0x000B, ['i'] = 0x000C, ['j'] = 0x000D,
    ['k'] = 0x000E, ['l'] = 0x000F, ['m'] = 0x0010, ['n'] = 0x0011, ['o'] = 0x0012,
    ['p'] = 0x0013, ['q'] = 0x0014, ['r'] = 0x0015, ['s'] = 0x0016, ['t'] = 0x0017,
    ['u'] = 0x0018, ['v'] = 0x0019, ['w'] = 0x001A, ['x'] = 0x001B, ['y'] = 0x001C,
    ['z'] = 0x001D,

    // Uppercase Letters (Same scan code, but with 0x02 Shift Modifier added)
    ['A'] = 0x0204, ['B'] = 0x0205, ['C'] = 0x0206, ['D'] = 0x0207, ['E'] = 0x0208,
    ['F'] = 0x0209, ['G'] = 0x020A, ['H'] = 0x020B, ['I'] = 0x020C, ['J'] = 0x020D,
    ['K'] = 0x020E, ['L'] = 0x020F, ['M'] = 0x0210, ['N'] = 0x0211, ['O'] = 0x0212,
    ['P'] = 0x0213, ['Q'] = 0x0214, ['R'] = 0x0215, ['S'] = 0x0216, ['T'] = 0x0217,
    ['U'] = 0x0218, ['V'] = 0x0219, ['W'] = 0x021A, ['X'] = 0x021B, ['Y'] = 0x021C,
    ['Z'] = 0x021D,

    // Numbers
    ['1'] = 0x001E, ['2'] = 0x001F, ['3'] = 0x0020, ['4'] = 0x0021, ['5'] = 0x0022,
    ['6'] = 0x0023, ['7'] = 0x0024, ['8'] = 0x0025, ['9'] = 0x0026, ['0'] = 0x0027,

    // Common Symbols (Mapped with Shift 0x02)
    ['!'] = 0x021E, ['@'] = 0x021F, ['#'] = 0x0220, ['$'] = 0x0221, ['%'] = 0x0222,
    ['^'] = 0x0223, ['&'] = 0x0224, ['*'] = 0x0225, ['('] = 0x0226, [')'] = 0x0227,

    // Miscellaneous
    [' '] = 0x002C, ['-'] = 0x002D, ['='] = 0x002E, ['_'] = 0x022D, ['+'] = 0x022E,
};

static uint8_t HID_Send_Safe(uint8_t *report)
{
    uint32_t start_time = HAL_GetTick();

    // Keep trying to send until successful or 50ms has passed
    while (USBD_HID_SendReport(&hUsbDeviceFS, report, 8) != USBD_OK)
    {
        if ((HAL_GetTick() - start_time) > 50)
        {
            return 0; // Transmission failed (timeout)
        }
        HAL_Delay(1); // Wait 1ms before retrying
    }
    return 1; // Transmission successful
}

void USB_Type_String(const char* str)
{
    uint8_t report[8] = {0};

    while (*str)
    {
        uint16_t map_val = ascii_to_hid[(uint8_t)(*str)];

        // Only process characters we have mapped
        if (map_val != 0x0000)
        {
            // Extract the Shift Modifier (High Byte) and Scan Code (Low Byte)
            report[0] = (uint8_t)(map_val >> 8);
            report[2] = (uint8_t)(map_val & 0xFF);

            // Send Key Down safely
            HID_Send_Safe(report);
            HAL_Delay(15);

            // Send Key Up safely
            report[0] = 0x00;
            report[2] = 0x00;
            HID_Send_Safe(report);
            HAL_Delay(15);
        }
        str++;
    }
}
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  __disable_irq();
  while (1)
  {
      // Blink an LED here later if you want a visual panic indicator
  }
  /* USER CODE END Error_Handler_Debug */
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */




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
