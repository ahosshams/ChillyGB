#include "../includes/cpu.h"
#include "../includes/apu.h"
#include "../includes/ppu.h"
#include "../includes/input.h"
#include "../includes/timer.h"
#include "../includes/opcodes.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

const uint8_t rst_vec[] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
const uint16_t clock_tac_shift[] = {0x200, 0x8, 0x20, 0x80};

uint8_t stretch_number(uint8_t num) {
    uint8_t t = (num | (num << 2)) & 0b00110011;
    return ((t + (t | 0b1010101)) ^ 0b1010101) & 0b11111111;
}

bool load_bootrom(rom *bootrom) {
    char *bootrom_path = "res/bootrom/dmg_boot.bin";
    FILE *file = fopen(bootrom_path, "rb");
    if (file == NULL) {
        return false;
    }
    fread(bootrom->data, 0x100, 1, file);
    return true;
}

void initialize_cpu_memory(cpu *c, settings *s) {
    srand(time(NULL));
    c->pc = 0;
    c->ime = false;
    c->ime_to_be_setted = 0;
    c->is_halted = false;
    c->bootrom.is_enabled = true;

    timer1.tima = 0;
    timer1.tma = 0;
    timer1.module = 0;
    timer1.is_tac_on = false;
    timer1.t_states = 0;
    timer1.rtc_timer = 0;

    reset_apu_regs(c);

    video.is_on = false;
    video.lyc_select = false;
    video.mode_select = 0;
    video.scan_line = 0;
    timer1.scanline_timer = 456;
    c->memory[DMA] = 0xff;
    c->memory[SCX] = 0;
    c->memory[SCY] = 0;
    c->memory[WX] = 0;
    c->memory[WY] = 0;
    video.obp[0][0] = 3;
    video.obp[0][1] = 3;
    video.obp[0][2] = 3;
    video.obp[0][3] = 3;
    video.obp[1][0] = 3;
    video.obp[1][1] = 3;
    video.obp[1][2] = 3;
    video.obp[1][3] = 3;
    c->memory[IE] = 0x00;

    c->cart.bank_select = 1;
    c->cart.bank_select_ram = 0;
    c->cart.ram_enable = false;
    c->cart.mbc1mode = false;

    // Randomize WRAM, HRAM
    for (uint16_t i = 0xc000; i <= 0xdfff; i++) {
        c->memory[i] = rand();
    }
    for (uint16_t i = 0xff80; i <= 0xfffe; i++) {
        c->memory[i] = rand();
    }
    // Initialize VRAM
    for (uint16_t i = 0x8000; i <= 0x9fff; i++) {
        c->memory[i] = 0;
    }
}

