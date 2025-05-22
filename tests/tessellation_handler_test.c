#include "minunit.h"
#include "tessellation_handler.h" // Should provide Point2D, ContourC, TessellationResult
#include "freetype_handler.h"   // For FT_Face declaration if needed by includes
#include <stdlib.h>
#include <stdio.h>

// Forward declaration for ftFace if not pulled in by freetype_handler.h in a way tests can see
// FT_Library ftLibrary; // Already declared as extern in freetype_handler.h
// FT_Face ftFace;       // Already declared as extern in freetype_handler.h


MU_TEST(test_null_tessellation) {
    TessellationResult result = generateGlyphTessellation(NULL, 0);
    mu_assert_int_eq(0, result.allocationFailed);
    mu_assert_int_eq(0, result.vertexCount);
    mu_assert_int_eq(0, result.elementCount);
    mu_assert(result.vertices == NULL, "Vertices should be NULL for no input");
    mu_assert(result.elements == NULL, "Elements should be NULL for no input");
}

MU_TEST(test_square_tessellation) {
    Point2D square_points[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    ContourC contour;
    contour.points = square_points;
    contour.count = 4; // Corrected from n_points
    contour.capacity = 4; // Assuming capacity is same as count for this test

    TessellationResult result = generateGlyphTessellation(&contour, 1);

    mu_assert_int_eq(0, result.allocationFailed);
    // For a simple convex quad, libtess2 might give 4 vertices and 2 triangles (6 elements/indices)
    mu_check(result.vertexCount == 4); 
    mu_check(result.elementCount == 2); // Number of polygons (triangles)

    // Optional: Check actual vertex data if predictable and stable
    // For example, if winding order is consistent. This might be fragile.
    // mu_assert_double_eq(0.0, result.vertices[0], 1e-5); 
    // mu_assert_double_eq(0.0, result.vertices[1], 1e-5);

    if (result.vertices) free(result.vertices);
    if (result.elements) free(result.elements);
}

MU_TEST(test_triangle_tessellation) {
    Point2D triangle_points[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}};
    ContourC contour;
    contour.points = triangle_points;
    contour.count = 3;
    contour.capacity = 3;

    TessellationResult result = generateGlyphTessellation(&contour, 1);
    
    mu_assert_int_eq(0, result.allocationFailed);
    mu_check(result.vertexCount == 3);
    mu_check(result.elementCount == 1); // One triangle

    if (result.vertices) free(result.vertices);
    if (result.elements) free(result.elements);
}


MU_TEST_SUITE(tessellation_tests) {
    // It's good practice to initialize FreeType if tessellation depends on it,
    // even if indirectly or for future tests.
    // For now, assuming tessellation_handler can be tested in isolation
    // or its dependencies are handled by the main app's ft_init.
    // If direct FT calls were made in tessellation_handler, we'd init/load here.

    MU_RUN_TEST(test_null_tessellation);
    MU_RUN_TEST(test_square_tessellation);
    MU_RUN_TEST(test_triangle_tessellation);
}

int main(int argc, char *argv[]) {
    (void)argc; 
    (void)argv; 
    MU_RUN_SUITE(tessellation_tests);
    MU_REPORT();
    // In minunit.h, MU_EXIT_CODE is defined as `minunit_fail`, 
    // which is the number of failed tests.
    // It's conventional for main to return 0 on success and non-zero on failure.
    return MU_EXIT_CODE; 
}
