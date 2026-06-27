#include "arch/ata.h"
#include "arch/ports.h"
#include "coreos/printk.h"

/* ---- Internal state ---- */
static int g_ata_initialized = 0;
static int g_ata_present = 0;

/* ---- Wait for drive to be ready (no BSY) ---- */
static int ata_wait_bsy(void) {
    uint32_t timeout = 1000000;
    while (timeout--) {
        if (!(inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY))
            return 1;
        __asm__ volatile("pause");
    }
    return 0;
}

/* ---- Wait for DRQ (data request) or error ---- */
static int ata_wait_drq(void) {
    uint8_t status;
    uint32_t timeout = 1000000;

    while (timeout--) {
        status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_STATUS_ERR)
            return 0;
        if (status & ATA_STATUS_DRQ)
            return 1;
        __asm__ volatile("pause");
    }
    return 0;
}

/* ---- Software reset ---- */
static void ata_soft_reset(void) {
    outb(ATA_PRIMARY_CTRL, 0x04);  /* SRST */
    uint32_t delay;
    for (delay = 0; delay < 10000; ++delay)
        __asm__ volatile("pause");
    outb(ATA_PRIMARY_CTRL, 0x00);
    for (delay = 0; delay < 10000; ++delay)
        __asm__ volatile("pause");
}

/* ---- Initialize ATA Primary Master ---- */
int ata_init(void) {
    if (g_ata_initialized)
        return g_ata_present;

    kputs("[ATA] Inicializando controladora IDE primaria...\n");

    ata_soft_reset();

    /* Select master drive */
    outb(ATA_PRIMARY_DRIVE, 0xA0);  /* Select master */
    {
        uint32_t delay;
        for (delay = 0; delay < 1000; ++delay)
            __asm__ volatile("pause");
    }

    /* Check if drive exists: write to sector count and LBA regs, read back */
    outb(ATA_PRIMARY_SECTORS, 0x55);
    outb(ATA_PRIMARY_LBA_LO, 0xAA);
    {
        uint8_t sc = inb(ATA_PRIMARY_SECTORS);
        uint8_t ll = inb(ATA_PRIMARY_LBA_LO);

        if (sc == 0x55 && ll == 0xAA) {
            kputs("[ATA] Drive mestre presente\n");
            g_ata_present = 1;
        } else {
            /* Try slave */
            outb(ATA_PRIMARY_DRIVE, 0xB0);
            for (uint32_t d = 0; d < 1000; ++d)
                __asm__ volatile("pause");
            outb(ATA_PRIMARY_SECTORS, 0x55);
            outb(ATA_PRIMARY_LBA_LO, 0xAA);
            sc = inb(ATA_PRIMARY_SECTORS);
            ll = inb(ATA_PRIMARY_LBA_LO);
            if (sc == 0x55 && ll == 0xAA) {
                kputs("[ATA] Drive escravo presente\n");
                g_ata_present = 1;
            } else {
                kputs("[ATA] Nenhum drive ATA detectado\n");
                g_ata_present = 0;
            }
        }
    }

    g_ata_initialized = 1;
    return g_ata_present;
}

int ata_drive_present(void) {
    if (!g_ata_initialized)
        return ata_init();
    return g_ata_present;
}

/* ---- Identify Drive ---- */
int ata_identify(uint16_t *buffer) {
    if (!g_ata_present)
        return 0;

    if (!ata_wait_bsy())
        return 0;

    /* Send IDENTIFY command */
    outb(ATA_PRIMARY_DRIVE, 0xA0);  /* Master */
    outb(ATA_PRIMARY_SECTORS, 0);
    outb(ATA_PRIMARY_LBA_LO, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HI, 0);
    outb(ATA_PRIMARY_CMD, ATA_CMD_IDENTIFY);

    /* Check if device is present */
    {
        uint8_t status = inb(ATA_PRIMARY_STATUS);
        if (status == 0) {
            kputs("[ATA] Nenhum dispositivo em IDENTIFY\n");
            return 0;
        }
    }

    if (!ata_wait_bsy())
        return 0;

    if (!ata_wait_drq()) {
        kputs("[ATA] ERRO no IDENTIFY\n");
        return 0;
    }

    /* Read 256 words (512 bytes) */
    for (int i = 0; i < 256; ++i)
        buffer[i] = inw(ATA_PRIMARY_DATA);

    kputs("[ATA] IDENTIFY concluido\n");
    return 1;
}

/* ---- Read Sectors (PIO, 28-bit LBA) ---- */
int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer) {
    uint16_t *buf = (uint16_t *)buffer;
    uint32_t i;

    if (!g_ata_present || !buffer || count == 0)
        return 0;

    if (!ata_wait_bsy())
        return 0;

    /* Set up LBA and sector count */
    outb(ATA_PRIMARY_SECTORS, count);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));  /* LBA mode, master */
    outb(ATA_PRIMARY_CMD, ATA_CMD_READ_PIO);

    /* Read sectors */
    for (i = 0; i < count; ++i) {
        if (!ata_wait_bsy())
            return 0;
        if (!ata_wait_drq())
            return 0;

        /* Read 256 words (512 bytes) */
        for (int j = 0; j < 256; ++j)
            buf[i * 256 + j] = inw(ATA_PRIMARY_DATA);
    }

    return 1;
}

/* ---- Write Sectors (PIO, 28-bit LBA) ---- */
int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer) {
    const uint16_t *buf = (const uint16_t *)buffer;
    uint32_t i;

    if (!g_ata_present || !buffer || count == 0)
        return 0;

    if (!ata_wait_bsy())
        return 0;

    /* Set up LBA and sector count */
    outb(ATA_PRIMARY_SECTORS, count);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_CMD, ATA_CMD_WRITE_PIO);

    for (i = 0; i < count; ++i) {
        if (!ata_wait_bsy())
            return 0;
        if (!ata_wait_drq())
            return 0;

        for (int j = 0; j < 256; ++j)
            outw(ATA_PRIMARY_DATA, buf[i * 256 + j]);

        /* Flush */
        outb(ATA_PRIMARY_CMD, ATA_CMD_FLUSH);
        if (!ata_wait_bsy())
            return 0;
    }

    return 1;
}
