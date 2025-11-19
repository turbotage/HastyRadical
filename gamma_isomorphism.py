#!/usr/bin/env python3
import os
from pathlib import Path

SRC_DIR = Path('generators_gamma')
DST_DIR = Path('generators_gamma_tilde')
DST_DIR.mkdir(exist_ok=True)

def parse_matrices(filename):
	with open(filename) as f:
		lines = [line.strip() for line in f if line.strip()]
	# Each 4 lines is a matrix
	matrices = []
	for i in range(0, len(lines), 4):
		mat = [int(lines[i]), int(lines[i+1]), int(lines[i+2]), int(lines[i+3])]
		matrices.append(mat)
	return matrices

def gamma_isomorphism(mat, n):
	# mat: [x11, x12, x21, x22]
	mat[0] -= 1
	mat[3] -= 1
	for i in range(4):
		mat[i] //= n
	return mat

def format_matrix(mat, width=16):
	# Format as comma-separated, fixed width
	return ', '.join(f'{x:>{width}}' for x in mat)

def process_file(src_path, dst_path, n):
	matrices = parse_matrices(src_path)
	with open(dst_path, 'w') as f:
		for mat in matrices:
			tmat = gamma_isomorphism(mat, n)
			f.write(format_matrix(tmat) + '\n')

def main():
	for fname in os.listdir(SRC_DIR):
		if not fname.endswith('.txt'):
			continue
		n = int(fname.split('_')[1])
		src_path = SRC_DIR / fname
		dst_path = DST_DIR / fname
		process_file(src_path, dst_path, n)

if __name__ == '__main__':
	main()
