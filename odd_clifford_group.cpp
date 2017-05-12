/**
    NCG++
    odd_clifford_group.cpp

    Purpose:
    Given the model parameters (p, q) initialise
    the entire odd big gamma matrices

    \Gamma_\mu = { \gamma_mu, \gamma_{\mu \nu \rho}, ... }

    and provide a print / read function.


    @author Peter Labus
    @version 0.1
    11.05.2017
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include "model_parameters.hpp"
#include "gamma_matrix.hpp"
#include "clifford_algebra.hpp"
#include "odd_clifford_group.hpp"



// Member Functions:
// =================

OddCliffordGroup::OddCliffordGroup(ModelParameters const pqn)
  :
    pqn_(pqn),
    Gammas_(generate_odd_clifford_group_())
{
}


std::vector<GammaMatrix> OddCliffordGroup::generate_odd_clifford_group_()
{
  std::vector<GammaMatrix> big_gammas;
  const CliffordAlgebra small_gammas(pqn_);
  const auto d = small_gammas.size();

  // FIXME: Set back to odd indices eventually
  // for(auto num_indices = 1; num_indices <= d; num_indices += 2) // odd numbers of indices
  for(auto num_indices = 0; num_indices <= d; num_indices += 1) // all numbers of indices
  {
    // Calculate number of Gamma matrices with fixed number of indices:
    //   # Gamma matrices = (d choose num_indices)
    const auto num_matrices = binomial(d, num_indices);

    // Iterations over big_gammas with fixed number of indices
    for(auto num_comb = 0; num_comb < num_matrices; ++num_comb)
    {
      // 1. Generate the [num_comb]th combination with num_indices elements
      //    out of the range [0, 1, ..., d-1]. This will represent the
      //    indices of the antisymmetric product of small gamma matrices.
      // 2. Calculate the antisymmetric product and add it to the big
      //    gamma matrices.
      const auto index_sequence = combination(d, num_indices, num_comb);
      const auto matrix = small_gammas.antisymmetric_product(index_sequence);
      big_gammas.push_back(matrix);
    }
  }

  // TODO:
  // add reshuffling!

  return big_gammas;
}



// Non-Member Functions:
// =====================

std::ostream& operator<<(std::ostream& os, OddCliffordGroup const& A)
{
  for(auto i= 0; i < A.Gammas_.size(); ++i)
  {
    os << " Gamma " << i+1 << ":" << std::endl;
    os << A.Gammas_[i] << std::endl;
  }
  return os;
}


// TODO: Move to a better place
uint64_t factorial(uint64_t const n)
{
  if( n == 0 || n == 1 ) return 1;
  else return ( n * factorial(n-1) );
}


// TODO: Move to a better place
uint64_t binomial(uint64_t const a, uint64_t const b)
{
  return factorial(a) / ( factorial(b) * factorial(a-b) );
}


// FIXME: This is hard to understand legacy code.
// Can we make it more understandable?
std::vector<int> combination(int const upper, int const num_elems, int const num_comb)
{
  /**
   *  \brief Generate [num_comb]th combination of [num_elems]
   *         in the range of integers [ 0, 1, ..., upper - 1 ]
   *
   *  [num+comb] runs from 0 ... (upper choose num_elems) - 1
   */

  if( num_elems == 0 ) return std::vector<int>(0);

  std::vector<int> combinations(num_elems, 0);
  int k = 0;
  int r = 0;

  for(auto i = 0; i < num_elems - 1; ++i)
  {
    combinations[i] = (i != 0) ? combinations[i-1] : 0;

    do
    {
      ++combinations[i];
      r = binomial( upper - combinations[i], num_elems-(i+1) );
      k = k + r;
    } while(k <= num_comb);

    k = k - r;
  }

  combinations[num_elems-1] = combinations[num_elems-2] + num_comb + 1 - k;

  // FIXME: Brute-Force solution to decrease all values by one
  for(auto& c : combinations) --c;

  return combinations;
}


void count_Hs_and_Ls(std::vector<int> const& sequence, int const p, int const q, int& num_H, int& num_L)
{
  // Assume n >= 1
  auto n = sequence.size();

  if(n == 1)
  {
    if(sequence[0] < p) num_H += 1;
    else                num_L += 1;
  }
  else
  {
    auto exp = ( n - 1 ) * n / 2;
    for(int i = 0; i < n; ++i)
    {
      if(sequence[i] >= p) exp++;
    }

    if(exp % 2 == 0) num_H += 1;
    else             num_L += 1;
  }
}


GammaMatrix antisymmetrise(
    std::vector<GammaMatrix> const& gammas,
    std::vector<int> const& sequence
)
{
  // Number of indices we have to antisymmetrise over
  const auto num_indices = sequence.size();
  const auto d = gammas.size();
  const auto k = gammas.front().size();

  assert(num_indices <= d && "antisymmetrise: ERROR: Number of indices bigger than dimension!");
  for(auto i = 0; i < num_indices; ++i)
  {
    assert(sequence[i] <= d && "antisymmetrise: ERROR: Number of indices bigger than dimension!");
  }

  // If there aren't any indices set the matrix to unity
  if( num_indices == 0 )
  {
    return Unity(k);
  }

  // If there is one index return relevant gamma matrix
  if( num_indices == 1 )
  {
    auto index = sequence[0];
    return gammas[index];
  }

  // If there are two indices calculate and return commutator
  if( num_indices == 2 )
  {
    auto index1 = sequence[0];
    auto index2 = sequence[1];
    return commutator(gammas[index1], gammas[index2]);
  }

  // FIXME: THIS DOESN'T SEEM TO WORK FOR d >= 5
  // If there are more than two indices anti-symmetrise recursively
  GammaMatrix matrix(k);

  if( num_indices > 2 )
  {
    // Iterate over all elements in sequence:
    //
    // 1. sequence_new = [a1, a2, ..., (an), ..., a_num_indices]
    // 2. call antisymmetrise recursively and save in buffer1
    // 3. take the product between buffer1 and the remaining
    //    gamma matrix and save in buffer2
    // 4. matrix += (-1)^(pos-1) * buffer2
    // 5. after iteration matrix = matrix / num_indices

    for(auto i = 0; i < num_indices; ++i)
    {
      // 1. Copy new sequence without the ith element
      auto new_sequence = sequence;
      new_sequence.erase( new_sequence.begin() + i );

      // 2. Recursive step
      auto buffer1 = antisymmetrise(gammas, new_sequence);

      // 3. Take the product
      auto index = sequence[i];
      auto buffer2 = gammas[index] * buffer1;

      // 4. Add to the result matrix
      if(i % 2 == 0) matrix = matrix + buffer2;
      else           matrix = matrix - buffer2;
    }

    // 5. Divide by the number of indices
    matrix = matrix / ( 2 * num_indices );
  }

  return matrix;
}