#include "arch/cpuid.h"
#include "coreos/types.h"

void cpuid_ler_info(cpu_info_t *info) {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t regs[3][4];
    size_t i;
    size_t j;
    size_t offset;

    if (!info) {
        return;
    }

    info->vendor[0] = '\0';
    info->brand[0] = '\0';
    info->family = 0;
    info->model = 0;
    info->stepping = 0;
    info->features_edx = 0;
    info->features_ecx = 0;

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    if (eax == 0) {
        return;
    }

    ((uint32_t *)info->vendor)[0] = ebx;
    ((uint32_t *)info->vendor)[1] = edx;
    ((uint32_t *)info->vendor)[2] = ecx;
    info->vendor[12] = '\0';

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    info->stepping = eax & 0xFu;
    info->model = (eax >> 4) & 0xFu;
    info->family = (eax >> 8) & 0xFu;
    if (info->family == 0xFu) {
        info->family += (eax >> 20) & 0xFFu;
        info->model += ((eax >> 16) & 0xFu) << 4;
    }
    info->features_edx = edx;
    info->features_ecx = ecx;

    for (i = 0; i < 3; ++i) {
        __asm__ volatile("cpuid"
                         : "=a"(eax), "=b"(regs[i][0]), "=c"(regs[i][2]), "=d"(regs[i][1])
                         : "a"(0x80000002u + i));
    }

    offset = 0;
    for (i = 0; i < 3; ++i) {
        const char *chunk = (const char *)regs[i];
        for (j = 0; j < 16 && offset + 1 < FIRMWARE_CPU_BRAND_LEN; ++j) {
            info->brand[offset++] = chunk[j];
        }
    }
    info->brand[FIRMWARE_CPU_BRAND_LEN - 1u] = '\0';
}

void x86_cpu_brand(char *buffer, size_t size) {
    cpu_info_t info;
    size_t i;

    if (!buffer || size == 0) {
        return;
    }

    cpuid_ler_info(&info);
    for (i = 0; i < size - 1 && info.brand[i] != '\0'; ++i) {
        buffer[i] = info.brand[i];
    }
    buffer[i] = '\0';
}

uint32_t x86_cpu_count(void) {
    return 1;
}
