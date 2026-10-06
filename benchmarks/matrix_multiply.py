def main():
    size = 64
    checksum = 0

    for i in range(size):
        for j in range(size):
            s = 0
            for k in range(size):
                aVal = (i + k) % 10
                bVal = (k + j) % 10
                s += aVal * bVal
            checksum = (checksum + s) % 1000000

    print("Matrix multiplication 64x64 complete.")
    print(f"Checksum: {checksum}")

if __name__ == "__main__":
    main()
