#!/usr/bin/env python
import subprocess

def main():
	subprocess.call(["rm", "stat.dat"])
	for i in range(1,25,5):
		for j in range(1,25,5):
			subprocess.call(["./run", "access_pattern.dat", "1", str(i), str(j)]) 


if __name__ == "__main__":
	main()
