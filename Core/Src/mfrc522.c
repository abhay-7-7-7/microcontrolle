/**
  ******************************************************************************
  * @file           : mfrc522.c
  * @brief          : MFRC522 RFID reader driver using STM32 HAL SPI
  ******************************************************************************
  */

#include "mfrc522.h"
#include <string.h>

static SPI_HandleTypeDef *MFRC522_SPI = NULL;

#define MFRC522_CS_LOW()    HAL_GPIO_WritePin(MFRC522_CS_PORT, MFRC522_CS_PIN, GPIO_PIN_RESET)
#define MFRC522_CS_HIGH()   HAL_GPIO_WritePin(MFRC522_CS_PORT, MFRC522_CS_PIN, GPIO_PIN_SET)

/**
  * @brief  Writes a single byte to an MFRC522 register
  * @param  reg: Register address (0x00 to 0x3F)
  * @param  value: Value to write
  * @retval None
  */
void MFRC522_WriteRegister(uint8_t reg, uint8_t value)
{
    if (MFRC522_SPI == NULL) return;

    uint8_t tx[2];
    tx[0] = (uint8_t)((reg << 1) & 0x7E); // MSB=0 for write
    tx[1] = value;

    MFRC522_CS_LOW();
    HAL_SPI_Transmit(MFRC522_SPI, tx, 2, 50);
    MFRC522_CS_HIGH();
}

/**
  * @brief  Reads a single byte from an MFRC522 register
  * @param  reg: Register address (0x00 to 0x3F)
  * @retval Register value
  */
uint8_t MFRC522_ReadRegister(uint8_t reg)
{
    if (MFRC522_SPI == NULL) return 0;

    uint8_t tx[2] = { (uint8_t)(((reg << 1) & 0x7E) | 0x80), 0x00 }; // MSB=1 for read
    uint8_t rx[2] = { 0, 0 };

    MFRC522_CS_LOW();
    HAL_SPI_TransmitReceive(MFRC522_SPI, tx, rx, 2, 50);
    MFRC522_CS_HIGH();

    return rx[1];
}

/**
  * @brief  Sets bits in an MFRC522 register
  * @param  reg: Register address
  * @param  mask: Bitmask to set
  * @retval None
  */
void MFRC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t current = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, current | mask);
}

/**
  * @brief  Clears bits in an MFRC522 register
  * @param  reg: Register address
  * @param  mask: Bitmask to clear
  * @retval None
  */
void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t current = MFRC522_ReadRegister(reg);
    MFRC522_WriteRegister(reg, current & (~mask));
}

/**
  * @brief  Performs hardware and software reset of MFRC522
  * @retval None
  */
void MFRC522_Reset(void)
{
    /* Hardware reset using PB0 */
    HAL_GPIO_WritePin(MFRC522_RST_PORT, MFRC522_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(MFRC522_RST_PORT, MFRC522_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);

    /* Software reset command */
    MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_RESETPHASE);

    /* Wait for PowerDown bit (bit 4) in CommandReg to clear with bounded timeout */
    uint32_t start = HAL_GetTick();
    while ((MFRC522_ReadRegister(MFRC522_REG_COMMAND) & (1 << 4)) != 0) {
        if (HAL_GetTick() - start > 50) {
            break;
        }
    }
}

/**
  * @brief  Reads the VersionReg (0x37)
  * @retval Register value (e.g., 0x91=v1.0, 0x92=v2.0, clones=0x88/0x90/0x12)
  */
uint8_t MFRC522_ReadVersion(void)
{
    return MFRC522_ReadRegister(MFRC522_REG_VERSION);
}

/**
  * @brief  Enables the RF antenna driver pins (TX1 and TX2)
  * @retval None
  */
void MFRC522_AntennaOn(void)
{
    uint8_t current = MFRC522_ReadRegister(MFRC522_REG_TX_CONTROL);
    if ((current & 0x03) != 0x03) {
        MFRC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
    }
}

/**
  * @brief  Disables the RF antenna driver pins
  * @retval None
  */
void MFRC522_AntennaOff(void)
{
    MFRC522_ClearBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}

/**
  * @brief  Initializes the MFRC522 reader
  * @param  hspi: Pointer to SPI_HandleTypeDef
  * @retval MFRC522_STATUS_OK if successful, MFRC522_STATUS_ERROR on communication failure
  */
