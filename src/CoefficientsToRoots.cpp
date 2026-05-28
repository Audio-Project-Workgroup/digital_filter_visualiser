// #pragma once
// #include <juce_audio_processors/juce_audio_processors.h>
// #include <juce_dsp/juce_dsp.h>

#include "CoefficientsToRoots.h"

#include <algorithm>
#include <iostream>
#include <cmath>

std::vector<std::pair<c128, int>> CoefficientsToRoots::GramSchmidt(std::vector<double> coefs)
{
    // filter out leading zeros and leading 1.0
    int degree = 0;
    while(coefs[degree] ==0 || coefs[degree] == 1.0)
        degree++;
    degree = coefs.size() - degree;
    jassert( degree!=0 );

    if (degree==1)
    {
        std::cout<<"Returing 1 root --> "<<coefs[degree]<<std::endl;
        return {std::make_pair(static_cast<c128>(-coefs[coefs.size() - 1]), 1)};
    }

    // build companion Matrix
    std::cout<<"Building companion matrix with degree "<<degree<<std::endl;
    std::vector<std::vector<double>> A(degree, std::vector<double>(degree, 0.0));
    // Build companion matrix (Hessenberg form)
    for (int i=0; i< degree; i++)
    {
        int j = i+1;
        int coef_idx = coefs.size()-1-i;
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
    
    int iter = 0;
    while(true) // could alternatively check the convergence of the values of the sub-diagonal elements
    {

        if (++iter > degree * MaxIterations) break;


        // Reset R,Q
        for (int r = 0; r < degree; ++r)
        {
            for (int c = 0; c < degree; ++c)
            {
                R[r][c] = 0.0;
                Q[r][c] = 0.0;
            }
        }

        // step 1 - calculate Q : Q = A[col] - projections onto the previous Q[col]
        for (int col = 0 ; col < degree; col++)
        {
            // set v with column A[col]
            std::vector<double> v(degree);
            for (int row = 0 ; row< degree; row++)
                v[row] = A[row][col];

            // subtract projection : v[col] = A[col] - proj onto the previous (<col) orthonormal colums of Q.
            for (int curr_col = 0; curr_col < col; curr_col++)
            {
                // compute dot product ...
                double dot_product {0.0};
                for (int curr_row = 0; curr_row < degree; ++curr_row)
                {
                    dot_product += Q[curr_row][curr_col] * A[curr_row][col];
                }

                // .. and subtract it from the current column vector
                for (int curr_row=0; curr_row < degree; ++curr_row)
                {
                    v[curr_row] -= dot_product * Q[curr_row][curr_col];
                }
            }

            // normalize column of q
            double norm = 0.0;
            for (int k = 0; k < degree; ++k)
            {
                norm += v[k] * v[k];
            }
            norm = std::sqrt(norm);
            
            for (int k = 0; k < degree; ++k)
            {
                Q[k][col] = v[k] / norm;
            }
        }

        // step 2 - calculate R :  R = Q A
        for (int row = 0; row < degree; row++)
        {
            for (int col = 0; col < degree; col++)
            {
                for (int inner = 0; inner < degree; inner++)
                {
                    R[row][col] += (Q[inner][row] * A[inner][col]);     // Q transposed
                }
            }
        }

        // step 3 - A = R Q
        for (int row = 0; row < degree; row++)
        {
            for (int col = 0; col < degree; col++)
            {
                double sum {0.0};
                for (int inner = 0; inner< degree; ++inner)
                {
                    sum += R[row][inner] * Q[inner][col];
                }
                A[row][col] = sum; // resetting A[row][col]
            }
        }
        
        // step 4 check all sub-diagonal entries
        bool subdiag_low {true};
        for (int i = 1; i < degree; ++i)
        {
            if (std::abs(A[i][i-1]) > Epsilon)
            {
                subdiag_low = false;
                break;
            }
        }

        if (subdiag_low)
            break;
    }

    // Extract roots — read diagonal
    return extractRoots(A, degree);
}

void CoefficientsToRoots::printCheck(const std::vector<std::vector<double>> &matrix)
{
    int n = matrix.size();
    for (int i=0; i< n; i++)
    {
        for (int j=0; j<n; j++)
        {
            std::cout<<matrix[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
}

std::vector<std::pair<c128, int>> CoefficientsToRoots::extractRoots(const std::vector<std::vector<double>>& M, int degree)
{

    std::cout<<"// Check Roots"<<std::endl;
    CoefficientsToRoots::printCheck(M);
    
    std::vector<std::pair<c128, int>> roots;

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

    int i = 0;
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
    return roots;
}