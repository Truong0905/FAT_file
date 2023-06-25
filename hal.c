/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Definitions
 *****************************************************************************/

/*
 *Initial value of total bytes per sector
 */
#define HAL_SIZE_SECTOR_DEFAULT (512u)

/*******************************************************************************
 * Variables
 ******************************************************************************/

static uint16_t s_SizeOfSector = HAL_SIZE_SECTOR_DEFAULT; /*Size in bytes of each sector*/

static FILE *s_Stream = NULL; /*Stream points to FAT file location*/

/*******************************************************************************
 * Code
 ******************************************************************************/

bool HAL_Init(const uint8_t *const filePath)
{
    bool status = true; /*return value */

    /*Open FAT file*/
    s_Stream = fopen(filePath, "rb");
    fflush(s_Stream);

    /*Check error*/
    if (NULL != s_Stream)
    {
        /*File opened successfully*/
        status = true;
    }
    else
    {
        /*File opening failed*/
        status = false;
    }

    return status;
}

int32_t HAL_ReadSector(uint32_t index, uint8_t *buff)
{
    int32_t sumByteOfSector = 0; /*return value */

    if (0 == fseek(s_Stream, (index * s_SizeOfSector), SEEK_SET)) /*Move file pointer to index*/
    {
        sumByteOfSector = fread(buff, sizeof(uint8_t), s_SizeOfSector, s_Stream); /*Read*/
    }
    else
    {
        /*Move file pointer to index location failed*/
    }

    return sumByteOfSector;
}

int32_t HAL_ReadMultiSector(uint32_t index, uint32_t num, uint8_t *buff)
{
    int32_t sumByte = 0; /*return value */

    if (0 == fseek(s_Stream, (index * s_SizeOfSector), SEEK_SET)) /*Move file pointer to index*/
    {
        sumByte = fread(buff, sizeof(uint8_t), (num * s_SizeOfSector), s_Stream); /*Read*/
    }
    else
    {
        /*Move file pointer to index location failed*/
    }

    return sumByte;
}

void HAL_UpdateSectorSize(const uint16_t sizeOfSector)
{
    s_SizeOfSector = sizeOfSector; /*Update size of sector (byte)*/
}

void HAL_DeInit(void)
{
    fclose(s_Stream); /*Close FAT file*/
}
