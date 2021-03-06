// Copyright (C) 2011 by the BEM++ Authors
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

#include "../type_template.hpp"
#include "../check_arrays_are_close.hpp"

#include "laplace_3d_dirichlet_fixture.hpp"

#include "assembly/blocked_boundary_operator.hpp"
#include "assembly/blocked_operator_structure.hpp"
#include "linalg/default_direct_solver.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/type_traits/is_complex.hpp>

using namespace Bempp;

// Tests

BOOST_AUTO_TEST_SUITE(DefaultDirectSolver)

BOOST_AUTO_TEST_CASE_TEMPLATE(boundary_operator_agrees_with_trivial_1x1_blocked_boundary_operator,
                              ValueType, result_types)
{
    typedef ValueType RT;
    typedef typename ScalarTraits<ValueType>::RealType RealType;
    typedef RealType BFT;

    typedef Bempp::DefaultDirectSolver<BFT, RT> DirectSolver;
    const RealType solverTol = 1e-5;

    Laplace3dDirichletFixture<BFT, RT> fixture;

    arma::Col<RT> solutionVectorNonblocked;

    // Solve using nonblocked operator
    {
        DirectSolver solver(fixture.lhsOp);       
        Solution<BFT, RT> solution = solver.solve(fixture.rhs);
        solutionVectorNonblocked = solution.gridFunction().coefficients();
    }

    // Solve using trivial (1 x 1) blocked operator
    {
        BlockedOperatorStructure<BFT, RT> structure;
        structure.setBlock(0, 0, fixture.lhsOp);
        
        BlockedBoundaryOperator<BFT, RT> lhsBlockedOp(structure);
        std::vector<GridFunction<BFT, RT> > blockedRhs(1);
        blockedRhs[0] = fixture.rhs;

        DirectSolver solver(lhsBlockedOp);       
        BlockedSolution<BFT, RT> solution = solver.solve(blockedRhs);
        arma::Col<RT> solutionVectorBlocked = solution.gridFunction(0).coefficients();

        BOOST_CHECK(check_arrays_are_close<ValueType>(
                        solutionVectorNonblocked, solutionVectorBlocked, solverTol * 10));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(boundary_operator_agrees_with_diagonal_2x2_blocked_boundary_operator,
                              ValueType, result_types)
{
    typedef ValueType RT;
    typedef typename ScalarTraits<ValueType>::RealType RealType;
    typedef RealType BFT;

    typedef Bempp::DefaultDirectSolver<BFT, RT> DirectSolver;
    const RealType solverTol = 1e-5;

    Laplace3dDirichletFixture<BFT, RT> fixture;

    arma::Col<RT> solutionVectorNonblocked;

    // Solve using nonblocked operator
    {
        DirectSolver solver(fixture.lhsOp);       
        Solution<BFT, RT> solution = solver.solve(fixture.rhs);
        solutionVectorNonblocked = solution.gridFunction().coefficients();
    }

    // Solve using diagonal 2x2 ([A, 0; 0, A]) blocked operator
    {
        BlockedOperatorStructure<BFT, RT> structure;
        structure.setBlock(0, 0, fixture.lhsOp);
        structure.setBlock(1, 1, fixture.lhsOp);
        
        BlockedBoundaryOperator<BFT, RT> lhsBlockedOp(structure);
        std::vector<GridFunction<BFT, RT> > blockedRhs(2);
        blockedRhs[0] = fixture.rhs;
        blockedRhs[1] = 2. * fixture.rhs;

        DirectSolver solver(lhsBlockedOp);       
        BlockedSolution<BFT, RT> solution = solver.solve(blockedRhs);
        arma::Col<RT> solutionVectorBlock0 = solution.gridFunction(0).coefficients();
        arma::Col<RT> solutionVectorBlock1 = solution.gridFunction(1).coefficients() / 2.;

        BOOST_CHECK(check_arrays_are_close<ValueType>(
                        solutionVectorNonblocked, solutionVectorBlock0, solverTol * 10));
        BOOST_CHECK(check_arrays_are_close<ValueType>(
                        solutionVectorNonblocked, solutionVectorBlock1, solverTol * 10));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(boundary_operator_agrees_with_2x2_blocked_boundary_operator,
                              ValueType, result_types)
{
    typedef ValueType RT;
    typedef typename ScalarTraits<ValueType>::RealType RealType;
    typedef RealType BFT;

    typedef Bempp::DefaultDirectSolver<BFT, RT> DirectSolver;
    const RealType solverTol = 1e-5;

    Laplace3dDirichletFixture<BFT, RT> fixture;

    arma::Col<RT> solutionVectorNonblocked;

    // Solve using nonblocked operator
    {
        DirectSolver solver(fixture.lhsOp);       
        Solution<BFT, RT> solution = solver.solve(fixture.rhs);
        solutionVectorNonblocked = solution.gridFunction().coefficients();
    }

    // Solve using 2x2 ([A, 0 * A; 0, A]) blocked operator
    {
        BlockedOperatorStructure<BFT, RT> structure;
        structure.setBlock(0, 0, fixture.lhsOp);
        structure.setBlock(0, 1, 0. * fixture.lhsOp);
        structure.setBlock(1, 1, fixture.lhsOp);
        
        BlockedBoundaryOperator<BFT, RT> lhsBlockedOp(structure);
        std::vector<GridFunction<BFT, RT> > blockedRhs(2);
        blockedRhs[0] = fixture.rhs;
        blockedRhs[1] = 2. * fixture.rhs;

        DirectSolver solver(lhsBlockedOp);       
        BlockedSolution<BFT, RT> solution = solver.solve(blockedRhs);
        arma::Col<RT> solutionVectorBlock0 = solution.gridFunction(0).coefficients();
        arma::Col<RT> solutionVectorBlock1 = solution.gridFunction(1).coefficients() / 2.;

        BOOST_CHECK(check_arrays_are_close<ValueType>(
                        solutionVectorNonblocked, solutionVectorBlock0, solverTol * 10));
        BOOST_CHECK(check_arrays_are_close<ValueType>(
                        solutionVectorNonblocked, solutionVectorBlock1, solverTol * 10));
    }
}

BOOST_AUTO_TEST_SUITE_END()
