public class Quicksort {
    public static int partition(int[] arr, int low, int high) {
        int pivot = arr[high];
        int i = low - 1;

        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }

        int temp2 = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp2;

        return i + 1;
    }

    public static void quicksort(int[] arr, int low, int high) {
        if (low < high) {
            int pi = partition(arr, low, high);
            quicksort(arr, low, pi - 1);
            quicksort(arr, pi + 1, high);
        }
    }

    public static void main(String[] args) {
        int[] data = {
            64, 34, 25, 12, 22, 11, 90, 88, 76, 54, 43, 21, 3, 9, 82, 10,
            45, 67, 89, 23, 56, 78, 91, 14, 28, 35, 49, 63, 77, 84, 99, 1
        };
        int n = data.length;
        quicksort(data, 0, n - 1);
        System.out.println("Sorted elements count: " + n);
        System.out.println("Min: " + data[0] + ", Max: " + data[n - 1]);
    }
}
