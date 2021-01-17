// Derived from
// https://github.com/PlatformLab/memtier_skewsyn/blob/skewSynthetic/generator.h
#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits>
#include <random>
#include <string>

// Generator types are based on distribution type.
// Given a request rate, a generator will return the arrival interval in
// seconds.
// Basic class will always return 0
// Now we support Poisson and Uniform, will add more as needed.
class Generator {
public:
  Generator() {}
  virtual ~Generator() {}

  virtual double generate() = 0;
  // Return false if no change is made
  virtual bool setreqRate(double lambda) = 0;
  virtual double getreqRate() = 0;

  static std::random_device rd_;
};

// Fixed interval. Will always return a fixed value based on request rate.
class Fixed : public Generator {
public:
  Fixed(double reqRate = 1.0) { updateInterval(reqRate); }

  virtual double generate() override { return interval_; }

  virtual bool setreqRate(double reqRate) override { updateInterval(reqRate); }

  virtual double getreqRate() override {
    if (interval_ <= 0.0) {
      return -1.0;
    } else {
      return 1.0 / interval_;
    }
  }

private:
  inline void updateInterval(double reqRate) {
    // If reqRate <= 0, set interval to 0. Basically no delay to send the next
    // request.
    if (reqRate <= 0.0) {
      interval_ = 0.0;
    } else {
      interval_ = 1.0 / reqRate;
    }
    return;
  }

  double interval_;
};

// Poisson distribution, lambda is creations per second
class Poisson : public Generator {
public:
  Poisson(double reqRate = 1.0)
      : lambda_(reqRate), gen(rd_()), expIG(reqRate) {}

  virtual double generate() override {
    if (this->lambda_ <= 0.0) return 86400;  // 24 hours!
    return this->expIG(gen);
  }

  virtual bool setreqRate(double lambda) override {
    if (this->lambda_ == lambda_) return false;
    this->lambda_ = lambda;
    if (lambda > 0.0) {
      this->expIG.param(
          std::exponential_distribution<double>::param_type(lambda));
    }
    return true;
  }

  virtual double getreqRate() override { return this->lambda_; }

private:
  double lambda_;
  std::mt19937 gen;
  std::exponential_distribution<double> expIG;
};

// Uniform distribution, lambda is creations per second
// We want the average value to be consistent with the expectation
// So we need to use 2.0 instead of 1.0 when setting max value.
class Uniform : public Generator {
public:
  Uniform(double reqRate = 1.0)
      : lambda_(reqRate), gen(rd_()), uniformIG(0, 2.0 / reqRate) {}

  virtual double generate() override {
    if (this->lambda_ <= 0.0) return 86400;
    return this->uniformIG(gen);
  }

  virtual bool setreqRate(double lambda) override {
    if (this->lambda_ == lambda) return false;
    this->lambda_ = lambda;
    if (lambda > 0.0) {
      this->uniformIG.param(
          std::uniform_real_distribution<double>::param_type(0, 2.0 / lambda));
    }
    return true;
  }

  virtual double getreqRate() override { return this->lambda_; }

private:
  double lambda_;
  std::mt19937 gen;
  std::uniform_real_distribution<double> uniformIG;
};

// A uniformly-distributed int random generator
// Used to seed the mt19937 pseudo-random generator
std::random_device Generator::rd_;

#endif  // #ifndef RANDOM_GENERATOR_H
