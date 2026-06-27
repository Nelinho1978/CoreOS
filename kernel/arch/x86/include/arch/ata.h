#ifndef COREOS_ARCH_X86_ATA_H
#define COREOS_ARCH_X86_ATA_H

#include "coreos/types.h"

/* Primary IDE controller I/O ports */
#define ATA_PRIMARY_DATA     0x1F0
#define ATA_PRIMARY_ERROR    0x1F1
#define ATA_PRIMARY_SECTORS  0x1F2
#define ATA_PRIMARY_LBA_LO   0x1F3
#define ATA_PRIMARY_LBA_MID  0x1F4
#define ATA_PRIMARY_LBA_HI   0x1F5
#define ATA_PRIMARY_DRIVE    0x1F6
#define ATA_PRIMARY_CMD      0x1F7
#define ATA_PRIMARY_STATUS   0x1F7
#define ATA_PRIMARY_CTRL     0x3F6

/* ATA commands */
#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_READ_PIO_EXT 0x24
#define ATA_CMD_WRITE_PIO    0x30
#define ATA_CMD_IDENTIFY     0xEC
#define ATA_CMD_FLUSH        0xE7

/* Status register bits */
#define ATA_STATUS_ERR       0x01
#define ATA_STATUS_DRQ       0x08
#define ATA_STATUS_SRV       0x10
#define ATA_STATUS_DF        0x20
#define ATA_STATUS_RDY       0x40
#define ATA_STATUS_BSY       0x80

/* Initialize ATA controller */
int ata_init(void);

/* Read 'count' sectors starting at LBA into buffer */
int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer);

/* Write 'count' sectors from buffer to disk at LBA */
int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer);

/* Identify drive - fills 512-byte buffer with IDENTIFY data */
int ata_identify(uint16_t *buffer);

/* Check if ATA drive is present */
int ata_drive_present(void);

#endif
