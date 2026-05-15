#!/usr/bin/env python3
import argparse
import threading
import time
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

# Global counters with lock for thread safety
lock = threading.Lock()
success_count = 0
fail_count = 0
total_bytes = 0
response_times = []

def single_request(url, timeout=30):
    global success_count, fail_count, total_bytes, response_times
    
    start = time.time()
    try:
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            data = resp.read()
            elapsed = time.time() - start
            
            with lock:
                success_count += 1
                total_bytes += len(data)
                response_times.append(elapsed)
            
            return True, elapsed, len(data)
    except Exception as e:
        elapsed = time.time() - start
        
        with lock:
            fail_count += 1
            response_times.append(elapsed)
        
        return False, elapsed, 0

def main():
    parser = argparse.ArgumentParser(description='HTTP Stress Test Tool')
    parser.add_argument('--url', required=True)
    parser.add_argument('--requests', type=int, default=100)
    parser.add_argument('--concurrency', type=int, default=10)
    parser.add_argument('--timeout', type=int, default=30)
    
    args = parser.parse_args()
    
    print("=" * 60)
    print("  HTTP Stress Test")
    print("=" * 60)
    print(f"  URL:         {args.url}")
    print(f"  Requests:    {args.requests}")
    print(f"  Concurrency: {args.concurrency}")
    print(f"  Timeout:     {args.timeout}s")
    print("=" * 60)
    
    start_time = time.time()
    
    # Fire all requests using thread pool
    with ThreadPoolExecutor(max_workers=args.concurrency) as executor:
        futures = [
            executor.submit(single_request, args.url, args.timeout)
            for _ in range(args.requests)
        ]
        
        # Wait for all to complete
        for future in as_completed(futures):
            pass
    
    total_time = time.time() - start_time
    
    # Calculate stats
    if response_times:
        avg = sum(response_times) / len(response_times)
        sorted_times = sorted(response_times)
        n = len(sorted_times)
        p50 = sorted_times[n // 2]
        p90 = sorted_times[int(n * 0.9)]
        p99 = sorted_times[int(n * 0.99)]
    else:
        avg = p50 = p99 = p90 = 0
    
    qps = args.requests / total_time if total_time > 0 else 0
    throughput_mb = (total_bytes / 1024 / 1024) / total_time if total_time > 0 else 0
    
    # Print results
    print("\n" + "=" * 60)
    print("  RESULTS")
    print("=" * 60)
    print(f"  Total Time:   {total_time:.2f}s")
    print(f"  Success:      {success_count}/{args.requests} ({success_count*100//max(args.requests,1)}%)")
    print(f"  Failed:       {fail_count}/{args.requests}")
    print("-" * 60)
    print(f"  QPS:          {qps:.1f} req/s")
    print(f"  Throughput:   {throughput_mb:.2f} MB/s")
    print(f"  Transferred:  {total_bytes / 1024 / 1024:.2f} MB")
    print("-" * 60)
    print(f"  Avg Response: {avg*1000:.0f}ms")
    print(f"  P50:          {p50*1000:.0f}ms")
    print(f"  P90:          {p90*1000:.0f}ms")
    print(f"  P99:          {p99*1000:.0f}ms")
    print("=" * 60)

if __name__ == "__main__":
    main()
