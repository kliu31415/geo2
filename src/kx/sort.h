#pragma once

#include <algorithm>
#include <cstdint>

/** These are fast radix sorts that probably work on both ints and doubles.
 *  If you use them, you should double check their correctness to be sure.
 */

template<class T>
void lsd_radix_sort64_u(T *aIn, int n)
{
    static_assert(sizeof(T) == 8);

    uint64_t *a = reinterpret_cast<uint64_t*>(aIn);
    uint64_t *t = new uint64_t[n];
    int *cnt = new int[256];
    for(int_fast8_t f=0, z=0; f<64; f+=8, z++)
    {
        for(int i=0; i<=0xff; i++)
            cnt[i] = 0;
        uint8_t *a8 = reinterpret_cast<uint8_t*>(a);
        for(int i=0; i<n; i++)
            cnt[a8[i*8+z]]++;
        for(int i=1; i<=0xff; i++)
            cnt[i] += cnt[i-1];
        for(int i=n-1; i>=0; i--)
            t[--cnt[a8[i*8+z]]] = a[i];
        std::swap(a, t);
    }
    delete[] cnt;
    delete[] t;
}
template<class T>
void lsd_radix_sort64(T *aIn, int n)
{
    static_assert(sizeof(T) == 8);

    uint64_t *a = reinterpret_cast<uint64_t*>(aIn);
    for(int i=0; i<n; i++)
    {
        if(a[i] & 0x8000000000000000) //is negative?
            a[i] ^= 0xffffffffffffffff;
        else a[i] ^= 0x8000000000000000;
    }
    lsd_radix_sort64_u<T>(aIn, n);
    for(int i=0; i<n; i++)
    {
        if(a[i] & 0x8000000000000000) //is negative?
            a[i] ^= 0x8000000000000000;
        else a[i] ^= 0xffffffffffffffff;
    }
}
