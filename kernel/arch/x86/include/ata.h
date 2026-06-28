#ifndef COREOS_ARCH_ATA_H
#define COREOS_ARCH_ATA_H

#include "nt/ntdef.h"

/* ATA I/O Ports */
#define ATA_DATA_PORT           0x1F0
#define ATA_ERROR_PORT          0x1F1
#define ATA_SECTOR_COUNT_PORT   0x1F2
#define ATA_SECTOR_NUM_PORT     0x1F3
#define ATA_CYL_LOW_PORT        0x1F4
#define ATA_CYL_HIGH_PORT       0x1F5
#define ATA_DRIVE_HEAD_PORT     0x1F6
#define ATA_STATUS_PORT         0x1F7
#define ATA_COMMAND_PORT        0x1F7
#define ATA_CONTROL_PORT        0x3F6

/* ATA Commands */
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_IDENTIFY        0xEC

/* Status bits */
#define ATA_SR_BSY              0x80  /* Controller busy */
#define ATA_SR_DRDY             0x40  /* Drive ready */
#define ATA_SR_DRQ              0x08  /* Data request ready */
#define ATA_SR_ERR              0x01  /* Error */

/* Public API */
int ata_init(void);
int ata_drive_present(void);
int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer);
int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer);

#endif
