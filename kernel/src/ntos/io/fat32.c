#include "ntos/fat32.h"
#include "coreos/printk.h"

extern int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer);
extern int ata_init(void);
extern int ata_drive_present(void);

static FAT32_FS g_fs;
static uint8_t g_sector_buf[FAT32_SECTOR_SIZE];

/* ---- Read a single sector from disk ---- */
static int read_sector(uint32_t lba, void *buf) {
    return ata_read_sectors(lba, 1, buf);
}

/* ---- Read a cluster from disk into buffer ---- */
static int read_cluster(uint32_t cluster, void *buf) {
    uint32_t first_sector = g_fs.data_start + (cluster - 2) * g_fs.sectors_per_cluster_val;
    return ata_read_sectors(first_sector, g_fs.sectors_per_cluster_val, buf);
}

/* ---- Read next cluster from FAT ---- */
static uint32_t fat_get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4; /* FAT32: 4 bytes per entry */
    uint32_t fat_sector = g_fs.fat_start + (fat_offset / FAT32_SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;

    if (!read_sector(fat_sector, g_sector_buf))
        return 0xFFFFFFFF;

    uint32_t entry = *(uint32_t *)(g_sector_buf + entry_offset);
    entry &= 0x0FFFFFFF; /* Upper 4 bits reserved */

    return entry;
}

/* ---- Short name to lowercase comparison ---- */
static int name_match(const char *short_name, const char *user_name) {
    int i;
    /* Compare 8.3 format with user name */
    for (i = 0; i < 11 && short_name[i]; ++i) {
        char c = short_name[i];
        if (c >= 'A' && c <= 'Z') c += 0x20;
        if (i == 0 && c == ' ') return 0;
        if (!user_name[i]) {
            if (c == ' ') continue;
            return 0;
        }
    }
    return 1;
}

/* ---- Parse short name (8.3) to display string ---- */
static void format_short_name(const uint8_t *name, char *out, uint32_t max) {
    uint32_t i, j = 0;
    (void)max;

    for (i = 0; i < 8 && name[i] && name[i] != ' '; ++i)
        out[j++] = name[i];

    if (name[8] && name[8] != ' ') {
        out[j++] = '.';
        for (i = 8; i < 11 && name[i] && name[i] != ' '; ++i)
            out[j++] = name[i];
    }
    out[j] = '\0';
}

/* ---- Read directory entries from a cluster chain ---- */
static int read_dir_entries(uint32_t start_cluster, FAT32_DIR_ENTRY *entries, uint32_t *count) {
    uint32_t cluster = start_cluster;
    uint32_t max_entries = 512;
    uint32_t entry_idx = 0;
    /* Use the static sector buffer and read sector-by-sector instead of full cluster */
    uint32_t bytes_per_cluster = g_fs.bytes_per_cluster;
    uint32_t secs_per_cluster = g_fs.sectors_per_cluster_val;
    uint32_t sec_in_cluster = 0;

    while (cluster >= 2 && cluster < 0x0FFFFFF8 && entry_idx < max_entries) {
        sec_in_cluster = 0;
        while (sec_in_cluster < secs_per_cluster && entry_idx < max_entries) {
            uint32_t lba = g_fs.data_start + (cluster - 2) * secs_per_cluster + sec_in_cluster;
            if (!read_sector(lba, g_sector_buf))
                return 0;

            uint32_t offset = 0;
            while (offset + sizeof(FAT32_DIR_ENTRY) <= FAT32_SECTOR_SIZE) {
                FAT32_DIR_ENTRY *entry = (FAT32_DIR_ENTRY *)(g_sector_buf + offset);

                if (entry->name[0] == 0x00) {
                    *count = entry_idx;
                    return 1;
                }

                if (entry->name[0] != 0xE5 && (entry->attr & FAT32_ATTR_LFN) != FAT32_ATTR_LFN) {
                    if (entry_idx < max_entries) {
                        entries[entry_idx] = *entry;
                        entry_idx++;
                    }
                }

                offset += sizeof(FAT32_DIR_ENTRY);
            }
            sec_in_cluster++;
        }
        cluster = fat_get_next_cluster(cluster);
    }

    *count = entry_idx;
    return 1;
}

/* ============================================================
 *  Public API
 * ============================================================ */

