#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define DEFAULT_COLUMNS 16
#define DEFAULT_START 0
#define DEFAULT_END 127

static void print_help(const char *prog) {
  printf("Usage: %s [options] <font-file> [font-size]\n", prog);
  printf("       %s [options] -f <font-file> -s <font-size>\n", prog);
  printf("\n");
  printf("Options:\n");
  printf("  -f, --font FILE         Font file path\n");
  printf("  -s, --size PIXELS       Font size in pixels (default: 16)\n");
  printf("  -w, --width PIXELS      Output glyph cell width (default: auto "
         "from font)\n");
  printf("  -h, --height PIXELS     Output glyph cell height (default: auto "
         "from font)\n");
  printf("  -C, --columns N         Characters per row in grid (default: %d)\n",
         DEFAULT_COLUMNS);
  printf(
      "  -S, --start N           First character to generate (default: %d)\n",
      DEFAULT_START);
  printf("  -E, --end N             Last character to generate (default: %d)\n",
         DEFAULT_END);
  printf(
      "  -o, --output FILE       Output header file (default: ascii_font.h)\n");
  printf(
      "  -p, --preview FILE      Preview image file (default: preview.ppm)\n");
  printf("  -P, --no-preview        Skip preview image generation\n");
  printf("  -H, --no-header         Skip header file generation\n");
  printf("  -i, --invert            Invert grayscale values\n");
  printf("  -t, --threshold N       Binarization threshold (0-255)\n");
  printf("  -?, --help              Show this help message\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s FreeMono.ttf\n", prog);
  printf("  %s FreeMono.ttf 32\n", prog);
  printf("  %s -f FreeMono.ttf -s 32 -w 24 -h 48\n", prog);
  printf("  %s -f FreeMono.ttf -s 16 --columns 10 -S 32 -E 126\n", prog);
}

