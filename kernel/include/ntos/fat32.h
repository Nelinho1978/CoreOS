#ifndef COREOS_NTOS_FAT32_H
#define COREOS_NTOS_FAT32_H

#include "coreos/types.h"

#define FAT32_SECTOR_SIZE   512
#define FAT32_MAX_PATH      256
#define FAT32_MAX_OPEN      8

/* FAT32 file attributes */
#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_ATTR_LFN        0x0F

/* FAT32 directory entry (short name, 32 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  name[11];        /* Short name (8.3) */
    uint8_t  attr;            /* File attributes */
    uint8_t  nt_reserved;     /* WinNT reserved */
    uint8_t  creation_tenth;  /* Creation time (tenths of sec) */
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_hi; /* High 16 bits of first cluster (FAT32) */
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_lo; /* Low 16 bits of first cluster */
    uint32_t file_size;
} FAT32_DIR_ENTRY;

/* FAT32 BPB (BIOS Parameter Block) */
typedef struct __attribute__((packed)) {
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_descriptor;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    /* FAT32 extended */
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} FAT32_BPB;

/* FAT32 filesystem state */
typedef struct {
    int         initialized;
    uint32_t    fat_start;         /* LBA of first FAT */
    uint32_t    data_start;        /* LBA of data region */
    uint32_t    root_cluster;      /* First cluster of root dir */
    uint32_t    sectors_per_cluster;
    uint32_t    bytes_per_cluster;
    uint32_t    total_sectors;
    uint32_t    fat_size;
    uint8_t     sectors_per_cluster_val;
    FAT32_BPB   bpb;
} FAT32_FS;

/* Open file descriptor */
typedef struct {
    char     name[FAT32_MAX_PATH];
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t file_size;
    uint32_t offset;
    uint32_t sector_in_cluster;
    int      open;
    uint8_t  attr;
} FAT32_FILE;

/* Initialize FAT32 from disk */
int fat32_init(void);

/* List root directory */
int fat32_list_root(void);

/* Open a file by path (from root) */
int fat32_open(const char *name, FAT32_FILE *file);

/* Read from an open file */
int fat32_read(FAT32_FILE *file, void *buffer, uint32_t size, uint32_t *bytes_read);

/* Load a file from root into buffer (convenience) */
int fat32_load_file(const char *name, void *buffer, uint32_t *size);

/* Close an open file */
void fat32_close(FAT32_FILE *file);

#endif
