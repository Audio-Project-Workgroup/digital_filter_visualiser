//
//  aberth.h
//  polynomial_root_finder
//
//  Created by Elizabeth Aryslanova on 02/06/2026.
//
#include <vector>
#include <complex>

class Aberth
{
public:
    static std::vector<std::complex<float>> solve(const std::vector<float>& coefficients);

private:
    inline static constexpr float epsilon = 0.00001f;
    inline static constexpr int max_iterations = 10000;

    using vector = std::vector<float>;
    using c_vector = std::vector<std::complex<float>>;

    static vector polynomial_coefficients(const vector& coefficients);
    static vector derivative_coefficients(const vector& polynomial, const int n);
    static c_vector initial_guesses(const vector& polynomial);
    static c_vector newton_coefficients(const vector& polynomial, const vector& derivative, const c_vector& guesses);
    static c_vector thing_in_parenths(const c_vector& guesses);
    static c_vector update_guesses(const vector& polynomial, const vector& derivative, const c_vector& guesses, c_vector& new_guesses);
    static bool continue_condition(const c_vector& prev_guesses, const c_vector& new_guesses);

    Aberth() = delete; // This class can't be instantiated, it's just a holder for static methods..
};