int main(int argc, char *argv[]) {
  const char *font_file = NULL;
  unsigned long font_size = 16;
  unsigned long cell_w_input = 0, cell_h_input = 0;
  unsigned long columns = DEFAULT_COLUMNS;
  unsigned long start_char = DEFAULT_START;
  unsigned long end_char = DEFAULT_END;
  const char *output_file = "ascii_font.h";
  const char *preview_file = "preview.ppm";
  int no_preview = 0;
  int no_header = 0;
  int invert = 0;
  int threshold = -1;

  static struct option long_opts[] = {{"font", required_argument, 0, 'f'},
                                      {"size", required_argument, 0, 's'},
                                      {"width", required_argument, 0, 'w'},
                                      {"height", required_argument, 0, 'h'},
                                      {"columns", required_argument, 0, 'C'},
                                      {"start", required_argument, 0, 'S'},
                                      {"end", required_argument, 0, 'E'},
                                      {"output", required_argument, 0, 'o'},
                                      {"preview", required_argument, 0, 'p'},
                                      {"no-preview", no_argument, 0, 'P'},
                                      {"no-header", no_argument, 0, 'H'},
                                      {"invert", no_argument, 0, 'i'},
                                      {"threshold", required_argument, 0, 't'},
                                      {"help", no_argument, 0, '?'},
                                      {0, 0, 0, 0}};

  FT_Library library = NULL;
  FT_Face face = NULL;
  uint8_t *grid = NULL;
  uint8_t *cell = NULL;
  int exit_code = EXIT_SUCCESS;

  int opt;
  while ((opt = getopt_long(argc, argv, "f:s:w:h:C:S:E:o:p:PHit:?", long_opts,
                            NULL)) != -1) {
    switch (opt) {
    case 'f':
      font_file = optarg;
      break;
    case 's':
      font_size = strtoul(optarg, NULL, 10);
      break;
    case 'w':
      cell_w_input = strtoul(optarg, NULL, 10);
      break;
    case 'h':
      cell_h_input = strtoul(optarg, NULL, 10);
      break;
    case 'C':
      columns = strtoul(optarg, NULL, 10);
      break;
    case 'S':
      start_char = strtoul(optarg, NULL, 10);
      break;
    case 'E':
      end_char = strtoul(optarg, NULL, 10);
      break;
    case 'o':
      output_file = optarg;
      break;
    case 'p':
      preview_file = optarg;
      break;
    case 'P':
      no_preview = 1;
      break;
    case 'H':
      no_header = 1;
      break;
    case 'i':
      invert = 1;
      break;
    case 't':
      threshold = atoi(optarg);
      break;
    case '?':
      print_help(argv[0]);
      return EXIT_SUCCESS;
    default:
      return EXIT_FAILURE;
    }
  }

  if (optind < argc)
    font_file = argv[optind++];
  if (optind < argc)
    font_size = strtoul(argv[optind++], NULL, 10);

  if (!font_file) {
    fprintf(stderr, "Error: No font file specified.\n");
    print_help(argv[0]);
    return EXIT_FAILURE;
  }

  if (font_size == 0)
    font_size = 16;

  if (start_char > end_char) {
    fprintf(stderr,
            "Error: start character (%lu) must be <= end character (%lu)\n",
            start_char, end_char);
    return EXIT_FAILURE;
  }
  if (end_char > 255) {
    fprintf(stderr, "Error: end character (%lu) exceeds 255\n", end_char);
    return EXIT_FAILURE;
  }
  if (threshold > 255) {
    fprintf(stderr, "Error: threshold must be 0-255\n");
    return EXIT_FAILURE;
  }
  if (columns == 0)
    columns = 1;

  size_t char_count = (size_t)(end_char - start_char + 1);

  FT_Error err = FT_Init_FreeType(&library);
  if (err != FT_Err_Ok) {
    fprintf(stderr, "Failed to initialize FreeType: %d %s\n", err,
            FT_Error_String(err));
    return EXIT_FAILURE;
  }

  err = FT_New_Face(library, font_file, 0, &face);
  if (err != FT_Err_Ok) {
    fprintf(stderr, "Failed to load font '%s': %d %s\n", font_file, err,
            FT_Error_String(err));
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  unsigned long pixel_w = 0, pixel_h = font_size;
  if (cell_w_input > 0 || cell_h_input > 0) {
    FT_Pos upem = face->units_per_EM;
    if (upem <= 0)
      upem = 1000;
    FT_Pos max_adv = face->max_advance_width;
    if (max_adv <= 0)
      max_adv = upem / 2;
    FT_Pos line_h = face->ascender - face->descender;
    if (line_h <= 0)
      line_h = upem;

    if (cell_w_input > 0)
      pixel_w = cell_w_input * upem / max_adv;
    if (cell_h_input > 0)
      pixel_h = cell_h_input * upem / line_h;

    if (cell_w_input > 0 && pixel_w == 0)
      pixel_w = 1;
    if (cell_h_input > 0 && pixel_h == 0)
      pixel_h = 1;
  }

  err = FT_Set_Pixel_Sizes(face, pixel_w, pixel_h);
  if (err != FT_Err_Ok) {
    fprintf(stderr, "Failed to set font size: %d %s\n", err,
            FT_Error_String(err));
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  size_t natural_w = (size_t)(face->size->metrics.max_advance / 64);
  if (natural_w == 0)
    natural_w = font_size;
  size_t natural_h =
      (size_t)(face->size->metrics.ascender - face->size->metrics.descender) /
      64;
  if (natural_h == 0)
    natural_h = font_size;

  size_t cell_w = cell_w_input > 0 ? cell_w_input : natural_w;
  size_t cell_h = cell_h_input > 0 ? cell_h_input : natural_h;

  size_t grid_cols = columns < char_count ? columns : char_count;
  size_t grid_rows = (char_count + grid_cols - 1) / grid_cols;
  size_t grid_w = cell_w * grid_cols;
  size_t grid_h = cell_h * grid_rows;
  size_t grid_size = grid_w * grid_h;

  grid = calloc(grid_size, 1);
  if (!grid) {
    fprintf(stderr, "Memory allocation failed\n");
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  cell = malloc(cell_w * cell_h);
  if (!cell) {
    fprintf(stderr, "Memory allocation failed\n");
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  for (size_t ch = start_char; ch <= end_char; ch++) {
    FT_UInt index = FT_Get_Char_Index(face, (FT_ULong)ch);
    err = FT_Load_Glyph(face, index, FT_LOAD_RENDER);
    if (err != FT_Err_Ok)
      continue;
    err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (err != FT_Err_Ok)
      continue;

    FT_Bitmap *glyph = &face->glyph->bitmap;
    ptrdiff_t top = (ptrdiff_t)(face->size->metrics.ascender / 64) -
                    face->glyph->bitmap_top;
    ptrdiff_t left = face->glyph->bitmap_left;

    memset(cell, 0, cell_w * cell_h);
    for (ptrdiff_t y = 0; y < (ptrdiff_t)glyph->rows; y++) {
      for (ptrdiff_t x = 0; x < (ptrdiff_t)glyph->width; x++) {
        ptrdiff_t cy = top + y;
        ptrdiff_t cx = left + x;
        if (cy >= 0 && cy < (ptrdiff_t)cell_h && cx >= 0 &&
            cx < (ptrdiff_t)cell_w) {
          cell[(size_t)cy * cell_w + (size_t)cx] =
              glyph->buffer[(size_t)y * glyph->width + (size_t)x];
        }
      }
    }

    if (invert) {
      for (size_t i = 0; i < cell_w * cell_h; i++)
        cell[i] = 255 - cell[i];
    }
    if (threshold >= 0) {
      uint8_t t = (uint8_t)threshold;
      for (size_t i = 0; i < cell_w * cell_h; i++)
        cell[i] = cell[i] >= t ? 255 : 0;
    }

    size_t char_idx = ch - start_char;
    size_t row = char_idx / grid_cols;
    size_t col = char_idx % grid_cols;
    for (size_t y = 0; y < cell_h; y++) {
      for (size_t x = 0; x < cell_w; x++) {
        grid[(row * cell_h + y) * grid_w + (col * cell_w + x)] =
            cell[y * cell_w + x];
      }
    }
  }

  free(cell);
  cell = NULL;

  if (!no_header) {
    FILE *header = fopen(output_file, "wb");
    if (!header) {
      fprintf(stderr, "Failed to open '%s' for writing\n", output_file);
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

    fprintf(header,
            "// Generated by: https://github.com/kevinalavik/ascii_font (fork "
            "off: https://github.com/hubenchang0515/ascii_font\n");
    fprintf(header, "#ifndef ASCII_FONT_H\n");
    fprintf(header, "#define ASCII_FONT_H\n");
    fprintf(header, "\n");
    fprintf(header, "#define ASCII_FONT_BEGIN    %lu\n", start_char);
    fprintf(header, "#define ASCII_FONT_END      %lu\n", end_char);
    fprintf(header, "#define ASCII_FONT_COUNT    %zu\n", char_count);
    fprintf(header, "#define ASCII_FONT_WIDTH    %zu\n", cell_w);
    fprintf(header, "#define ASCII_FONT_HEIGHT   %zu\n", cell_h);
    fprintf(header, "#define ASCII_FONT_COLUMNS  %zu\n", grid_cols);
    fprintf(header, "\n");
    fprintf(header, "#ifndef ASCII_FONT_IMPLEMENTATION\n");
    fprintf(header, "    unsigned char "
                    "ascii_font[][ASCII_FONT_HEIGHT][ASCII_FONT_WIDTH];\n");
    fprintf(header, "#else\n");
    fprintf(header, "    unsigned char "
                    "ascii_font[][ASCII_FONT_HEIGHT][ASCII_FONT_WIDTH] = \n");
    fprintf(header, "    {\n");
    for (size_t ch = start_char; ch <= end_char; ch++) {
      size_t char_idx = ch - start_char;
      fprintf(header, "        {\n");
      for (size_t y = 0; y < cell_h; y++) {
        fprintf(header, "            {");
        for (size_t x = 0; x < cell_w; x++) {
          size_t gi = (char_idx / grid_cols) * cell_h + y;
          size_t gj = (char_idx % grid_cols) * cell_w + x;
          fprintf(header, "%u, ", (unsigned)grid[gi * grid_w + gj]);
        }
        fprintf(header, "},\n");
      }
      fprintf(header, "        },\n");
    }
    fprintf(header, "    };\n");
    fprintf(header, "#endif  // ASCII_FONT_IMPLEMENTATION\n");
    fprintf(header, "\n");
    fprintf(header, "#endif // ASCII_FONT_H\n");
    fclose(header);
    printf("Generated: %s\n", output_file);
  }

  if (!no_preview) {
    FILE *preview = fopen(preview_file, "wb");
    if (!preview) {
      fprintf(stderr, "Failed to open '%s' for writing\n", preview_file);
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

    fprintf(preview, "P5\n%zu %zu 255\n", grid_w, grid_h);
    fwrite(grid, grid_size, 1, preview);
    fclose(preview);
    printf("Generated: %s\n", preview_file);
  }

cleanup:
  free(cell);
  free(grid);
  if (face)
    FT_Done_Face(face);
  if (library)
    FT_Done_FreeType(library);
  return exit_code;
}
