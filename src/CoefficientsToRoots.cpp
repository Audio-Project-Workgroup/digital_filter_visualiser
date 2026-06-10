// #pragma once
// #include <juce_audio_processors/juce_audio_processors.h>
// #include <juce_dsp/juce_dsp.h>

#include "CoefficientsToRoots.h"

#include <algorithm>
#include <iostream>
#include <cmath>

std::vector<std::pair<c128, int>> CoefficientsToRoots::GramSchmidt(std::vector<double> coefs)
{

    // filter out leading zeros, increasing counter until the leading 1.0 is found.
    size_t degree = 0;
    while( degree < coefs.size() && coefs[degree] != 1.0)
        degree++;
    degree = coefs.size() - degree -1; // subtract 1 for the leading 1.0
 
   // filter out trailing zeros increasing the order of root at Zero (usually for default poles)
    size_t orderAtZero = 0;
    for( int i = static_cast<int>(coefs.size()-1); i >= 0 && std::abs(coefs[(size_t)i]) == 0.0 ; --i)
    {
        ++orderAtZero;
        --degree;
    }

    // append root at zero of "orderAtZero" order
    std::vector<std::pair<c128, int>> roots;
    if (orderAtZero>0)
        roots.emplace_back(std::make_pair(0.0, orderAtZero));

    if (degree == 0 )
        // Returing root at (0,0) if any
        return roots;

    if (degree==1)
    {
        // Returing 1 non-zero root + root at (0,0) if any
        roots.emplace_back( std::make_pair(static_cast<c128>(-coefs[coefs.size() - 1]), 1) );
        return roots;
    }

    // build companion Matrix
    std::cout<<"Building companion matrix with degree "<<degree<<std::endl;
    std::vector<std::vector<double>> A(degree, std::vector<double>(degree, 0.0));
    // Build companion matrix (Hessenberg form)
    for (size_t i=0; i< degree; i++)
    {
        size_t j = i+1;
        size_t coef_idx = coefs.size()-1 - orderAtZero -i;
        A[i][degree-1] = -coefs[coef_idx];  // fill last column with negative vals of coefs
        if (i!=degree-1)
        {
            A[j][i] = 1.0;  // fill subdiagonal entries         
        }
    }

        
    std::cout<<"// Check companion matrix"<<std::endl;
    printCheck(A);


    // QR algorithm
    std::vector<std::vector<double>> Q(degree, std::vector<double>(degree));
    std::vector<std::vector<double>> R(degree, std::vector<double>(degree));
    
    size_t shift_idx {degree}, iter {0};
    while(shift_idx > 1)
    {

        if (++iter > degree * MaxIterations) break;


        // Reset R,Q
        for (size_t r = 0; r < shift_idx; ++r)
        {
            for (size_t c = 0; c < shift_idx; ++c)
            {
                R[r][c] = 0.0;
                Q[r][c] = 0.0;
            }
        }

        // subtract rayleigh quotient shift
        const double shift = A[shift_idx - 1][shift_idx - 1];
        for (size_t i = 0; i < shift_idx; ++i) 
            A[i][i] -= shift; // Decompose (Ak - sI)

        // step 1 - calculate Q : Q = A[col] - projections onto the previous Q[col]
        for (size_t col = 0 ; col < shift_idx; col++)
        {
            // set v with column A[col]
            std::vector<double> v(shift_idx);
            for (size_t row = 0 ; row< shift_idx; row++)
                v[row] = A[row][col];

            // subtract projection : v[col] = A[col] - proj onto the previous (<col) orthonormal colums of Q.
            for (size_t curr_col = 0; curr_col < col; curr_col++)
            {
                // compute dot product ...
                double dot_product {0.0};
                for (size_t curr_row = 0; curr_row < shift_idx; ++curr_row)
                {
                    dot_product += Q[curr_row][curr_col] * A[curr_row][col];
                }

                // .. and subtract it from the current column vector
                for (size_t curr_row=0; curr_row < shift_idx; ++curr_row)
                {
                    v[curr_row] -= dot_product * Q[curr_row][curr_col];
                }
            }

            // normalize column of q
            double norm = 0.0;
            for (size_t k = 0; k < shift_idx; ++k)
            {
                norm += v[k] * v[k];
            }
            norm = std::sqrt(norm);
            
            for (size_t k = 0; k < shift_idx; ++k)
            {
                Q[k][col] = v[k] / norm;
            }
        }

        // step 2 - calculate R :  R = Q A
        for (size_t row = 0; row < shift_idx; row++)
        {
            for (size_t col = 0; col < shift_idx; col++)
            {
                for (size_t inner = 0; inner < shift_idx; inner++)
                {
                    R[row][col] += (Q[inner][row] * A[inner][col]);     // Q transposed
                }
            }
        }

        // step 3 - A = R Q
        for (size_t row = 0; row < shift_idx; row++)
        {
            for (size_t col = 0; col < shift_idx; col++)
            {
                double sum {0.0};
                for (size_t inner = 0; inner< shift_idx; ++inner)
                {
                    sum += R[row][inner] * Q[inner][col];
                }
                A[row][col] = sum; // resetting A[row][col]
            }
        }


        // step 4 add shift back in A and check sub-diagonal entries
        for (size_t i = 0; i < shift_idx; ++i) 
            A[i][i] += shift;

        // check only the last subdiagonal entry (real eigenvalue)
        if (std::abs(A[shift_idx-1][shift_idx-2]) < Epsilon)
        {
            shift_idx--;
        }
        // check the entry above the 2x2 block in search of complex conjugate pairs
        else if (shift_idx > 2 && std::abs(A[shift_idx-2][shift_idx-3]) < Epsilon)
        {
            shift_idx -= 2;
        }

    }

    // Extract roots — read diagonal
    extractRoots(roots, A, degree);
    return roots;
}

