def partition(arr, low, high):
    pivot = arr[high]
    i = low - 1
    for j in range(low, high):
        if arr[j] <= pivot:
            i += 1
            arr[i], arr[j] = arr[j], arr[i]
    arr[i + 1], arr[high] = arr[high], arr[i + 1]
    return i + 1

def quicksort(arr, low, high):
    if low < high:
        pi = partition(arr, low, high)
        quicksort(arr, low, pi - 1)
        quicksort(arr, pi + 1, high)

def main():
    data = [
        64, 34, 25, 12, 22, 11, 90, 88, 76, 54, 43, 21, 3, 9, 82, 10,
        45, 67, 89, 23, 56, 78, 91, 14, 28, 35, 49, 63, 77, 84, 99, 1
    ]
    n = len(data)
    quicksort(data, 0, n - 1)
    print(f"Sorted elements count: {n}")
    print(f"Min: {data[0]}, Max: {data[n - 1]}")

if __name__ == "__main__":
    main()
