#include "qucs_typedefs.h"
#include "complex.h"
#include "real.h"
#include "constants.h"
#include "matrix.h"
#include "spfile.h"

#include "testDefine.h"
#include "gtest/gtest.h"

using namespace qucs;

TEST(spfile, passivityCheck_active_network)
{
  // An active (amplifying) network: S = [[0, 2], [0.5, 0]]
  // S·S† has eigenvalue > 1 → isPassive returns false

  matrix S(2);
  S.set(0, 0, 0.0);
  S.set(0, 1, 2.0);
  S.set(1, 0, 0.5);
  S.set(1, 1, 0.0);

  EXPECT_FALSE(spfile::isPassive(S));
}

TEST(spfile, passivityCheck_3port_active)
{
  // 3-port with one active port
  // S = [[0, 0.5, 1.2], [0.3, 0, 0.2], [0.1, 0.2, 0]]

  matrix S(3);
  S.set(0, 0, 0.0);
  S.set(0, 1, 0.5);
  S.set(0, 2, 1.2);
  S.set(1, 0, 0.3);
  S.set(1, 1, 0.0);
  S.set(1, 2, 0.2);
  S.set(2, 0, 0.1);
  S.set(2, 1, 0.2);
  S.set(2, 2, 0.0);

  EXPECT_FALSE(spfile::isPassive(S));
}

TEST(spfile, passivityCheck_lossless_boundary)
{
  // Lossless network at the boundary: S·S† = I exactly
  // Largest eigenvalue = 1.0 → isPassive returns true (within tolerance)

  matrix S(2);
  S.set(0, 0, 0.0);
  S.set(0, 1, 1.0);
  S.set(1, 0, 1.0);
  S.set(1, 1, 0.0);

  EXPECT_TRUE(spfile::isPassive(S));
}

TEST(spfile, passivityCheck_diagonal_passive_eigenvalue_active)
{
  // Edge case: all diagonals of S·S† are < 1, but largest eigenvalue > 1
  // S = [[0.6, 0.6], [0.6, 0.6]]
  // S·S† = [[0.72, 0.72], [0.72, 0.72]]
  // Diagonals: 0.72, 0.72 → would pass naive diagonal check
  // Eigenvalues: 1.44, 0.0 → actually ACTIVE (λ_max = 1.44 > 1)

  matrix S(2);
  S.set(0, 0, 0.6);
  S.set(0, 1, 0.6);
  S.set(1, 0, 0.6);
  S.set(1, 1, 0.6);

  matrix ssh = S * adjoint(S);

  // Diagonal check would pass (both diagonals < 1)
  EXPECT_LT(real(ssh.get(0, 0)), 1.0);
  EXPECT_LT(real(ssh.get(1, 1)), 1.0);

  // But eigenvalue check correctly detects active network
  EXPECT_FALSE(spfile::isPassive(S));
}

TEST(spfile, passivityCheck_passive_3port)
{
  // 3-port passive network: all eigenvalues of S·S† ≤ 1
  // S = [[0.1, 0.3, 0.2], [0.3, 0.1, 0.2], [0.2, 0.2, 0.1]]

  matrix S(3);
  S.set(0, 0, 0.1);
  S.set(0, 1, 0.3);
  S.set(0, 2, 0.2);
  S.set(1, 0, 0.3);
  S.set(1, 1, 0.1);
  S.set(1, 2, 0.2);
  S.set(2, 0, 0.2);
  S.set(2, 1, 0.2);
  S.set(2, 2, 0.1);

  EXPECT_TRUE(spfile::isPassive(S));
}

TEST(spfile, passivityCheck_4port_power_iteration)
{
  // 4-port network to exercise the power iteration path (n > 2)
  // Passive: S = small values everywhere
  matrix S_passive(4);
  S_passive.set(0, 0, 0.1);
  S_passive.set(0, 1, 0.2);
  S_passive.set(0, 2, 0.1);
  S_passive.set(0, 3, 0.1);
  S_passive.set(1, 0, 0.2);
  S_passive.set(1, 1, 0.1);
  S_passive.set(1, 2, 0.1);
  S_passive.set(1, 3, 0.1);
  S_passive.set(2, 0, 0.1);
  S_passive.set(2, 1, 0.1);
  S_passive.set(2, 2, 0.1);
  S_passive.set(2, 3, 0.2);
  S_passive.set(3, 0, 0.1);
  S_passive.set(3, 1, 0.1);
  S_passive.set(3, 2, 0.2);
  S_passive.set(3, 3, 0.1);

  EXPECT_TRUE(spfile::isPassive(S_passive));

  // Active: scale up to push eigenvalue > 1
  matrix S_active(4);
  S_active.set(0, 0, 0.3);
  S_active.set(0, 1, 0.6);
  S_active.set(0, 2, 0.3);
  S_active.set(0, 3, 0.3);
  S_active.set(1, 0, 0.6);
  S_active.set(1, 1, 0.3);
  S_active.set(1, 2, 0.3);
  S_active.set(1, 3, 0.3);
  S_active.set(2, 0, 0.3);
  S_active.set(2, 1, 0.3);
  S_active.set(2, 2, 0.3);
  S_active.set(2, 3, 0.6);
  S_active.set(3, 0, 0.3);
  S_active.set(3, 1, 0.3);
  S_active.set(3, 2, 0.6);
  S_active.set(3, 3, 0.3);

  EXPECT_FALSE(spfile::isPassive(S_active));
}
