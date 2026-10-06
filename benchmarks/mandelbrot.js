function computeMandelbrot(maxIter) {
    const width = 80;
    const height = 40;
    let totalIter = 0;

    for (let y = 0; y < height; y++) {
        const yDiff = y - 20;
        const ci = yDiff * 0.05;
        for (let x = 0; x < width; x++) {
            const xDiff = x - 40;
            const cr = xDiff * 0.05;
            let zr = 0.0;
            let zi = 0.0;
            let iter = 0;

            while (zr * zr + zi * zi <= 4.0 && iter < maxIter) {
                const temp = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = temp;
                iter++;
            }
            totalIter += iter;
        }
    }
    return totalIter;
}

function main() {
    const result = computeMandelbrot(500);
    console.log(`Mandelbrot checksum: ${result}`);
}

main();
