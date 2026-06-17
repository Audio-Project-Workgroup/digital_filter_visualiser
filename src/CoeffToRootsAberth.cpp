//
//  aberth.cpp
//  polynomial_root_finder
//
//  Created by Elizabeth Aryslanova on 02/06/2026.
//
#include "CoeffToRootsAberth.h"

Aberth::c_vector Aberth::solve(const vector& coefficients)
{
    // helper variables
    bool keep_going = true;
    int iteration_counter = 0;
    c_vector new_guesses; // TO-DO add the size

    // polynomial
    vector polynomial (polynomial_coefficients(coefficients));
    auto polynomial_length = static_cast<int>(polynomial.size());
    int n = polynomial_length - 1;

    // derivative
    vector derivative (derivative_coefficients(polynomial));

    // initial guesses
    c_vector previous_guesses (initial_guesses(polynomial, n));


    while (keep_going && (iteration_counter < max_iterations))
    {
        update_guesses(polynomial, derivative, previous_guesses, new_guesses);
        keep_going = continue_condition(prev_guesses, new_guesses);

        previous_guesses = new_guesses; // it is expensive
        iteration_counter++;
    }

    return new_guesses;
}

Aberth::vector Aberth::polynomial_coefficients(const vector& coefficients)
{
    // get rid of 0 in the beginning
    // reverse the order
}

Aberth::vector Aberth::derivative_coefficients(const vector& polynomial, const int n)
{

}

Aberth::c_vector Aberth::initial_guesses(const vector& polynomial, const int n)
{

}

Aberth::c_vector Aberth::newton_coefficients(const vector& polynomial, const vector& derivative, const c_vector& guesses)
{
    // unites p of rn and newton coeff
}


Aberth::c_vector Aberth::thing_in_parenths(const c_vector& guesses)
{

}

Aberth::c_vector Aberth::update_guesses(const vector& polynomial, const vector& derivative, const c_vector& guesses, c_vector& new_guesses)
{

}

bool Aberth::continue_condition(const c_vector& prev_guesses, const c_vector& new_guesses)
{

}

