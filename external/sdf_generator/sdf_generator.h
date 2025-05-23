#ifndef SDF_GENERATOR_H
#define SDF_GENERATOR_H

// Forward declaration for FT_Bitmap to avoid including freetype headers here
// if it becomes problematic, though for a library it might be fine.
// Alternatively, pass raw buffer, width, height, pitch.
struct FT_Bitmap_; 
typedef struct FT_Bitmap_ FT_Bitmap;

// Generates an SDF from a monochrome bitmap.
// Caller owns the returned buffer and must free it with free_sdf_bitmap.
unsigned char* generate_sdf_from_bitmap(
    const unsigned char* mono_bitmap_buffer, 
    int width, 
    int height, 
    int pitch, // pitch of the mono_bitmap_buffer
    int padding, // padding to add around the glyph for SDF generation
    int* out_sdf_width, 
    int* out_sdf_height
);

void free_sdf_bitmap(unsigned char* sdf_data);

#endif // SDF_GENERATOR_H
