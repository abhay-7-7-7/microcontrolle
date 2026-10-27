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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mfrc522.h"
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/*
 * ==============================================================================
 * RFID CONFIGURATION: AUTHORIZATION WHITELIST
 * ==============================================================================
 */
#define RFID_DISCOVERY_MODE         0

/* Authorized Card Structure */
typedef struct {
  uint8_t length;
  uint8_t bytes[MFRC522_MAX_UID_LEN];
  const char *label;
} AuthorizedCard_t;

/*
 * Whitelist of authorized RFID cards.
 * - Authorized Card:     67 46 49 64
 * - Unauthorized Card:   A1 B3 34 66 (explicitly not included, will be denied)
 * - Any unknown card:    Denied by default
 */
static const AuthorizedCard_t AUTHORIZED_WHITELIST[] = {
  {
    .length = 4,
    .bytes  = {0x67, 0x46, 0x49, 0x64},
    .label  = "Authorized Card"
  },
  /* Additional authorized cards can be appended here */
};

#define AUTHORIZED_CARD_COUNT (sizeof(AUTHORIZED_WHITELIST) / sizeof(AUTHORIZED_WHITELIST[0]))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
bool isAuthorized(const uint8_t *uid, uint8_t uidLength);
static void Format_UID_String(const MFRC522_Uid *uid, char *buf, size_t bufSize);
static void Show_Idle_Screen(void);
static void Show_Discovery_UID(const MFRC522_Uid *uid) __attribute__((unused));
static void Show_Access_Granted(const MFRC522_Uid *uid);
static void Show_Access_Denied(const MFRC522_Uid *uid);
static void Wait_For_Card_Removal(void);
static void LEDs_Init_State(void);
static void Blink_Green_LED(void);
static void Blink_Red_LED(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  Checks if a scanned RFID UID matches an authorized entry in the whitelist.
  * @param  uid: Pointer to array of UID bytes
  * @param  uidLength: Number of bytes in the UID
  * @retval true if authorized, false otherwise
  */
bool isAuthorized(const uint8_t *uid, uint8_t uidLength)
{
  if (uid == NULL || uidLength == 0) {
    return false;
  }

  for (size_t i = 0; i < AUTHORIZED_CARD_COUNT; i++) {
    /* Check UID length first */
    if (AUTHORIZED_WHITELIST[i].length == uidLength) {
      /* Compare every UID byte */
      bool match = true;
      for (uint8_t b = 0; b < uidLength; b++) {
        if (uid[b] != AUTHORIZED_WHITELIST[i].bytes[b]) {
          match = false;
          break;
        }
      }
      if (match) {
        return true; /* Exact match found */
      }
    }
  }

  return false; /* Deny all others by default */
}

static void Format_UID_String(const MFRC522_Uid *uid, char *buf, size_t bufSize)
{
  if (uid == NULL || buf == NULL || bufSize == 0) return;

  if (uid->size == 4) {
    snprintf(buf, bufSize, "%02X %02X %02X %02X",
             uid->uidByte[0], uid->uidByte[1], uid->uidByte[2], uid->uidByte[3]);
  } else if (uid->size == 7) {
    snprintf(buf, bufSize, "%02X %02X %02X %02X %02X %02X %02X",
             uid->uidByte[0], uid->uidByte[1], uid->uidByte[2], uid->uidByte[3],
             uid->uidByte[4], uid->uidByte[5], uid->uidByte[6]);
  } else {
    snprintf(buf, bufSize, "UID LEN %d", uid->size);
  }
}

static void Show_Idle_Screen(void)
{
  SSD1306_Clear();
  /* Rows 0-15: Yellow band on dual-color OLED */
  SSD1306_PrintCentered(3, "RFID SYSTEM", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(0, 15, 127, 15, SSD1306_COLOR_WHITE);
  /* Rows 16-63: Blue band */
  SSD1306_PrintCentered(36, "SCAN CARD", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
}

static void Show_Discovery_UID(const MFRC522_Uid *uid)
{
  SSD1306_Clear();
  SSD1306_PrintCentered(4, "CARD DETECTED", &Font_7x10, SSD1306_COLOR_WHITE);

  if (uid->size == 4) {
    char hexLine[20];
    snprintf(hexLine, sizeof(hexLine), "%02X %02X %02X %02X",
             uid->uidByte[0], uid->uidByte[1], uid->uidByte[2], uid->uidByte[3]);
    SSD1306_PrintCentered(24, hexLine, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_PrintCentered(44, "UID: 4 BYTES", &Font_6x8, SSD1306_COLOR_WHITE);
  } else if (uid->size == 7) {
    char hexLine1[20];
    char hexLine2[20];
    snprintf(hexLine1, sizeof(hexLine1), "%02X %02X %02X %02X",
             uid->uidByte[0], uid->uidByte[1], uid->uidByte[2], uid->uidByte[3]);
    snprintf(hexLine2, sizeof(hexLine2), "%02X %02X %02X",
             uid->uidByte[4], uid->uidByte[5], uid->uidByte[6]);
    SSD1306_PrintCentered(20, hexLine1, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_PrintCentered(34, hexLine2, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_PrintCentered(48, "UID: 7 BYTES", &Font_6x8, SSD1306_COLOR_WHITE);
  } else {
    char lenLine[20];
    snprintf(lenLine, sizeof(lenLine), "LEN: %d BYTES", uid->size);
    SSD1306_PrintCentered(28, lenLine, &Font_7x10, SSD1306_COLOR_WHITE);
  }

  SSD1306_UpdateScreen();
}

static void Show_Access_Granted(const MFRC522_Uid *uid)
{
  SSD1306_Clear();
  /* Yellow band */
  SSD1306_PrintCentered(3, "CARD DETECTED", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(0, 15, 127, 15, SSD1306_COLOR_WHITE);

  /* Blue band */
  char uidStr[32];
  Format_UID_String(uid, uidStr, sizeof(uidStr));
  SSD1306_PrintCentered(20, uidStr, &Font_6x8, SSD1306_COLOR_WHITE);
  SSD1306_PrintCentered(34, "ACCESS GRANTED", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_PrintCentered(49, "WELCOME", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
}

static void Show_Access_Denied(const MFRC522_Uid *uid)
{
  SSD1306_Clear();
  /* Yellow band */
  SSD1306_PrintCentered(3, "CARD DETECTED", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(0, 15, 127, 15, SSD1306_COLOR_WHITE);

  /* Blue band */
  char uidStr[32];
  Format_UID_String(uid, uidStr, sizeof(uidStr));
  SSD1306_PrintCentered(20, uidStr, &Font_6x8, SSD1306_COLOR_WHITE);
  SSD1306_PrintCentered(34, "ACCESS DENIED", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_PrintCentered(49, "UNAUTHORISED", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
}

static void Wait_For_Card_Removal(void)
{
  /* While card is still on antenna, REQALL (WUPA) returns OK */
  uint32_t waitStart = HAL_GetTick();
  while (HAL_GetTick() - waitStart < 3000) {
    uint8_t atqa[2];
    if (MFRC522_Request(PICC_CMD_REQALL, atqa) != MFRC522_STATUS_OK) {
      break; /* Card physically lifted away */
    }
    /* Re-halt card so it stays quiet */
    MFRC522_Halt();
    HAL_Delay(80);
  }
}

/**
  * @brief  Initializes status LEDs to OFF state
  * @retval None
  */
static void LEDs_Init_State(void)
{
  HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  Blinks Green LED three times (authorized access indication)
  * @retval None
  */
static void Blink_Green_LED(void)
{
  /* Ensure red LED is off and green starts off */
  HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);

  /* Blink green LED 3 times: 150ms ON, 150ms OFF */
  for (uint8_t i = 0; i < 3; i++) {
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
    HAL_Delay(150);
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
    HAL_Delay(150);
  }
}

/**
  * @brief  Blinks Red LED three times (unauthorized access indication)
  * @retval None
  */
static void Blink_Red_LED(void)
{
  /* Ensure green LED is off and red starts off */
  HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);

  /* Blink red LED 3 times: 150ms ON, 150ms OFF */
  for (uint8_t i = 0; i < 3; i++) {
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
    HAL_Delay(150);
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
    HAL_Delay(150);
  }
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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  /* Initialize status LEDs (both OFF initially) */
  LEDs_Init_State();

  /* 1. Initialise SSD1306 OLED Display */
  SSD1306_Status oledStatus = SSD1306_Init(&hi2c1);
  if (oledStatus == SSD1306_OK) {
    SSD1306_Clear();
    SSD1306_PrintCentered(3, "RFID SYSTEM", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_DrawLine(0, 15, 127, 15, SSD1306_COLOR_WHITE);
    SSD1306_PrintCentered(36, "STARTING...", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
    HAL_Delay(400);
  }

  /* 2. Initialise MFRC522 RFID Reader */
  MFRC522_Status rfidStatus = MFRC522_Init(&hspi1);
  uint8_t versionReg = MFRC522_ReadVersion();

  /* Validate communication with RC522 */
  if (rfidStatus != MFRC522_STATUS_OK || versionReg == 0x00 || versionReg == 0xFF) {
    if (oledStatus == SSD1306_OK) {
      char verMsg[22];
      SSD1306_Clear();
      SSD1306_PrintCentered(12, "RFID SYSTEM", &Font_7x10, SSD1306_COLOR_WHITE);
      SSD1306_PrintCentered(28, "RC522 ERROR", &Font_7x10, SSD1306_COLOR_WHITE);
      snprintf(verMsg, sizeof(verMsg), "VER: 0x%02X", versionReg);
      SSD1306_PrintCentered(44, verMsg, &Font_7x10, SSD1306_COLOR_WHITE);
      SSD1306_UpdateScreen();
    }

    /* Bounded non-blocking retry loop every 2 seconds */
    uint32_t lastRetry = HAL_GetTick();
    while (versionReg == 0x00 || versionReg == 0xFF) {
      if (HAL_GetTick() - lastRetry >= 2000) {
        lastRetry = HAL_GetTick();
        rfidStatus = MFRC522_Init(&hspi1);
        versionReg = MFRC522_ReadVersion();
      }
    }
  }

  /* Both peripherals ready */
  if (oledStatus == SSD1306_OK) {
    SSD1306_Clear();
    SSD1306_PrintCentered(3, "RFID SYSTEM", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_DrawLine(0, 15, 127, 15, SSD1306_COLOR_WHITE);
    SSD1306_PrintCentered(36, "RFID READY", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
    HAL_Delay(800);
    Show_Idle_Screen();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  MFRC522_Uid scannedUid;

  while (1)
  {
    /* Check for presence of an RFID card */
    if (MFRC522_CheckCard(&scannedUid) == MFRC522_STATUS_OK)
    {
#if RFID_DISCOVERY_MODE
      /* Phase A: Discovery Mode - Display real UID on OLED */
      Show_Discovery_UID(&scannedUid);

      /* Card halt and cleanup */
      MFRC522_Halt();

      /* Display UID for approximately 2 seconds */
      HAL_Delay(2000);

      /* Wait until card is physically lifted away to prevent continuous triggers */
      Wait_For_Card_Removal();

      /* Return to idle prompt */
      Show_Idle_Screen();
#else
      /* Authorization Check */
      if (isAuthorized(scannedUid.uidByte, scannedUid.size)) {
        Show_Access_Granted(&scannedUid);
        MFRC522_Halt();
        /* Authorized UID (67 46 49 64): turn red LED off and blink green LED three times */
        Blink_Green_LED();
        HAL_Delay(1100);
      } else {
        Show_Access_Denied(&scannedUid);
        MFRC522_Halt();
        /* Unauthorized / Invalid UID: turn green LED off and blink red LED three times */
        Blink_Red_LED();
        HAL_Delay(1100);
      }

      /* When idle, both LEDs must remain off */
      LEDs_Init_State();

      /* Wait until card is physically lifted away to prevent repeated flickering */
      Wait_For_Card_Removal();

      /* Return to idle prompt */
      Show_Idle_Screen();
#endif
    }

    HAL_Delay(50);
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
  hi2c1.Init.ClockSpeed = 100000;
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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level for Status LEDs (PA0=Green, PA1=Red, Initial LOW) */
  HAL_GPIO_WritePin(GPIOA, LED_GREEN_PIN|LED_RED_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

  /*Configure GPIO pins : PC13 PC14 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 (Green LED), PA1 (Red LED) */
  GPIO_InitStruct.Pin = LED_GREEN_PIN|LED_RED_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
