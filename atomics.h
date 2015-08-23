
#ifndef ATOMICS_H
#define ATOMICS_H 1


/////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>

/////////////////////////////////////////////////////////
// DEFINES
/////////////////////////////////////////////////////////
#define __ADDL(m,v)                                  \
  ({ int _x = (v) ;                                  \
     __asm__ __volatile__ (                          \
       "lock;addl %[SET],%[MEM]; "                   \
       : [SET] "+r" (_x)                             \
       : [MEM] "m" (*(volatile int *)(m))            \
       : "memory" ) ;                                \
   _x ; })

#if defined(__x86_64)
# define LOCKED_ADD(m,v) __ADDL((m),(v))
#elif defined(__PPC64__)
# define LOCKED_ADD(m,v) __sync_fetch_and_add((m),(v))
#endif

//#define membarstoreload() { __asm__ __volatile__ ("mfence;") ; } 

#if defined(__x86_64)
# define membarstoreload() { __asm__ __volatile__ ("lock;addb $0,(%%rsp);" ::: "cc") ; }
#elif defined(__PPC64__)
# define membarstoreload() __sync_synchronize()
#endif

//#define MEMBARSTLD() { \
	unsigned long dummy = 0; \
	LOCKED_ADD(&dummy, 0); \
}

#define MEMBARSTLD() membarstoreload()
  
#define CAS64(m,c,s)                                            \
  ({ int64_t _x = (c);                                          \
     __asm__ __volatile (                                       \
       "lock; cmpxchgq %1,%2;"                                  \
       : "+a" (_x)                                              \
       : "r" ((int64_t)(s)), "m" (*(volatile int64_t *)(m))     \
       : "cc", "memory") ;                                      \
   _x; })



#if defined(__x86_64)
# define CPU_RELAX() asm volatile("pause\n": : :"memory");
#elif defined(__PPC64__)
# define CPU_RELAX() 
#endif

/////////////////////////////////////////////////////////
// EXTERNAL FUNCTIONS
/////////////////////////////////////////////////////////
volatile int64_t atomic_add(volatile int64_t * addr, int64_t dx);
volatile int64_t CAS(volatile int64_t * addr, int64_t expected_value, int64_t new_value);


#endif // ATOMICS_H