void initialize_cpu_memory_no_bootrom(cpu *c, settings *s) {
    srand(time(NULL));
    c->r.reg8[A] = 0x01;
    c->r.reg8[B] = 0x00;
    c->r.reg8[C] = 0x13;
    c->r.reg8[D] = 0x00;
    c->r.reg8[E] = 0xd8;
    c->r.reg8[F] = 0xb0;
    c->r.reg8[H] = 0x01;
    c->r.reg8[L] = 0x4d;
    c->pc = 0x100;
    c->sp = 0xfffe;
    c->ime = false;
    c->ime_to_be_setted = 0;
    c->is_halted = false;
    video.need_bg_wn_reload = true;
    video.tilemap_write = true;
    video.tiles_write = true;
    video.need_sprites_reload = true;
    c->bootrom.is_enabled = false;

    c->memory[SB] = 0x00;
    c->memory[SC] = 0x7e;
    timer1.tima = 0;
    timer1.tma = 0;
    timer1.module = 0;
    timer1.is_tac_on = false;
    timer1.rtc_timer = 0;
    c->memory[IF] = 0xe1;

    audio.is_on = true;
    audio.ch1.is_active = true;
    audio.ch2.is_active = false;
    audio.ch3.is_active = false;
    audio.ch4.is_active = false;
    audio.ch1.volume = 0;
    audio.ch1.env_dir = 0;
    audio.ch1.is_triggered = false;
    audio.ch2.is_triggered = false;
    audio.ch3.is_triggered = false;
    audio.ch4.is_triggered = false;
    audio.ch4.period_value = 8;
    audio.pan[0] = 0.5f;
    audio.pan[1] = 0.5f;
    audio.pan[2] = 0.0f;
    audio.pan[3] = 0.0f;
    c->memory[NR10] = 0x80;
    c->memory[NR11] = 0xbf;
    c->memory[NR12] = 0xf3;
    c->memory[NR13] = 0xff;
    c->memory[NR14] = 0xbf;

    c->memory[NR21] = 0x3f;
    c->memory[NR22] = 0x00;
    c->memory[NR23] = 0xff;
    c->memory[NR24] = 0xbf;

    c->memory[NR30] = 0x7f;
    c->memory[NR31] = 0xff;
    c->memory[NR32] = 0x9f;
    c->memory[NR33] = 0xff;
    c->memory[NR34] = 0xbf;

    c->memory[NR41] = 0xff;
    c->memory[NR42] = 0x00;
    c->memory[NR43] = 0x00;
    c->memory[NR44] = 0xbf;

    c->memory[NR50] = 0x77;
    c->memory[NR51] = 0xf3;
    c->memory[NR52] = 0xf1;

    c->memory[LCDC] = 0x91;
    video.bg_enable = true;
    video.obj_enable = false;
    video.obj_size = false;
    video.bg_tilemap = false;
    video.bg_tiles = true;
    video.window_enable = false;
    video.window_tilemap = false;
    video.is_on = true;
    video.lyc_select = false;
    video.mode_select = 0;
    video.mode = 1;
    c->memory[SCY] = 0x00;
    c->memory[SCX] = 0x00;
    video.scan_line = 0x00;
    timer1.scanline_timer = 456;
    c->memory[LYC] = 0x00;
    c->memory[DMA] = 0xff;
    c->memory[BGP] = 0xfc;
    video.bgp[0] = 0;
    video.bgp[1] = 3;
    video.bgp[2] = 3;
    video.bgp[3] = 3;
    video.obp[0][0] = 3;
    video.obp[0][1] = 3;
    video.obp[0][2] = 3;
    video.obp[0][3] = 3;
    video.obp[1][0] = 3;
    video.obp[1][1] = 3;
    video.obp[1][2] = 3;
    video.obp[1][3] = 3;
    c->memory[WY] = 0x00;
    c->memory[WX] = 0x00;
    c->memory[IE] = 0x00;

    c->cart.bank_select = 1;
    c->cart.bank_select_ram = 0;
    c->cart.ram_enable = false;
    c->cart.mbc1mode = false;

    // Initialize WRAM
    for (uint16_t i = 0xc000; i <= 0xdfff; i++) {
        c->memory[i] = rand();
    }

    // Initialize HRAM
    for (uint16_t i = 0xff80; i <= 0xfffe; i++) {
        c->memory[i] = rand();
    }

    // Initialize VRAM
    for (uint16_t i = 0x8000; i <= 0x9fff; i++) {
        c->memory[i] = 0;
    }

    // Initialize Background tiles
    for (uint16_t i = 0; i < 12; i++) {
        c->memory[0x9904 + i] = i+1;
        c->memory[0x9924 + i] = i+13;
    }
    c->memory[0x9910] = 0x19;

    // Initialize tiles data
    uint8_t logo_tiles_initial[24][2];
    uint8_t logo_tiles[24][4];

    for (uint16_t i = 0; i < 24; i++) {
        logo_tiles_initial[i][0] = c->cart.data[0][0x104 + (i * 2)];
        logo_tiles_initial[i][1] = c->cart.data[0][0x104 + (i * 2) + 1];
    }
    for (uint16_t i = 0; i < 24; i++) {
        logo_tiles[i][0] = stretch_number(logo_tiles_initial[i][0] >> 4);
        logo_tiles[i][1] = stretch_number(logo_tiles_initial[i][0] & 0xf);
        logo_tiles[i][2] = stretch_number(logo_tiles_initial[i][1] >> 4);
        logo_tiles[i][3] = stretch_number(logo_tiles_initial[i][1] & 0xf);
    }

    for (uint16_t i = 0; i < 24; i++) {
        for (uint16_t j = 0; j < 4; j ++) {
            c->memory[0x8010 + (i << 4) + (j*4)] = logo_tiles[i][j];
            c->memory[0x8010 + (i << 4) + (j*4)+2] = logo_tiles[i][j];
        }
    }

    uint8_t r_tile[] = {0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c};
    for (uint16_t i = 0; i < 8; i++) {
        c->memory[0x8190 + (i * 2)] = r_tile[i];
    }

    // Initialize internal timer
    timer1.t_states = 23440324;
}

