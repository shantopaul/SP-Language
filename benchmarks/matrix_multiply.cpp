#include <iostream>

int main() {
    int size = 64;
    int checksum = 0;

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            int sum = 0;
            for (int k = 0; k < size; k++) {
                int aVal = (i + k) % 10;
                int bVal = (k + j) % 10;
                sum += aVal * bVal;
            }
            checksum = (checksum + sum) % 1000000;
        }
    }

    std::cout << "Matrix multiplication 64x64 complete.\n";
    std::cout << "Checksum: " << checksum << "\n";
    return 0;
}
