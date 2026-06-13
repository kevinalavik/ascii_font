# ascii_font
Generate ASCII font bitmap as a C header file.

Based on [hubenchang0515/ascii_font](https://github.com/hubenchang0515/ascii_font).

## Depends
* libfreetype2

## Build
```
make
```

## Usage
```
./ascii_font [options] <font-file> [font-size]
```

### Options

| Option | Description |
|--------|-------------|
| `-f, --font FILE` | Font file path |
| `-s, --size PIXELS` | Font size in pixels (default: 16) |
| `-w, --width PIXELS` | Output glyph cell width (default: auto from font) |
| `-h, --height PIXELS` | Output glyph cell height (default: auto from font) |
| `-C, --columns N` | Characters per row in grid (default: 16) |
| `-S, --start N` | First character to generate (default: 0) |
| `-E, --end N` | Last character to generate (default: 127) |
| `-o, --output FILE` | Output header file (default: ascii_font.h) |
| `-p, --preview FILE` | Preview image file (default: preview.ppm) |
| `-P, --no-preview` | Skip preview image generation |
| `-H, --no-header` | Skip header file generation |
| `-i, --invert` | Invert grayscale values |
| `-t, --threshold N` | Binarization threshold (0-255) |

### Output Files
* `ascii_font.h` - C header with font bitmap data (configurable via `-o`)
* `preview.ppm` - preview image (configurable via `-p`)

Read the array as `ascii_font[character][y][x]` to get pixel gray scale value.

### Examples

Basic usage:
```
./ascii_font FreeMono.ttf 32
```

Custom glyph dimensions:
```
./ascii_font -f FreeMono.ttf -s 32 -w 24 -h 48
```

Custom character range and grid columns:
```
./ascii_font -f FreeMono.ttf -s 16 --columns 10 -S 32 -E 126
```

Binarized + inverted output:
```
./ascii_font -f FreeMono.ttf -s 16 -i -t 128
```

### SDL2 Usage Example
```c
#include <SDL2/SDL.h>

#define ASCII_FONT_IMPLEMENTATION
#include "ascii_font.h"

void draw_char(SDL_Renderer* renderer, int x, int y, char ch, Uint8 r, Uint8 g, Uint8 b)
{
    for (int dy = 0; dy < ASCII_FONT_HEIGHT; dy++)
    {
        for (int dx = 0; dx < ASCII_FONT_WIDTH; dx++)
        {
            SDL_SetRenderDrawColor(renderer, r, g, b, ascii_font[(size_t)ch][dy][dx]);
            SDL_RenderDrawPoint(renderer, x + dx, y + dy);
        }
    }
}

void draw_text(SDL_Renderer* renderer, int x, int y, const char* text, Uint8 r, Uint8 g, Uint8 b)
{
    for (size_t i = 0; text[i] != 0; i++)
    {
        draw_char(renderer, x + i * ASCII_FONT_WIDTH, y, text[i], r, g, b);
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow("ascii_font demo",
                                            SDL_WINDOWPOS_UNDEFINED,
                                            SDL_WINDOWPOS_UNDEFINED,
                                            26 * ASCII_FONT_WIDTH,
                                            7 * ASCII_FONT_HEIGHT,
                                            SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    while (1)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Event ev;
        while(SDL_PollEvent(&ev) > 0)
        {
            if (ev.type == SDL_QUIT)
                goto EXIT;
        }

        draw_text(renderer, 0, 0, "ascii_font example", 255, 255, 255);
        draw_text(renderer, 0, ASCII_FONT_HEIGHT, "ABCDEFGHIJKLMNOPQRSTUVWXUZ", 255, 0, 0);
        draw_text(renderer, 0, 2 * ASCII_FONT_HEIGHT, "abcdefghijklmnopqrstuvwxyz", 0, 255, 0);
        draw_text(renderer, 0, 3 * ASCII_FONT_HEIGHT, "ABCDEFGHIJKLMNOPQRSTUVWXUZ", 0, 0, 255);
        draw_text(renderer, 0, 4 * ASCII_FONT_HEIGHT, "abcdefghijklmnopqrstuvwxyz", 255, 255, 0);
        draw_text(renderer, 0, 5 * ASCII_FONT_HEIGHT, "ABCDEFGHIJKLMNOPQRSTUVWXUZ", 0, 255, 255);
        draw_text(renderer, 0, 6 * ASCII_FONT_HEIGHT, "abcdefghijklmnopqrstuvwxyz", 255, 0, 255);

        SDL_RenderPresent(renderer);
    }

EXIT:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
```
