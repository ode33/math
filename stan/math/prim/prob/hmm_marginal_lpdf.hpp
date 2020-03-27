#ifndef STAN_MATH_REV_FUN_HMM_MARGINAL_LPDF_HPP
#define STAN_MATH_REV_FUN_HMM_MARGINAL_LPDF_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/fun/row.hpp>
#include <stan/math/prim/fun/col.hpp>
#include <stan/math/prim/fun/transpose.hpp>
#include <stan/math/prim/fun/exp.hpp>
#include <stan/math/prim/fun/value_of_rec.hpp>
#include <stan/math/prim/core.hpp>
#include <vector>
#include <iostream>

namespace stan {
namespace math {

namespace internal {
/**
 * For a Hidden Markov Model with observation y, hidden state x,
 * and parameters theta, return the log marginal density, log
 * p(y | theta). In this setting, the hidden states are discrete
 * and take values over the finite space {1, ..., K}.
 * The marginal lpdf is obtained via a forward pass.
 * The [in, out] argument are saved so that we can use them when
 * calculating the derivatives.
 *
 * @param[in] log_omega log matrix of observational densities.
 *              The (i, j)th entry corresponds to the
 *              density of the ith observation, y_i,
 *              given x_i = j.
 * @param[in] Gamma transition density between hidden states.
 *              The (i, j)th entry is the probability that x_n = j,
 *              given x_{n - 1} = i. The rows of Gamma are simplexes.
 * @param[in] rho initial state
 * @param[in, out] alphas unnormalized partial marginal density.
 *                   The jth column is the joint density over all
 *                   observations y and the hidden state j.
 * @param[in, out] alpha_log_norms max coefficient for column of alpha,
 *                   to be used to normalize alphas.
 * @param[in, out] omegas term-wise exponential of omegas.
 * @return log marginal density.
 */
inline
double hmm_marginal_lpdf(const Eigen::MatrixXd& log_omegas,
                         const Eigen::MatrixXd& Gamma,
                         const Eigen::VectorXd& rho, Eigen::MatrixXd& alphas,
                         Eigen::VectorXd& alpha_log_norms,
                         Eigen::MatrixXd& omegas) {
  omegas = log_omegas.array().exp();
  int n_states = log_omegas.rows();
  int n_transitions = log_omegas.cols() - 1;

  alphas.col(0) = omegas.col(0).cwiseProduct(rho);

  double norm = alphas.col(0).maxCoeff();
  alphas.col(0) /= norm;
  alpha_log_norms(0) = std::log(norm);

  for (int n = 0; n < n_transitions; ++n) {
    alphas.col(n + 1)
        = omegas.col(n + 1).cwiseProduct(Gamma.transpose() * alphas.col(n));

    double norm = alphas.col(n + 1).maxCoeff();
    alphas.col(n + 1) /= norm;
    alpha_log_norms(n + 1) = std::log(norm) + alpha_log_norms(n);
  }

  return log(alphas.col(n_transitions).sum()) + alpha_log_norms(n_transitions);
}

}  // namespace internal

/**
 * For a Hidden Markov Model with observation y, hidden state x,
 * and parameters theta, return the log marginal density, log
 * p(y | theta). In this setting, the hidden states are discrete
 * and take values over the finite space {1, ..., K}.
 * The marginal lpdf is obtained via a forward pass, and
 * the derivative is calculated with an adjoint method,
 * see (Betancourt, Margossian, & Leos-Barajas, 2020).
 *
 * @tparam T_omega type of the log likelihood matrix
 * @tparam T_Gamma type of the transition matrix
 * @tparam T_rho type of the initial guess vector
 *
 * @param[in] log_omega log matrix of observational densities.
 *              The (i, j)th entry corresponds to the
 *              density of the ith observation, y_i,
 *              given x_i = j.
 * @param[in] Gamma transition density between hidden states.
 *              The (i, j)th entry is the probability that x_n = j,
 *              given x_{n - 1} = i. The rows of Gamma are simplexes.
 * @param[in] rho initial state
 * @throw `std::domain_error` if Gamma is not square.
 * @throw `std::domain_error` if the rows of Gamma are
 * not a simplex.
 * @throw `std::invalid_argument` if the size of rho is not
 * the number of rows of Gamma.
 * @throw `std::domain_error` if rho is not a simplex.
 * @return log marginal density.
 */
template <typename T_omega, typename T_Gamma, typename T_rho>
inline return_type_t<T_omega, T_Gamma, T_rho> hmm_marginal_lpdf(
    const Eigen::Matrix<T_omega, Eigen::Dynamic, Eigen::Dynamic>& log_omegas,
    const Eigen::Matrix<T_Gamma, Eigen::Dynamic, Eigen::Dynamic>& Gamma,
    const Eigen::Matrix<T_rho, Eigen::Dynamic, 1>& rho) {
  int n_states = log_omegas.rows();
  int n_transitions = log_omegas.cols() - 1;

  check_square("hmm_marginal_lpdf", "Gamma", Gamma);
  check_consistent_size("hmm_marginal_lpdf", "Gamma", row(Gamma, 1), n_states);

  // Temporary vector to use check_simplex, which only works once
  // column vectors.
  Eigen::Matrix<T_Gamma, Eigen::Dynamic, Eigen::Dynamic> Gamma_transpose
      = Gamma.transpose();
  for (int i = 0; i < Gamma.cols(); ++i) {
    // CHECK -- does check_simplex not work on row-vectors?
    check_simplex("hmm_marginal_lpdf", "Gamma[i, ]",
                  col(Gamma_transpose, i + 1));
  }

  check_consistent_size("hmm_marginal_lpdf", "rho", rho, n_states);
  check_simplex("hmm_marginal_lpdf", "rho", rho);

  using T_partials_return = partials_return_t<T_omega, T_Gamma, T_rho>;
  operands_and_partials<Eigen::Matrix<T_omega, Eigen::Dynamic, Eigen::Dynamic>,
                        Eigen::Matrix<T_Gamma, Eigen::Dynamic, Eigen::Dynamic>,
                        Eigen::Matrix<T_rho, Eigen::Dynamic, 1> >
      ops_partials(log_omegas, Gamma, rho);

  Eigen::MatrixXd alphas(n_states, n_transitions + 1);
  Eigen::VectorXd alpha_log_norms(n_transitions + 1);
  Eigen::MatrixXd omegas;
  Eigen::MatrixXd Gamma_dbl = value_of_rec(Gamma);

  T_partials_return log_marginal_density = internal::hmm_marginal_lpdf(
      value_of_rec(log_omegas), Gamma_dbl, value_of_rec(rho), alphas,
      alpha_log_norms, omegas);

  // Variables required for all three Jacobian-adjoint products.
  double norm_norm = alpha_log_norms(n_transitions);
  double unnormed_marginal = alphas.col(n_transitions).sum();

  std::vector<Eigen::VectorXd> kappa(n_transitions);
  Eigen::VectorXd kappa_log_norms(n_transitions);
  std::vector<double> grad_corr(n_transitions);

  if (n_transitions > 0) {
    kappa[n_transitions - 1] = Eigen::VectorXd::Ones(n_states);
    kappa_log_norms(n_transitions - 1) = 0;
    grad_corr[n_transitions - 1]
        = std::exp(alpha_log_norms(n_transitions - 1) - norm_norm);
  }

  for (int n = n_transitions - 2; n >= 0; --n) {
    kappa[n] = Gamma_dbl * (omegas.col(n + 2).cwiseProduct(kappa[n + 1]));

    double norm = kappa[n].maxCoeff();
    kappa[n] /= norm;
    kappa_log_norms(n) = std::log(norm) + kappa_log_norms(n + 1);
    grad_corr[n]
        = std::exp(alpha_log_norms(n) + kappa_log_norms(n) - norm_norm);
  }

  if (!is_constant_all<T_Gamma>::value) {
    Eigen::MatrixXd Gamma_jacad = Eigen::MatrixXd::Zero(n_states, n_states);

    for (int n = n_transitions - 1; n >= 0; --n) {
      Gamma_jacad += (grad_corr[n] * kappa[n].cwiseProduct(omegas.col(n + 1))
                      * alphas.col(n).transpose())
                         .transpose();
    }

    Gamma_jacad /= unnormed_marginal;
    ops_partials.edge2_.partials_ = Gamma_jacad;
  }

  bool sensitivities_for_omega_or_rho
      = (!is_constant_all<T_omega>::value) || (!is_constant_all<T_rho>::value);

  // boundary terms
  if (sensitivities_for_omega_or_rho) {
    Eigen::MatrixXd log_omega_jacad
        = Eigen::MatrixXd::Zero(n_states, n_transitions + 1);

    if (!is_constant_all<T_omega>::value) {
      for (int n = n_transitions - 1; n >= 0; --n)
        log_omega_jacad.col(n + 1)
            = grad_corr[n]
              * kappa[n].cwiseProduct(Gamma_dbl.transpose() * alphas.col(n));
    }

    // Boundary terms
    if (n_transitions == 0) {
      if (!is_constant_all<T_omega>::value) {
        log_omega_jacad.col(0) = omegas.col(0).cwiseProduct(value_of_rec(rho))
                                 / exp(value_of_rec(log_marginal_density));
        ops_partials.edge1_.partials_ = log_omega_jacad;
      }

      if (!is_constant_all<T_rho>::value) {
        ops_partials.edge3_.partials_
            = omegas.col(0) / exp(value_of_rec(log_marginal_density));
      }
    } else {
      double grad_corr_boundary = std::exp(kappa_log_norms(0) - norm_norm);
      Eigen::VectorXd c = Gamma_dbl * omegas.col(1).cwiseProduct(kappa[0]);

      if (!is_constant_all<T_omega>::value) {
        log_omega_jacad.col(0)
            = grad_corr_boundary * c.cwiseProduct(value_of_rec(rho));
        ops_partials.edge1_.partials_
            = log_omega_jacad.cwiseProduct(omegas / unnormed_marginal);
      }

      if (!is_constant_all<T_rho>::value) {
        ops_partials.edge3_.partials_ = grad_corr_boundary
                                        * c.cwiseProduct(omegas.col(0))
                                        / unnormed_marginal;
      }
    }
  }

  return ops_partials.build(log_marginal_density);
}

}  // namespace math
}  // namespace stan
#endif
