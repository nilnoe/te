#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
// #include <SDL3_ttf/SDL_ttf.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "la.h"

#define WINDOW_WIDTH 1244
#define WINDOW_HEIGHT 740

#define FONT_SCALE 5.0f * 2
#define FONT_WIDTH 128.0
#define FONT_HEIGHT 64.0
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)

void scc(bool code) {
    if (!code) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
}

void *scp(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        exit(1);
    }
    return ptr;
}

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

typedef struct {
    SDL_Texture *spritesheet;
    SDL_FRect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

Font fontLoadFromFile(const char *file_path, SDL_Renderer *r) {
    Font font = {0};
    SDL_Surface *font_surface = IMG_Load(file_path);
    font.spritesheet = scp(SDL_CreateTextureFromSurface(r, font_surface));
    SDL_DestroySurface(font_surface);
    SDL_SetTextureScaleMode(font.spritesheet, SDL_SCALEMODE_NEAREST);
    for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ++ascii) {
        const size_t index = ascii - ASCII_DISPLAY_LOW;
        const size_t col = index % FONT_COLS;
        const size_t row = index / FONT_COLS;

        font.glyph_table[index] = (SDL_FRect) {
            .x = col * floorf(FONT_CHAR_WIDTH),
            .y = row * floorf(FONT_CHAR_HEIGHT),
            .w = floorf(FONT_CHAR_WIDTH),
            .h = floorf(FONT_CHAR_HEIGHT)
        };
    }
    return font;
}

void renderChar
(SDL_Renderer *r, Font *font, Vec2f pos, char c, float scale)
{
    const SDL_FRect dst = {
        .x = floorf(pos.x),
        .y = floorf(pos.y),
        .w = floorf(FONT_CHAR_WIDTH * scale),
        .h = floorf(FONT_CHAR_HEIGHT * scale)
    };
    assert(c >= ASCII_DISPLAY_LOW);
    assert(c <= ASCII_DISPLAY_HIGH);
    const size_t index = c - ASCII_DISPLAY_LOW;

    scc(SDL_RenderTexture(r, font->spritesheet, 
                &font->glyph_table[index], &dst));
}

void renderTextSized(SDL_Renderer *r, Font *font, Vec2f pos, const char *text, 
 size_t text_size, Uint32 color, float scale) {
    scc(SDL_SetTextureColorMod(
		font->spritesheet,
		(color >> (8 * 0)) & 0xff,
		(color >> (8 * 1)) & 0xff,
		(color >> (8 * 2)) & 0xff));
    scc(SDL_SetTextureAlphaMod(font->spritesheet, (color >> (8 * 3)) & 0xff));
    Vec2f pen = pos;
    for (size_t i = 0; i < text_size; ++i) {
        renderChar(r, font, pen, text[i], scale);
        pen.x += FONT_CHAR_WIDTH * scale;
    }

}

void renderText
(SDL_Renderer *r, Font *font, Vec2f pos, const char *text, Uint32 color, float scale)
{
    renderTextSized(r, font, pos, text, strlen(text), color, scale);
}

#define BUFFER_CAPACITY 1024
char buffer[BUFFER_CAPACITY];
size_t buffer_size = 0;
size_t buffer_cursor = 0;

#define UNHEX(color) \
    ((color) >> (8 * 0)) & 0xFF, \
    ((color) >> (8 * 1)) & 0xFF, \
    ((color) >> (8 * 2)) & 0xFF, \
    ((color) >> (8 * 3)) & 0xFF

void renderCursor(SDL_Renderer *r, Uint32 color) {
    const SDL_FRect rect = {
        .x = floorf(buffer_cursor * FONT_CHAR_WIDTH * FONT_SCALE) + 10.0f,
        .y = 0 + 10.0f,
        .w = FONT_CHAR_WIDTH * FONT_SCALE,
        .h = FONT_CHAR_HEIGHT * FONT_SCALE,
    };
    scc(SDL_SetRenderDrawColor(r, UNHEX(color)));
    scc(SDL_RenderFillRect(r, &rect));
}

int main(void) {
    scc(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = scp(SDL_CreateWindow("Text Editor",
                WINDOW_WIDTH, WINDOW_HEIGHT,
                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));
    SDL_Renderer *renderer = scp(SDL_CreateRenderer(window, NULL));
    // 开启 vsync,让 SDL_RenderPresent 跟显示器刷新率同步
    scc(SDL_SetRenderVSync(renderer, 1));
    Font font = fontLoadFromFile("font.png", renderer);
    SDL_StartTextInput(window);
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_WaitEventTimeout(&event, 16)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: { running = false; SDL_HideWindow(window);} break;
                case SDL_EVENT_KEY_DOWN: {
                    switch (event.key.key) {
                        case SDLK_BACKSPACE: {
                            if (buffer_size > 0) { 
                                buffer_size -= 1;
                                buffer_cursor = buffer_size;
                            }
                        } break;
                    }
                } break;
                case SDL_EVENT_TEXT_INPUT: {
                    size_t text_size = strlen(event.text.text);
                    const size_t free_space = BUFFER_CAPACITY - text_size;
                    if (text_size > free_space) { text_size = free_space; }
                    memcpy(buffer + buffer_size, event.text.text, text_size);
                    buffer_size += text_size;
                    buffer_cursor = buffer_size;
                } break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        renderTextSized(renderer, &font,vec2f(10.0f, 10.0f),
                buffer, buffer_size, 0xFFFFFFFF, FONT_SCALE);
        renderCursor(renderer, 0xFFFFFFFF);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
