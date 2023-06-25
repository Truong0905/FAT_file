/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "hal.h"
#include "fatfs.h"

/*******************************************************************************
 * Definitions
 *****************************************************************************/

/*
 * Macros check what kind of fat this is. It is also a condition used to check the end of the file
 */

#define FATFS_END_OF_FILE_FAT12 (0xfff)
#define FATFS_END_OF_FILE_FAT16 (0xffff)
#define FATFS_END_OF_FILE_FAT32 (0xfffffff)

/*************************************************************/

/*
 * Macros are used to read information in the boot sector
 */

#define FATFS_BYTE_PER_SECTOR_OFFSET (11u)
#define FATFS_SECTOR_PER_CLUSTER_OFFSET (13u)
#define FATFS_NUMBER_RESERVED_SECTORS_OFFSET (14u)
#define FATFS_NUMBER_FAT_OFFSET (16u)
#define FATFS_NUMBER_ENTRY_OFFSET (17u)
#define FATFS_TOTAL_SECTORS_OFFSET (19U)
#define FATFS_TOTAL_SECTORS_FAT16_32_OFFSET (32U)
#define FATFS_SECTOR_PER_FAT_12_16_OFFSET (22u)
#define FATFS_SECTOR_PER_FAT_32_OFFSET (36u)

/*************************************************************/

/*
 * Macros are used to read information in the entry
 */

#define FATFS_ATTRIBUTE_OF_FILE_OFFSET (11u)
#define FATFS_CREATE_TIME_FILE_OFFSET (13u)
#define FATFS_CREATE_DATE_FILE_OFFSET (16u)
#define FATFS_LAST_ACCESS_DATE_FILE_OFFSET (18u)
#define FATFS_HIGH_WORD_OF_ADDRESS_CLUSTER_OFFSET (20u)
#define FATFS_LAST_MOD_TIME_FILE_OFFSET (22u)
#define FATFS_LAST_MOD_DATE_FILE_OFFSET (24u)
#define FATFS_LOW_WORD_OF_ADDRESS_CLUSTER_OFFSET (26u)
#define FATFS_FILE_SIZE_OFFSET (28u)
#define FATFS_STATRT_CLUSTER_ROOT_FAT32_OFFSET (44u)

#define FATFS_FIELD_SECONDS_SHIFT_RIGHT (0u)
#define FATFS_FIELD_MINUTES_SHIFT_RIGHT (5u)
#define FATFS_FIELD_HOURS_SHIFT_RIGHT (11u)

#define FATFS_FIELD_DAY_SHIFT_RIGHT (0u)
#define FATFS_FIELD_MONTH_SHIFT_RIGHT (5u)
#define FATFS_FIELD_YEAR_SHIFT_RIGHT (9u)

#define FATFS_FIELD_SECONDS_MASK (0x1f)
#define FATFS_FIELD_MINUTES_MASK (0x3f)
#define FATFS_FIELD_HOURS_MASK (0x1f)

#define FATFS_FIELD_DAY_MASK (0x1f)
#define FATFS_FIELD_MONTH_MASK (0x0f)
#define FATFS_FIELD_YEAR_MASK (0x7f)

#define FATFS_SIZE_ENTRY_BYTE (32u)
#define FATFS_SUB_ENTRY (15u)
#define FATFS_END_OF_ENTRY (0u)

/*************************************************************/

/*
 * Function like Macros : convert little endian
 */

#define FATFS_CONVERT_2_BYTES(x) ((x)[0] | (x)[1] << 8u)
#define FATFS_CONVERT_3_BYTES(x) ((x)[0] | (x)[1] << 8u | (x)[2] << 16u)
#define FATFS_CONVERT_4_BYTES(x) ((x)[0] | (x)[1] << 8u | (x)[2] << 16u | (x)[3] << 24u)

/*************************************************************/

/*
 * struct stores information of fat flie
 */
