/**
 * @author Andre Anjos <andre.anjos@idiap.ch>
 * @date Wed 13 Jul 2011 12:48:56 CEST
 *
 * @brief Implementation of the DataShuffler.
 */

#include <sys/time.h>
#include "core/array_assert.h"
#include "trainer/Exception.h"
#include "trainer/DataShuffler.h"

namespace array = Torch::core::array;
namespace train = Torch::trainer;

train::DataShuffler::DataShuffler(const std::vector<Torch::io::Arrayset>& data,
    const std::vector<blitz::Array<double,1> >& target):
  m_data(data),
  m_target(target.size()),
  m_range(),
  m_do_stdnorm(false),
  m_mean(),
  m_stddev()
{
  if (data.size() == 0) throw train::WrongNumberOfClasses(0);
  if (target.size() == 0) throw train::WrongNumberOfClasses(0);
  
  array::assertSameDimensionLength(data.size(), target.size());
  
  // checks shapes, minimum number of examples
  for (size_t k=0; k<data.size(); ++k) {
    if (data[k].size() == 0) throw WrongNumberOfFeatures(0, 1, k);
    //this may also trigger if I cannot get doubles from the Arrayset
    array::assertSameShape(data[0].get<double,1>(0), data[k].get<double,1>(0));
    array::assertSameShape(target[0], target[k]);
  }

  // set save values for the mean and stddev (even if not used at start)
  m_mean.resize(data[0].getShape()[0]);
  m_mean = 0.;
  m_stddev.resize(data[0].getShape()[0]);
  m_stddev = 1.;

  // copies the target data to my own variable
  for (size_t k=0; k<target.size(); ++k) 
    m_target[k].reference(Torch::core::array::ccopy(target[k]));

  // creates one range tailored for the range of each Arrayset
  for (size_t i=0; i<data.size(); ++i) {
    m_range.push_back(boost::uniform_int<size_t>(0, m_data[i].size()-1));
  }
}

train::DataShuffler::DataShuffler(const train::DataShuffler& other):
  m_data(other.m_data),
  m_target(other.m_target.size()),
  m_range(other.m_range),
  m_do_stdnorm(other.m_do_stdnorm),
  m_mean(Torch::core::array::ccopy(other.m_mean)),
  m_stddev(Torch::core::array::ccopy(other.m_stddev))
{
  for (size_t k=0; k<m_target.size(); ++k) 
    m_target[k].reference(Torch::core::array::ccopy(other.m_target[k]));
}

train::DataShuffler::~DataShuffler() { }

train::DataShuffler& train::DataShuffler::operator= 
(const train::DataShuffler::DataShuffler& other) {
  m_data = other.m_data;
  m_target.resize(other.m_target.size());
  for (size_t k=0; k<m_target.size(); ++k) 
    m_target[k].reference(Torch::core::array::ccopy(other.m_target[k]));

  m_range = other.m_range;
  
  m_mean.reference(Torch::core::array::ccopy(other.m_mean));
  m_stddev.reference(Torch::core::array::ccopy(other.m_stddev));

  return *this;
}

/**
 * Calculates mean and std.dev. in a single loop.
 * see: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
 */
void evaluateStdNormParameters(const std::vector<Torch::io::Arrayset>& data,
    blitz::Array<double,1>& mean, blitz::Array<double,1>& stddev) {
  
  mean = 0.;
  stddev = 0.; ///< temporarily used to accumulate square sum!
  double samples = 0;

  for (size_t k=0; k<data.size(); ++k) {
    for (size_t i=0; i<data[k].size(); ++i) {
      mean += data[k].get<double,1>(i);
      stddev += blitz::pow2(data[k].get<double,1>(i));
      ++samples;
    }
  }
  stddev -= blitz::pow2(mean) / samples;
  stddev /= (samples-1); ///< note: unbiased sample variance
  stddev = blitz::sqrt(stddev);

  mean /= (samples);
}

/**
 * Applies standard normalization parameters to all Arraysets given
 */
void applyStdNormParameters(std::vector<Torch::io::Arrayset>& data,
    const blitz::Array<double,1>& mean, const blitz::Array<double,1>& stddev) {
  for (size_t k=0; k<data.size(); ++k) {
    for (size_t i=0; i<data[k].size(); ++i) {
      blitz::Array<double,1> orig = data[k].get<double,1>(i);
      blitz::Array<double,1> buffer(orig.shape());
      buffer = (orig - mean) / stddev;
      data[k].set(i, buffer);
    }
  }
}

/**
 * Inverts the application of std normalization parameters
 */
void invertApplyStdNormParameters(std::vector<Torch::io::Arrayset>& data,
    const blitz::Array<double,1>& mean, const blitz::Array<double,1>& stddev) {
  for (size_t k=0; k<data.size(); ++k) {
    for (size_t i=0; i<data[k].size(); ++i) {
      blitz::Array<double,1> orig = data[k].get<double,1>(i);
      blitz::Array<double,1> buffer(orig.shape());
      buffer = (orig * stddev) + mean;
      data[k].set(i, buffer);
    }
  }
}

void train::DataShuffler::setAutoStdNorm(bool s) {
  if (s && !m_do_stdnorm) {
    evaluateStdNormParameters(m_data, m_mean, m_stddev);
    applyStdNormParameters(m_data, m_mean, m_stddev);
  }
  if (!s && m_do_stdnorm) {
    invertApplyStdNormParameters(m_data, m_mean, m_stddev);
    m_mean = 0.;
    m_stddev = 1.;
  }
  m_do_stdnorm = s;
}

void train::DataShuffler::getStdNorm(blitz::Array<double,1>& mean,
    blitz::Array<double,1>& stddev) const {
  if (m_do_stdnorm) {
    mean.reference(Torch::core::array::ccopy(m_mean));
    stddev.reference(Torch::core::array::ccopy(m_stddev));
  }
  else { //do it on-the-fly
    evaluateStdNormParameters(m_data, mean, stddev);
  }
}

void train::DataShuffler::operator() (boost::mt19937& rng, 
    blitz::Array<double,2>& data, blitz::Array<double,2>& target) {
  
  array::assertSameDimensionLength(data.extent(0), target.extent(0));

  size_t counter = 0;
  size_t max = data.extent(0);
  blitz::Range all = blitz::Range::all();
  while (true) {
    for (size_t i=0; i<m_data.size(); ++i) { //for all classes
      size_t index = m_range[i](rng); //pick a random position within class
      data(counter, all) = m_data[i].get<double,1>(index);
      target(counter, all) = m_target[i];
      ++counter;
      if (counter >= max) break;
    }
    if (counter >= max) break;
  }

}

void train::DataShuffler::operator() (blitz::Array<double,2>& data,
    blitz::Array<double,2>& target) {
  struct timeval tv;
  gettimeofday(&tv, 0);
  boost::mt19937 rng(tv.tv_sec + tv.tv_usec);
  operator()(rng, data, target); 
}
