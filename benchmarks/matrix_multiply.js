function main() {
    const size = 64;
    let checksum = 0;

    for (let i = 0; i < size; i++) {
        for (let j = 0; j < size; j++) {
            let sum = 0;
            for (let k = 0; k < size; k++) {
                const aVal = (i + k) % 10;
                const bVal = (k + j) % 10;
                sum += aVal * bVal;
            }
            checksum = (checksum + sum) % 1000000;
        }
    }

    console.log("Matrix multiplication 64x64 complete.");
    console.log(`Checksum: ${checksum}`);
}

main();