typedef struct
{
    uint16_t bytePerSector;             /*Number of bytes per sector*/
    uint8_t sectorPerCluster;           /*Number of sectors per cluster*/
    uint8_t numberOfFat;                /*Number of FAT*/
    uint32_t sectorPerFat;              /*Number of sectors per FAT*/
    uint16_t sumSectorOfRoot;           /*Total number of sectors of Root*/
    uint32_t locationOfFirstFat;        /*Location of the first sector of fat*/
    uint32_t locationOfRoot;            /*Location of the first sector of root directory*/
    uint32_t locationOfData;            /*Location of the first sector of data region*/
    uint32_t stratClusterOfRootOfFat32; /*First cluster of root directory (using in FAT32)*/
} FATFS_FatFileSystemInfo_Struct_t;

/*
 *Struct for list of long file name
 */
typedef struct __FATFS_LongFileName_struct_t
{
    uint8_t stringName[27]; /*In the sub-entry up to 26 characters of the file name are stored + 1 '\0'*/
    struct __FATFS_LongFileName_struct_t *next;
} FATFS_LongFileName_struct_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

static FATFS_FatFileSystemInfo_Struct_t s_InformationOfFatFs; /*Stores information of fat flie*/
static FATFS_ListEntry_struct_t *s_HeadOfListEntry = NULL;    /*Head pointer of entry list*/
static uint32_t *s_BufferForFat = NULL;                       /*Store information of FAT table*/
static uint32_t s_EndOfFile = 0;                              /*Check what kind of fat this is. It is also a condition used to check the end of the file*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/** FATFS_ProcessSubEntry
 * @brief Processing sub entry
 * @param[in] buffer array of entry
 * @param[out] node for list entry
 * @return bool Returns true if this is a sub entry
 */
static bool FATFS_ProcessSubEntry(const uint8_t *const buffer, FATFS_ListEntry_struct_t *const node);

/** FATFS_ProcessMainEntry
 * @brief Processing main entry
 * @param[in] buffer array of entry
 * @param[out] node for list entry
 * @return bool Returns true if this is a main entry
 */
static bool FATFS_ProcessMainEntry(const uint8_t *const buffer, FATFS_ListEntry_struct_t *const node);

/*******************************************************************************
 * Code
 ******************************************************************************/

