/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define I2C_ADDRESS 			0xC8 	// Endereço I2C do ATSHA204A
#define SERIAL_NUMBER_SIZE 		9 		// Tamanho do número de série do ATSHA204A
#define KEY_SIZE 				32 		// Tamanho da chave criptográfica gerada
#define ZONE_ACCESS_CONFIG_SIZE 4 		// Tamanho da zona de configuração
#define PUBLIC_KEY_SIZE 		64 		// Tamanho da chave pública gerada


// Definir os códigos de comando do ATSHA204A
#define COMMAND_CHECKMAC        0x28
#define COMMAND_DERIVE_KEY      0x1C
#define COMMAND_INFO            0x30
#define COMMAND_GENKEY          0x40
#define COMMAND_GENDIG          0x15
#define COMMAND_LOCK            0x17
#define COMMAND_MAC             0x08
#define COMMAND_NONCE           0x16
#define COMMAND_PAUSE           0x01
#define COMMAND_RANDOM          0x1B
#define COMMAND_READ            0x02
#define COMMAND_SHA             0x47
#define COMMAND_UPDATE_EXTRA    0x20
#define COMMAND_WRITE           0x12

// Definir os códigos de zonas do ATSHA204A
#define ZONE_CONFIG 0x00 // Zona de configuração
#define ZONE_DATA 0x02 // Zona de dados

// Definir as configurações de slot do ATSHA204A
#define ATSHA204A_SLOT_CONFIG_SECRET      	0x03 // Configuração de slot para chave secreta
#define ATSHA204A_SLOT_CONFIG_WRITE_ONLY  	0x06 // Configuração de slot para escrita somente

// Definir as configurações de bloqueio de zona do ATSHA204A
#define ATSHA204A_ZONE_LOCK_CONFIG_LOCKED 	0x55 // Zona bloqueada
#define ATSHA204A_ZONE_LOCK_CONFIG_UNLOCKED 0x00 // Zona desbloqueada

