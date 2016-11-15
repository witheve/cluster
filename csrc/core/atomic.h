static inline u64 fetch_and_add(volatile u64 * variable, u64 value)
{
    asm volatile("lock; xaddq %0, %1"
                 : "=r" (value), "=m" (*variable)
                 : "0"(value), "m"(*variable)
                 :"memory", "cc");
    return value;
}
