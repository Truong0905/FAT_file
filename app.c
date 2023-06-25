/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "mystring.h"
#include "fatfs.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**  APP_ShowInfo
 * @brief      show information of directory
 * @param[in] headNodeEntry  head of list entries
 * @return uint16_t Returns the maximum number of choices the user can select
 */
static uint16_t APP_ShowInfo(const FATFS_ListEntry_struct_t *const headNodeEntry);

/**  APP_Selection
 * @brief      Get User Choices
 * @param[in] sumSeclect  The maximum number of choices the user can select
 * @return uint16_t Returns user's choice
 */
static uint16_t APP_Selection(const uint16_t sumSeclect);

/**  APP_SelectiontHandler
 * @brief      Selection Handler
 * @param[in] select  User's choice
 * @param[in] sumByteOfCluster  Total bytes of a cluster. Smallest unit for reading data
 * @param[in] headNodeEntry  head of list entries
 * @param[out] subDriect  Check if user chooses folder or file
 * @return uint32_t Returns the first cluster of the selected entry. Used to open a new folder if the user selects a folder
 */
static uint32_t APP_SelectiontHandler(const uint16_t select, const uint16_t sumByteOfCluster, const FATFS_ListEntry_struct_t *const headNodeEntry, bool *const subDriect);

/*******************************************************************************
 * Code
 ******************************************************************************/

void APP_MainMenu(void)
{
    uint8_t *path = "floppy.img"; /* file path of FAT file system */
    FATFS_ListEntry_struct_t *headNodeEntry = NULL;
    uint16_t sumSeclect = 0;
    uint16_t select = 0;
    uint16_t sumByteOfCluster = 0;
    uint32_t locationForReadEntry = 0;
    bool subDriect = false;

    sumByteOfCluster = FATFS_Init(path);

    if (0 != sumByteOfCluster)
    {
        headNodeEntry = FATFS_ReadDirectory(0); /*Read root directory*/
    }
    else
    {
        /*Do nothing*/
    }

    if (NULL != headNodeEntry)
    {
        while (1)
        {
            sumSeclect = APP_ShowInfo(headNodeEntry); /*Show information */
            select = APP_Selection(sumSeclect);       /*User enters selection*/
            if (select == sumSeclect)                 /*Exit the program*/
            {
                FATFS_DeInit();
                break;
            }
            else
            {
                locationForReadEntry = APP_SelectiontHandler(select, sumByteOfCluster, headNodeEntry, &subDriect);

                if (true == subDriect)
                {
                    headNodeEntry = FATFS_ReadDirectory(locationForReadEntry);
                    if (NULL == headNodeEntry)
                    {
                        printf("Error file");
                        FATFS_DeInit();
                        break;
                    }
                }
                else
                {
                    /*Do nothing*/
                }
            }
        }
    }
    else
    {
        printf("Can not open FAT file\n");
    }
}

/************************************************************************************
 * Static function
 *************************************************************************************/

static uint16_t APP_ShowInfo(const FATFS_ListEntry_struct_t *const headNodeEntry)
{
    const FATFS_ListEntry_struct_t *temp = NULL;
    uint8_t file[7] = "File  ";
    uint8_t folder[7] = "Folder";
    uint8_t attributes[7];
    uint16_t i = 0; /*Index value*/
    uint16_t yearConvert = 0;

    temp = headNodeEntry;
    printf("\n\n%-6s%-12s%-24s%-15s%-10s\n", "No", "Name", "Date modifiled", "Type", "Size");
    while (NULL != temp)
    {
        i++;
        /*Year (0 = 1980, 119 = 2099 */
        /*0->19 : 1980 ->1999*/
        /*20->.. : 2000->...*/
        if (19 >= temp->entry.lastModDate.year)
        {
            yearConvert = 1900u + temp->entry.lastModDate.year - 19u;
        }
        else
        {
            yearConvert = 2000u + temp->entry.lastModDate.year - 20u;
        }
        if (16 == temp->entry.attributes) /*16 ( decimal) means this entry is a folder*/
        {
            MYSTRING_MyMemCpy(folder, 7, attributes); /*size of folder string  = 7*/
        }
        else
        {
            MYSTRING_MyMemCpy(file, 7, attributes); /*size of file string  = 7*/
        }

        /*Show information*/
        printf("%-6d", i);
        printf("%-8s   ", temp->entry.shortFileName);
        printf("%.2d/%.2d/%.4d %.2d:%.2d         ", temp->entry.lastModDate.month, temp->entry.lastModDate.day, yearConvert, temp->entry.lastModTime.hours, temp->entry.lastModTime.minutes);
        printf("%-15s", attributes);
        printf("%-10d ", temp->entry.fileSize);
        if (0 != temp->entry.longFileName[0])
        {
            printf("%s\n", temp->entry.longFileName);
        }
        else
        {
            printf("\n");
        }
        temp = temp->next;
    }
    i = i + 1;
    printf("\n%d  : Exit Program\n", i);

    return i;
}

static uint16_t APP_Selection(const uint16_t sumSeclect)
{
    uint8_t checkSeclect[MYSTRING_CHARACTERS_LIMIT];
    uint8_t sizeOfStringInput = 0;
    bool status = false;
    uint16_t select = 0;

    while (false == status)
    {
        sizeOfStringInput = MYSTRING_EnterSelection(checkSeclect);

        if (0 != sizeOfStringInput)
        {
            status = MYSTRING_ConvertCharToNum(checkSeclect, sizeOfStringInput, &select);

            if ((1 <= select) && (sumSeclect >= select))
            {
                break;
            }
            else
            {
                status = false;
                printf("Invalid syntax \n");
            }
        }
        else
        {
            printf("Invalid syntax \n");
        }
    }

    return select;
}

static uint32_t APP_SelectiontHandler(const uint16_t select, const uint16_t sumByteOfCluster, const FATFS_ListEntry_struct_t *const headNodeEntry, bool *const subDriect)
{
    uint8_t *buffer = NULL;
    uint16_t i = 0;
    uint32_t locationReturn = 0;
    uint32_t positionOfcluster = 0;
    const FATFS_ListEntry_struct_t *temp = NULL;
    uint32_t locationForReadEntry = 0;
    bool statusStart = true;
    bool statusNext = true;

    temp = headNodeEntry;

    for (i = 1; i < select; i++)
    {
        temp = temp->next;
    }
    if (16 != temp->entry.attributes) /*16 ( decimal) means this entry is a folder*/
    {
        if (0 != temp->entry.fileSize)
        {
            buffer = (uint8_t *)malloc(sumByteOfCluster);
            positionOfcluster = temp->entry.firstCluster;
            statusNext = FATFS_ReadData(&positionOfcluster, buffer);

            printf("\n");
            while (true == statusStart)
            {

                for (i = 0; i < sumByteOfCluster; i++)
                {
                    printf("%c", buffer[i]);
                }

                if (true == statusNext)
                {
                    statusNext = FATFS_ReadData(&positionOfcluster, buffer);
                }
                else
                {
                    statusStart = false;
                }
            }
            free(buffer);
            buffer = NULL;
        }
        else
        {
            /*Do nothing*/
        }

        *subDriect = false;
    }
    else if ((46 == temp->entry.longFileName[0]) && (46 != temp->entry.longFileName[1])) /*46 in the asscii table is a dot. If the name is a dot, it means select this folder again*/
    {
        *subDriect = false;
    }
    else
    {
        *subDriect = true;
    }
    locationForReadEntry = temp->entry.firstCluster;

    return locationForReadEntry;
}