bool is_stat_condition() {
    if (video.ly_eq_lyc && video.lyc_select) {
        return true;
    }
    else if ((video.mode_select >> video.mode) & 1) {
        return true;
    }
    return false;
}

void tick_scanline(cpu *c) {
    timer1.scanline_timer -= 4;
    bool prev_stat = is_stat_condition();
    if (timer1.scanline_timer < 0) {
        timer1.scanline_timer += 456;
        video.scan_line++;
        video.mode3_duration = 0;
        if (video.scan_line == 144) {
            video.mode = 1;
            c->memory[IF] |= 1;
            if (video.mode_select >> 2 && !prev_stat)
                c->memory[IF] |= 2;
            video.draw_screen = true;
        }

        if (video.scan_line > 153) {
            video.scan_line = 0;
            video.wy_trigger = false;
            video.window_internal_line = 0;
        }

        if (video.scan_line == 153 && c->memory[LYC] == 153 && timer1.scanline_timer == 452) {
            video.ly_eq_lyc = true;
        }
        else {
            video.ly_eq_lyc = (get_mem(c, LY) == c->memory[LYC]);
        }

        if (update_keys())
            c->memory[IF] |= 16;
    }

    if (video.scan_line < 144) {
        // Mode 2
        if (timer1.scanline_timer > 376 && video.mode != 2) {
            oam_scan();
            if (video.scan_line == c->memory[WY])
                video.wy_trigger = true;
            video.mode = 2;
        }
        // Mode 3
        else if (timer1.scanline_timer >= (205 - video.mode3_duration) && timer1.scanline_timer <= 376 &&
                 video.mode != 3) {
            video.mode3_duration = get_mode3_duration(c);
            video.mode = 3;
        }
        // Mode 0
        else if (timer1.scanline_timer < (205 - video.mode3_duration) && video.mode != 0) {
            load_display(c);
            video.mode = 0;
        }
    }
    if (is_stat_condition() && !prev_stat)
        c->memory[IF] |= 2;
}

void add_ticks(cpu *c, uint16_t ticks) {
    timer1.timer_global += ticks;
    ticks >>= 2;
    for (int i = 0; i < ticks; i++) {
        if (video.is_on) {
            tick_scanline(c);
        }
        else {
            timer1.lcdoff_timer -= 4;
            if (timer1.lcdoff_timer < 0) {
                timer1.lcdoff_timer += 70224;
                load_display(c);
                video.draw_screen = true;
                if (update_keys())
                    c->memory[IF] |= 16;
            }
        }

        uint32_t next_timer = timer1.t_states + 4;
        if (timer1.reset_timer) {
            timer1.reset_timer = false;
            next_timer = 4;
        }

        if ((timer1.t_states & 0x1000) > (next_timer & 0x1000)) {
            c->apu_div++;
            if (c->apu_div % 2 == 0) {
                c->sound_lenght = true;
            }
            if (c->apu_div % 4 == 0) {
                c->freq_sweep = true;
                c->freq_sweep_pace++;
            }
            if (c->apu_div % 8 == 0) {
                c->envelope_sweep = true;
                c->envelope_sweep_pace++;
            }
        }

        if (timer1.delay) {
            timer1.delay = false;
            timer1.tima = timer1.tma;
            c->memory[IF] |= 4;
        }

        if (timer1.is_tac_on) {
            if ((timer1.t_states & clock_tac_shift[timer1.module]) >
                (next_timer & clock_tac_shift[timer1.module])) {
                timer1.tima++;
                if (timer1.tima == 0) {
                    timer1.delay = true;
                }
            }
        }

        timer1.t_states = next_timer;

        if (!c->cart.rtc.is_halted) {
            timer1.rtc_timer += 4;
            if (set.accurate_rtc) {
                if (timer1.rtc_timer > 4194304) {
                    timer1.rtc_timer = 0;
                    c->cart.rtc.time++;
                }
            }
            else {
                if (timer1.rtc_timer > (4194304) * speed_mult) {
                    timer1.rtc_timer = 0;
                    c->cart.rtc.time++;
                }
            }
        }
    }
}

