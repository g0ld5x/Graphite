import time

# Start the timer
start_time = time.perf_counter()

def fib(n):
    if n <= 1:
        return n
    return fib(n-1)+fib(n-2)

print(fib(30));

end_time = time.perf_counter()

execution_time = end_time - start_time
print(f"Execution time: {execution_time:.6f} seconds")
