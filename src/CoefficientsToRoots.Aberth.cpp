//
//  aberth.cpp
//  polynomial_root_finder
//
//  Created by Elizabeth Aryslanova on 02/06/2026.
//
#include "CoefficientsToRoots.Aberth.h"
#define _USE_MATH_DEFINES
#include <math.h>
//#include <iostream>

Aberth::clustered_root_vector Aberth::Solve(Coefficients coefficients)
{
    // helper variables
    bool keep_going = true;
    int iteration_counter = 0;


    // polynomial
    vector polynomial (polynomial_coefficients(coefficients));
    size_t polynomial_length = polynomial.size();
    size_t n = polynomial_length - 1;
    c_vector new_guesses(n);

    // derivative
    vector derivative (derivative_coefficients(polynomial));
    // initial guesses
    c_vector previous_guesses (initial_guesses(polynomial));

    while (keep_going && (iteration_counter < max_iterations))
    {
        update_guesses(polynomial, derivative, previous_guesses, new_guesses);
        keep_going = continue_condition(previous_guesses, new_guesses);

        previous_guesses = new_guesses; // it is expensive
        iteration_counter++;
    }

    zero_small_values(new_guesses);
    remove_conjugates(new_guesses);
    // clustering routes
    clustered_root_vector clustered_rootes;
    clustering_rootes(new_guesses, clustered_rootes);
    return clustered_rootes;
}


void Aberth::zero_small_values(c_vector& new_guesses)
{
    for (size_t i = 1; i < new_guesses.size(); i++)
    {
        if (abs(std::imag(new_guesses[i])) <= 20.0f * epsilon) // zero the imaginary part
        {
            new_guesses[i] = std::complex<double> (new_guesses[i].real(), 0.0);
        }
        if (abs(std::real(new_guesses[i])) <= 20.0f * epsilon) // zero the real part
        {
            new_guesses[i] = std::complex<double> (0.0, new_guesses[i].imag());
        }
    }
}


void Aberth::clustering_rootes(c_vector& new_guesses, clustered_root_vector& clustered_rootes)
{
    for (const auto& new_guess : new_guesses)
    {
        bool is_clustered = false;

        for (auto& [val, order] : clustered_rootes)
        {
            if (std::abs(new_guess - val) < 2000.0f * epsilon)
            {
                order++;

                if (val.imag() > epsilon)
                {
                    double val_real = (new_guess.real() + val.real()) * 0.5;
                    double val_imag = (new_guess.imag() + val.imag()) * 0.5;

                    val = std::complex<double> (val_real, val_imag);
                }

                is_clustered = true;

                break;
            }
        }
        if (is_clustered == false)
        {
            clustered_rootes.emplace_back(new_guess, 1);
        }
    }
}



void Aberth::remove_conjugates(c_vector& new_guesses)
{
    int n = static_cast<int>(new_guesses.size());
    for (int i = n - 1; i >= 0; i--)
    {
        if (new_guesses[static_cast<size_t>(i)].imag() < 0)
        {
            new_guesses.erase(new_guesses.begin() + i);
        }
    }
}



Aberth::vector Aberth::polynomial_coefficients(const vector& coefficients)
{
    // get rid of 0 in the beginning
    const size_t coeff_length = coefficients.size();
    std::vector<double> polynomial = coefficients;
    int to_remove = -1;

    for (size_t i = 0; i < coeff_length; i++)
    {
        if (polynomial[i] < epsilon) to_remove = static_cast<int>(i);
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

Aberth::vector Aberth::derivative_coefficients(const vector& polynomial)
{
    const size_t p_length = polynomial.size();
    std::vector<double> dp(p_length-1);
    for (size_t i = 0; i < p_length-1; i++)
    {
        dp[i] = polynomial[i+1] * (i+1);
    }

    return dp;
}

Aberth::c_vector Aberth::initial_guesses(const vector& polynomial)
{
    const size_t n = polynomial.size() - 1;
    const double theta = 2 * M_PI / n;
    const double offset = theta / (n+1);
    const double radius = std::pow(abs(polynomial[0] / polynomial[n]), 1/n);
    const std::complex<double> j(0.0, 1.0);

    // initial guesses calculation
    std::vector<std::complex<double>> guesses(n);
    for (size_t i = 0; i < n; i++)
    {
        // guesses[k] =(radius * cmath.exp( 1j * (k * theta + offset)))
        guesses[i] = radius * std::exp(j * (i * theta + offset));
    }
    return guesses;
}

Aberth::c_vector Aberth::newton_coefficients(const vector& polynomial, const vector& derivative, const c_vector& guesses)
{
    // unites p_of_rn and newton_coeff
    const size_t n = polynomial.size() - 1;
    std::vector<std::complex<double>>  coeff(n);
    std::complex<double> p_rn = 0;
    std::complex<double> dp_rn = 0;

    for (size_t i = 0; i < n; i++) // loop over guesses
    {
        p_rn = 0;
        dp_rn = 0;

        for( size_t k = 0; k < n; k++)
        {
            p_rn += static_cast<std::complex<double>>(std::pow(guesses[i], k)) * polynomial[k];
            dp_rn += static_cast<std::complex<double>>(std::pow(guesses[i], k)) * derivative[k];
        }
        p_rn += static_cast<std::complex<double>>(std::pow(guesses[i], n)) * polynomial[n];

        coeff[i] = p_rn / dp_rn;
    }
    return coeff;
}


Aberth::c_vector Aberth::thing_in_parenths(const c_vector& guesses)
{
    const size_t n = guesses.size();
    std::vector<std::complex<double>> paren_content(n);

    for (size_t i = 0; i < n; i++ )
    {
        std::complex<double> rn_sum = 0;
        for (size_t k = 0; k < n; k++)
        {
            if (i != k)
            {
                rn_sum += 1.0 / (guesses[i] - guesses[k]);
            }
        }
        paren_content[i] = rn_sum;
    }
    return paren_content;
}

Aberth::c_vector Aberth::update_guesses(const vector& polynomial, const vector& derivative, const c_vector& guesses, c_vector& new_guesses)
{
    const size_t n = guesses.size();
    std::vector<std::complex<double>> parenths = thing_in_parenths(guesses);
    std::vector<std::complex<double>> newton = newton_coefficients(polynomial, derivative, guesses);

    for (size_t i = 0; i < n; i++)
    {
        new_guesses[i] = guesses[i] - newton[i] / (1.0 - newton[i]*parenths[i]);
    }
    return new_guesses;
}

bool Aberth::continue_condition(const c_vector& prev_guesses, const c_vector& new_guesses)
{
    bool do_i_stop = true;
    const size_t n = prev_guesses.size();

    for (size_t i = 0; i < n; i++)
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
