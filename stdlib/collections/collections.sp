module std.collections;

// Standard library collections module providing generic List, Map, Set, Stack, Queue, and PriorityQueue data structures.

/**
 * Pure SP generic dynamically resizing array list.
 */
public class ArrayList<T> {
    private List<T> backend;

    public ArrayList() {
        this.backend = new List<T>();
    }

    public fn add(val: T) {
        this.backend.push(val);
    }

    public fn push(val: T) {
        this.backend.push(val);
    }

    public fn get(index: int) -> T {
        return this.backend.get(index);
    }

    public fn set(index: int, val: T) {
        this.backend.set(index, val);
    }

    public fn pop() -> T {
        return this.backend.pop();
    }

    public fn size() -> int {
        return this.backend.size();
    }

    public fn length() -> int {
        return this.backend.length();
    }

    public fn isEmpty() -> bool {
        return this.backend.isEmpty();
    }

    public fn contains(val: T) -> bool {
        return this.backend.contains(val);
    }

    public fn removeAt(index: int) {
        this.backend.removeAt(index);
    }

    public fn clear() {
        this.backend.clear();
    }
}

/**
 * Pure SP generic open-addressing hash map with key-value associations.
 */
public class HashMap<K, V> {
    private Map<K, V> backend;

    public HashMap() {
        this.backend = new Map<K, V>();
    }

    public fn set(key: K, val: V) {
        this.backend.set(key, val);
    }

    public fn get(key: K) -> V {
        return this.backend.get(key);
    }

    public fn contains(key: K) -> bool {
        return this.backend.contains(key);
    }

    public fn containsKey(key: K) -> bool {
        return this.backend.contains(key);
    }

    public fn remove(key: K) {
        this.backend.remove(key);
    }

    public fn size() -> int {
        return this.backend.size();
    }

    public fn length() -> int {
        return this.backend.size();
    }

    public fn isEmpty() -> bool {
        return this.backend.isEmpty();
    }

    public fn clear() {
        this.backend.clear();
    }
}

/**
 * Pure SP generic hash set of unique elements.
 */
public class HashSet<T> {
    private Set<T> backend;

    public HashSet() {
        this.backend = new Set<T>();
    }

    public fn add(val: T) {
        this.backend.add(val);
    }

    public fn contains(val: T) -> bool {
        return this.backend.contains(val);
    }

    public fn remove(val: T) {
        this.backend.remove(val);
    }

    public fn size() -> int {
        return this.backend.size();
    }

    public fn length() -> int {
        return this.backend.size();
    }

    public fn isEmpty() -> bool {
        return this.backend.isEmpty();
    }

    public fn clear() {
        this.backend.clear();
    }
}

/**
 * Pure SP generic LIFO (Last-In, First-Out) stack.
 */
public class Stack<T> {
    private List<T> items;
    private int count;

    public Stack() {
        this.items = new List<T>();
        this.count = 0;
    }

    public fn push(item: T) {
        this.items.push(item);
        this.count = this.count + 1;
    }

    public fn pop() -> T {
        if (this.count == 0) {
            panic("Stack is empty");
        }
        this.count = this.count - 1;
        return this.items.pop();
    }

    public fn peek() -> T {
        if (this.count == 0) {
            panic("Stack is empty");
        }
        return this.items.get(this.count - 1);
    }

    public fn size() -> int {
        return this.count;
    }

    public fn isEmpty() -> bool {
        return this.count == 0;
    }

    public fn clear() {
        this.items.clear();
        this.count = 0;
    }
}

/**
 * Pure SP generic FIFO (First-In, First-Out) queue.
 */
public class Queue<T> {
    private List<T> items;
    private int head;
    private int count;

    public Queue() {
        this.items = new List<T>();
        this.head = 0;
        this.count = 0;
    }

    public fn enqueue(item: T) {
        this.items.push(item);
        this.count = this.count + 1;
    }

    public fn dequeue() -> T {
        if (this.count == 0) {
            panic("Queue is empty");
        }
        var item = this.items.get(this.head);
        this.head = this.head + 1;
        this.count = this.count - 1;
        return item;
    }

    public fn peek() -> T {
        if (this.count == 0) {
            panic("Queue is empty");
        }
        return this.items.get(this.head);
    }

    public fn size() -> int {
        return this.count;
    }

    public fn isEmpty() -> bool {
        return this.count == 0;
    }

    public fn clear() {
        this.items.clear();
        this.head = 0;
        this.count = 0;
    }
}

/**
 * Pure SP generic PriorityQueue backed by a binary min-heap.
 * Items with lower priority integer values are dequeued first.
 */
public class PriorityQueue<T> {
    private List<T> items;
    private List<int> priorities;
    private int count;

    public PriorityQueue() {
        this.items = new List<T>();
        this.priorities = new List<int>();
        this.count = 0;
    }

    public fn push(item: T, priority: int) {
        this.items.push(item);
        this.priorities.push(priority);
        this.count = this.count + 1;
        this.siftUp(this.count - 1);
    }

    public fn peek() -> T {
        if (this.count == 0) {
            panic("PriorityQueue is empty");
        }
        return this.items.get(0);
    }

    public fn peekPriority() -> int {
        if (this.count == 0) {
            panic("PriorityQueue is empty");
        }
        return this.priorities.get(0);
    }

    public fn pop() -> T {
        if (this.count == 0) {
            panic("PriorityQueue is empty");
        }
        var topItem = this.items.get(0);
        var lastIdx = this.count - 1;

        if (lastIdx == 0) {
            this.items.pop();
            this.priorities.pop();
            this.count = 0;
            return topItem;
        }

        var lastItem = this.items.get(lastIdx);
        var lastPrio = this.priorities.get(lastIdx);
        this.items.set(0, lastItem);
        this.priorities.set(0, lastPrio);
        this.items.pop();
        this.priorities.pop();
        this.count = this.count - 1;

        this.siftDown(0);
        return topItem;
    }

    private fn siftUp(index: int) {
        var curr = index;
        while (curr > 0) {
            var parent = (curr - 1) / 2;
            if (this.priorities.get(curr) < this.priorities.get(parent)) {
                this.swap(curr, parent);
                curr = parent;
            } else {
                return;
            }
        }
    }

    private fn siftDown(index: int) {
        var curr = index;
        while (true) {
            var left = 2 * curr + 1;
            var right = 2 * curr + 2;
            var smallest = curr;

            if (left < this.count) {
                if (this.priorities.get(left) < this.priorities.get(smallest)) {
                    smallest = left;
                }
            }
            if (right < this.count) {
                if (this.priorities.get(right) < this.priorities.get(smallest)) {
                    smallest = right;
                }
            }

            if (smallest != curr) {
                this.swap(curr, smallest);
                curr = smallest;
            } else {
                return;
            }
        }
    }

    private fn swap(i: int, j: int) {
        var tempItem = this.items.get(i);
        var tempPrio = this.priorities.get(i);

        this.items.set(i, this.items.get(j));
        this.priorities.set(i, this.priorities.get(j));

        this.items.set(j, tempItem);
        this.priorities.set(j, tempPrio);
    }

    public fn size() -> int {
        return this.count;
    }

    public fn isEmpty() -> bool {
        return this.count == 0;
    }

    public fn clear() {
        this.items.clear();
        this.priorities.clear();
        this.count = 0;
    }
}

public fn isListEmpty(List<int> list) -> bool {
    return list.isEmpty();
}

public fn isMapEmpty(Map<string, int> map) -> bool {
    return map.isEmpty();
}

public fn isSetEmpty(Set<int> set) -> bool {
    return set.isEmpty();
}
