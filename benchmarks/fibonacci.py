import sys

def fib(n: int) -> int:
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

def main():
    n = 35
    res = fib(n)
    print(f"fib(35) = {res}")

if __name__ == "__main__":
    main()
