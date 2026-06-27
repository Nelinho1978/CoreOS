#ifndef COREOS_GUI_WIN10_H
#define COREOS_GUI_WIN10_H

#include "coreos/types.h"

/* Cores RGB (32 bpp) — tema Windows 10 */
#define W10_DESKTOP      0x00808080u
#define W10_ACCENT       0x0078D7u
#define W10_ACCENT_DARK  0x002133u
#define W10_TASKBAR      0xC0C0C0u
#define W10_FACE         0xC0C0C0u
#define W10_FACE_DARK    0x808080u
#define W10_FACE_LIGHT   0xFFFFFFu
#define W10_HIGHLIGHT    0xFFFFFFu
#define W10_SHADOW       0x808080u
#define W10_SHADOW_DARK  0x404040u
#define W10_TITLE_A      0x000080u
#define W10_TITLE_B      0x1084D0u
#define W10_TITLE_TEXT   0xFFFFFFu
#define W10_TEXT         0x000000u
#define W10_BLACK        0x000000u
#define W10_WHITE        0xFFFFFFu
#define W10_CMD_BG       0x000000u
#define W10_CMD_FG       0xC0C0C0u
#define W10_DIM          0x808080u
#define W10_CLOUD        0xE8E8E8u

#define W10_TASKBAR_H    28u
#define W10_TITLE_H      22u

#define W95_DESKTOP      W10_DESKTOP
#define W95_FACE         W10_FACE
#define W95_FACE_DARK    W10_FACE_DARK
#define W95_FACE_LIGHT   W10_FACE_LIGHT
#define W95_HIGHLIGHT    W10_HIGHLIGHT
#define W95_SHADOW       W10_SHADOW
#define W95_SHADOW_DARK  W10_SHADOW_DARK
#define W95_TITLE_A      W10_TITLE_A
#define W95_TITLE_B      W10_TITLE_B
#define W95_TITLE_TEXT   W10_TITLE_TEXT
#define W95_TEXT         W10_TEXT
#define W95_BLACK        W10_BLACK
#define W95_WHITE        W10_WHITE
#define W95_CMD_BG       W10_CMD_BG
#define W95_CMD_FG       W10_CMD_FG
#define W95_DIM          W10_DIM
#define W95_CLOUD        W10_CLOUD
#define W95_TASKBAR_H    W10_TASKBAR_H
#define W95_TITLE_H      W10_TITLE_H

void desktop_run(void);
void boot_gfx_show(void);

#endif
