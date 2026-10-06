function partition(arr, low, high) {
    let pivot = arr[high];
    let i = low - 1;

    for (let j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            let temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    let temp2 = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp2;

    return i + 1;
}

function quicksort(arr, low, high) {
    if (low < high) {
        let pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

function main() {
    let data = [
        64, 34, 25, 12, 22, 11, 90, 88, 76, 54, 43, 21, 3, 9, 82, 10,
        45, 67, 89, 23, 56, 78, 91, 14, 28, 35, 49, 63, 77, 84, 99, 1
    ];
    let n = data.length;
    quicksort(data, 0, n - 1);
    console.log(`Sorted elements count: ${n}`);
    console.log(`Min: ${data[0]}, Max: ${data[n - 1]}`);
}

main();
