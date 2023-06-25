#ifndef __HAL_H__
#define __HAL_H__

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**  HAL_Init
 * @brief Open the file FAT
 * @param[in] filePath   The path to the file
 * @return bool Returns True if the file is opened successfully
 */
bool HAL_Init(const uint8_t *const filePath);

/**  HAL_ReadSector
 * @brief Read only one sector
 * @param[in] index   Location of sectors
 * @param[out] buff   Receiver array
 * @return int32_t Returns the number of bytes read
 */
int32_t HAL_ReadSector(uint32_t index, uint8_t *buff);

/**  HAL_ReadSector
 * @brief Read multiple sectors
 * @param[in] index   Location of first sector to read
 * @param[in] num   Total number of sectors to read
 * @param[out] buff   Receiver array
 * @return int32_t Returns the number of bytes read
 */
int32_t HAL_ReadMultiSector(uint32_t index, uint32_t num, uint8_t *buff);

/**  HAL_UpdateSectorSize
 * @brief Update size of sector
 * @param[in] sizeOfSector   Location of first sector to read
 * @return none
 */
void HAL_UpdateSectorSize(const uint16_t sizeOfSector);

/**  HAL_DeInit
 * @brief Close the file FAT
 * @return none
 */
void HAL_DeInit(void);

#endif /*__HAL_H__*/
