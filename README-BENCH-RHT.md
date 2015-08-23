
Read-Log-Update: A Lightweight Synchronization Mechanism for Concurrent Programming
===================================================================================

Authors
-------
Alexander Matveev (MIT)

Nir Shavit (MIT and Tel-Aviv University)

Pascal Felber (University of Neuchatel)

Patrick Marlier (University of Neuchatel)

Code Maintainer
-----------------
Name:  Alexander Matveev

Email: amatveev@csail.mit.edu

RLU-RHT Benchmark
=================
Our RLU-RHT (resizable hash-table) benchmark provides:

(1) RCU resizable hash table of Triplett et al. (http://dl.acm.org/citation.cfm?id=2002192)
    
	- no support for concurrent inserts/removes

(2) RLU resizable hash table
    
	- Full support for concurrent inserts/removes

In this benchmark we use:

(1) RLU coarse-grained

(2) Userspace RCU library of Arbel and Morrison (http://dl.acm.org/citation.cfm?id=2611471)

Compilation
-----------
Execute "make"

Execution Options
-----------------
  -h, --help
        
        Print this message

  -a, --do-not-alternate
	    
        Do not alternate insertions and removals

  -w, --rlu-max-ws
	    
        Maximum number of write-sets aggregated in RLU deferral.
        In this benchmark, it must be 1 (RLU coarse-grained).

  -g, --resize-rate
        
        If 1 then the first thread is dedicated to resizes (constant expand-shrink).
        Else, must be set to 0 (no resizes).

  -b, --buckets
        
        Number of buckets (for list use 1, default=(1))

  -d, --duration <int>
        
        Test duration in milliseconds (0=infinite, default=(10000))

  -i, --initial-size <int>
        
        Number of elements to insert before test (default=(256))

  -r, --range <int>
        
        Range of integer values inserted in set (default=((256) * 2))

  -s, --seed <int>
        
        RNG seed (0=time-based, default=(0))

  -u, --update-rate <int>
        
        Percentage of update transactions: 0-1000 (100%) (default=(200))

  -n, --num-threads <int>
	    
        Number of threads (default=(1))

Example
-------
./bench-rlu -a -g1 -b8192 -d10000 -i65536 -r131072 -w1 -u0 -n16

  => Initializes a 65,536 items RLU hash-table with 8,192 buckets (8 items per bucket).

  => The key range is 131,072, and the update ratio is 0%.

  => First thread is dedicated for resizes: constant expand-shrink between 8192 and 16384 buckets.

  => Executes 15 (+1 resizer) threads for 10 seconds.
 
./bench-rcu -a -g1 -b8192 -d10000 -i65536 -r131072 -w1 -u0 -n16

  => Works as the previous example but uses RCU instead.

