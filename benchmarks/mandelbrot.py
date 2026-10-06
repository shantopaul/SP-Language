def compute_mandelbrot(max_iter: int) -> int:
    width = 80
    height = 40
    total_iter = 0

    for y in range(height):
        y_diff = float(y - 20)
        ci = y_diff * 0.05
        for x in range(width):
            x_diff = float(x - 40)
            cr = x_diff * 0.05
            zr = 0.0
            zi = 0.0
            iter_count = 0

            while zr * zr + zi * zi <= 4.0 and iter_count < max_iter:
                temp = zr * zr - zi * zi + cr
                zi = 2.0 * zr * zi + ci
                zr = temp
                iter_count += 1
            total_iter += iter_count

    return total_iter

def main():
    result = compute_mandelbrot(500)
    print(f"Mandelbrot checksum: {result}")

if __name__ == "__main__":
    main()
