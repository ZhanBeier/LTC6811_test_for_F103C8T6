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
#include "adc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "LTC6804-1.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TOTAL_IC 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t tx_cfg[1][6] = {};

void init_cfg(void) {
    int i;

    for (i = 0; i < TOTAL_IC; i++) {
        tx_cfg[i][0] = 0xFE; // GPIO下拉关断 | REFON=1 | DTEN=1 | ADCOPT=0
        tx_cfg[i][1] = 0x4C; // VUV[7:0] = 0x4C
        tx_cfg[i][2] = 0x18; // VOV[3:0]=0x1, VUV[11:8]=0x8
        tx_cfg[i][3] = 0xA4; // VOV[11:4] = 0xA4
        tx_cfg[i][4] = 0x00; // DCC[8:1] = 0x00 (不放电)
        tx_cfg[i][5] = 0xF0; // DCTO[3:0]=0xF (120min), DCC[12:9]=0
    }
}

void MY_LTC6804_rdcv_reg(uint8_t reg, //Determines which cell voltage register is read back 确定读回哪个单元电压寄存器
                         uint8_t total_ic, //the number of ICs in the
                         uint8_t *data //An array of the unparsed cell codes  未解析单元格代码的数组
) {
    const uint8_t REG_LEN = 8; //number of bytes in each ICs register + 2 bytes for the PEC  每个ICs寄存器的字节数+ 2字节的PEC
    uint8_t cmd[4];
    uint16_t cmd_pec;

    //1
    if (reg == 1) //1: RDCVA
    {
        cmd[1] = 0x04;
        cmd[0] = 0x00;
    } else if (reg == 2) //2: RDCVB
    {
        cmd[1] = 0x06;
        cmd[0] = 0x00;
    } else if (reg == 3) //3: RDCVC
    {
        cmd[1] = 0x08;
        cmd[0] = 0x00;
    } else if (reg == 4) //4: RDCVD
    {
        cmd[1] = 0x0A;
        cmd[0] = 0x00;
    }

    //2
    cmd_pec = pec15_calc(2, cmd);
    cmd[2] = (uint8_t) (cmd_pec >> 8);
    cmd[3] = (uint8_t) (cmd_pec);

    //3
    wakeup_idle(); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.
    wakeup_idle();
    wakeup_idle();
    wakeup_idle();
    wakeup_idle();
    //4
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4, GPIO_PIN_RESET);
    spi_write_read(cmd, 4, data, (REG_LEN * total_ic));
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4, GPIO_PIN_SET);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
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
    MX_ADC1_Init();
    MX_USART1_UART_Init();
    MX_SPI1_Init();
    /* USER CODE BEGIN 2 */
    LTC6804_initialize();
    wakeup_sleep();
    volatile uint16_t cell_voltage[3][12] = {0}, temp = 0;
    volatile uint8_t origin_data[24] = {}, auxregdata[16] = {};

    char output_buf[256] = "";
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        wakeup_sleep();
        LTC6804_wrcfg(2, tx_cfg);
        HAL_Delay(1);
        LTC6804_adcvax();
        HAL_Delay(20);
        for (int i = 1; i < 5; ++i) {
            LTC6804_rdcv_reg(i, 3, origin_data);
            for (int j = 0; j < 3; j++) {
                cell_voltage[0][i * 3 - 3 + j] = origin_data[2 * j] + (origin_data[2 * j + 1] << 8);
                /*
                 sniprintf(output_buf, 32, "CELL%2d %.4f\n", i * 3 - 2 + j,
                          (float) cell_voltage[0][i * 3 - 3 + j] / 10000);
                HAL_UART_Transmit(&huart1, (uint8_t *) output_buf, 16, 10);
                 */
            }
            for (int j = 4; j < 7; j++) {
                cell_voltage[1][i * 3 - 7 + j] = origin_data[2 * j] + (origin_data[2 * j + 1] << 8);
                /*
                 sniprintf(output_buf, 32, "CELL%2d %.4f\n", i * 3 - 2 + j,
                          (float) cell_voltage[0][i * 3 - 3 + j] / 10000);
                HAL_UART_Transmit(&huart1, (uint8_t *) output_buf, 16, 10);
                 */
            }
            for (int j = 8; j < 11; j++) {
                cell_voltage[2][i * 3 - 11 + j] = origin_data[2 * j] + (origin_data[2 * j + 1] << 8);
                /*
                 sniprintf(output_buf, 32, "CELL%2d %.4f\n", i * 3 - 2 + j,
                          (float) cell_voltage[0][i * 3 - 3 + j] / 10000);
                HAL_UART_Transmit(&huart1, (uint8_t *) output_buf, 16, 10);
                 */
            }
            uint16_t pec_2 = pec15_calc(6, origin_data + 8);
            uint8_t pec = (((pec_2 & 0xff00) >> 8) == origin_data[14]) && ((pec_2 & 0x00ff) == origin_data[15]);

            HAL_Delay(10);
        }
        //LTC6804_adax();
        HAL_Delay(10);
        LTC6804_rdaux_reg(1, 2, auxregdata);
        //LTC6804_rdaux_reg(2, 1, auxregdata);
        temp = auxregdata[0] | (auxregdata[1] << 8);

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        HAL_Delay(50);
        HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
