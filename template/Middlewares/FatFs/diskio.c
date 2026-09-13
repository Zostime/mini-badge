/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "spi_sdcard.h"  // 添加SD卡驱动头文件

/* Definitions of physical drive number for each drive */
#define SPI_SDCARD		0	// SD卡
//#define SPI_FLASH     1   //外部SPI
//#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
//#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
//#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

#define SD_BLOCKSIZE     SDCardInfo.CardBlockSize  // SD卡块大小

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* 用于标识驱动器的物理驱动器号 */
)
{
	DSTATUS status = STA_NOINIT;
	
	switch (pdrv) {
		case SPI_SDCARD: /* SD CARD */
			/*
		    if (SD_Detect()) {    
				status &= ~STA_NOINIT;
			}
			else
			{
				status = STA_NOINIT;
			}
			*/
			status &= ~STA_NOINIT;
			break;
			
		default:
			status = STA_NOINIT;
	}
	return status;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS status = STA_NOINIT;
 
	switch (pdrv) {
		
		case SPI_SDCARD:   /* SD CARD */
			// 初始化SD卡
			if (SD_Init() == SD_RESPONSE_NO_ERROR) 
			{
				status &= ~STA_NOINIT;
			} 
			else 
			{
				status = STA_NOINIT;
			}
			break;
			
		default:
			status = STA_NOINIT;
	}
	return status;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
 
	DRESULT status = RES_PARERR;
	SD_Error SD_state = SD_RESPONSE_NO_ERROR;
	
	switch (pdrv) {
		
		case SPI_SDCARD: /* SD CARD */
			// 读取SD卡扇区
			SD_state = SD_ReadMultiBlocks(buff,
										sector * SD_BLOCKSIZE,
										SD_BLOCKSIZE,
										count);
			
			if (SD_state != SD_RESPONSE_NO_ERROR)
				status = RES_ERROR;
			else
				status = RES_OK;
			break;
			
		default:
			status = RES_PARERR;
	}
 
	return status;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
 
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT status = RES_PARERR;
	SD_Error SD_state = SD_RESPONSE_NO_ERROR;
	
	if (!count){
		return RES_PARERR;		/* Check parameter */
	}
 
	switch (pdrv) {
		
		case SPI_SDCARD: /* SD CARD */
			// 写入SD卡扇区
			SD_state = SD_WriteMultiBlocks((uint8_t *)buff,
										 sector * SD_BLOCKSIZE,
										 SD_BLOCKSIZE,
										 count);
			
			if (SD_state != SD_RESPONSE_NO_ERROR)
				status = RES_ERROR;
			else
				status = RES_OK;
			break;
			
		default:
			status = RES_PARERR;
	}
 
	return status;
}
 
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
 
	DRESULT status = RES_PARERR;
	
	switch (pdrv) {
			
		case SPI_SDCARD: /* SD CARD */
			switch (cmd) {
				// 获取扇区大小 (WORD)
				case GET_SECTOR_SIZE :
					*(WORD *)buff = SD_BLOCKSIZE;
					break;
					
				// 获取擦除块大小 (DWORD)
				case GET_BLOCK_SIZE :
					*(DWORD *)buff = 1;
					break;
					
				// 获取扇区数量
				case GET_SECTOR_COUNT:
					*(DWORD *)buff = SDCardInfo.CardCapacity / SDCardInfo.CardBlockSize;
					break;
					
				case CTRL_SYNC :
				// SD卡不需要额外的同步操作
					break;
			}
			status = RES_OK;
			break;
			
		default:
			status = RES_PARERR;
	}
 
	return status;
}
