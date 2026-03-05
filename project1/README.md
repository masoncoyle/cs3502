# Project 1: Multi-Threaded Banking System

## Description
A multi-threaded banking system built on Ubuntu using C programming language.
This system is composed of accounts, transactions, threads acting as tellers/ATMs, and transfers.
Phase 1 and 3 demonstrate race conditions and deadlock. Phase 2 and 4 solve these issues.

## Requirements
- GCC Compiler

## Installation
git clone https://github.com/masoncoyle/cs3502/tree/main/project1/c-programs
cd cs3502/project1/c-programs

## Configuration
- Set number of accounts with #define NUM_ACCOUNTS
- Set number of threads with #define NUM_THREADS
- Set number of transactions per thread with #define TRANSACTIONS_PER_THREAD
- Set initial balance with #define INITIAL_BALANCE
- For phase4.c only, set #define to 0 to use lock ordering, or 1 to use timeout mechanism

## Compilation
gcc -Wall -Wextra -pthread phase1.c -o phase1
gcc -Wall -Wextra -pthread phase2.c -o phase2
gcc -Wall -Wextra -pthread phase3.c -o phase3
gcc -Wall -Wextra -pthread phase4.c -o phase4

## Running
./phase1
./phase2
./phase3
./phase4
