/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mystring.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*
 * define special characters
 */
#define MYSTRING_END_STRING '\0'
#define MYSTRING_ENTER '\n'

/*
 * Position of character 0 and character 9 in the ascii
 */
#define MYSTRING_ZERO_ASCII ((uint8_t)48) /*Position character 0 in the ascii*/
#define MYSTRING_NINE_ASCII ((uint8_t)57) /*/*Position character 9 in the ascii*/

/*
 * Macro that checks if the user entered a string as a positive integer
 */
#define MYSTRING_LETTER false /*The enter string is not positive integer*/
#define MYSTRING_NUMBER true  /*The enter string is  a positive integerr*/

/*
 * define errors
 */
#define MYSTRING_ERROR_SIZE ((uint8_t)0) /*Size  does not match*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**  MYSTRING_CaculateStringLengthOfUser
 * @brief       Caculate string length
 *  @param[in] inputArray  Address of the input array of user entered
 *  @return uint8_t  size of string
 */
static uint8_t MYSTRING_CaculateStringLengthOfUser(const uint8_t *const inputArray);

/**  MYSTRING_IntegerCheck
 * @brief      Check if string is positive integer
 *  @param[in] string  Address of the input string
 *   @param[in] sizeOfString  Size of the input string
 *  @return bool    Returns true as a positive integer and vice versa
 */
static bool MYSTRING_IntegerCheck(const uint8_t *const string, const uint8_t sizeOfString);

/*******************************************************************************
 * Code
 ******************************************************************************/

uint8_t MYSTRING_EnterSelection(uint8_t *const checkSelect)
{
   uint8_t sizeOfString = 0; /*Size of string used to store the entered data*/

   printf("Select :");
   /*Enter characters from the keyboard*/
   fgets(checkSelect, MYSTRING_CHARACTERS_LIMIT, stdin);
   fflush(stdin);

   /*If 0 is returned, it means that the input size is over the allowed size*/
   sizeOfString = MYSTRING_CaculateStringLengthOfUser(checkSelect);

   return sizeOfString;
}

bool MYSTRING_ConvertCharToNum(const uint8_t *const string, const uint8_t sizeOfString, uint16_t *const value)
{

   int8_t i;        /*use for loop*/
   uint8_t exp = 1; /*Determine if this is hundreds, tens, units,...*/
   bool status = false;

   if ((MYSTRING_ERROR_SIZE < sizeOfString) && (sizeOfString < MYSTRING_CHARACTERS_LIMIT))
   {
      status = MYSTRING_IntegerCheck(string, sizeOfString);
   }
   else
   {
      /*Do nothing*/
   }
   /*Make the conversion*/
   if (true == status) /*If this is a positive integer then do the conversion*/
   {
      for (i = sizeOfString - 1; 0 <= i; i--)
      {
         if (i == (sizeOfString - 1))
         {
            *value += ((uint8_t)string[i] - MYSTRING_ZERO_ASCII);
         }
         else
         {
            exp = exp * 10;
            *value += ((uint8_t)string[i] - MYSTRING_ZERO_ASCII) * exp;
         }
      }
   }
   else
   {
      /*Do nothing*/
   }

   return status;
}

void MYSTRING_MyMemCpy(const uint8_t *const srouce, uint32_t num, uint8_t *const destination)
{
   uint32_t i; /*Index value*/

   for (i = 0; i < num; i++)
   {
      destination[i] = srouce[i];
   }
}

/************************************************************************************
 * Static function
 *************************************************************************************/

static uint8_t MYSTRING_CaculateStringLengthOfUser(const uint8_t *const inputArray)
{
   uint8_t i = 0;               /* Use for loop*/
   uint8_t overFlow = 0;        /*Character overflow check*/
   uint8_t searchLastEnter = 0; /*Check the enter key*/
   uint8_t sizeOfString = 0;    /*size of input string */

   for (i = 0; i < MYSTRING_CHARACTERS_LIMIT; i++)
   {
      if ((MYSTRING_END_STRING != inputArray[i]))
      {
         sizeOfString++;
         overFlow++;
      }
      else
      {
         searchLastEnter = i - 1;
         i = MYSTRING_CHARACTERS_LIMIT;
      }
   }
   if (MYSTRING_ENTER == inputArray[searchLastEnter]) /*Check the enter key*/
   {
      /*Remove last enter*/
      sizeOfString--;
   }
   else
   {
      /*Do nothing*/
   }
   if (overFlow == sizeOfString) /*If enter more than CHARACTERS_LIMIT (5) characters to allow*/
   {
      sizeOfString = MYSTRING_ERROR_SIZE;
   }
   else
   {
      /*Do nothing*/
   }

   return sizeOfString;
}

static bool MYSTRING_IntegerCheck(const uint8_t *const string, const uint8_t sizeOfString)
{
   uint8_t i;                     /*Index value*/
   bool status = MYSTRING_NUMBER; /*Suppose to be a positive integer*/

   for (i = 0; i < sizeOfString; i++)
   {
      if (MYSTRING_ZERO_ASCII > ((uint8_t)string[i]) || MYSTRING_NINE_ASCII < ((uint8_t)string[i]))
      {
         status = MYSTRING_LETTER;
         i = sizeOfString;
      }
   }

   return status;
}
