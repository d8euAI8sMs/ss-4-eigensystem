#pragma once

#include <util/common/math/common.h>
#include <util/common/math/dsolve.h>

namespace model
{

    using cv3 = math::v3 < math::complex < > > ;
    using rv3 = math::v3 < > ;

    static inline math::continuous_t make_barrier_fn
    (
        double rho0,
        double v0
    )
    {
        return [=] (double s)
        {
            return ((s > rho0) ? 0 : -v0);
        };
    }

    static inline math::dfunc3_t < rv3 > make_wavefunc_dfunc
    (
        math::continuous_t barrier_fn,
        double s0,
        double e,
        int l
    )
    {
        return [=] (double s, const rv3 & rho_phi) -> rv3
        {
            double E = ((l * (l + 1)) / s0 / s / s + s0 * (e - barrier_fn(s)));
            return
            {
                (1 - E) * rho_phi.at<0>() * std::sin(2 * rho_phi.at<1>()) / 2,

                - E * std::cos(rho_phi.at<1>()) * std::cos(rho_phi.at<1>())
                - s0 * std::sin(rho_phi.at<1>()) * std::sin(rho_phi.at<1>())
            };
        };
    }

    static inline math::complex < > make_wavefunc_value
    (
        double s0,
        double s,
        double rho,
        double phi
    )
    {
        return rho * math::exp(math::_i * phi) / s / s0;
    }
}
