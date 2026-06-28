#include "ata.h"
#include "coreos/printk.h"
#include "arch/io.h"

static int g_ata_initialized = 0;
static int g_drive_present = 0;

/* Read a byte from ATA port */
static uint8_t ata_read_byte(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Write a byte to ATA port */
static void ata_write_byte(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Read a word (2 bytes) from ATA port */
static uint16_t ata_read_word(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Poll ATA status until ready or error */
static int ata_poll_status(uint8_t mask, uint8_t expected) {
    int timeout = 10000;
    while (timeout--) {
        uint8_t status = ata_read_byte(ATA_STATUS_PORT);
        if ((status & mask) == expected)
            return 1;
        if (status & ATA_SR_ERR)
            return 0;
    }
    return 0;
}

/* Wait for data ready */
static int ata_wait_data_ready(void) {
    return ata_poll_status(ATA_SR_DRQ, ATA_SR_DRQ);
}

/* Read 512 bytes (one sector) */
static void ata_read_sector_data(void *buffer) {
    uint16_t *ptr = (uint16_t *)buffer;
    int i;
    for (i = 0; i < 256; ++i) {  /* 512 bytes = 256 words */
        *ptr++ = ata_read_word(ATA_DATA_PORT);
    }
}

/* Write 512 bytes (one sector) */
static void ata_write_sector_data(const void *buffer) {
    const uint16_t *ptr = (const uint16_t *)buffer;
    int i;
    for (i = 0; i < 256; ++i) {  /* 512 bytes = 256 words */
        uint16_t value = *ptr++;
        __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(ATA_DATA_PORT));
    }
}

int ata_init(void) {
    if (g_ata_initialized)
        return 1;
    
    kputs("[ATA] Initializing ATA controller...\n");
    
    /* Soft reset */
    ata_write_byte(ATA_CONTROL_PORT, 0x04);
    for (int i = 0; i < 100; ++i) {
        __asm__ volatile("pause");
    }
    ata_write_byte(ATA_CONTROL_PORT, 0x00);
    
    /* Wait for drive to be ready */
    if (!ata_poll_status(ATA_SR_BSY, 0)) {
        kputs("[ATA] ERRO: Drive not responding\n");
        return 0;
    }
    
    /* Check if drive is present by issuing IDENTIFY command */
    ata_write_byte(ATA_DRIVE_HEAD_PORT, 0xA0);  /* Select drive 0 */
    ata_write_byte(ATA_COMMAND_PORT, ATA_CMD_IDENTIFY);
    
    uint8_t status = ata_read_byte(ATA_STATUS_PORT);
    if (status == 0) {
        kputs("[ATA] ERRO: No drive detected\n");
        return 0;
    }
    
    if (!ata_wait_data_ready()) {
        kputs("[ATA] ERRO: Drive not ready\n");
        return 0;
    }
    
    /* Read IDENTIFY data (but discard it for now) */
    uint8_t identify_buf[512];
    ata_read_sector_data(identify_buf);
    
    g_ata_initialized = 1;
    g_drive_present = 1;
    kputs("[ATA] ATA controller initialized successfully\n");
    
    return 1;
}

int ata_drive_present(void) {
    return g_drive_present;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer) {
    if (!g_ata_initialized || !buffer || count == 0 || count > 256)
        return 0;
    
    if (!ata_poll_status(ATA_SR_BSY, 0)) {
        kprintf("[ATA] Controller busy\n");
        return 0;
    }
    
    /* Select drive 0 and set LBA mode */
    ata_write_byte(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Write sector count */
    ata_write_byte(ATA_SECTOR_COUNT_PORT, count);
    
    /* Write LBA address */
    ata_write_byte(ATA_SECTOR_NUM_PORT, (uint8_t)(lba & 0xFF));
    ata_write_byte(ATA_CYL_LOW_PORT, (uint8_t)((lba >> 8) & 0xFF));
    ata_write_byte(ATA_CYL_HIGH_PORT, (uint8_t)((lba >> 16) & 0xFF));
    
    /* Issue READ PIO command */
    ata_write_byte(ATA_COMMAND_PORT, ATA_CMD_READ_PIO);
    
    /* Read sectors */
    int i;
    uint8_t *buf = (uint8_t *)buffer;
    for (i = 0; i < count; ++i) {
        if (!ata_wait_data_ready()) {
            kprintf("[ATA] Read error at sector %u\n", lba + i);
            return 0;
        }
        ata_read_sector_data(buf);
        buf += 512;
    }
    
    kprintf("[ATA] Read %u sectors from LBA %u\n", count, lba);
    return 1;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer) {
    if (!g_ata_initialized || !buffer || count == 0 || count > 256)
        return 0;
    
    if (!ata_poll_status(ATA_SR_BSY, 0)) {
        kprintf("[ATA] Controller busy\n");
        return 0;
    }
    
    /* Select drive 0 and set LBA mode */
    ata_write_byte(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Write sector count */
    ata_write_byte(ATA_SECTOR_COUNT_PORT, count);
    
    /* Write LBA address */
    ata_write_byte(ATA_SECTOR_NUM_PORT, (uint8_t)(lba & 0xFF));
    ata_write_byte(ATA_CYL_LOW_PORT, (uint8_t)((lba >> 8) & 0xFF));
    ata_write_byte(ATA_CYL_HIGH_PORT, (uint8_t)((lba >> 16) & 0xFF));
    
    /* Issue WRITE PIO command */
    ata_write_byte(ATA_COMMAND_PORT, ATA_CMD_WRITE_PIO);
    
    /* Write sectors */
    int i;
    const uint8_t *buf = (const uint8_t *)buffer;
    for (i = 0; i < count; ++i) {
        if (!ata_wait_data_ready()) {
            kprintf("[ATA] Write error at sector %u\n", lba + i);
            return 0;
        }
        ata_write_sector_data(buf);
        buf += 512;
    }
    
    kprintf("[ATA] Wrote %u sectors to LBA %u\n", count, lba);
    return 1;
}
