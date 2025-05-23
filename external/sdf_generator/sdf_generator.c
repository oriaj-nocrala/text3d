#include "sdf_generator.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy

// Dummy implementation
unsigned char* generate_sdf_from_bitmap(
    const unsigned char* mono_bitmap_buffer, 
    int width, 
    int height, 
    int pitch,
    int padding,
    int* out_sdf_width, 
    int* out_sdf_height) {

    if (!mono_bitmap_buffer || width <= 0 || height <= 0) {
        if (out_sdf_width) *out_sdf_width = 0;
        if (out_sdf_height) *out_sdf_height = 0;
        return NULL;
    }

    int padded_width = width + 2 * padding;
    int padded_height = height + 2 * padding;
    
    // For this dummy version, let's just create a slightly larger blank canvas
    // or copy the original bitmap with padding.
    // A real implementation would perform distance transform here.
    unsigned char* sdf_bitmap = (unsigned char*)malloc(padded_width * padded_height);
    if (!sdf_bitmap) {
        if (out_sdf_width) *out_sdf_width = 0;
        if (out_sdf_height) *out_sdf_height = 0;
        return NULL;
    }

    // Fill with a default value (e.g., 0, or 128 for mid-grey if that's the SDF convention)
    // For simplicity, let's fill with 0 (representing "outside")
    memset(sdf_bitmap, 0, padded_width * padded_height);

    // Copy the original bitmap into the center of the padded one
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Assuming mono_bitmap_buffer is 1 byte per pixel
            // and sdf_bitmap is also 1 byte per pixel.
            // A real SDF would be distances, not pixel values.
            // This is just a placeholder to have *some* data.
            sdf_bitmap[(y + padding) * padded_width + (x + padding)] = mono_bitmap_buffer[y * pitch + x];
        }
    }
    
    if (out_sdf_width) *out_sdf_width = padded_width;
    if (out_sdf_height) *out_sdf_height = padded_height;
    
    // In a real scenario, this bitmap would now contain SDF values.
    // For this stub, it just contains the original bitmap centered in a padded area.
    return sdf_bitmap;
}

void free_sdf_bitmap(unsigned char* sdf_data) {
    if (sdf_data) {
        free(sdf_data);
    }
}
