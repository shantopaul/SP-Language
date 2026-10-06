#include <stdio.h>

int computeMandelbrot(int maxIter) {
    int width = 80;
    int height = 40;
    int totalIter = 0;

    for (int y = 0; y < height; y++) {
        double yDiff = y - 20;
        double ci = yDiff * 0.05;
        for (int x = 0; x < width; x++) {
            double xDiff = x - 40;
            double cr = xDiff * 0.05;
            double zr = 0.0;
            double zi = 0.0;
            int iter = 0;

            while (zr * zr + zi * zi <= 4.0 && iter < maxIter) {
                double temp = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = temp;
                iter++;
            }
            totalIter += iter;
        }
    }
    return totalIter;
}

int main(void) {
    int result = computeMandelbrot(500);
    printf("Mandelbrot checksum: %d\n", result);
    return 0;
}
