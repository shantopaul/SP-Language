fn partition(arr: &mut [i32], low: usize, high: usize) -> usize {
    let pivot = arr[high];
    let mut i = low;

    for j in low..high {
        if arr[j] <= pivot {
            arr.swap(i, j);
            i += 1;
        }
    }
    arr.swap(i, high);
    i
}

fn quicksort(arr: &mut [i32], low: usize, high: usize) {
    if low < high {
        let pi = partition(arr, low, high);
        if pi > 0 {
            quicksort(arr, low, pi - 1);
        }
        quicksort(arr, pi + 1, high);
    }
}

fn main() {
    let mut data = [
        64, 34, 25, 12, 22, 11, 90, 88, 76, 54, 43, 21, 3, 9, 82, 10,
        45, 67, 89, 23, 56, 78, 91, 14, 28, 35, 49, 63, 77, 84, 99, 1
    ];
    let n = data.len();
    quicksort(&mut data, 0, n - 1);
    println!("Sorted elements count: {}", n);
    println!("Min: {}, Max: {}", data[0], data[n - 1]);
}
