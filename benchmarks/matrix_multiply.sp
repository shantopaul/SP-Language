// SP Matrix Multiplication Benchmark (Flat array 64x64)

fn main() -> int {
    let size: int = 64;
    let total: int = size * size;

    // Use loop to calculate dot products
    let checksum: int = 0;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            let sum: int = 0;
            for (int k = 0; k < size; k++) {
                let aVal: int = (i + k) % 10;
                let bVal: int = (k + j) % 10;
                sum = sum + aVal * bVal;
            }
            checksum = (checksum + sum) % 1000000;
        }
    }

    println("Matrix multiplication 64x64 complete.");
    println("Checksum: ${checksum}");
    return 0;
}