void run_interrupt(cpu *c) {
    if ((c->memory[IE] & c->memory[IF]) != 0) {
        c->ime = false;
        c->is_halted = false;
        c->sp--;
        set_mem(c, c->sp, (uint8_t)(c->pc >> 8));
        c->sp--;
        set_mem(c, c->sp, (uint8_t)(c->pc));
        if (((c->memory[IE] & 1) == 1) && ((c->memory[IF] & 1) == 1)) {
            c->memory[IF] &= 0b11111110;
            c->pc = 0x40;
        }
        else if (((c->memory[IE] & 2) == 2) && ((c->memory[IF] & 2) == 2)) {
            c->memory[IF] &= 0b11111101;
            c->pc = 0x48;
        }
        else if (((c->memory[IE] & 4) == 4) && ((c->memory[IF] & 4) == 4)) {
            c->memory[IF] &= 0b11111011;
            c->pc = 0x50;
        }
        else if (((c->memory[IE] & 8) == 8) && ((c->memory[IF] & 8) == 8)) {
            c->memory[IF] &= 0b11110111;
            c->pc = 0x58;
        }
        else if (((c->memory[IE] & 16) == 16) && ((c->memory[IE] & 16) == 16)) {
            c->memory[IF] &= 0b11101111;
            c->pc = 0x60;
        }
        add_ticks(c, 20);
    }
}

void dma_transfer(cpu *c) {
    uint16_t to_transfer = (uint16_t)(c->memory[DMA]) << 8;
    for (int i = 0; i < 160; i++) {
        c->memory[0xfe00 + i] = get_mem(c, (to_transfer+i));
    }
}

