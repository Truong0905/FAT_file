#ifndef __MY_STRING_H__
#define __MY_STRING_H__

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*
 *The maximum number of characters entered from the keyboard must be less than this value (including enter )
 */
#define MYSTRING_CHARACTERS_LIMIT ((uint8_t)9)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**  MYSTRING_MyMemCpy
 * @brief      copy string
 * @param[in] srouce  srouce string
 * @param[in] num   Number of bytes to copy
 * @param[out] destinations   destinations string
 * @return none
 */
void MYSTRING_MyMemCpy(const uint8_t *const srouce, uint32_t num, uint8_t *const destination);
uint8_t MYSTRING_EnterSelection(uint8_t *const checkSelect);
bool MYSTRING_ConvertCharToNum(const uint8_t *const string, const uint8_t sizeOfString, uint16_t *const value);

#endif /*__MY_STRING_H__*/
