#ifndef STAN_MATH_REV_FUN_SQUARED_DISTANCE_HPP
#define STAN_MATH_REV_FUN_SQUARED_DISTANCE_HPP

#include <stan/math/rev/core.hpp>
#include <stan/math/prim/fun/squared_distance.hpp>
#include <stan/math/rev/fun/sqrt.hpp>
#include <stan/math/rev/fun/typedefs.hpp>
#include <stan/math/prim/err/check_vector.hpp>
#include <stan/math/prim/err/check_matching_sizes.hpp>
#include <stan/math/prim/fun/Eigen.hpp>
#include <stan/math/prim/fun/typedefs.hpp>
#include <stan/math/prim/meta/index_type.hpp>

#include <vector>





namespace stan {
namespace math {

class scal_squared_distance_vv_vari : public op_vv_vari {
 public:
  scal_squared_distance_vv_vari(vari* avi, vari* bvi)
      : op_vv_vari(squared_distance(avi->val_, bvi->val_), avi, bvi) {}
  void chain() {
    double diff = avi_->val_ - bvi_->val_;
    avi_->adj_ += adj_ * 2.0 * diff;
    bvi_->adj_ -= adj_ * 2.0 * diff;
  }
};
class scal_squared_distance_vd_vari : public op_vd_vari {
 public:
  scal_squared_distance_vd_vari(vari* avi, double b)
      : op_vd_vari(squared_distance(avi->val_, b), avi, b) {}
  void chain() { avi_->adj_ += adj_ * 2 * (avi_->val_ - bd_); }
};
class scal_squared_distance_dv_vari : public op_dv_vari {
 public:
  scal_squared_distance_dv_vari(double a, vari* bvi)
      : op_dv_vari(squared_distance(a, bvi->val_), a, bvi) {}
  void chain() { bvi_->adj_ -= adj_ * 2 * (ad_ - bvi_->val_); }
};

/**
 * Returns the log sum of exponentials.
 */
inline var squared_distance(const var& a, const var& b) {
  return var(new scal_squared_distance_vv_vari(a.vi_, b.vi_));
}
/**
 * Returns the log sum of exponentials.
 */
inline var squared_distance(const var& a, double b) {
  return var(new scal_squared_distance_vd_vari(a.vi_, b));
}
/**
 * Returns the log sum of exponentials.
 */
inline var squared_distance(double a, const var& b) {
  return var(new scal_squared_distance_dv_vari(a, b.vi_));
}






















namespace internal {

class squared_distance_vv_vari : public vari {
 protected:
  vari** v1_;
  vari** v2_;
  size_t length_;

  template <int R1, int C1, int R2, int C2>
  inline static double var_squared_distance(
      const Eigen::Matrix<var, R1, C1>& v1,
      const Eigen::Matrix<var, R2, C2>& v2) {
    using Eigen::Matrix;
    typedef typename index_type<Matrix<var, R1, R2> >::type idx_t;
    double result = 0;
    for (idx_t i = 0; i < v1.size(); i++) {
      double diff = v1(i).vi_->val_ - v2(i).vi_->val_;
      result += diff * diff;
    }
    return result;
  }

 public:
  template <int R1, int C1, int R2, int C2>
  squared_distance_vv_vari(const Eigen::Matrix<var, R1, C1>& v1,
                           const Eigen::Matrix<var, R2, C2>& v2)
      : vari(var_squared_distance(v1, v2)), length_(v1.size()) {
    v1_ = reinterpret_cast<vari**>(
        ChainableStack::instance().memalloc_.alloc(length_ * sizeof(vari*)));
    for (size_t i = 0; i < length_; i++)
      v1_[i] = v1(i).vi_;

    v2_ = reinterpret_cast<vari**>(
        ChainableStack::instance().memalloc_.alloc(length_ * sizeof(vari*)));
    for (size_t i = 0; i < length_; i++)
      v2_[i] = v2(i).vi_;
  }
  virtual void chain() {
    for (size_t i = 0; i < length_; i++) {
      double di = 2 * adj_ * (v1_[i]->val_ - v2_[i]->val_);
      v1_[i]->adj_ += di;
      v2_[i]->adj_ -= di;
    }
  }
};
class squared_distance_vd_vari : public vari {
 protected:
  vari** v1_;
  double* v2_;
  size_t length_;

  template <int R1, int C1, int R2, int C2>
  inline static double var_squared_distance(
      const Eigen::Matrix<var, R1, C1>& v1,
      const Eigen::Matrix<double, R2, C2>& v2) {
    using Eigen::Matrix;
    typedef typename index_type<Matrix<double, R1, C1> >::type idx_t;

    double result = 0;
    for (idx_t i = 0; i < v1.size(); i++) {
      double diff = v1(i).vi_->val_ - v2(i);
      result += diff * diff;
    }
    return result;
  }

 public:
  template <int R1, int C1, int R2, int C2>
  squared_distance_vd_vari(const Eigen::Matrix<var, R1, C1>& v1,
                           const Eigen::Matrix<double, R2, C2>& v2)
      : vari(var_squared_distance(v1, v2)), length_(v1.size()) {
    v1_ = reinterpret_cast<vari**>(
        ChainableStack::instance().memalloc_.alloc(length_ * sizeof(vari*)));
    for (size_t i = 0; i < length_; i++)
      v1_[i] = v1(i).vi_;

    v2_ = reinterpret_cast<double*>(
        ChainableStack::instance().memalloc_.alloc(length_ * sizeof(double)));
    for (size_t i = 0; i < length_; i++)
      v2_[i] = v2(i);
  }
  virtual void chain() {
    for (size_t i = 0; i < length_; i++) {
      v1_[i]->adj_ += 2 * adj_ * (v1_[i]->val_ - v2_[i]);
    }
  }
};
}  // namespace internal

template <int R1, int C1, int R2, int C2>
inline var squared_distance(const Eigen::Matrix<var, R1, C1>& v1,
                            const Eigen::Matrix<var, R2, C2>& v2) {
  check_vector("squared_distance", "v1", v1);
  check_vector("squared_distance", "v2", v2);
  check_matching_sizes("squared_distance", "v1", v1, "v2", v2);
  return var(new internal::squared_distance_vv_vari(v1, v2));
}
template <int R1, int C1, int R2, int C2>
inline var squared_distance(const Eigen::Matrix<var, R1, C1>& v1,
                            const Eigen::Matrix<double, R2, C2>& v2) {
  check_vector("squared_distance", "v1", v1);
  check_vector("squared_distance", "v2", v2);
  check_matching_sizes("squared_distance", "v1", v1, "v2", v2);
  return var(new internal::squared_distance_vd_vari(v1, v2));
}
template <int R1, int C1, int R2, int C2>
inline var squared_distance(const Eigen::Matrix<double, R1, C1>& v1,
                            const Eigen::Matrix<var, R2, C2>& v2) {
  check_vector("squared_distance", "v1", v1);
  check_vector("squared_distance", "v2", v2);
  check_matching_sizes("squared_distance", "v1", v1, "v2", v2);
  return var(new internal::squared_distance_vd_vari(v2, v1));
}

}  // namespace math
}  // namespace stan
#endif