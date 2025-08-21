# Text3D - High-Performance SDF Text Rendering Engine

A modern 3D text rendering engine written in C using OpenGL and Signed Distance Field (SDF) technology for crisp, scalable text rendering with real-time effects.

## Features

- **SDF Text Rendering**: High-quality, resolution-independent text using Signed Distance Fields
- **Real-time Text Input**: Interactive text editing with cursor support
- **Advanced Typography**: Full UTF-8 and Unicode support including emoji
- **Visual Effects**: 
  - Text outlining with customizable colors and thickness
  - Drop shadows with adjustable softness
  - Smooth antialiasing at any scale
- **Font Support**: Multiple font formats via FreeType integration
- **Optimized Performance**: Hardware-accelerated rendering with OpenGL
- **Comprehensive Testing**: Full unit test suite with modular architecture

## Dependencies

- **OpenGL 3.3+** - Core graphics rendering
- **GLEW** - OpenGL extension loading
- **FreeGLUT** - Window management and input handling
- **FreeType 2** - Font loading and glyph rasterization
- **libtess2** - Complex polygon tessellation (included)

## Building

### Prerequisites

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential libgl1-mesa-dev libglew-dev freeglut3-dev libfreetype6-dev pkg-config
```

On Fedora/CentOS:
```bash
sudo yum install gcc mesa-libGL-devel glew-devel freeglut-devel freetype-devel pkgconfig
```

### Compilation

```bash
# Build the main application
make

# Build and run tests
make test

# Clean build artifacts
make clean
```

## Usage

### Basic Usage
```bash
./texto
```

The application opens an interactive window where you can:
- Type text in real-time
- Use arrow keys to move the cursor
- Backspace to delete characters
- ESC to exit

### Keyboard Controls

- **Text Input**: Type normally to add text
- **Backspace**: Delete character before cursor
- **Left/Right Arrows**: Move cursor position
- **Escape**: Exit application

## Project Structure

```
text3d/
├── src/                    # Core source code
│   ├── main.c             # Application entry point
│   ├── renderer.c/.h      # Text rendering engine
│   ├── glyph_manager.c/.h # Glyph caching and management
│   ├── freetype_handler.c/.h # Font loading via FreeType
│   ├── tessellation_handler.c/.h # Polygon tessellation
│   ├── opengl_setup.c/.h  # OpenGL initialization
│   ├── input_handler.c/.h # Keyboard/mouse input
│   └── utils.c/.h         # Utility functions
├── shaders/               # GLSL shaders
│   ├── vertex_shader.glsl
│   └── fragment_shader.glsl
├── external/              # Third-party libraries
│   ├── libtess2-1.0.2/   # Tessellation library
│   └── sdf_generator/     # SDF generation module
├── tests/                 # Unit tests
└── build/                 # Generated build artifacts
```

## Technical Details

### SDF Technology
This project uses Signed Distance Fields for text rendering, which provides:
- **Sharp text at any scale** - No pixelation when zooming
- **Efficient GPU rendering** - Single texture lookup per fragment
- **Advanced effects** - Outlines, shadows, and glows with minimal performance cost
- **Memory efficiency** - Smaller texture memory footprint

### Architecture
- **Modular design** with clear separation of concerns
- **Test-driven development** with comprehensive unit tests
- **Memory-safe** C code with proper cleanup
- **Cross-platform** OpenGL implementation

## Testing

The project includes comprehensive unit tests for all major components:

```bash
# Run all tests
make test

# Individual test components
./build/freetype_test      # Font loading tests
./build/tessellation_test  # Polygon tessellation tests
./build/glyph_manager_test # Glyph caching tests
./build/text_input_test    # Input handling tests
./build/renderer_test      # Text layout tests
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines

- Follow the existing C code style
- Add unit tests for new functionality
- Update documentation as needed
- Ensure all tests pass before submitting

## License

This project is open source. See the individual component licenses in the `external/` directory for third-party dependencies.

## Acknowledgments

- **FreeType** - Font rendering foundation
- **libtess2** - Robust polygon tessellation
- **OpenGL community** - Graphics programming resources
- **SDF techniques** - Inspired by Valve's SDF text rendering research