MFRC522_Status MFRC522_Init(SPI_HandleTypeDef *hspi)
{
    MFRC522_SPI = hspi;

    /* Ensure Chip Select starts in unselected (HIGH) state */
    MFRC522_CS_HIGH();

    /* Reset chip */
    MFRC522_Reset();

    /* Configure timer:
     * TModeReg: TAuto=1 (bit 7), TGated=00, TAutoRestart=0, TPrescaler_Hi=0xD
     * TPrescalerReg: 0x3E -> Prescaler = 0xD3E = 3390
     * f_timer = 13.56 MHz / (2 * 3390 + 1) = ~2000 Hz (0.5 ms per timer count)
     * TReload = 30 -> 15 ms timeout
     */
    MFRC522_WriteRegister(MFRC522_REG_T_MODE, 0x8D);
    MFRC522_WriteRegister(MFRC522_REG_T_PRESCALER, 0x3E);
    MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_H, 0x00);
    MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_L, 30);

    /* 100% ASK modulation */
    MFRC522_WriteRegister(MFRC522_REG_TX_ASK, 0x40);

    /* CRCPreset = 0x6363 (ISO 14443-A) */
    MFRC522_WriteRegister(MFRC522_REG_MODE, 0x3D);

    /* Configure Receiver Gain: 0x70 = 48 dB (maximum sensitivity for detection) */
    MFRC522_WriteRegister(MFRC522_REG_RF_CFG, 0x70);

    /* Enable Antenna output */
    MFRC522_AntennaOn();

    /* Verify communication by reading VersionReg */
    uint8_t version = MFRC522_ReadVersion();
    if (version == 0x00 || version == 0xFF) {
        /* Failed to communicate: SPI lines disconnected or module unpowered */
        return MFRC522_STATUS_ERROR;
    }

    return MFRC522_STATUS_OK;
}

/**
  * @brief  Calculates 16-bit CRC using MFRC522 hardware coprocessor
  * @param  pDataIn: Pointer to input data
  * @param  len: Length of input data
  * @param  pDataOut: Pointer to 2-byte output buffer (CRC_L, CRC_H)
  * @retval MFRC522_Status
  */
MFRC522_Status MFRC522_CalculateCRC(const uint8_t *pDataIn, uint8_t len, uint8_t *pDataOut)
{
    if (pDataIn == NULL || pDataOut == NULL || len == 0) return MFRC522_STATUS_ERROR;

    MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_IDLE);
    MFRC522_ClearBitMask(MFRC522_REG_DIV_IRQ, 0x04);   // Clear CRCIRq bit
    MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);  // Flush FIFO

    /* Write data into FIFO */
    for (uint8_t i = 0; i < len; i++) {
        MFRC522_WriteRegister(MFRC522_REG_FIFO_DATA, pDataIn[i]);
    }

    /* Start CRC calculation */
    MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_CALCCRC);

    /* Wait for CRC calculation to finish with bounded timeout */
    uint32_t start = HAL_GetTick();
    while (!(MFRC522_ReadRegister(MFRC522_REG_DIV_IRQ) & 0x04)) {
        if (HAL_GetTick() - start > 40) {
            return MFRC522_STATUS_TIMEOUT;
        }
    }

    /* Read result */
    pDataOut[0] = MFRC522_ReadRegister(MFRC522_REG_CRC_RESULT_L);
    pDataOut[1] = MFRC522_ReadRegister(MFRC522_REG_CRC_RESULT_H);

    return MFRC522_STATUS_OK;
}

/**
  * @brief  Sends command and data to PICC card via MFRC522 and receives reply
  * @param  command: PCD command (e.g. PCD_TRANSCEIVE)
  * @param  sendData: Pointer to data to send
  * @param  sendLen: Length of data to send
  * @param  backData: Pointer to buffer for received data
  * @param  backLen: In/Out pointer: maximum buffer length in, received length out
  * @param  validBits: Pointer to output number of valid bits in last received byte
  * @retval MFRC522_Status
  */
