// Copyright (C) 2011-2013 by the BEM++ Authors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef bempp_modified_aca_hpp
#define bempp_modified_aca_hpp

#include "../common/common.hpp"
#include "bempp/common/config_ahmed.hpp"

#ifdef WITH_AHMED

#ifdef __INTEL_COMPILER
#pragma warning(disable : 381)
#endif

#include <apprx.h>

#ifdef __INTEL_COMPILER
#pragma warning(default : 381)
#endif

#include <boost/scoped_array.hpp>
#include <limits>

namespace Bempp {

// returns true if a pivot could be found (there were any nonapproximated
// rows or columns), false otherwise
template <class abs_T>
bool select_pivot_with_min_norm2(unsigned n, const abs_T *norm2,
                                 const int *apprx_times /* S or Z */,
                                 unsigned &pivot) {
  pivot = n; // invalid value
  abs_T min_norm2 = std::numeric_limits<abs_T>::max();
  for (unsigned i = 0; i < n; ++i)
    if (apprx_times[i] >= 0 && norm2[i] < min_norm2) {
      min_norm2 = norm2[i];
      pivot = i;
    }
  return (pivot != n);
}

template <class T, class MATGEN_T>
bool ACAs(MATGEN_T &MatGen, unsigned b1, unsigned n1, unsigned b2, unsigned n2,
          double eps, unsigned kmax, unsigned i0, unsigned &k, T *&U, T *&V,
          const cluster *c1, const cluster *c2) {
  typedef typename num_traits<T>::abs_type abs_T;
  unsigned no = 0; // number of crosses calculated so far
  unsigned klast;  // unused
  const unsigned maxit = std::min(n1, n2);

  abs_T scale = MatGen.scale(b1, n1, b2, n2, c1, c2); // set initial scale

  U = new T[(kmax + 1) * n1]; // these arrays are expected to be
  V = new T[(kmax + 1) * n2]; // deallocated by the caller
  assert(U != NULL && V != NULL);

  // arrays storing number of successful approximations of rows and columns
  boost::scoped_array<int> Z(new int[n1]), S(new int[n2]);
  for (unsigned l = 0; l < n1; ++l)
    Z[l] = 0;
  for (unsigned l = 0; l < n2; ++l)
    S[l] = 0;

  boost::scoped_array<T> orig_row(new T[n2]);
  boost::scoped_array<T> orig_col(new T[n1]);

  boost::scoped_array<abs_T> orig_row_norm2(new abs_T[n2]);
  boost::scoped_array<abs_T> orig_col_norm2(new abs_T[n1]);

  abs_T nrms2 = 0.; // squared Frobenius norm of U V^H

  const unsigned ROW = 0, COL = 1;
  const unsigned NORMAL = 0, FIRST_SHOT = 1, SECOND_SHOT = 2;
  unsigned mode = ROW;
  unsigned stage = NORMAL;

  k = 0;
  unsigned next_pivot = i0;

  // The relativeScale() function is expected to return an estimate of the
  // magnitude of the largest element in the block, relative to the magnitude
  // of the largest element in the whole matrix.
  // If the result is deemed small enough relative to eps, the block is taken to
  // be zero and its elements are not evaluate.
  // This can be useful in the approximation of strongly decaying kernels.
  // abs_T relscale = MatGen.relativeScale(b1, n1, b2, n2, c1, c2);
  // if (relscale < 1e-2 * eps)
  //     return true;

  do {
    ACA_status status;
    abs_T nrmlsk2; // product of squared norms of new columns of U and V
    bool retry_if_zero = (stage == NORMAL); // don't retry if shooting
    // compute a cross
    if (mode == ROW)
      status = ACA_row_step(MatGen, b1, n1, b2, n2, klast, next_pivot, k, no,
                            Z.get(), S.get(), U, V, nrmlsk2, scale, c1, c2,
                            retry_if_zero, orig_row.get(), orig_col.get());
    else
      status = ACA_col_step(MatGen, b1, n1, b2, n2, next_pivot, k, no, Z.get(),
                            S.get(), U, V, nrmlsk2, scale, c1, c2,
                            retry_if_zero, orig_row.get(), orig_col.get());
    // std::cout << "status = " << status << std::endl;

    bool stpcrit = false;
    if (status == ACA_STATUS_SUCCESS) {
      // check stopping criterion
      T sum = 0.; // update nrms2
      for (unsigned l = 0; l < k; ++l)
        sum += blas::scpr(n1, U + l * n1, U + k * n1) *
               blas::scpr(n2, V + k * n2, V + l * n2);
      nrms2 += 2. * Re(sum) + nrmlsk2;

      stpcrit = (nrmlsk2 < eps * eps * nrms2);
      // adjust scale (estimated entry size of the next remainder)
      scale = sqrt(nrmlsk2 / (n1 * n2));
      // std::cout << "nrmlsk2: " << nrmlsk2 << ", nrms2: " << nrms2 << ", scale
      // = " << scale
      //           << std::endl;

      ++k;
    } else if (status == ACA_STATUS_EARLY_EXIT)
      stpcrit = true;
    else {
      // in the last step no non-zero row/column could be found
      assert(status == ACA_STATUS_REMAINDER_IS_ZERO);
      return true;
    }

    if (stpcrit) {
      if (stage == SECOND_SHOT)
        return true;
      else if (stage == FIRST_SHOT) {
        if (mode == ROW) {
          // select column pivot
          bool found = select_pivot_with_min_norm2(n2, orig_row_norm2.get(),
                                                   S.get(), next_pivot);
          if (!found) // no nonapproximated column could be found
            return true;
          mode = COL;
        } else {
          // select row pivot
          bool found = select_pivot_with_min_norm2(n1, orig_col_norm2.get(),
                                                   Z.get(), next_pivot);
          if (!found) // no nonapproximated row could be found
            return true;
          mode = ROW;
        }
        stage = SECOND_SHOT;
      } else {
        assert(stage == NORMAL);
        for (unsigned i = 0; i < n2; ++i)
          orig_row_norm2[i] = abs2(orig_row[i]);
        for (unsigned i = 0; i < n1; ++i)
          orig_col_norm2[i] = abs2(orig_col[i]);
        if (mode == ROW) {
          // select row pivot
          bool found = select_pivot_with_min_norm2(n1, orig_col_norm2.get(),
                                                   Z.get(), next_pivot);
          if (!found) // no nonapproximated row could be found
            return true;
        } else {
          // select column pivot
          bool found = select_pivot_with_min_norm2(n2, orig_row_norm2.get(),
                                                   S.get(), next_pivot);
          if (!found) // no nonapproximated column could be found
            return true;
        }
        stage = FIRST_SHOT;
        // mode stays the same
      }
    } else
      stage = NORMAL;
    // mode stays the same

    // std::cout << "Stage: " << stage << ", mode: " << mode << "\n";
  } while (no < maxit && k < kmax);

  // std::cout << "Giving up" << std::endl;
  return false;
}

template <class T, class T1, class T2, class MATGEN_T>
void apprx_unsym_shooting(MATGEN_T &MatGen, mblock<T> *&mbl,
                          bbxbemblcluster<T1, T2> *bl, double eps,
                          unsigned rankmax) {
  apprx_unsym_generic(&ACAs<T, MATGEN_T>, MatGen, mbl, bl, eps, rankmax);
}

} // namespace Bempp

#endif // WITH_AHMED

#endif