int fat32_init(void) {
    if (g_fs.initialized)
        return 1;

    kputs("[FAT32] Inicializando...\n");

    /* Initialize ATA first */
    if (!ata_init()) {
        kputs("[FAT32] ERRO: ATA nao disponivel\n");
        return 0;
    }

    /* Read boot sector (LBA 0) */
    if (!read_sector(0, g_sector_buf)) {
        kputs("[FAT32] ERRO: nao foi possivel ler setor de boot\n");
        return 0;
    }

    /* Parse BPB */
    FAT32_BPB *bpb = (FAT32_BPB *)g_sector_buf;

    /* Check for 0x55AA signature */
    if (g_sector_buf[510] != 0x55 || g_sector_buf[511] != 0xAA) {
        kputs("[FAT32] ERRO: assinatura de boot invalida\n");
        return 0;
    }

    g_fs.bpb = *bpb;
    g_fs.sectors_per_cluster_val = bpb->sectors_per_cluster;
    g_fs.sectors_per_cluster = bpb->sectors_per_cluster;
    g_fs.bytes_per_cluster = (uint32_t)bpb->bytes_per_sector * bpb->sectors_per_cluster;
    g_fs.fat_size = bpb->fat_size_32;
    g_fs.fat_start = bpb->reserved_sectors;
    g_fs.root_cluster = bpb->root_cluster;
    g_fs.data_start = bpb->reserved_sectors + bpb->num_fats * bpb->fat_size_32;
    g_fs.total_sectors = bpb->total_sectors_32 ? bpb->total_sectors_32 : bpb->total_sectors_16;

    kprintf("[FAT32] Setor/cluster: %u\n", (uint32_t)bpb->bytes_per_sector);
    kprintf("[FAT32] Setores/cluster: %u\n", (uint32_t)bpb->sectors_per_cluster);
    kprintf("[FAT32] Tamanho FAT: %u setores\n", bpb->fat_size_32);
    kprintf("[FAT32] Root cluster: %u\n", bpb->root_cluster);
    kprintf("[FAT32] Data start LBA: %u\n", g_fs.data_start);
    kprintf("[FAT32] Total setores: %u\n", g_fs.total_sectors);
    kprintf("[FAT32] Volume: %c%c%c%c%c%c%c%c%c%c%c\n",
            bpb->volume_label[0], bpb->volume_label[1],
            bpb->volume_label[2], bpb->volume_label[3],
            bpb->volume_label[4], bpb->volume_label[5],
            bpb->volume_label[6], bpb->volume_label[7],
            bpb->volume_label[8], bpb->volume_label[9],
            bpb->volume_label[10]);

    /* Verify FAT32 */
    if (bpb->fat_size_32 == 0) {
        kputs("[FAT32] AVISO: fat_size_32=0, pode ser FAT16\n");
    }

    g_fs.initialized = 1;
    kputs("[FAT32] Sistema de arquivos pronto\n");
    return 1;
}

int fat32_list_root(void) {
    FAT32_DIR_ENTRY entries[256];
    uint32_t count = 256;
    char name_buf[64];
    uint32_t i;

    if (!g_fs.initialized) {
        if (!fat32_init())
            return 0;
    }

    if (!read_dir_entries(g_fs.root_cluster, entries, &count)) {
        kputs("[FAT32] Erro ao ler diretorio raiz\n");
        return 0;
    }

    kprintf("Diretorio raiz: %u entradas\n", count);

    for (i = 0; i < count; ++i) {
        format_short_name(entries[i].name, name_buf, sizeof(name_buf));
        uint32_t cluster = (uint32_t)entries[i].first_cluster_hi << 16 | entries[i].first_cluster_lo;
        kprintf("  %c%c%c%c  %-12s  Cluster=%u  Size=%u\n",
                (entries[i].attr & FAT32_ATTR_DIRECTORY) ? 'D' : ' ',
                (entries[i].attr & FAT32_ATTR_HIDDEN) ? 'H' : ' ',
                (entries[i].attr & FAT32_ATTR_SYSTEM) ? 'S' : ' ',
                (entries[i].attr & FAT32_ATTR_ARCHIVE) ? 'A' : ' ',
                name_buf, cluster, entries[i].file_size);
    }

    return 1;
}

