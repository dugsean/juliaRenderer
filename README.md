# Julia Set Renderer

This C code generates a Julia set fractal image and saves it as a BMP file. It utilizes multithreading for parallel processing to enhance performance.

## Prerequisites

Ensure you have a C compiler installed to build and run the program.

## Compilation

Compile the code using the following command:

```bash
gcc multithreadedJuliaSet.c -o julia_set_renderer -lm -pthread
```

## Usage

Run the compiled executable:

```bash
./julia_set_renderer
```

This will generate an image file named `img.bmp` in the same directory.

## Julia Set Parameters

Adjust the parameters in the `juliaInputs` structure within the `main` function to explore different Julia sets:

- `c`: Complex constant determining the Julia set shape.
- `minX`, `maxX`, `minY`, `maxY`: Define the region of the complex plane to render.
- `density`: Density of points in the complex plane.
- `maxRadius`: Maximum radius to consider for iteration.
- `maxIter`: Maximum number of iterations for each point.
- `oversample`: Level of oversampling for smoother images.

## Multithreading

The code uses pthreads to parallelize the rendering process, enhancing efficiency, especially for large images.

## File Output

The resulting image is saved as `img.bmp` with a BMP file header created using the `bmpHeader` function.

Feel free to experiment with the code and customize the Julia set by adjusting parameters!
