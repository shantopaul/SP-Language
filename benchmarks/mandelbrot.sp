// SP Mandelbrot Benchmark (Floating-point arithmetic & nested loops)

fn computeMandelbrot(maxIter: int) -> int {
    let width: int = 80;
    let height: int = 40;
    let totalIter: int = 0;

    for (int y = 0; y < height; y++) {
        let yDiff: double = y - 20;
        let ci: double = yDiff * 0.05;
        for (int x = 0; x < width; x++) {
            let xDiff: double = x - 40;
            let cr: double = xDiff * 0.05;
            let zr: double = 0.0;
            let zi: double = 0.0;
            let iter: int = 0;

            while (zr * zr + zi * zi <= 4.0 && iter < maxIter) {
                let temp: double = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = temp;
                iter++;
            }
            totalIter = totalIter + iter;
        }
    }
    return totalIter;
}

fn main() -> int {
    let result: int = computeMandelbrot(500);
    println("Mandelbrot checksum: ${result}");
    return 0;
}
