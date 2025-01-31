#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

#ifndef CHILLYGB_PPU_H
#define CHILLYGB_PPU_H

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t tile[8][8];
    uint8_t tile_16[16][8];

    bool priority;
    bool palette;
}sprite;

typedef struct {
    uint8_t color;
    bool priority;
    bool palette;
    uint8_t x;
}sprite_px;

typedef struct {
    uint8_t mode;
    uint8_t scan_line;
    uint8_t display[144][160];
    uint8_t background[256][256];
    uint8_t window[256][256];
    uint8_t tiles[256][8][8];
    uint8_t tilemap[2][1024];
    sprite sprites[40];
    sprite_px sprite_display[2][176][176];

    sprite_px sprite_line[176];
    uint8_t oam_buffer[40];
    uint8_t buffer_size;

    uint8_t window_internal_line;
    bool wy_trigger;

    uint8_t mode3_duration;

    uint8_t bgp[4];
    uint8_t obp[2][4];

    bool is_on;
    bool bg_enable;
    bool window_enable;
    bool obj_enable;

    bool bg_tiles;
    bool bg_tilemap;
    bool window_tilemap;
    bool obj_size;

    bool is_scan_line;
    bool draw_screen;
    bool dma_transfer;
    bool tilemap_write;
    bool tiles_write;
    bool need_bg_wn_reload;
    bool need_sprites_reload;
    bool reset_sprite_display;

    bool lyc_select;
    uint8_t mode_select;
    bool ly_eq_lyc;
}ppu;

extern ppu video;

void load_display(cpu *c);
uint16_t get_mode3_duration(cpu *c);
void oam_scan();

#endif //CHILLYGB_PPU_H
