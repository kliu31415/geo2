#pragma once

#include "kx/debug.h"

#ifdef KX_DEBUG
#define PURE_IF_NOT_KX_DEBUG
#else
#define PURE_IF_NOT_KX_DEBUG [[gnu::pure]]
#endif

#include <vector>
#include <cmath>

namespace kx { namespace stats {

/** All functions should be thread-safe. The functions involving normal distributions
 *  are thread-safe as long as std::erf is.
 */

[[gnu::pure]] ///not const because it could rely on the global rounding mode
double standard_normal_cdf(double z);

PURE_IF_NOT_KX_DEBUG
double normal_cdf(double z, double mean_, double sd);

/** WARNING: These two functions work for all inputs I've tested it on, but
 *  that doesn't mean that they'll give an accurate result for all inputs. In
 *  particular, they might even result in an infinite loop, which can definitely
 *  happen for some illegal inputs, but might happen for legal ones as well.
 *
 *  -v is the number of degrees of freedom
 *  -this is quite slow (up to 0.028ms per call if t>3)
 *  -I think if |t|<10, the relative error of this function is at most 1e-3.
 *  -v is a int32_t because there are internal precision issues that start
 *  somewhere between v=2e9 and v=1e12.
 */
double student_t_cdf(int v, double t);
double t_to_one_sided_p(int v, double t);

template<class DataSet, int power> PURE_IF_NOT_KX_DEBUG
double sumpow(const DataSet &X)
{
    double sum_ = 0;
    for(const auto &x: X)
        sum_ += std::pow(x, power);
    return sum_;
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
double sum(const DataSet &X)
{
    return sumpow<DataSet, 1>(X);
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
double sumsq(const DataSet &X)
{
    return sumpow<DataSet, 2>(X);
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
double mean(const DataSet &X) ///computes E[X]
{
    k_expects(!X.empty());
    return sum(X) / X.size();
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
double var(const DataSet &X) ///computes Var(X) = E[X^2] - E[X]^2
{
    k_expects(!X.empty());
    double avg = mean(X);
    double sum = 0;
    for(const auto &value: X) {
        sum += std::pow(value - avg, 2);
    }
    return sum / X.size();
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
double stdev(const DataSet &X) ///computes SD(X) = sqrt(Var(X))
{
    k_expects(X.size() > 0);
    return std::sqrt(var(X));
}

template<class DataSet1, class DataSet2> PURE_IF_NOT_KX_DEBUG
double cov(const DataSet1 &X, const DataSet2 &Y) ///computes Cov(X, Y) = E[XY] - E[X]E[Y]
{
    k_expects(!X.empty());
    //k_expects(Y.size() != 0); //unnecessary
    k_expects(X.size() == Y.size());
    double E_XY=0;
    for(size_t i=0; i<X.size(); i++) {
        E_XY += X[i] * Y[i];
    }
    E_XY /= X.size();
    return E_XY - mean(X) * mean(Y);
}

template<class DataSet1, class DataSet2> PURE_IF_NOT_KX_DEBUG
double corr(const DataSet1 &X, const DataSet2 &Y)
{
    k_expects(!X.empty());
    //k_expects(Y.size() != 0); //unnecessary
    k_expects(X.size() == Y.size());
    return cov(X, Y) / std::sqrt(var(X) * var(Y));
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
std::vector<double> calc_ranks(const DataSet &X)
{
    k_expects(!X.empty());
    DataSet X_sorted = X;
    std::sort(X_sorted.begin(), X_sorted.end());

    std::vector<double> X_ranks(X.size());
    for(size_t i=0; i<X.size(); i++) {
        auto lb = std::lower_bound(X_sorted.begin(), X_sorted.end(), X[i]);
        //can we use lb+1? lb+1 might equal X_sorted.end()-1, which results in a 0 length input
        //idk if upper_bound supports this or not
        auto ub = std::upper_bound(lb, X_sorted.end(), X[i]);
        X_ranks[i] = ((lb - X_sorted.begin()) + (ub - X_sorted.begin()) + 1) / 2.0;
    }
    return X_ranks;
}

template<class DataSet1, class DataSet2> PURE_IF_NOT_KX_DEBUG
double spearman_corr(const DataSet1 &X, const DataSet2 &Y)
{
    k_expects(!X.empty());
    //k_expects(Y.size() != 0); //unnecessary
    k_expects(X.size() == Y.size());

    auto X_ranks = calc_ranks(X);
    auto Y_ranks = calc_ranks(Y);

    return corr(X_ranks, Y_ranks);
}

template<class DataSet1, class DataSet2> PURE_IF_NOT_KX_DEBUG
double OLS_slope(const DataSet1 &X, const DataSet2 &Y)
{
    k_expects(!X.empty());
    //k_expects(Y.size() != 0); //unnecessary
    k_expects(X.size() == Y.size());
    return cov(X, Y) / var(X);
}

template<class DataSet1, class DataSet2> PURE_IF_NOT_KX_DEBUG
double OLS_intercept(const DataSet1 &X, const DataSet2 &Y, double slope)
{
    k_expects(!X.empty());
    //k_expects(Y.size() != 0); //unnecessary
    k_expects(X.size() == Y.size());
    return mean(Y) - slope * mean(X);
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
double skewness(const DataSet &X)
{
    k_expects(!X.empty());
    double avg = mean(X);
    double sum_ = 0;
    for(const auto &value: X) {
        sum_ += std::pow(value - avg, 3);
    }
    sum_ /= X.size();
    return sum_ / std::pow(var(X), 1.5);
}

template<class DataSet> PURE_IF_NOT_KX_DEBUG
double kurtosis(const DataSet &X)
{
    k_expects(!X.empty());
    double avg = mean(X);
    double sum_ = 0;
    for(const auto &value: X) {
        sum_ += std::pow(value - avg, 4);
    }
    sum_ /= X.size();
    return sum_ / std::pow(var(X), 2);
}

}}

#undef PURE_IF_NOT_KX_DEBUG
