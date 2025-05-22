#include "minunit.h"
#include "tessellation_handler.h" // For Point2D, ContourC, TessellationResult
#include "freetype_handler.h"   // For OutlineDataC and its helpers
#include <stdlib.h>
#include <stdio.h>

// Note: FT_Library and FT_Face are not directly used here but freetype_handler.h
// is needed for OutlineDataC structure. If tests were to build real OutlineDataC
// from fonts, then FreeType initialization would be needed.
// For now, we manually construct a simple OutlineDataC.

static OutlineDataC create_test_outline_data(Point2D* points, int point_count) {
    OutlineDataC outline;
    initOutlineData(&outline, 1); // Initialize with capacity for 1 contour
    ContourC* contour = &outline.contours[outline.count++]; // Manually get and increment
                                                          // This is a simplified way, addContourToOutline is better
                                                          // but requires more setup.
    initContour(contour, point_count);
    for(int i=0; i < point_count; ++i) {
        addPointToContour(contour, points[i]);
    }
    return outline;
}


MU_TEST(test_null_tessellation) {
    // Test with NULL OutlineDataC
    TessellationResult result = generateGlyphTessellation(NULL);
    mu_assert_int_eq(1, result.allocationFailed); // Expect allocationFailed = 1 for NULL input
    mu_assert_int_eq(0, result.vertexCount);
    mu_assert_int_eq(0, result.elementCount);
    mu_assert(result.vertices == NULL, "Vertices should be NULL for NULL input");
    mu_assert(result.elements == NULL, "Elements should be NULL for NULL input");

    // Test with empty (but valid) OutlineDataC
    OutlineDataC empty_outline;
    initOutlineData(&empty_outline, 0); // No contours
    result = generateGlyphTessellation(&empty_outline);
    mu_assert_int_eq(0, result.allocationFailed); // Should not fail allocation, just produce no geometry
    mu_assert_int_eq(0, result.vertexCount);
    mu_assert_int_eq(0, result.elementCount);
    mu_assert(result.vertices == NULL, "Vertices should be NULL for no contours");
    mu_assert(result.elements == NULL, "Elements should be NULL for no contours");
    freeOutlineData(&empty_outline);
}

MU_TEST(test_square_tessellation) {
    Point2D square_points[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    OutlineDataC outline = create_test_outline_data(square_points, 4);

    TessellationResult result = generateGlyphTessellation(&outline);

    mu_assert_int_eq(0, result.allocationFailed);
    mu_check(result.vertexCount == 4); 
    mu_check(result.elementCount == 2); // 2 triangles

    if (result.vertices) free(result.vertices);
    if (result.elements) free(result.elements);
    freeOutlineData(&outline);
}

MU_TEST(test_triangle_tessellation) {
    Point2D triangle_points[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}};
    OutlineDataC outline = create_test_outline_data(triangle_points, 3);

    TessellationResult result = generateGlyphTessellation(&outline);
    
    mu_assert_int_eq(0, result.allocationFailed);
    mu_check(result.vertexCount == 3);
    mu_check(result.elementCount == 1); // One triangle

    if (result.vertices) free(result.vertices);
    if (result.elements) free(result.elements);
    freeOutlineData(&outline);
}


MU_TEST_SUITE(tessellation_tests) {
    MU_RUN_TEST(test_null_tessellation);
    MU_RUN_TEST(test_square_tessellation);
    MU_RUN_TEST(test_triangle_tessellation);
}

int main(int argc, char *argv[]) {
    (void)argc; 
    (void)argv; 
    MU_RUN_SUITE(tessellation_tests);
    MU_REPORT();
    return MU_EXIT_CODE; 
}