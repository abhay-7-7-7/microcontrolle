/**
  ******************************************************************************
  * @file           : mfrc522.h
  * @brief          : Header for MFRC522 RFID reader driver using STM32 HAL SPI
  ******************************************************************************
  */

#ifndef __MFRC522_H__
#define __MFRC522_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Hardware Pin Definitions */
#define MFRC522_CS_PORT             GPIOA
#define MFRC522_CS_PIN              GPIO_PIN_4

#define MFRC522_RST_PORT            GPIOB
#define MFRC522_RST_PIN             GPIO_PIN_0

/* MFRC522 Registers */
// Page 0: Command and Status
#define MFRC522_REG_COMMAND         0x01
#define MFRC522_REG_COMM_IEN        0x02
#define MFRC522_REG_DIV_IEN         0x03
#define MFRC522_REG_COMM_IRQ        0x04
#define MFRC522_REG_DIV_IRQ         0x05
#define MFRC522_REG_ERROR           0x06
#define MFRC522_REG_STATUS1         0x07
#define MFRC522_REG_STATUS2         0x08
#define MFRC522_REG_FIFO_DATA       0x09
#define MFRC522_REG_FIFO_LEVEL      0x0A
#define MFRC522_REG_WATER_LEVEL     0x0B
#define MFRC522_REG_CONTROL         0x0C
#define MFRC522_REG_BIT_FRAMING     0x0D
#define MFRC522_REG_COLL            0x0E

// Page 1: Command
#define MFRC522_REG_MODE            0x11
#define MFRC522_REG_TX_MODE         0x12
#define MFRC522_REG_RX_MODE         0x13
#define MFRC522_REG_TX_CONTROL      0x14
#define MFRC522_REG_TX_ASK          0x15
#define MFRC522_REG_TX_SEL          0x16
#define MFRC522_REG_RX_SEL          0x17
#define MFRC522_REG_RX_THRESHOLD    0x18
#define MFRC522_REG_DEMOD           0x19
#define MFRC522_REG_MF_TX           0x1C
#define MFRC522_REG_MF_RX           0x1D
#define MFRC522_REG_SERIAL_SPEED    0x1F

// Page 2: Configuration
#define MFRC522_REG_CRC_RESULT_H    0x21
#define MFRC522_REG_CRC_RESULT_L    0x22
#define MFRC522_REG_MOD_WIDTH       0x24
#define MFRC522_REG_RF_CFG          0x26
#define MFRC522_REG_GS_N            0x27
#define MFRC522_REG_CW_GS_P         0x28
#define MFRC522_REG_MOD_GS_P        0x29
#define MFRC522_REG_T_MODE          0x2A
#define MFRC522_REG_T_PRESCALER     0x2B
#define MFRC522_REG_T_RELOAD_H      0x2C
#define MFRC522_REG_T_RELOAD_L      0x2D
#define MFRC522_REG_T_COUNTER_VAL_H 0x2E
#define MFRC522_REG_T_COUNTER_VAL_L 0x2F

// Page 3: Test
#define MFRC522_REG_TEST_SEL1       0x31
#define MFRC522_REG_TEST_SEL2       0x32
#define MFRC522_REG_TEST_PIN_EN     0x33
#define MFRC522_REG_TEST_PIN_VALUE  0x34
#define MFRC522_REG_TEST_BUS        0x35
#define MFRC522_REG_AUTO_TEST       0x36
#define MFRC522_REG_VERSION         0x37
#define MFRC522_REG_ANALOG_TEST     0x38

/* MFRC522 PCD Commands */
#define PCD_IDLE                    0x00
#define PCD_CALCCRC                 0x03
#define PCD_TRANSMIT                0x04
#define PCD_NOCMDCHANGE             0x07
#define PCD_RECEIVE                 0x08
#define PCD_TRANSCEIVE              0x0C
#define PCD_MFAUTHENT               0x0E
#define PCD_RESETPHASE              0x0F

/* PICC Commands (ISO 14443-A) */
#define PICC_CMD_REQIDL             0x26
#define PICC_CMD_REQALL             0x52
#define PICC_CMD_ANTICOLL_CL1       0x93
#define PICC_CMD_ANTICOLL_CL2       0x95
#define PICC_CMD_ANTICOLL_CL3       0x97
#define PICC_CMD_SELECT_CL1         0x93
#define PICC_CMD_SELECT_CL2         0x95
#define PICC_CMD_SELECT_CL3         0x97
#define PICC_CMD_HALT               0x50

/* Cascade tag byte */
#define PICC_CASCADE_TAG            0x88

/* Status enumeration */
typedef enum {
    MFRC522_STATUS_OK = 0,
    MFRC522_STATUS_ERROR,
    MFRC522_STATUS_NO_CARD,
    MFRC522_STATUS_COLLISION,
    MFRC522_STATUS_TIMEOUT
} MFRC522_Status;

/* Maximum UID length supported (4, 7, or 10 bytes) */
#define MFRC522_MAX_UID_LEN         10

/* Structure to store card UID */
typedef struct {
    uint8_t size;                           /* UID length in bytes (usually 4 or 7) */
    uint8_t uidByte[MFRC522_MAX_UID_LEN];   /* UID bytes */
    uint8_t sak;                            /* Select Acknowledge byte */
} MFRC522_Uid;

/* Public Function Prototypes */
MFRC522_Status MFRC522_Init(SPI_HandleTypeDef *hspi);
void MFRC522_Reset(void);
uint8_t MFRC522_ReadVersion(void);
void MFRC522_AntennaOn(void);
void MFRC522_AntennaOff(void);

void MFRC522_WriteRegister(uint8_t reg, uint8_t value);
uint8_t MFRC522_ReadRegister(uint8_t reg);
void MFRC522_SetBitMask(uint8_t reg, uint8_t mask);
void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask);

MFRC522_Status MFRC522_ToCard(uint8_t command, const uint8_t *sendData, uint8_t sendLen,
                              uint8_t *backData, uint8_t *backLen, uint8_t *validBits);
MFRC522_Status MFRC522_CalculateCRC(const uint8_t *pDataIn, uint8_t len, uint8_t *pDataOut);

MFRC522_Status MFRC522_Request(uint8_t reqMode, uint8_t *tagType);
MFRC522_Status MFRC522_Anticoll(uint8_t cascadeLevel, uint8_t *uidOut);
MFRC522_Status MFRC522_SelectTag(uint8_t cascadeLevel, const uint8_t *uidIn, uint8_t *sak);
MFRC522_Status MFRC522_ReadCardSerial(MFRC522_Uid *uid);
MFRC522_Status MFRC522_CheckCard(MFRC522_Uid *uid);
void MFRC522_Halt(void);
void MFRC522_StopCrypto1(void);

#ifdef __cplusplus
}
#endif

#endif /* __MFRC522_H__ */
