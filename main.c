#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define MIN(a, b) ((a)<(b)? (a) : (b))

int main(void) {
    // Initialize CPU and memory
    cpu c = {.pc = 0x100, .sp = 0xfffe};
    c.r.reg8[A] = 0x01;
    c.r.reg8[B] = 0x00;
    c.r.reg8[C] = 0x13;
    c.r.reg8[D] = 0x00;
    c.r.reg8[E] = 0xd8;
    c.r.reg8[F] = 0xb0;
    c.r.reg8[H] = 0x01;
    c.r.reg8[L] = 0x4d;
    c.memory[0xff44] = 0x90;

    // Initialize Timer
    tick t = {.tima_counter = 0, .divider_register = 0, .scan_line_tick = 0, .t_states = 0};

    // Initialize PPU
    ppu p = {};
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*3, 144*3, "ChillyGB");
    SetTargetFPS(60);
    RenderTexture2D display = LoadRenderTexture(160, 144);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color){185, 237, 186, 255};

    //FILE *cartridge = fopen("../Roms/HelloWorld.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/DrMario.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/Tetris.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/01-special.gb", "r");
    FILE *cartridge = fopen("../Roms/Private/bgbtest.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/02-interrupts.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/03-op sp,hl.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/04-op r,imm.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/05-op rp.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/06-ld r,r.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/07-jr,jp,call,ret,rst.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/08-misc instrs.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/09-op r,r.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/10-bit ops.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/11-op a,(hl).gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/dmg-acid2.gb", "r");
    assert(cartridge != NULL && "File not found");
    assert(fread(&c.memory[0], 0x8000, 1, cartridge) >= 0);

    int ticks = 0;
    while(!WindowShouldClose()) {
        if (t.t_states == 7974256)
            printf("A:%.2X F:%.2X B:%.2X C:%.2X D:%.2X E:%.2X H:%.2X L:%.2X SP:%.4X PC:%.4X PCMEM:%.2X,%.2X,%.2X,%.2X\n",
                c.r.reg8[A], c.r.reg8[F], c.r.reg8[B], c.r.reg8[C], c.r.reg8[D], c.r.reg8[E], c.r.reg8[H], c.r.reg8[L],
                c.sp, c.pc, c.memory[c.pc], c.memory[c.pc+1], c.memory[c.pc+2], c.memory[c.pc+3]);
        execute(&c, &t);
        c.memory[0xff00] = (c.memory[0xff00] & 0xf0) + 0xf;

        if (t.is_frame) {
            t.is_frame = false;
            load_display(&c, &p);
            for (int y = 0; y < 144; y++) {
                for (int x = 0; x < 160; x++) {
                    if (p.display[y][x] == 0)
                        pixels[y][x] = (Color) {185, 237, 186, 255};
                    else if (p.display[y][x] == 1)
                        pixels[y][x] = (Color) {118, 196, 123, 255};
                    else if (p.display[y][x] == 2)
                        pixels[y][x] = (Color) {49, 106, 64, 255};
                    else
                        pixels[y][x] = (Color) {10, 38, 16, 255};
                }
            }

            float scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
            BeginTextureMode(display);
                ClearBackground(RAYWHITE);  // Clear render texture background color
                for (int i = 0; i < 144; i++)
                    for (int j = 0; j < 160; j++)
                        DrawRectangle(j, -i+143, 1, 1, pixels[i][j]);
            EndTextureMode();

            // Draw
            BeginDrawing();
                ClearBackground(BLACK);
                DrawTexturePro(display.texture, (Rectangle) {0.0f, 0.0f, (float) display.texture.width, (float) display.texture.height},
                               (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                            (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                            (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
            EndDrawing();
        }

        printf("%d\n", t.t_states);
        ticks += 1;
    }

    UnloadRenderTexture(display);

    CloseWindow();

    return 0;
}