uint16_t FATFS_Init(const uint8_t const *filePath)
{
    uint8_t bufferForBoot[512]; /*Read the first 512 bytes information of boot sector */
    uint8_t *bufferOfFat = NULL;
    uint32_t sumByteOfFat = 0;
    uint32_t totalElemmentOfFat = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t totalSectors = 0;     /*Total number of sectors of the file*/
    uint16_t totalClusters = 0;    /*Total number of clusters of the file*/
    uint16_t sumEntryOfRoot = 0;   /*Total number of entries of the root directory*/
    uint16_t sumByteOfCluster = 0; /*return value */

    /*Read boot sector ( in sector 0) */
    if ((true == HAL_Init(filePath)) && (512 == HAL_ReadSector(0, bufferForBoot)))
    {
        /*Open the file successfully and read the Boot Sectorsuccessfully*/

        s_InformationOfFatFs.bytePerSector = FATFS_CONVERT_2_BYTES(&bufferForBoot[FATFS_BYTE_PER_SECTOR_OFFSET]);

        s_InformationOfFatFs.sectorPerCluster = bufferForBoot[FATFS_SECTOR_PER_CLUSTER_OFFSET];

        s_InformationOfFatFs.locationOfFirstFat = FATFS_CONVERT_2_BYTES(&bufferForBoot[FATFS_NUMBER_RESERVED_SECTORS_OFFSET]);

        s_InformationOfFatFs.numberOfFat = bufferForBoot[FATFS_NUMBER_FAT_OFFSET];

        sumEntryOfRoot = FATFS_CONVERT_2_BYTES(&bufferForBoot[FATFS_NUMBER_ENTRY_OFFSET]);
        s_InformationOfFatFs.sumSectorOfRoot = (sumEntryOfRoot * FATFS_SIZE_ENTRY_BYTE) / s_InformationOfFatFs.bytePerSector;

        s_InformationOfFatFs.sectorPerFat = FATFS_CONVERT_2_BYTES(&bufferForBoot[FATFS_SECTOR_PER_FAT_12_16_OFFSET]);

        /*Determine what type of FAT this is*/
        totalSectors = FATFS_CONVERT_2_BYTES(&bufferForBoot[FATFS_TOTAL_SECTORS_OFFSET]);
        if ((0 == totalSectors) && (0 == s_InformationOfFatFs.sectorPerFat)) /*total Sectors and sectorPerFat read from the boot sector are 0*/
        {
            /*This is FAT 32*/

            s_EndOfFile = FATFS_END_OF_FILE_FAT32;
            s_InformationOfFatFs.sectorPerFat = FATFS_CONVERT_4_BYTES(&bufferForBoot[FATFS_SECTOR_PER_FAT_32_OFFSET]);
            s_InformationOfFatFs.locationOfData = s_InformationOfFatFs.locationOfFirstFat + s_InformationOfFatFs.numberOfFat * s_InformationOfFatFs.sectorPerFat;
            s_InformationOfFatFs.stratClusterOfRootOfFat32 = FATFS_CONVERT_4_BYTES(&bufferForBoot[FATFS_STATRT_CLUSTER_ROOT_FAT32_OFFSET]);
            s_InformationOfFatFs.locationOfRoot = s_InformationOfFatFs.locationOfData + (s_InformationOfFatFs.stratClusterOfRootOfFat32 - 2) * s_InformationOfFatFs.sectorPerCluster;

            sumByteOfFat = s_InformationOfFatFs.sectorPerFat * s_InformationOfFatFs.bytePerSector;
            totalElemmentOfFat = sumByteOfFat / 4; /*size element : 4 byte*/
        }
        else
        {
            s_InformationOfFatFs.locationOfRoot = s_InformationOfFatFs.locationOfFirstFat + s_InformationOfFatFs.numberOfFat * s_InformationOfFatFs.sectorPerFat;
            s_InformationOfFatFs.locationOfData = s_InformationOfFatFs.locationOfRoot + s_InformationOfFatFs.sumSectorOfRoot;

            sumByteOfFat = s_InformationOfFatFs.sectorPerFat * s_InformationOfFatFs.bytePerSector;
            if (0 == totalSectors) /*total Sectors  read from the boot sector is 0*/
            {
                /*This is FAT 16*/
                s_EndOfFile = FATFS_END_OF_FILE_FAT16;
                totalElemmentOfFat = sumByteOfFat / 2; /*size element : 2 byte*/
            }
            else
            {

                totalClusters = (totalSectors - s_InformationOfFatFs.locationOfData) / s_InformationOfFatFs.sectorPerCluster;
                if (4085 > totalClusters)
                {
                    /*This is FAT 12*/

                    s_EndOfFile = FATFS_END_OF_FILE_FAT12;
                    totalElemmentOfFat = sumByteOfFat * 2 / 3; /*size element : 1,5 byte*/
                }
                else
                {
                    /*This is FAT 16*/
                    s_EndOfFile = FATFS_END_OF_FILE_FAT16;
                    totalElemmentOfFat = sumByteOfFat / 2; /*size element : 2 byte*/
                }
            }
        }
        /*Update sector size*/
        HAL_UpdateSectorSize(s_InformationOfFatFs.bytePerSector);

        /*Read FAT table */
        bufferOfFat = (uint8_t *)malloc(sumByteOfFat);
        HAL_ReadMultiSector(s_InformationOfFatFs.locationOfFirstFat, s_InformationOfFatFs.sectorPerFat, bufferOfFat);

        s_BufferForFat = (uint32_t *)malloc(totalElemmentOfFat * sizeof(uint32_t));
        if (FATFS_END_OF_FILE_FAT32 == s_EndOfFile)
        {
            for (i = 0; i < sumByteOfFat; i += 4) /*size element : 4 byte*/
            {
                s_BufferForFat[j] = FATFS_CONVERT_4_BYTES(&bufferOfFat[i]);
                j++;
            }
        }
        else if (FATFS_END_OF_FILE_FAT16 == s_EndOfFile)
        {
            for (i = 0; i < sumByteOfFat; i += 2) /*size element : 2 byte*/
            {
                s_BufferForFat[j] = FATFS_CONVERT_2_BYTES(&bufferOfFat[i]);
                j++;
            }
        }
        else
        {
            for (i = 0; i < sumByteOfFat; i += 3) /*size 2 element : 3 byte*/
            {
                s_BufferForFat[j] = FATFS_CONVERT_3_BYTES(&bufferOfFat[i]) & 0xfff;
                j++;
                s_BufferForFat[j] = FATFS_CONVERT_3_BYTES(&bufferOfFat[i]) >> 12;
                j++;
            }
        }

        /*Return value*/
        sumByteOfCluster = s_InformationOfFatFs.bytePerSector * s_InformationOfFatFs.sectorPerCluster;
    }
    else
    {
        /*Error*/
        sumByteOfCluster = 0;
    }

    return sumByteOfCluster;
}

