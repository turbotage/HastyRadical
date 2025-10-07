#!/usr/bin/env python3
"""
Parallel computation of Gamma(n) generators using local SageMath
"""

import subprocess
import re
import numpy as np
import threading
import queue
import time
import os
from concurrent.futures import ThreadPoolExecutor, as_completed

def call_sage_gamma_direct(n, filename):
    """
    Call local SageMath to compute Gamma(n).generators() and write directly to file
    """
    try:
        # Ensure generators directory exists
        os.makedirs('generators', exist_ok=True)
        # Prepare the SageMath command that writes directly to file
        sage_code = f"""
gamma = Gamma({n})
gens = gamma.generators()

# Build output string first, then write once
output_lines = []
for g in gens:
    matrix = g.matrix()
    # Add each matrix entry: a, b, c, d for [[a,b],[c,d]]
    output_lines.append(str(matrix[0,0]))
    output_lines.append(str(matrix[0,1]))
    output_lines.append(str(matrix[1,0]))
    output_lines.append(str(matrix[1,1]))

# Write all data in one operation
with open("generators/{filename}", "w") as f:
    f.write("\\n".join(output_lines) + "\\n")

print(f"SUCCESS: {{len(gens)}} generators written to {filename}")
"""
        
        print(f"Computing Gamma({n}) generators using local SageMath...")
        
        # Call SageMath in the sageenv conda environment
        # No timeout - larger n values can take hours or days
        result = subprocess.run(
            ['conda', 'run', '-n', 'sageenv', 'sage', '-c', sage_code],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            # Parse the success message to get number of generators
            output = result.stdout
            lines = output.split('\n')
            
            for line in lines:
                if line.startswith("SUCCESS:"):
                    # Extract number of generators from success message
                    parts = line.split()
                    if len(parts) >= 2:
                        num_generators = int(parts[1])
                        print(f"Successfully computed {num_generators} generators for Gamma({n}) -> {filename}")
                        return num_generators
            
            print("Could not parse success message from SageMath output")
            print("Raw output:", output)
            return None
        else:
            print(f"SageMath execution failed with return code {result.returncode}")
            print("Error output:", result.stderr)
            return None
            
    except Exception as e:
        print(f"Error calling SageMath: {e}")
        return None

# save_generators_to_file is no longer needed - SageMath writes directly

def load_generators_from_file(filename):
    """
    Load generators from text file
    """
    try:
        with open(filename, 'r') as f:
            entries = [line.strip() for line in f if line.strip()]
        
        if len(entries) % 4 != 0:
            raise ValueError(f"Invalid file format: {len(entries)} entries (should be multiple of 4)")
        
        matrices_data = []
        for i in range(0, len(entries), 4):
            a, b, c, d = entries[i:i+4]
            matrices_data.append([[a, b], [c, d]])
        
        print(f"Loaded {len(matrices_data)} generators from {filename}")
        return matrices_data
    except Exception as e:
        print(f"Error loading from {filename}: {e}")
        return None

def matrices_to_numpy(matrices_data):
    """
    Convert matrix data to numpy arrays (handling string entries)
    """
    result = []
    for m in matrices_data:
        # Convert string entries to integers if needed
        if isinstance(m[0][0], str):
            matrix = np.array([[int(m[0][0]), int(m[0][1])], 
                              [int(m[1][0]), int(m[1][1])]], dtype=object)
        else:
            matrix = np.array(m, dtype=object)
        result.append(matrix)
    return result

def print_matrices(matrices):
    """
    Pretty print the matrices
    """
    for i, matrix in enumerate(matrices):
        print(f"Generator {i+1}:")
        print(matrix)
        print()

def compute_and_save_gamma(n, filename=None):
    """
    Compute Gamma(n) generators and save to file
    """
    if filename is None:
        filename = f"gamma_{n}_generators.txt"
    
    # Check if file already exists in generators/ directory
    full_path = f"generators/{filename}"
    if os.path.exists(full_path):
        print(f"Gamma({n}) generators already exist in {filename}, skipping...")
        return filename
    
    print(f"Computing Gamma({n}) generators...")
    start_time = time.time()
    
    # SageMath writes directly to file now
    num_generators = call_sage_gamma_direct(n, filename)
    
    if num_generators:
        elapsed = time.time() - start_time
        print(f"Gamma({n}): {num_generators} generators computed in {elapsed:.2f}s -> {full_path}")
        return filename
    else:
        print(f"Failed to compute Gamma({n}) generators")
        return None

def compute_gamma_worker(n):
    """
    Worker function for computing a single Gamma(n)
    """
    try:
        result = compute_and_save_gamma(n)
        return (n, result, None)
    except Exception as e:
        return (n, None, str(e))

def compute_gamma_range_parallel(start_n, end_n, max_workers=32, timeout_per_computation=300):
    """
    Compute Gamma(n) generators for n from start_n to end_n using parallel processing
    
    Args:
        start_n: Starting value of n
        end_n: Ending value of n (inclusive)
        max_workers: Maximum number of parallel threads
        timeout_per_computation: Timeout in seconds for each computation
    """
    print(f"Computing Gamma(n) generators for n = {start_n} to {end_n}")
    print(f"Using {max_workers} parallel workers")
    print(f"Timeout per computation: {timeout_per_computation}s")
    print("-" * 60)
    
    start_time = time.time()
    completed = 0
    failed = 0
    skipped = 0
    
    # Create list of n values to compute
    n_values = list(range(start_n, end_n + 1))
    
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        # Submit all tasks
        future_to_n = {executor.submit(compute_gamma_worker, n): n for n in n_values}
        
        # Process completed tasks
        for future in as_completed(future_to_n, timeout=timeout_per_computation * len(n_values)):
            n = future_to_n[future]
            try:
                n_result, filename, error = future.result()
                
                if error:
                    print(f"ERROR Gamma({n_result}): {error}")
                    failed += 1
                elif filename:
                    if "already exist" in str(filename):
                        skipped += 1
                    else:
                        completed += 1
                else:
                    print(f"FAILED Gamma({n_result}): Unknown error")
                    failed += 1
                    
            except Exception as e:
                print(f"EXCEPTION Gamma({n}): {e}")
                failed += 1
    
    total_time = time.time() - start_time
    total_attempted = len(n_values)
    
    print("-" * 60)
    print(f"SUMMARY:")
    print(f"Total range: Gamma({start_n}) to Gamma({end_n}) ({total_attempted} values)")
    print(f"Completed: {completed}")
    print(f"Skipped (already existed): {skipped}")
    print(f"Failed: {failed}")
    print(f"Total time: {total_time:.2f}s")
    if completed > 0:
        print(f"Average time per computation: {total_time/completed:.2f}s")
    
    return completed, skipped, failed

def call_sage_gamma_for_matrices(n):
    """
    Legacy function for getting matrices data (for GammaGroup class)
    """
    try:
        # Use temporary file approach
        import tempfile
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as tmp_file:
            tmp_filename = tmp_file.name
        
        num_generators = call_sage_gamma_direct(n, tmp_filename)
        if num_generators:
            matrices_data = load_generators_from_file(tmp_filename)
            os.unlink(tmp_filename)  # Clean up temp file
            return matrices_data
        else:
            return None
    except Exception as e:
        print(f"Error in call_sage_gamma_for_matrices: {e}")
        return None

class GammaGroup:
    """
    Python interface to SageMath Gamma groups
    """
    def __init__(self, n):
        self.n = n
        self.level = n
        self._generators = None
    
    def generators(self):
        """
        Get generators as numpy arrays
        """
        if self._generators is None:
            matrices_data = call_sage_gamma_for_matrices(self.n)
            if matrices_data:
                self._generators = matrices_to_numpy(matrices_data)
            else:
                self._generators = []
        return self._generators
    
    def __repr__(self):
        return f"Gamma({self.n})"

def Gamma(n):
    """
    Create a Gamma(n) group object
    """
    return GammaGroup(n)

def main():
    """
    Parallel computation of Gamma(n) generators
    """
    print("=== Parallel SageMath Gamma(n) Generator Computation ===")
    print()
    
    # Ask user for parameters
    try:
        start_n = int(input("Start n (default 1): ") or "1")
        end_n = int(input("End n (default 100): ") or "100")
        max_workers = int(input("Number of parallel workers (default 32): ") or "32")
        
        print(f"\nStarting parallel computation...")
        completed, skipped, failed = compute_gamma_range_parallel(
            start_n=start_n,
            end_n=end_n, 
            max_workers=max_workers
        )
        
        print(f"\nParallel computation finished!")
        print(f"Check the generated gamma_*_generators.txt files")
        
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")

def test_small():
    """
    Test function for small values
    """
    print("=== Testing small values ===")
    compute_gamma_range_parallel(1, 5, max_workers=5)

if __name__ == "__main__":
    main()