"""Convert file.c from https://lvgl.io/tools/fontconverter to header supported by this project."""

import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert a font file from LVGL's converter to one supported by this project"
    )
    parser.add_argument(
        "-i", "--input-file", help="Path to the file.c font file from LVGL"
    )
    parser.add_argument(
        "-o", "--output-file", help="Output path to the generated output file"
    )
    args = parser.parse_args()

    input_file = args.input_file
    output_file = args.output_file

    with open(input_file) as file:
        file_content = file.read()

    skipped_header, bitmap_header, rest_of_file = (
        file_content.partition("""/*-----------------
 *    BITMAPS
 *----------------*/""")
    )
    bitmap_and_glyph_descriptors, header, kerning_and_font_des = (
        rest_of_file.partition("""/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/""")
    )
    glyph_bitmap, header, glyph_descriptors = (
        bitmap_and_glyph_descriptors.partition("""/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/""")
    )

    upper_cap_font_name = skipped_header.partition("\n#ifndef ")[2].partition("\n")[0]
    font_name = upper_cap_font_name.lower()
    print(f"Font name: {font_name}")
    font_size = skipped_header.partition("--size ")[2].partition(" ")[0]
    print(f"Font size: {font_size}")

    line_height = kerning_and_font_des.partition(".line_height = ")[2].partition(",")[0]
    base_line = kerning_and_font_des.partition(".base_line = ")[2].partition(",")[0]
    range_start = kerning_and_font_des.partition(".range_start = ")[2].partition(",")[0]
    range_length = kerning_and_font_des.partition(".range_length = ")[2].partition(",")[
        0
    ]
    glyph_id_start = kerning_and_font_des.partition(".glyph_id_start = ")[2].partition(
        ","
    )[0]

    format_type = kerning_and_font_des.partition(".type = ")[2].partition("\n")[0]

    if format_type not in [
        "LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY",
        "LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL",
    ]:
        # TODO add support for LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL
        raise RuntimeError(f"Unsupported {format_type} font format")

    # If there is a gap in the list of symbol, a glyph_id_ofs_list_0 list created, this list map unicode in the range to their glyph
    # For instance, if you're encoding -0123456789 , there is a gap between "-" (code 45) and "0" (code 48)
    glyph_id_ofs_list = "NULL"
    glyph_id_ofs_list_0_block = ""
    if (
        "glyph_id_ofs_list =" in kerning_and_font_des
        and "kerning_and_font_des = NULL" not in kerning_and_font_des
    ):
        # look for this:
        # static const uint8_t glyph_id_ofs_list_0[] = {
        #    0, 0, 0, 1, 2, 3, 4, 5,
        #    6, 7, 8, 9, 10
        # };
        maybe_glyph_id_ofs_list_0 = kerning_and_font_des.split("\n\n")[1]
        if "glyph_id_ofs_list_0" in maybe_glyph_id_ofs_list_0:
            print("Found a glyph id list")
            glyph_id_ofs_list = f"{font_name}_glyph_id_ofs_list_0"
            glyph_id_ofs_list_0_block = maybe_glyph_id_ofs_list_0.replace(
                "glyph_id_ofs_list_0", glyph_id_ofs_list
            )

    print(f"Line height: {line_height}")
    print(f"Range: {range_start}-{int(range_start) + int(range_length)}")

    file_content_output = f"""#ifndef {upper_cap_font_name}
#define {upper_cap_font_name} 1

#include "draw_primitives.h"

/*******************************************************************************
 * Size: {font_size} px (actual height = {line_height} px)
 * Bpp: 4 bit per pixels
 ******************************************************************************/

// clang-format off
{glyph_bitmap}
// clang-format on

{glyph_descriptors}

{glyph_id_ofs_list_0_block}

lv_font_t {font_name} = {{
  .h_top_skip_px = 0,
  .h_bot_skip_px = 0,
  .spacing_px = 1,
  .unicode_first = {range_start},                                 /*First Unicode letter in this font*/
  .unicode_last = {int(range_start) + int(range_length)},                                 /*Last Unicode letter in this font*/
  .h_px = {line_height},                                          /*Font height in pixels*/
  .base_line = {base_line},                                      /*Baseline measured from the bottom of the line*/
  .glyph_bitmap = {font_name}_glyph_bitmap, /*Bitmap of glyphs*/
  .glyph_dsc = nullptr,
  .new_glyph_dsc = {font_name}_glyph_dsc, /*Description of glyphs*/
  .unicode_list = nullptr, /*Every character in the font from 'unicode_first' to 'unicode_last'*/
  .glyph_id_ofs_list = {glyph_id_ofs_list},
  .glyph_id_start = {glyph_id_start},
  .format_type = {format_type}
}};
#endif /*#if {upper_cap_font_name}*/
"""

    find_replace = {
        " glyph_bitmap": f" {font_name}_glyph_bitmap",
        " glyph_dsc": f" {font_name}_glyph_dsc",
        " LV_ATTRIBUTE_LARGE_CONST": "",
    }

    for search, replace in find_replace.items():
        file_content_output = file_content_output.replace(search, replace)

    with open(output_file, "w") as file:
        file.write(file_content_output)
