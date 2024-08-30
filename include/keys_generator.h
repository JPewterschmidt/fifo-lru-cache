#ifndef NBTLRU_KEYS_GENERATOR_H
#define NBTLRU_KEYS_GENERATOR_H

#include <random>
#include <concepts>
#include <generator>

namespace nbtlru
{
    template<template<typename> typename Dist, 
             ::std::integral Int = uint64_t>
    ::std::generator<Int> gen(Int scale)
    {
        ::std::mt19937 rng(42);
        Dist<Int> dist(Int{}, scale);
        for (size_t i{}; i < scale; ++i)
        {
            co_yield dist(rng);
        }
    }

} // namespace nbtlru::keys

#endif