void CoefficientsToRoots::printCheck(const std::vector<std::vector<double>> &matrix)
{
    size_t n = matrix.size();
    for (size_t i=0; i< n; i++)
    {
        for (size_t j=0; j<n; j++)
        {
            std::cout<<matrix[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
}

void CoefficientsToRoots::extractRoots(std::vector<std::pair<c128, int>> & roots, const std::vector<std::vector<double>>& M, size_t degree)
{

    std::cout<<"// Check Roots"<<std::endl;
    CoefficientsToRoots::printCheck(M);
    
    // std::vector<std::pair<c128, int>> roots;

    auto addRoot = [&](c128 newVal) {
        for (auto& [val, order] : roots)
        {
            double diff_re = std::abs(val.real() - newVal.real());
            double diff_im = std::abs(val.imag() - newVal.imag());
            double scale = std::abs(val) + std::abs(newVal) + 1e-7; // + 1e-7 to avoid division with zero

            if (diff_re / scale < tolerance && diff_im / scale < tolerance)
            {
                order++;
                return;
            }
        }
        roots.emplace_back(newVal, 1);
    };

    size_t i = 0;
    while (i < degree)
    {
        
        if ( i == degree - 1 || std::abs(M[i+1][i]) < Epsilon )
        {
            // Real eigenvalue on diagonal
            c128 newRoot (M[i][i], 0.0);
            addRoot(newRoot);
            ++i;
        }
        else
        {
            // check if conjugate by solving the equation which instantiates by the 2x2 cell:
            // https://www.physicsforums.com/threads/how-do-i-estimate-complex-eigenvalues.170108/post-1330547
            // https://www.mosismath.com/Eigenvalues/EigenvalsQR.html
            // det(A - λI) = 0
            // =>  det| a-λ    b |  
            //        |   c  d-λ | = 0 
            // => (a-λ)(d-λ) - bc = 0
            // => λ^2 - (a+d)λ + (ad-bc) = 0
            // => λ^2 - (a+d)λ + detA = 0 --> solve with discriminant
            const double a = M[i][i];
            const double b = M[i][i+1];
            const double c = M[i+1][i];
            const double d = M[i+1][i+1];
            const double det  = a*d - b*c;
            // solve with discriminant
            const double discriminant = (a+d)*(a+d) - 4.0*det;

            if ( discriminant >= 0.0)
            {
                // tow real roots
                addRoot(c128(((a+d) + std::sqrt(discriminant)) / 2.0, 0.0));
                addRoot(c128(((a+d) - std::sqrt(discriminant)) / 2.0, 0.0));
            }
            else
            {
                // Complex conjugate pair
                const double re = (a+d) / 2.0;
                const double im = std::sqrt(-discriminant) / 2.0;
                addRoot(c128(re,im));
                // addRoot(c128(re,-im)); // Note: this is added automatically using addRoot. Commenting this, removes the bug of overlapping roots.
            }
            i += 2;
        }
    }

#ifdef DEBUG_C2R
    std::cout<<"Calculated Roots: ";
    for (auto root : roots)
        std::cout<<"("<<root.first.real()<<","<<root.first.imag()<<") - "<<root.second<<", ";
    std::cout<<std::endl;
#endif
}