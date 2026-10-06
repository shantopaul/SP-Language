fn main() {
    let size = 64;
    let mut checksum = 0;

    for i in 0..size {
        for j in 0..size {
            let mut sum = 0;
            for k in 0..size {
                let a_val = (i + k) % 10;
                let b_val = (k + j) % 10;
                sum += a_val * b_val;
            }
            checksum = (checksum + sum) % 1000000;
        }
    }

    println!("Matrix multiplication 64x64 complete.");
    println!("Checksum: {}", checksum);
}
