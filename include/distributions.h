#ifndef NBTLRU_DISTRIBUTIONS_H
#define NBTLRU_DISTRIBUTIONS_H

#include <random>
#include <concepts>
#include <string_view>

#include "dirty_zipfian_int_distribution.h"

namespace nbtlru
{

template<::std::integral Int>
class zipf : public dirtyzipf::dirty_zipfian_int_distribution<Int>
{
public:
    using result_type = Int;
    using dirtyzipf::dirty_zipfian_int_distribution<Int>::dirty_zipfian_int_distribution;
    static constexpr ::std::string_view name() noexcept { return "Zipfian"; }
};

template<::std::integral Int>
class uniform : public ::std::uniform_int_distribution<Int>
{
public:
    using result_type = Int;
    using ::std::uniform_int_distribution<Int>::uniform_int_distribution;
    static constexpr ::std::string_view name() noexcept { return "Uniform"; }
};

template<::std::integral Int>
class normal
{
public:
    using result_type = Int;
    normal(Int min, Int max)
        : m_dist((min + max) / 2.0, (max - min) / 6.0)
    {
    }

    static constexpr ::std::string_view name() noexcept { return "Normal"; }

    void reset() { m_dist.reset(); }
    result_type operator()(auto& gen) { return static_cast<result_type>(::std::round(m_dist(gen))); }
    result_type operator()(auto& gen, const auto& params) { return static_cast<result_type>(::std::round(m_dist(gen, params))); }

private:
    ::std::normal_distribution<double> m_dist;
};

template<::std::integral Int>
class lognormal 
{
public:
    using result_type = Int;
    lognormal(Int min, Int max)
        : m_dist(::std::log((min + max) / 2.0), 1.0)
    {
    }

    static constexpr ::std::string_view name() noexcept { return "Log-Normal"; }

    void reset() { m_dist.reset(); }
    result_type operator()(auto& gen) { return static_cast<result_type>(::std::round(m_dist(gen))); }
    result_type operator()(auto& gen, const auto& params) { return static_cast<result_type>(::std::round(m_dist(gen, params))); }

private:
    ::std::lognormal_distribution<double> m_dist;
};

} // namespace nbtlru

#endif