FATFS_ListEntry_struct_t *FATFS_ReadDirectory(const uint32_t locationToRead)
{
    FATFS_ListEntry_struct_t *returnListEntry = NULL; /*return value */
    bool status = false;                              /*Check if any entry is used*/
    bool checkWhileLoop = true;                       /*Using for while loop*/
    uint16_t i = 0;                                   /*Index value*/
    uint8_t *buffer = NULL;                           /*buffer for receive data*/
    uint32_t sizeOfBuffer = 0;                        /*Size of buffer*/
    uint16_t sumSectorToRead = 0;                     /*Total sectors for 1 read*/
    uint32_t positionOfcluster = 0;                   /*Location of cluster to read (for reading root 32 or reading sub)*/
    bool checkSubEntry = true;
    bool checkMainEntry = true;
    FATFS_ListEntry_struct_t *entry = NULL;
    FATFS_ListEntry_struct_t *previousEntry = NULL;
    FATFS_ListEntry_struct_t *tailEntry = NULL;
    uint32_t location = 0;

    location = locationToRead;
    /*Delete the old list*/
    previousEntry = s_HeadOfListEntry;
    while (NULL != tailEntry)
    {
        previousEntry->next = tailEntry;
        free(previousEntry);
        previousEntry = tailEntry;
    }
    if ((0 == location) && (FATFS_END_OF_FILE_FAT32 != s_EndOfFile)) /*If reading root of fat 12 or 16*/
    {
        /*Root 12 or 16*/
        location = s_InformationOfFatFs.locationOfRoot;
        sumSectorToRead = s_InformationOfFatFs.sumSectorOfRoot;
        sizeOfBuffer = sumSectorToRead * s_InformationOfFatFs.bytePerSector;
        /*Initialize buffer and first node*/
        buffer = (uint8_t *)malloc(sizeOfBuffer);
        entry = (FATFS_ListEntry_struct_t *)malloc(sizeof(FATFS_ListEntry_struct_t));
        entry->next = NULL;
        entry->entry.longFileName[0] = 0; /*Assume there is no Long file name*/
        s_HeadOfListEntry = entry;
        previousEntry = entry;

        if (sizeOfBuffer == HAL_ReadMultiSector(location, sumSectorToRead, buffer))
        {
            for (i = 0; i < sizeOfBuffer; i += FATFS_SIZE_ENTRY_BYTE) /*Because an entry has 32 bytes, we read in hops of 32 .*/
            {
                checkSubEntry = FATFS_ProcessSubEntry(&buffer[i], entry);   /*Read by sub entry. If true, return true*/
                checkMainEntry = FATFS_ProcessMainEntry(&buffer[i], entry); /*Read by main entry. If true, return true*/
                if (true == checkMainEntry)
                {
                    /*This is the main entry*/
                    status = true; /*Had node entry*/

                    /*Save node*/
                    if (previousEntry == s_HeadOfListEntry)
                    {
                        /*Do nothing*/
                    }
                    else if (NULL == s_HeadOfListEntry->next)
                    {
                        s_HeadOfListEntry->next = previousEntry;
                        tailEntry = previousEntry;
                    }
                    else
                    {
                        tailEntry->next = previousEntry;
                        tailEntry = previousEntry;
                    }

                    /*Creat next node*/
                    entry = (FATFS_ListEntry_struct_t *)malloc(sizeof(FATFS_ListEntry_struct_t));
                    entry->next = NULL;
                    entry->entry.longFileName[0] = 0; /*Assume there is no Long file name*/
                    previousEntry = entry;
                }
                else if ((false == checkSubEntry) && ((false == checkMainEntry)))
                {
                    /*This is the end of the file*/
                    free(entry);
                    break;
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
        if (0 == location)
        {
            /*Root 32*/
            positionOfcluster = s_InformationOfFatFs.stratClusterOfRootOfFat32;
        }
        else
        {
            /*Sub directory*/
            positionOfcluster = location;
        }
        location = s_InformationOfFatFs.locationOfData + (positionOfcluster - 2) * s_InformationOfFatFs.sectorPerCluster; /*Because the data area starts to be used from cluster 2. So must be subtracted*/
        sumSectorToRead = s_InformationOfFatFs.sectorPerCluster;
        sizeOfBuffer = sumSectorToRead * s_InformationOfFatFs.bytePerSector;
        /*Initialize buffer and first node*/
        buffer = (uint8_t *)malloc(sizeOfBuffer * sizeof(uint8_t));
        entry = (FATFS_ListEntry_struct_t *)malloc(sizeof(FATFS_ListEntry_struct_t));
        entry->next = NULL;
        entry->entry.longFileName[0] = 0; /*Assume there is no Long file name*/
        s_HeadOfListEntry = entry;
        previousEntry = entry;

        /*Read and save entry*/
        while (sizeOfBuffer == HAL_ReadMultiSector(location, sumSectorToRead, buffer))
        {
            for (i = 0; i < sizeOfBuffer; i += FATFS_SIZE_ENTRY_BYTE) /*Because an entry has 32 bytes, we read in hops of 32 .*/
            {
                checkSubEntry = FATFS_ProcessSubEntry(&buffer[i], entry);   /*Read by sub entry. If true, return true*/
                checkMainEntry = FATFS_ProcessMainEntry(&buffer[i], entry); /*Read by main entry. If true, return true*/
                if (true == checkMainEntry)
                {
                    /*This is the main entry*/
                    status = true; /*Had node entry*/

                    /*Save node*/
                    if (previousEntry == s_HeadOfListEntry)
                    {
                        /*Do nothing*/
                    }
                    else if (NULL == s_HeadOfListEntry->next)
                    {
                        s_HeadOfListEntry->next = previousEntry;
                        tailEntry = previousEntry;
                    }
                    else
                    {
                        tailEntry->next = previousEntry;
                        tailEntry = previousEntry;
                    }

                    /*Creat next node*/
                    entry = (FATFS_ListEntry_struct_t *)malloc(sizeof(FATFS_ListEntry_struct_t));
                    entry->next = NULL;
                    entry->entry.longFileName[0] = 0; /*Assume there is no Long file name*/
                    previousEntry = entry;
                }
                else if ((false == checkSubEntry) && ((false == checkMainEntry)))
                {
                    /*This is the end of the file*/
                    free(entry);
                    break;
                    checkWhileLoop = false;
                }
                else
                {
                    /*Do nothing*/
                }
            }

            if (false == checkWhileLoop)
            {
                break; /*Break for while*/
            }
            else
            {
                /*read next cluster*/
                positionOfcluster = s_BufferForFat[positionOfcluster];
                location = s_InformationOfFatFs.locationOfData + (positionOfcluster - 2) * s_InformationOfFatFs.sectorPerCluster; /*Because the data area starts to be used from cluster 2. So must be subtracted*/
                if ((s_EndOfFile != positionOfcluster))
                {
                    /*Do nothing*/
                }
                else
                {
                    break; /*Break for while*/
                }
            }
        }
    }

    if (true == status)
    {
        /*Had node*/
        returnListEntry = s_HeadOfListEntry;
    }
    else
    {
        free(entry);
        returnListEntry = NULL;
        s_HeadOfListEntry = NULL;
    }

    free(buffer);

    return returnListEntry;
}

bool FATFS_ReadData(uint32_t *const positionOfcluster, uint8_t *const buffer)
{
    uint32_t locationOfSelected = 0; /*Start sector position to read*/
    bool status = true;              /*return value */

    /*Read data of cluster (*positionOfcluster) */
    locationOfSelected = s_InformationOfFatFs.locationOfData + (*positionOfcluster - 2) * s_InformationOfFatFs.sectorPerCluster; /*Because the data area starts to be used from cluster 2. So must be subtracted*/
    HAL_ReadMultiSector(locationOfSelected, s_InformationOfFatFs.sectorPerCluster, buffer);

    /*Check next cluster*/
    *positionOfcluster = s_BufferForFat[*positionOfcluster];
    if (s_EndOfFile != *positionOfcluster)
    {
        status = true;
    }
    else
    {
        status = false;
    }

    return status;
}

void FATFS_DeInit(void)
{
    HAL_DeInit(); /*Close FAT file system*/
}

/************************************************************************************
 * Static function
 *************************************************************************************/

static bool FATFS_ProcessSubEntry(const uint8_t *const buffer, FATFS_ListEntry_struct_t *const node)
{
    bool status = true;                                              /*return value */
    uint8_t i = 0;                                                   /*Index value*/
    uint8_t j = 0;                                                   /*Index value*/
    static bool s_checkNewSubEntry = true;                           /*check new sub directory*/
    static FATFS_LongFileName_struct_t *s_headOfLongFileName = NULL; /*Head pointer of list long file name(LFN)*/
    FATFS_LongFileName_struct_t *nodeOfLongFileName = NULL;          /*node of list LFN*/
    FATFS_LongFileName_struct_t *previousNodeOfLongFileName = NULL;  /*node of list LFN*/

    /*If this is a subdirectory and the 0th element of the buffer is non-zero (end of file message)*/
    if (FATFS_SUB_ENTRY == buffer[FATFS_ATTRIBUTE_OF_FILE_OFFSET] && (FATFS_END_OF_ENTRY != buffer[0]))
    {
        /*Creat node of list LFN*/
        if (true == s_checkNewSubEntry)
        {
            nodeOfLongFileName = (FATFS_LongFileName_struct_t *)malloc(sizeof(FATFS_LongFileName_struct_t));
            nodeOfLongFileName->next = NULL;
            s_headOfLongFileName = nodeOfLongFileName;
            s_checkNewSubEntry = false;
        }
        else
        {
            nodeOfLongFileName = (FATFS_LongFileName_struct_t *)malloc(sizeof(FATFS_LongFileName_struct_t));
            nodeOfLongFileName->next = s_headOfLongFileName;
            s_headOfLongFileName = nodeOfLongFileName;
        }

        /*Save name fields of this sub entry*/
        for (i = 0; i < FATFS_SIZE_ENTRY_BYTE; i++)
        {
            if ((1 <= i) && (10 >= i) && (0 != buffer[i]) && (0xff != buffer[i])) /*Field 1 : From bit 1 to bit 10 . Do not store characters 0 and 0xff*/
            {
                nodeOfLongFileName->stringName[j] = buffer[i];
                j++;
            }
            else if ((14 <= i) && (25 >= i) && (0 != buffer[i]) && (0xff != buffer[i])) /*Field 2 : From bit 14 to bit 25 . Do not store characters 0 and 0xff*/
            {
                nodeOfLongFileName->stringName[j] = buffer[i];
                j++;
            }
            else if ((28 <= i) && (31 >= i) && (0 != buffer[i]) && (0xff != buffer[i])) /*Field 3 : From bit 28 to bit 32 . Do not store characters 0 and 0xff*/
            {
                nodeOfLongFileName->stringName[j] = buffer[i];
                j++;
            }
            else
            {
                /*Do nothing*/
            }
        }
        nodeOfLongFileName->stringName[j] = 0; /* add end of string*/

        /*Check if this is the last LFN by comparing the first 5 bits ( 0x1f - mask) of the 0th byte with 0x01*/
        if (0x01u == (buffer[0] & 0x1f))
        {

            previousNodeOfLongFileName = s_headOfLongFileName;
            s_headOfLongFileName = NULL;
            j = 0;
            /*Save all characters of list LFn to (node->entry.fileName) */
            while (NULL != previousNodeOfLongFileName)
            {
                i = 0;
                while (0 != previousNodeOfLongFileName->stringName[i])
                {
                    node->entry.longFileName[j] = previousNodeOfLongFileName->stringName[i];
                    i++;
                    j++;
                }
                nodeOfLongFileName = previousNodeOfLongFileName;
                previousNodeOfLongFileName = previousNodeOfLongFileName->next;
                free(nodeOfLongFileName);
            }
            node->entry.longFileName[j] = 0; /* add end of string*/
        }
        else
        {
            /*do nothing*/
        }
        status = true;
    }
    else
    {
        s_headOfLongFileName = NULL;
        s_checkNewSubEntry = true;
        status = false;
    }

    return status;
}

static bool FATFS_ProcessMainEntry(const uint8_t *const buffer, FATFS_ListEntry_struct_t *const node)
{
    bool status = true; /*return value */
    uint16_t temp = 0;
    uint8_t i = 0;

    /*If this is not a subdirectory and the 0th element of the buffer is non-zero (end of file message)*/
    if (FATFS_SUB_ENTRY != buffer[FATFS_ATTRIBUTE_OF_FILE_OFFSET] && (FATFS_END_OF_ENTRY != buffer[0]))
    {

        for (i = 0; i < 8; i++)
        {
            node->entry.shortFileName[i] = buffer[i];
        }
        node->entry.shortFileName[8] = 0; /* add end of string*/
        /*Save information of file (folder)*/

        /*attributes*/
        node->entry.attributes = buffer[FATFS_ATTRIBUTE_OF_FILE_OFFSET];
        /*creat time*/
        temp = FATFS_CONVERT_2_BYTES(&buffer[FATFS_CREATE_TIME_FILE_OFFSET]);
        node->entry.creatTime.seconds = 0;
        node->entry.creatTime.seconds |= (temp >> FATFS_FIELD_SECONDS_SHIFT_RIGHT) & FATFS_FIELD_SECONDS_MASK;
        node->entry.creatTime.minutes = 0;
        node->entry.creatTime.minutes |= (temp >> FATFS_FIELD_MINUTES_SHIFT_RIGHT) & FATFS_FIELD_MINUTES_MASK;
        node->entry.creatTime.hours = 0;
        node->entry.creatTime.hours |= (temp >> FATFS_FIELD_HOURS_SHIFT_RIGHT) & FATFS_FIELD_HOURS_MASK;
        /*Creat date*/
        temp = FATFS_CONVERT_2_BYTES(&buffer[FATFS_CREATE_DATE_FILE_OFFSET]);
        node->entry.creatDate.day = 0;
        node->entry.creatDate.day |= (temp >> FATFS_FIELD_DAY_SHIFT_RIGHT) & FATFS_FIELD_DAY_MASK;
        node->entry.creatDate.month = 0;
        node->entry.creatDate.month |= (temp >> FATFS_FIELD_MONTH_SHIFT_RIGHT) & FATFS_FIELD_MONTH_MASK;
        node->entry.creatDate.year = 0;
        node->entry.creatDate.year |= (temp >> FATFS_FIELD_YEAR_SHIFT_RIGHT) & FATFS_FIELD_YEAR_MASK;
        /*last modified time*/
        temp = FATFS_CONVERT_2_BYTES(&buffer[FATFS_LAST_MOD_TIME_FILE_OFFSET]);
        node->entry.lastModTime.seconds = 0;
        node->entry.lastModTime.seconds |= (temp >> FATFS_FIELD_SECONDS_SHIFT_RIGHT) & FATFS_FIELD_SECONDS_MASK;
        node->entry.lastModTime.minutes = 0;
        node->entry.lastModTime.minutes |= (temp >> FATFS_FIELD_MINUTES_SHIFT_RIGHT) & FATFS_FIELD_MINUTES_MASK;
        node->entry.lastModTime.hours = 0;
        node->entry.lastModTime.hours |= (temp >> FATFS_FIELD_HOURS_SHIFT_RIGHT) & FATFS_FIELD_HOURS_MASK;
        /*last modified date*/
        temp = FATFS_CONVERT_2_BYTES(&buffer[FATFS_LAST_MOD_DATE_FILE_OFFSET]);
        node->entry.lastModDate.day = 0;
        node->entry.lastModDate.day |= (temp >> FATFS_FIELD_DAY_SHIFT_RIGHT) & FATFS_FIELD_DAY_MASK;
        node->entry.lastModDate.month = 0;
        node->entry.lastModDate.month |= (temp >> FATFS_FIELD_MONTH_SHIFT_RIGHT) & FATFS_FIELD_MONTH_MASK;
        node->entry.lastModDate.year = 0;
        node->entry.lastModDate.year |= (temp >> FATFS_FIELD_YEAR_SHIFT_RIGHT) & FATFS_FIELD_YEAR_MASK;
        /*last access date*/
        temp = FATFS_CONVERT_2_BYTES(&buffer[FATFS_LAST_ACCESS_DATE_FILE_OFFSET]);
        node->entry.lastAccessDate.day = 0;
        node->entry.lastAccessDate.day |= (temp >> FATFS_FIELD_DAY_SHIFT_RIGHT) & FATFS_FIELD_DAY_MASK;
        node->entry.lastAccessDate.month = 0;
        node->entry.lastAccessDate.month |= (temp >> FATFS_FIELD_MONTH_SHIFT_RIGHT) & FATFS_FIELD_MONTH_MASK;
        node->entry.lastAccessDate.year = 0;
        node->entry.lastAccessDate.year |= (temp >> FATFS_FIELD_YEAR_SHIFT_RIGHT) & FATFS_FIELD_YEAR_MASK;
        /*First cluster of file ( folder)*/
        node->entry.firstCluster = 0;
        node->entry.firstCluster = FATFS_CONVERT_2_BYTES(&buffer[FATFS_LOW_WORD_OF_ADDRESS_CLUSTER_OFFSET]);
        node->entry.firstCluster |= (FATFS_CONVERT_2_BYTES(&buffer[FATFS_HIGH_WORD_OF_ADDRESS_CLUSTER_OFFSET])) << 16;
        /*size of file ( folder)*/
        node->entry.fileSize = FATFS_CONVERT_2_BYTES(&buffer[FATFS_FILE_SIZE_OFFSET]);

        status = true;
    }
    else
    {
        status = false;
    }

    return status;
}
