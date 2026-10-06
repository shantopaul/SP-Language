fn compute_mandelbrot(max_iter: i32) -> i32 {
    let width = 80;
    let height = 40;
    let mut total_iter = 0;

    for y in 0..height {
        let y_diff = (y - 20) as f64;
        let ci = y_diff * 0.05;
        for x in 0..width {
            let x_diff = (x - 40) as f64;
            let cr = x_diff * 0.05;
            let mut zr = 0.0;
            let mut zi = 0.0;
            let mut iter = 0;

            while zr * zr + zi * zi <= 4.0 && iter < max_iter {
                let temp = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = temp;
                iter += 1;
            }
            total_iter += iter;
        }
    }
    total_iter
}

fn main() {
    let result = compute_mandelbrot(500);
    println!("Mandelbrot checksum: {}", result);
}
