/**
* Copyright (c) 2019 CNRS
* Author: Joseph Mirabel
*
* This file is part of the qserl package.
* qserl is free software: you can redistribute it
* and/or modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation, either version
* 3 of the License, or (at your option) any later version.
*
* qserl is distributed in the hope that it will be
* useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Lesser Public License for more details.  You should have
* received a copy of the GNU Lesser General Public License along with
* qserl.  If not, see
* <http://www.gnu.org/licenses/>.
**/

#include <qserl/rod3d/ik.h>
#include <qserl/util/explog.h>

#include <iostream>
#include <chrono>

namespace qserl {
namespace rod3d {

  InverseKinematics::InverseKinematics (const RodConstShPtr& rod) :
    m_rod (rod),
    m_squareErrorThr (1e-6),
    m_maxIter (20),
    m_verbosity (INT_MAX),
    m_scale (1.)
  {}

  InverseKinematics::ResultT InverseKinematics::compute (const WorkspaceIntegratedStateShPtr& state,
      std::size_t iNode, Displacement oMi) const
  {
    assert (state->integrationOptions().keepJMatrices);

    Displacement iMo (inv(oMi)), iMt;
    Wrench w (state->wrench (0)), dw;
    typedef Eigen::Matrix<double,6,1> Vector6;
    Vector6 error;

    typedef Eigen::FullPivLU<Matrix6d> Decomposition;
    Decomposition decomposition (6,6);

    int iter = m_maxIter;
    while (true) {
      auto t0 = std::chrono::high_resolution_clock::now();
      iMt = iMo * state->nodes()[iNode];
      error = log6 (iMt);
      double errorNorm2 = error.squaredNorm();
      auto t1 = std::chrono::high_resolution_clock::now();
      if (iter % m_verbosity == 0)
        std::cout << iter << '\t' << errorNorm2 << '\t' << w.transpose() << std::endl;
      if (errorNorm2 < m_squareErrorThr) return IK_VALID;
      if (iter == 0) return IK_MAX_ITER_REACHED;

      auto t2 = std::chrono::high_resolution_clock::now();
      const Matrix6d& J (state->getJMatrix (iNode));
      decomposition.compute (J);
      if (!decomposition.isInvertible())
        return IK_JACOBIAN_SINGULAR;
      dw = decomposition.solve (error);

      w -= m_scale * dw;

      auto t3 = std::chrono::high_resolution_clock::now();

      m_lastResult = state->integrateFromBaseWrenchRK4 (w);
      auto t4 = std::chrono::high_resolution_clock::now();

      if (iter % m_verbosity == 0) {
        std::cout << (t1-t0).count()
          << '\t' << (t3-t2).count()
          << '\t' << (t4-t3).count()
          << "ns" << std::endl;
      }

      if (m_lastResult != WorkspaceIntegratedState::IR_VALID)
        return IK_INTEGRATION_FAILED;

      iter--;
    }
    return IK_MAX_ITER_REACHED;
  }
}  // namespace rod3d
}  // namespace qserl