int fat32_open(const char *name, FAT32_FILE *file) {
    FAT32_DIR_ENTRY entries[256];
    uint32_t count = 256;
    uint32_t i;

    if (!g_fs.initialized) {
        if (!fat32_init())
            return 0;
    }

    if (!name || !file)
        return 0;

    if (!read_dir_entries(g_fs.root_cluster, entries, &count))
        return 0;

    for (i = 0; i < count; ++i) {
        char short_name[32];
        /* Skip volume label and LFN */
        if (entries[i].attr == FAT32_ATTR_VOLUME_ID)
            continue;
        if ((entries[i].attr & FAT32_ATTR_LFN) == FAT32_ATTR_LFN)
            continue;

        format_short_name(entries[i].name, short_name, sizeof(short_name));

        /* Compare short name with requested name (case-insensitive) */
        {
            const char *a = short_name;
            const char *b = name;
            int match = 1;
            while (*a && *b) {
                char ca = *a, cb = *b;
                if (ca >= 'A' && ca <= 'Z') ca += 0x20;
                if (cb >= 'A' && cb <= 'Z') cb += 0x20;
                if (ca != cb) { match = 0; break; }
                ++a; ++b;
            }
            if (match && *a == *b) {
                uint32_t cluster = (uint32_t)entries[i].first_cluster_hi << 16 | entries[i].first_cluster_lo;
                {
                    uint32_t j;
                    for (j = 0; j < FAT32_MAX_PATH - 1 && short_name[j]; ++j)
                        file->name[j] = short_name[j];
                    file->name[j] = '\0';
                }
                file->first_cluster = cluster;
                file->current_cluster = cluster;
                file->file_size = entries[i].file_size;
                file->offset = 0;
                file->sector_in_cluster = 0;
                file->open = 1;
                file->attr = entries[i].attr;
                return 1;
            }
        }
    }

    return 0;
}

int fat32_read(FAT32_FILE *file, void *buffer, uint32_t size, uint32_t *bytes_read) {
    uint32_t total = 0;
    uint32_t to_read = size;

    if (!file || !file->open || !buffer)
        return 0;

    if (file->offset >= file->file_size) {
        *bytes_read = 0;
        return 1;
    }

    if (to_read > file->file_size - file->offset)
        to_read = file->file_size - file->offset;

    while (to_read > 0) {
        uint32_t sec_size = FAT32_SECTOR_SIZE;
        uint32_t sec_in_cluster = file->offset / sec_size;
        uint32_t sec_offset = file->offset % sec_size;
        uint32_t chunk = sec_size - sec_offset;

        if (chunk > to_read) chunk = to_read;
        if (total + chunk > size) chunk = size - total;

        /* Read the current sector */
        uint32_t secs_per_cluster = g_fs.sectors_per_cluster_val;
        uint32_t lba = g_fs.data_start + (file->current_cluster - 2) * secs_per_cluster + sec_in_cluster;

        if (!read_sector(lba, g_sector_buf))
            return 0;

        /* Copy data */
        uint32_t i;
        for (i = 0; i < chunk; ++i)
            ((uint8_t *)buffer)[total + i] = g_sector_buf[sec_offset + i];

        total += chunk;
        file->offset += chunk;
        to_read -= chunk;

        /* If we've reached the end of this sector, advance to next cluster */
        uint32_t new_sec_in_cluster = (file->offset / sec_size) % secs_per_cluster;
        if (new_sec_in_cluster == 0 && file->offset > 0 && file->offset < file->file_size) {
            uint32_t next = fat_get_next_cluster(file->current_cluster);
            if (next >= 0x0FFFFFF8 || next == 0)
                break;
            file->current_cluster = next;
        }
    }

    *bytes_read = total;
    return 1;
}

int fat32_load_file(const char *name, void *buffer, uint32_t *size) {
    FAT32_FILE file;
    uint32_t bytes_read = 0;

    if (!fat32_open(name, &file))
        return 0;

    if (*size < file.file_size)
        *size = file.file_size;

    if (!fat32_read(&file, buffer, file.file_size, &bytes_read)) {
        fat32_close(&file);
        return 0;
    }

    *size = bytes_read;
    fat32_close(&file);
    return 1;
}

void fat32_close(FAT32_FILE *file) {
    if (file)
        file->open = 0;
}