/*
void sha204c_calculate_crc(uint8_t length, uint8_t *data, uint8_t *crc) {
	uint8_t counter;
	uint16_t crc_register = 0;
	uint16_t polynom = 0x8005;
	uint8_t shift_register;
	uint8_t data_bit, crc_bit;

	for (counter = 0; counter < length; counter++) {
	  for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
		 data_bit = (data[counter] & shift_register) ? 1 : 0;
		 crc_bit = crc_register >> 15;
		 crc_register <<= 1;
		 if (data_bit != crc_bit)
			crc_register ^= polynom;
	  }
	}
	crc[0] = (uint8_t) (crc_register & 0x00FF);
	crc[1] = (uint8_t) (crc_register >> 8);
}
*/
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	HAL_StatusTypeDef ret1;
	HAL_StatusTypeDef ret2;
	HAL_StatusTypeDef ret3;
	HAL_StatusTypeDef ret4;
	HAL_StatusTypeDef ret5;
	HAL_StatusTypeDef ret6;
	uint8_t end = 0x64;
	uint32_t saida;
	uint32_t saida1;
	uint32_t saida2;
	uint32_t saida3;
	uint32_t saida4;
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
  MX_I2C1_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	    // command packet: 8.5.1
	    // 6.2.1: Word Address Value: COMMAND == 0x03
	    // read command: 8.5.15
	    // read configuration zone: {COMMAND, COUNT, OPCODE, ZONE, ADDRESS_1, ADDRESS_2, CRC_LSB, CRC_MSB}
	    // read configuration zone: {  0x03,    0x07, 0x02, 0x00,      0x00,      0x00,      0xB2,    0x7E}
	    // CRC-16 Polinomial: 0x8005: includes COUNT, OPCODE, ZONE, ADDRESS_1, ADDRESS_2, CRC_LSB, CRC_MSB (does not include COMMAND)
	    // CRC https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
	    // Zone encoding table 8-5

	    // wake-up
	    uint8_t data = 0;
	    ret1 = HAL_I2C_Master_Receive(&hi2c2, 0xF7, &data, sizeof(data), 1000); // Ver onde fala do 0XFE
	    HAL_Delay(10); // 2.5 ms para acordar; 45 ms para entrar em sleep
	    // first read: 0 byte read - should receive an ACK
	    ret2 = HAL_I2C_Master_Receive(&hi2c2, 0xC8, &data, 1, 1000);

	    // LEITURA SERIAL NUMBER
	    // SHA204_READ = 0x03
	    uint8_t readCommand[10] = {0x03, 0x07, 0x02, 0x80, 0x00, 0x00, 0x09, 0xAD};
	    uint8_t reply[4];

	    HAL_Delay(5);
	    ret3 = HAL_I2C_Master_Transmit(&hi2c2, 0xC8, &data, sizeof(data), 1000); // Tem que enviar 1 byte
	    HAL_Delay(5);
	    saida1 = HAL_I2C_Master_Receive(&hi2c2, 0xC8, reply, 4, 1000); // tem que receber 0x04 0x11 0x33 0x43
	    HAL_Delay(5);
	    uint8_t reply1[32];
	   // ret4 = HAL_I2C_Master_Transmit(&hi2c2, 0xC8, readCommand, 8, 1000); // enviar o comando de leitura
	    HAL_I2C_Master_Transmit(&hi2c2, 0xC8, readCommand, 8, 1000); // enviar o comando de leitura
	    HAL_Delay(5);
	    saida2 = HAL_I2C_Master_Receive(&hi2c2, 0xC8, reply1, 32, 1000); // tem que receber (byte de tamanho, 35 em decimal) .. 0x01 0x23 ...
	    HAL_Delay(5);


	    uint8_t snCmd[4] = {0x02, 0x00, 0x00, 0x00}; // Comando para leitura do serial number
	    uint8_t snData[9];
	    HAL_I2C_Master_Transmit(&hi2c1, 0xC8, snCmd, 4, 1000);
	    HAL_I2C_Master_Receive(&hi2c1, 0xC8, snData, 9, 1000);
	    // snData = 0x00, 0xC8, 0x00, 0x55, 0x00
	    HAL_Delay(100);
/*
	    //COMANDO DE BLOQUEIO DE CONFIGURAÇÃO
	    uint8_t blockCommand[10] = {0x03, 0x80, 0x01, 0x23, 0x04, 0x00, 0x00, 0x00};
	    HAL_Delay(5);
	  //  ret5 = HAL_I2C_Master_Transmit(&hi2c2, 0xC8, &data, sizeof(data), 1000); // Tem que enviar 1 byte
	  //  HAL_Delay(5);
	  //  saida3 = HAL_I2C_Master_Receive(&hi2c2, 0xC8, reply, 4, 1000); // tem que receber 0x04 0x11 0x33 0x43
	  //  HAL_Delay(5);
	    uint8_t reply_block[32];
	    ret5 = HAL_I2C_Master_Transmit(&hi2c2, 0xC8, blockCommand, 8, 1000); // enviar o comando de bloqueio de configuração
	    HAL_Delay(5);
	    saida3 = HAL_I2C_Master_Receive(&hi2c2, 0xC8, reply_block, 32, 1000); // tem que receber (byte de tamanho, 35 em decimal) .. 0x01 0x23 ...
	    HAL_Delay(5);

	    uint8_t writedata[8] = {0x03, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55};
	    HAL_I2C_Master_Transmit(&hi2c2, 0xC8, blockCommand, 8, 1000); // enviar o comando de bloqueio de configuração
	   	HAL_Delay(5);
	    HAL_I2C_Master_Receive(&hi2c2, 0xC8, reply_block, 32, 1000); // tem que receber (byte de tamanho, 35 em decimal) .. 0x01 0x23 ...
	   	HAL_Delay(5);

	    HAL_Delay(100);
*/
    /* USER CODE END WHILE */

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
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
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
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
