//
//  aberth.cpp
//  polynomial_root_finder
//
//  Created by Elizabeth Aryslanova on 02/06/2026.
//
#include "CoeffToRootsAberth.h"
#define _USE_MATH_DEFINES
#include <math.h>
//#include <iostream>

Aberth::c_vector Aberth::solve(const vector& coefficients)
{
    // helper variables
    bool keep_going = true;
    int iteration_counter = 0;


    // polynomial
    vector polynomial (polynomial_coefficients(coefficients));
    auto polynomial_length = static_cast<int>(polynomial.size());
    int n = polynomial_length - 1;
    c_vector new_guesses(n); // TO-DO add the size

    // derivative
    vector derivative (derivative_coefficients(polynomial, n));
    // initial guesses
    c_vector previous_guesses (initial_guesses(polynomial));

    while (keep_going && (iteration_counter < max_iterations))
    {
        update_guesses(polynomial, derivative, previous_guesses, new_guesses);
        keep_going = continue_condition(previous_guesses, new_guesses);

        previous_guesses = new_guesses; // it is expensive
        iteration_counter++;
    }
    return new_guesses;
}

Aberth::vector Aberth::polynomial_coefficients(const vector& coefficients)
{
    // get rid of 0 in the beginning
    const auto coeff_length = static_cast<int>(coefficients.size());
    std::vector<float> polynomial = coefficients;
    int to_remove = -1;

    for (int i = 0; i < coeff_length; i++)
    {
        if (polynomial[i] == 0) to_remove = i;
        else break;
    }

    if (to_remove != -1)
    {
        polynomial.erase(polynomial.begin(), polynomial.begin()+(to_remove+1));
    }

    // reverse the order
    std::reverse(polynomial.begin(), polynomial.end());

    return polynomial;
}

Aberth::vector Aberth::derivative_coefficients(const vector& polynomial, const int n)
{
    const auto p_length = static_cast<int>(polynomial.size());
    std::vector<float> dp(p_length-1);
    for (int i = 0; i < p_length-1; i++)
    {
        dp[i] = polynomial[i+1] * (i+1);
    }

    return dp;
}

Aberth::c_vector Aberth::initial_guesses(const vector& polynomial)
{
    const int n = static_cast<int>(polynomial.size()) - 1;
    const float theta = 2 * M_PI / n;
    const float offset = theta / (n+1);
    const float radius = std::pow(abs(polynomial[0] / polynomial[n]), 1/n);
    const std::complex<float> j(0.0, 1.0);

    // initial guesses calculation
    std::vector<std::complex<float>> guesses(n);
    for (int i = 0; i < n; i++)
    {
        // guesses[k] =(radius * cmath.exp( 1j * (k * theta + offset)))
        guesses[i] = radius * std::exp(j * (i * theta + offset));
    }
    return guesses;
}

Aberth::c_vector Aberth::newton_coefficients(const vector& polynomial, const vector& derivative, const c_vector& guesses)
{
    // unites p_of_rn and newton_coeff
    const int n = static_cast<int>(polynomial.size()) - 1;
    std::vector<std::complex<float>>  coeff(n);
    std::complex<float> p_rn = 0;
    std::complex<float> dp_rn = 0;

    for (int i = 0; i < n; i++) // loop over guesses
    {
        p_rn = 0;
        dp_rn = 0;

        for( int k = 0; k < n; k++)
        {
            p_rn += static_cast<std::complex<float>>(std::pow(guesses[i], k)) * polynomial[k];
            dp_rn += static_cast<std::complex<float>>(std::pow(guesses[i], k)) * derivative[k];
        }
        p_rn += static_cast<std::complex<float>>(std::pow(guesses[i], n)) * polynomial[n];

        coeff[i] = p_rn / dp_rn;
    }
    return coeff;
}


Aberth::c_vector Aberth::thing_in_parenths(const c_vector& guesses)
{
    const int n = static_cast<int>(guesses.size());
    std::vector<std::complex<float>> paren_content(n);

    for (int i = 0; i < n; i++ )
    {
        std::complex<float> rn_sum = 0;
        for (int k = 0; k < n; k++)
        {
            if (i != k)
            {
                rn_sum += 1.0f / (guesses[i] - guesses[k]);
            }
        }
        paren_content[i] = rn_sum;
    }
    return paren_content;
}

Aberth::c_vector Aberth::update_guesses(const vector& polynomial, const vector& derivative, const c_vector& guesses, c_vector& new_guesses)
{
    const int n = static_cast<int>(guesses.size());
    std::vector<std::complex<float>> parenths = thing_in_parenths(guesses);
    std::vector<std::complex<float>> newton = newton_coefficients(polynomial, derivative, guesses);

    for (int i = 0; i < n; i++)
    {
        new_guesses[i] = guesses[i] - newton[i] / (1.0f - newton[i]*parenths[i]);
    }
    return new_guesses;
}

bool Aberth::continue_condition(const c_vector& prev_guesses, const c_vector& new_guesses)
{
    bool do_i_stop = true;
    const int n = static_cast<int>(prev_guesses.size());

    for (int i = 0; i < n; i++)
    {
        if (abs(new_guesses[i]-prev_guesses[i]) < epsilon )
        {
            do_i_stop = do_i_stop && true;
        }
        else
        {
            do_i_stop = do_i_stop && false;
        }
    }
    return !do_i_stop;
}

