// SP Quicksort Benchmark

fn partition(arr: int[], low: int, high: int) -> int {
    let pivot: int = arr[high];
    let i: int = low - 1;

    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            let temp: int = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    let temp2: int = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp2;

    return i + 1;
}

fn quicksort(arr: int[], low: int, high: int) {
    if (low < high) {
        let pi: int = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

fn main() -> int {
    let data: int[] = {
        64, 34, 25, 12, 22, 11, 90, 88, 76, 54, 43, 21, 3, 9, 82, 10,
        45, 67, 89, 23, 56, 78, 91, 14, 28, 35, 49, 63, 77, 84, 99, 1
    };

    let n: int = data.length();
    quicksort(data, 0, n - 1);

    println("Sorted elements count: ${n}");
    println("Min: ${data[0]}, Max: ${data[n - 1]}");
    return 0;
}
