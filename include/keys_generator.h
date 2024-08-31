#ifndef NBTLRU_KEYS_GENERATOR_H
#define NBTLRU_KEYS_GENERATOR_H

#include <random>
#include <concepts>
#include <generator>

namespace nbtlru
{
    template<template<typename> typename Dist, 
             ::std::integral Int = uint64_t>
    ::std::generator<Int> gen(Int scale, bool random_device_enable = false)
    {
        ::std::mt19937_64 rng(random_device_enable ? ::std::random_device{}() : 42);
        Dist<Int> dist(Int{}, scale);
        for (size_t i{}; i < scale; ++i)
        {
            co_yield dist(rng);
        }
    }

} // namespace nbtlru::keys

#endif
