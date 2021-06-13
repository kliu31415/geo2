#include "kx/debug.h"

#define __STDCPP_WANT_MATH_SPEC_FUNCS__ 1
#include <cmath>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

namespace kx { namespace stats {

using std::sqrt;
using std::atan;
using std::pow;
using std::log; //apparently there's no logl function but log is overloaded with long double

double standard_normal_cdf(double z)
{
    return 0.5 + std::erf(z / sqrt(2)) / 2;
}
double normal_cdf(double z, double mean_, double sd)
{
    k_expects(sd >= 0);
    return 0.5 * (1 + std::erf((z - mean_) / (sd * sqrt(2))));
}

static inline double beta_integrand(double t, double a)
{
    return pow(t, a-1) / std::sqrt(1-t);
}
//DON'T move this to the global scope. This function is SPECIFICALLY
//tailored for student_t_cdf and won't work for all inputs
//(notably, it returns nan/inf if a=0.5 and b=0.5)
static double reg_incomplete_beta_numer_integrate(double x, double a) //assumes b=0.5
{
    if(x <= 0)
        return 0;
    if(x >= 1)
        return 1;

    constexpr int NUM_BIN_SEARCH_ITER = 40;
    constexpr double RATIO_CUTOFF = 1e5;

    double lo = 0;
    double hi = x;
    double prev_hi_beta_integrand = beta_integrand(hi, a);
    for(int i=0; i<NUM_BIN_SEARCH_ITER; i++) {
        double mid = (lo + hi) / 2;
        double mid_beta_integrand = beta_integrand(mid, a);
        if(prev_hi_beta_integrand / mid_beta_integrand > RATIO_CUTOFF) {
            lo = mid;
        } else {
            hi = mid;
            prev_hi_beta_integrand = mid_beta_integrand;
        }
    }

    constexpr int NUM_INTEGRAL_PIECES = 1000;
    constexpr double EPS = 1e-10;

    const double INTEGRAL_STEP = ((x-lo)/NUM_INTEGRAL_PIECES) * (1 - EPS);
    k_ensures(INTEGRAL_STEP > 0);

    double res = 0;

    for(double i=x; i>lo; i-=INTEGRAL_STEP)
        res += beta_integrand(i, a);

    res -= 0.5 * (beta_integrand(lo, a) + beta_integrand(x, a));
    res *= INTEGRAL_STEP;

    return res / std::beta(a, 0.5);
}
static double reg_incomplete_beta_cont_frac(double x, double a, double b)
{
    if(a > b)
        return 1 - reg_incomplete_beta_cont_frac(1-x, b, a);

    if(x <= 0)
        return 0;
    if(x >= 1)
        return 1;

    double f = 1;
    for(int i=20; i>=1; i--) {

        double d;
        int m = i/2;
        if(i%2 == 1)
            d = -(a+m)*(a+b+m)*x / ((a+2*m)*(a+2*m+1));
        else
            d = m*(b-m)*x / ((a+2*m-1)*(a+2*m));
        f = 1 + d / f;
    }

    return pow(x, a) * pow(1-x, b) / (a * std::beta(a, b) * f);
}
double student_t_cdf(int v, double t)
{
    k_expects(v>=1 && !std::isnan(t) && !std::isinf(t));

    if(v == 1) //beta_integrand(0, 0.5, 0.5) is inf, so v=1 is a special case
        return 0.5 + atan(t) / M_PI;

    //using the incomplete fraction has precision issues for t>4, and using
    //numerical integration has precision issues for t<1e-3, so pick which one
    //to use depending on the value of t
    double beta_val;
    if(std::fabs(t) < 3)
        beta_val = 0.5*reg_incomplete_beta_cont_frac(v / (t*t+v), 0.5*v, 0.5);
    else
        beta_val = 0.5*reg_incomplete_beta_numer_integrate(v / (t*t+v), 0.5*v);
    if(t > 0)
        return 1 - beta_val;
    return beta_val;

    /*
    //takes up to 0.8ms if v=6
    switch(v) {
    case 1:
        return 0.5 + atan(t) / M_PI;
    case 2:
        return 0.5 + t / sqrt(8 * (1 + t*t/2));
    case 3:
        return 0.5 + (t / (sqrt(3) * (1 + t*t/3)) + atan(t / sqrt(3))) / M_PI;
    case 4:
        return 0.5 + 3*t/(8*sqrt(1 + t*t/4)) * (1 - t*t / (12 + 3*t*t));
    case 5:
        return 0.5 + (t / (sqrt(5) * (1 + t*t/5)) * (1 + 2 / (3 * (1 + t*t/5))) + atan(t / sqrt(5))) / M_PI;
    default: {
        bool t_is_positive;
        if(t > 0) {
            t_is_positive = true;
            t = -t;
        } else
            t_is_positive = false;

        static constexpr long double T_TEST_INTEGRAL_STEP = 0.005;
        //lower values of n have much fatter tails, so decrease their start offset
        long double recip = 100.0 / (v-3);
        const long double T_TEST_START_OFFSET = 5 + 1e-6 + recip - std::fmod(recip, T_TEST_INTEGRAL_STEP);

        long double start = t-T_TEST_START_OFFSET;
        long double res = 0;

        for(long double i=start; i<=t; i+=T_TEST_INTEGRAL_STEP) {
            res += pow(1 + i*i / v, -0.5*(v+1));
        }

        long double first_val = pow(1 + start*start/v, -0.5*(v+1));
        long double last_val = pow(1 + t*t/v, -0.5*(v+1));
        res -= 0.5 * (first_val + last_val);
        res *= T_TEST_INTEGRAL_STEP;

        res /= sqrt((long double)v) * std::betal(0.5, 0.5*v);

        if(t_is_positive)
            res += 1 - 2*res;
        return res;}
    }
    */
}
double t_to_one_sided_p(int v, double t)
{
    double val = student_t_cdf(v, t);
    return std::min(val, 1-val);
}
}}
