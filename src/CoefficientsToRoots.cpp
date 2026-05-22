// #pragma once
// #include <juce_audio_processors/juce_audio_processors.h>
// #include <juce_dsp/juce_dsp.h>

#include "CoefficientsToRoots.h"

#include <algorithm>
#include <iostream>
#include <cmath>

std::vector<c128> CoefficientsToRoots::GramSchmidt(std::vector<double> coefs)
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
        return {static_cast<c128>(coefs[degree])};
    }

    // build companion Matrix
    std::cout<<"Building companion matrix with degree "<<degree<<std::endl;
    std::vector<std::vector<double>> A(degree, std::vector<double>(degree, 0.0));
    // Build companion matrix (Hessenberg form)
    for (int i=0; i< degree; i++)
    {
        int j = i+1;
        int coef_idx = degree-i;
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

        // step 3 - A = Q R
        for (int row = 0; row < degree; row++)
        {
            for (int col = 0; col < degree; col++)
            {
                double sum {0.0};
                for (int inner = 0; inner< degree; ++inner)
                {
                    sum += Q[inner][col] * R[row][inner];
                }
                A[row][col] = sum; // resetting A[row][col]
            }
        }
        
        // step 4 sum all sub-diagonal entries
        double subdiag_sum {0.0};
        for (int i = 1; i < degree; ++i)
        {
            subdiag_sum += std::abs(A[i][i-1]);
        }

        if (subdiag_sum < Epsilon)
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


std::vector<c128> CoefficientsToRoots::extractRoots(const std::vector<std::vector<double>>& M, int degree)
{
    // https://www.mosismath.com/Eigenvalues/EigenvalsBasics.html --> public void CalcEigenvals()
    std::vector<c128> roots;
 
    int i = 0;
    while (i < degree)
    {
        double a, b, c;
        a = 1.0;
        b = -M[i][i] - M[i+1][i+1];
        c = M[i][i] * M[i+1][i+1] - M[i][i+1] * M[i+1][i];

        if ((b * b) >= (4.0 * a * c))
        {
            roots.emplace_back(
                (-b + std::sqrt((b * b) - (4.0 * a * c))) / 2.0 / a, 
                0.0);
            roots.emplace_back(
                (-b - std::sqrt((b * b) - (4.0 * a * c))) / 2.0 / a,
                0.0);
            ++i;
        }
        else
        {
            double realPart = -b / 2.0 / a;
            double imagPart =
                std::sqrt((4.0 * a * c) - (b * b))
                / 2.0 / a;

            roots.emplace_back(realPart,  imagPart);
            roots.emplace_back(realPart, -imagPart);

            i += 2;
        }
    }

    std::cout<<"Calculated Roots: ";
    for (auto root : roots)
        std::cout<<"("<<root.real()<<","<<root.imag()<<")  ";
    std::cout<<std::endl;

    return roots;

}