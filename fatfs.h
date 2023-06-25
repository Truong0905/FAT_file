#ifndef __FATFS_H__
#define __FATFS_H__

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*
 *Struct stores the file's time field
 */
typedef struct
{
    uint8_t seconds; /*Seconds/2 (0–29)*/
    uint8_t minutes; /*Minutes (0–59)*/
    uint8_t hours;   /*Hours (0–23)*/

} FATFS_Time_Struct_t; // time

/*
 *Struct stores the file's date field
 */
typedef struct
{
    uint8_t day;   /*Day (1–31)*/
    uint8_t month; /*Month (1–12)*/
    uint8_t year;  /*Year (0 = 1980, 119 = 2099 supported under DOS/Windows, theoretically up to 127 = 2107)*/

} FATFS_Date_Struct_t; // date

/*
 * Store the information in the entry
 */
typedef struct
{
    uint8_t longFileName[256]; /*Use to save long file name (the maximum size of long file name according to wiki is 255 characters) +1 '\0'*/
    uint8_t shortFileName[9];  /*Use to save long file name (the maximum size of short file name according to wiki is 8 characters) +1 '\0'*/
    uint8_t attributes;
    FATFS_Time_Struct_t creatTime;
    FATFS_Date_Struct_t creatDate;
    FATFS_Date_Struct_t lastAccessDate;
    FATFS_Time_Struct_t lastModTime;
    FATFS_Date_Struct_t lastModDate;
    uint32_t firstCluster;
    uint32_t fileSize;
} FATFS_Entry_Struct_t;

/*
 *Struct for list of entry
 */
typedef struct __FATFS_ListEntry_struct_t
{
    FATFS_Entry_Struct_t entry;
    struct __FATFS_ListEntry_struct_t *next;
} FATFS_ListEntry_struct_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**  FATFS_Init
 * @brief Open the file FAT and store information in the boot sector
 * @param[in] filePath   The path to the file
 * @return uint16_t Returns the number of bytes per cluster (if zero, initialization fails)
 */
uint16_t FATFS_Init(const uint8_t *const filePath);

/**  FATFS_ReadDirectory
 * @brief Read root directory or sub directory
 * @param[in] locationToRead   Location of root or first cluster of sub. If it's fat 32, it could be the first cluster location of root
 * @return FATFS_ListEntry_struct_t* returns the head pointer of a singly linked list of entries
 */
FATFS_ListEntry_struct_t *FATFS_ReadDirectory(const uint32_t locationToRead);

/**  FATFS_ReadData
 * @brief Read data at specified cluster
 * @param[inout] positionOfcluster   Cluster location to read
 * @param[out] buffer   Receiver array
 * @return bool If there is data in the next cluster, return true
 */
bool FATFS_ReadData(uint32_t *const positionOfcluster, uint8_t *const buffer);

/**  FATFS_DeInit
 * @brief Close the file FAT
 * @return none
 */
void FATFS_DeInit(void);

#endif /*__FATFS_H__*/