MFRC522_Status MFRC522_ToCard(uint8_t command, const uint8_t *sendData, uint8_t sendLen,
                              uint8_t *backData, uint8_t *backLen, uint8_t *validBits)
{
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;

    if (command == PCD_TRANSCEIVE) {
        irqEn = 0x77;
        waitIRq = 0x30; // RxIRq | IdleIRq
    }

    MFRC522_WriteRegister(MFRC522_REG_COMM_IEN, irqEn | 0x80);
    MFRC522_ClearBitMask(MFRC522_REG_COMM_IRQ, 0x80); // Clear interrupt bits
    MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);  // Flush FIFO

    MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_IDLE);

    /* Write data to FIFO */
    for (uint8_t i = 0; i < sendLen; i++) {
        MFRC522_WriteRegister(MFRC522_REG_FIFO_DATA, sendData[i]);
    }

    /* Execute command */
    MFRC522_WriteRegister(MFRC522_REG_COMMAND, command);
    if (command == PCD_TRANSCEIVE) {
        MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80); // StartSend=1
    }

    /* Bounded loop waiting for completion */
    uint32_t start = HAL_GetTick();
    uint8_t n = 0;
    while (1) {
        n = MFRC522_ReadRegister(MFRC522_REG_COMM_IRQ);
        if (n & waitIRq) {
            break; // Finished
        }
        if (n & 0x01) { // TimerIRq indicates timeout
            return MFRC522_STATUS_NO_CARD;
        }
        if (HAL_GetTick() - start > 35) { // 35 ms safety timeout
            return MFRC522_STATUS_TIMEOUT;
        }
    }

    MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80); // StartSend=0

    /* Check error register */
    uint8_t errorReg = MFRC522_ReadRegister(MFRC522_REG_ERROR);
    if (errorReg & 0x13) { // BufferOvfl, ParityErr, ProtocolErr
        return MFRC522_STATUS_ERROR;
    }
    if (errorReg & 0x08) { // CollErr
        return MFRC522_STATUS_COLLISION;
    }

    /* Read back received bytes */
    uint8_t fifoLevel = MFRC522_ReadRegister(MFRC522_REG_FIFO_LEVEL);
    if (backData != NULL && backLen != NULL) {
        if (fifoLevel > *backLen) {
            fifoLevel = *backLen;
        }
        *backLen = fifoLevel;
        for (uint8_t i = 0; i < fifoLevel; i++) {
            backData[i] = MFRC522_ReadRegister(MFRC522_REG_FIFO_DATA);
        }
    }

    if (validBits != NULL) {
        *validBits = MFRC522_ReadRegister(MFRC522_REG_CONTROL) & 0x07;
    }

    return MFRC522_STATUS_OK;
}

/**
  * @brief  Sends ISO 14443-A REQA or WUPA request to look for cards
  * @param  reqMode: PICC_CMD_REQIDL (0x26) or PICC_CMD_REQALL (0x52)
  * @param  tagType: Pointer to 2-byte buffer to receive ATQA (Answer to Request)
  * @retval MFRC522_Status
  */
MFRC522_Status MFRC522_Request(uint8_t reqMode, uint8_t *tagType)
{
    uint8_t backLen = 2;

    /* Set transmission of 7 bits for REQA / WUPA */
    MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x07);

    MFRC522_Status status = MFRC522_ToCard(PCD_TRANSCEIVE, &reqMode, 1, tagType, &backLen, NULL);
    if ((status != MFRC522_STATUS_OK) || (backLen != 2)) {
        return MFRC522_STATUS_NO_CARD;
    }

    return MFRC522_STATUS_OK;
}

/**
  * @brief  Executes anticollision loop for specified cascade level
  * @param  cascadeLevel: PICC_CMD_ANTICOLL_CL1 (0x93), CL2 (0x95), or CL3 (0x97)
  * @param  uidOut: Pointer to 5-byte buffer (4 bytes UID/data + 1 byte BCC checksum)
  * @retval MFRC522_Status
  */
MFRC522_Status MFRC522_Anticoll(uint8_t cascadeLevel, uint8_t *uidOut)
{
    if (uidOut == NULL) return MFRC522_STATUS_ERROR;

    uint8_t sendData[2] = { cascadeLevel, 0x20 };
    uint8_t backLen = 5;

    /* 8 bits per byte */
    MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x00);

    MFRC522_Status status = MFRC522_ToCard(PCD_TRANSCEIVE, sendData, 2, uidOut, &backLen, NULL);
    if (status != MFRC522_STATUS_OK || backLen != 5) {
        return MFRC522_STATUS_ERROR;
    }

    /* Verify BCC checksum: BCC = byte0 ^ byte1 ^ byte2 ^ byte3 */
    uint8_t bccCalc = uidOut[0] ^ uidOut[1] ^ uidOut[2] ^ uidOut[3];
    if (bccCalc != uidOut[4]) {
        return MFRC522_STATUS_ERROR;
    }

    return MFRC522_STATUS_OK;
}

/**
  * @brief  Selects a card for a given cascade level
  * @param  cascadeLevel: PICC_CMD_SELECT_CL1 (0x93) or CL2 (0x95)
  * @param  uidIn: 5 bytes from Anticoll (4 bytes UID + 1 byte BCC)
  * @param  sak: Pointer to receive SAK (Select Acknowledge) byte
  * @retval MFRC522_Status
  */