uint8_t execute_instruction(cpu *c) {
    parameters p = {};

    uint8_t opcode = get_mem(c, c->pc);
    p.imm8 = get_mem(c, (c->pc+1));
    p.imm16 = ((uint16_t)get_mem(c, (c->pc+2)) << 8) | get_mem(c, (c->pc+1));

    p.condition = (opcode & 0b00011000) >> 3;

    p.dest_source_r16mem = (opcode & 0b00110000) >> 4;

    if (opcode != 0xcb)
        p.operand_r8 = opcode & 0b00000111;
    else
        p.operand_r8 = p.imm8 & 0b00000111;

    p.operand_r16 = (opcode & 0b00110000) >> 4;
    p.operand_stk_r8 = (opcode & 0b00111000) >> 3;
    p.operand_stk_r16 = (opcode & 0b00110000) >> 4;

    p.tgt3 = rst_vec[(opcode & 0b00111000) >> 3];
    switch(opcode) {
        case 0xc3:
            return jp(c, &p);
        case 0x06: case 0x16: case 0x26: case 0x36: case 0x0e: case 0x1e: case 0x2e: case 0x3e:
            return ld_r8_imm8(c, &p);
        case 0x04: case 0x14: case 0x24: case 0x34: case 0x0c: case 0x1c: case 0x2c: case 0x3c:
            return inc_r8(c, &p);
        case 0x05: case 0x15: case 0x25: case 0x35: case 0x0d: case 0x1d: case 0x2d: case 0x3d:
            return dec_r8(c, &p);
        case 0xea:
            return ld_imm16_a(c, &p);
        case 0xfa:
            return ld_a_imm16(c, &p);
        case 0xfe:
            return cp_a_imm8(c, &p);
        case 0xda: case 0xc2: case 0xd2: case 0xca:
            return jp_cond(c, &p);
        case 0x01: case 0x11: case 0x21: case 0x31:
            return ld_r16_imm16(c, &p);
        case 0xe9:
            return jp_hl(c, &p);
        case 0x0a: case 0x1a: case 0x2a: case 0x3a:
            return ld_a_r16mem(c, &p);
        case 0x02: case 0x12: case 0x22: case 0x32:
            return ld_r16mem_a(c, &p);
        case 0x03: case 0x13: case 0x23: case 0x33:
            return inc_r16(c, &p);
        case 0x0b: case 0x1b: case 0x2b: case 0x3b:
            return dec_r16(c, &p);
        case 0x40 ... 0x75: case 0x77 ... 0x7f:
            return ld_r8_r8(c, &p);
        case 0xb0 ... 0xb7:
            return or_a_r8(c, &p);
        case 0x00:
            return nop(c, &p);
        case 0x08:
            return ld_imm16_sp(c, &p);
        case 0x20: case 0x30: case 0x28: case 0x38:
            return jr_cond(c, &p);
        case 0x18:
            return jr(c, &p);
        case 0xf3:
            return di(c, &p);
        case 0xe0:
            return ldh_imm8_a(c, &p);
        case 0xf0:
            return ldh_a_imm8(c, &p);
        case 0xcd:
            return call(c, &p);
        case 0xc9:
            return ret(c, &p);
        case 0xc1: case 0xd1: case 0xe1: case 0xf1:
            return pop(c, &p);
        case 0xc5: case 0xd5: case 0xe5: case 0xf5:
            return push(c, &p);
        case 0xa0 ... 0xa7:
            return and_a_r8(c, &p);
        case 0xa8 ... 0xaf:
            return xor_a_r8(c, &p);
        case 0xb8 ... 0xbf:
            return cp_a_r8(c, &p);
        case 0xe6:
            return and_a_imm8(c, &p);
        case 0xee:
            return xor_a_imm8(c, &p);
        case 0xc4: case 0xd4: case 0xcc: case 0xdc:
            return call_cond(c, &p);
        case 0xc6:
            return add_a_imm8(c, &p);
        case 0xd6:
            return sub_a_imm8(c, &p);
        case 0xcb:
            return prefix(c, &p);
        case 0x1f:
            return rra(c, &p);
        case 0xce:
            return adc_a_imm8(c, &p);
        case 0xc0: case 0xd0: case 0xc8: case 0xd8:
            return ret_cond(c, &p);
        case 0x09: case 0x19: case 0x29: case 0x39:
            return add_hl_r16(c, &p);
        case 0x27:
            return daa(c, &p);
        case 0xe2:
            return ldh_c_a(c, &p);
        case 0xf2:
            return ldh_a_c(c, &p);
        case 0xd9:
            return reti(c, &p);
        case 0x2f:
            return cpl(c, &p);
        case 0x3f:
            return ccf(c, &p);
        case 0x37:
            return scf(c, &p);
        case 0xf9:
            return ld_sp_hl(c, &p);
        case 0xfb:
            return ei(c, &p);
        case 0xc7: case 0xd7: case 0xe7: case 0xf7: case 0xcf: case 0xdf: case 0xef: case 0xff:
            return rst(c, &p);
        case 0xde:
            return sbc_a_imm8(c, &p);
        case 0xf6:
            return or_a_imm8(c, &p);
        case 0x80 ... 0x87:
            return add_a_r8(c, &p);
        case 0x88 ... 0x8f:
            return adc_a_r8(c, &p);
        case 0x90 ... 0x97:
            return sub_a_r8(c, &p);
        case 0x98 ... 0x9f:
            return sbc_a_r8(c, &p);
        case 0x17:
            return rla(c, &p);
        case 0x0f:
            return rrca(c, &p);
        case 0x07:
            return rlca(c, &p);
        case 0xf8:
            return ld_hl_sp_imm8(c, &p);
        case 0xe8:
            return add_sp_imm8(c, &p);
        case 0x76:
            return halt(c, &p);
        case 0x10:
            return stop(c, &p);
    }
    printf("%x Invalid OPCODE", opcode);
    return 0;
}

void execute(cpu *c) {
    if (c->ime == true)
        run_interrupt(c);

    if (!c->is_halted) {
        uint8_t ticks = execute_instruction(c);
        add_ticks(c, ticks);
    }
    else {
        add_ticks(c, 4);
        if ((c->memory[IE] & c->memory[IF]) != 0) {
            c->pc++;
            c->is_halted = false;
        }
    }

    if (c->ime_to_be_setted == 1) {
        c->ime_to_be_setted = 2;
    }
    else if (c->ime_to_be_setted == 2) {
        c->ime_to_be_setted = 0;
        c->ime = true;
    }
}