MFRC522_Status MFRC522_SelectTag(uint8_t cascadeLevel, const uint8_t *uidIn, uint8_t *sak)
{
    if (uidIn == NULL || sak == NULL) return MFRC522_STATUS_ERROR;

    uint8_t buffer[9];
    buffer[0] = cascadeLevel;
    buffer[1] = 0x70; // 0x70 indicates 40 bits of UID (NVB=0x70)

    for (uint8_t i = 0; i < 5; i++) {
        buffer[2 + i] = uidIn[i];
    }

    /* Calculate CRC on first 7 bytes */
    if (MFRC522_CalculateCRC(buffer, 7, &buffer[7]) != MFRC522_STATUS_OK) {
        return MFRC522_STATUS_ERROR;
    }

    uint8_t backData[3];
    uint8_t backLen = 3;

    MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x00);
    MFRC522_Status status = MFRC522_ToCard(PCD_TRANSCEIVE, buffer, 9, backData, &backLen, NULL);

    if (status != MFRC522_STATUS_OK || backLen < 1) {
        return MFRC522_STATUS_ERROR;
    }

    *sak = backData[0];
    return MFRC522_STATUS_OK;
}

/**
  * @brief  Retrieves full card UID, supporting both 4-byte and 7-byte UIDs
  * @param  uid: Pointer to MFRC522_Uid struct
  * @retval MFRC522_Status
  */
MFRC522_Status MFRC522_ReadCardSerial(MFRC522_Uid *uid)
{
    if (uid == NULL) return MFRC522_STATUS_ERROR;

    uint8_t bufferCL1[5];
    uint8_t sakCL1 = 0;

    /* Cascade Level 1 */
    MFRC522_Status status = MFRC522_Anticoll(PICC_CMD_ANTICOLL_CL1, bufferCL1);
    if (status != MFRC522_STATUS_OK) {
        return status;
    }

    status = MFRC522_SelectTag(PICC_CMD_SELECT_CL1, bufferCL1, &sakCL1);
    if (status != MFRC522_STATUS_OK) {
        return status;
    }

    /* Check if card has a 7-byte UID (Cascade Tag 0x88 in byte 0 or SAK bit 2 set) */
    if (bufferCL1[0] == PICC_CASCADE_TAG || (sakCL1 & 0x04) != 0) {
        /* 7-Byte UID: First 3 bytes are bufferCL1[1..3] */
        uid->uidByte[0] = bufferCL1[1];
        uid->uidByte[1] = bufferCL1[2];
        uid->uidByte[2] = bufferCL1[3];

        /* Cascade Level 2 */
        uint8_t bufferCL2[5];
        uint8_t sakCL2 = 0;

        status = MFRC522_Anticoll(PICC_CMD_ANTICOLL_CL2, bufferCL2);
        if (status != MFRC522_STATUS_OK) {
            return status;
        }

        status = MFRC522_SelectTag(PICC_CMD_SELECT_CL2, bufferCL2, &sakCL2);
        if (status != MFRC522_STATUS_OK) {
            return status;
        }

        /* Next 4 bytes are bufferCL2[0..3] */
        uid->uidByte[3] = bufferCL2[0];
        uid->uidByte[4] = bufferCL2[1];
        uid->uidByte[5] = bufferCL2[2];
        uid->uidByte[6] = bufferCL2[3];

        uid->size = 7;
        uid->sak = sakCL2;
    } else {
        /* Standard 4-Byte UID */
        uid->uidByte[0] = bufferCL1[0];
        uid->uidByte[1] = bufferCL1[1];
        uid->uidByte[2] = bufferCL1[2];
        uid->uidByte[3] = bufferCL1[3];

        uid->size = 4;
        uid->sak = sakCL1;
    }

    return MFRC522_STATUS_OK;
}

/**
  * @brief  High-level function: detects presence of a card and retrieves its UID
  * @param  uid: Pointer to MFRC522_Uid struct
  * @retval MFRC522_STATUS_OK if card UID was successfully retrieved
  */
MFRC522_Status MFRC522_CheckCard(MFRC522_Uid *uid)
{
    uint8_t atqa[2];
    MFRC522_Status status = MFRC522_Request(PICC_CMD_REQIDL, atqa);
    if (status != MFRC522_STATUS_OK) {
        return status;
    }

    return MFRC522_ReadCardSerial(uid);
}

/**
  * @brief  Sends PICC_HALT command to place active card into sleep state
  * @retval None
  */
void MFRC522_Halt(void)
{
    uint8_t buffer[4];
    buffer[0] = PICC_CMD_HALT;
    buffer[1] = 0x00;

    /* Calculate CRC */
    if (MFRC522_CalculateCRC(buffer, 2, &buffer[2]) == MFRC522_STATUS_OK) {
        /* Card does not reply to HALT; timeout is normal */
        MFRC522_ToCard(PCD_TRANSCEIVE, buffer, 4, NULL, NULL, NULL);
    }

    /* Stop Crypto1 */
    MFRC522_StopCrypto1();
}

/**
  * @brief  Clears Crypto1 authentication bit
  * @retval None
  */
void MFRC522_StopCrypto1(void)
{
    MFRC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);
}
