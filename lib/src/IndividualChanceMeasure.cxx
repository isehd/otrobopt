//                                               -*- C++ -*-
/**
 *  @brief IndividualChanceMeasure
 *
 *  Copyright 2005-2016 Airbus-EDF-IMACS-Phimeca
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include "otrobopt/IndividualChanceMeasure.hxx"
#include <openturns/PersistentObjectFactory.hxx>
#include <openturns/GaussKronrod.hxx>
#include <openturns/IteratedQuadrature.hxx>

using namespace OT;

namespace OTROBOPT
{

CLASSNAMEINIT(IndividualChanceMeasure);

static Factory<IndividualChanceMeasure> RegisteredFactory;


/* Default constructor */
IndividualChanceMeasure::IndividualChanceMeasure()
  : MeasureFunctionImplementation()
  , alpha_(0.0)
{
  // Nothing to do
}

/* Parameter constructor */
IndividualChanceMeasure::IndividualChanceMeasure (const Distribution & distribution,
                                        const NumericalMathFunction & function,
                                        const NumericalScalar alpha)
  : MeasureFunctionImplementation(distribution, function)
  , alpha_(0.0)
{
  setAlpha(alpha);
}

/* Virtual constructor method */
IndividualChanceMeasure * IndividualChanceMeasure::clone() const
{
  return new IndividualChanceMeasure(*this);
}


class IndividualChanceMeasureParametricFunctionWrapper : public NumericalMathFunctionImplementation
{
public:
  IndividualChanceMeasureParametricFunctionWrapper(const NumericalPoint & x,
                            const NumericalMathFunction & function)
  : NumericalMathFunctionImplementation()
  , x_(x)
  , function_(function)
  {}

  virtual IndividualChanceMeasureParametricFunctionWrapper * clone() const
  {
    return new IndividualChanceMeasureParametricFunctionWrapper(*this);
  }

  NumericalPoint operator()(const NumericalPoint & theta) const
  {
    NumericalMathFunction function(function_);
    return function(x_, theta);
  }

  NumericalSample operator()(const NumericalSample & theta) const
  {
    const UnsignedInteger size = theta.getSize();
    NumericalSample outS(size, function_.getOutputDimension());
    for (UnsignedInteger i = 0; i < size; ++ i)
    {
      outS[i] = operator()(theta[i]);
    }
    return outS;
  }

  UnsignedInteger getInputDimension() const
  {
    return function_.getInputDimension();
  }

  UnsignedInteger getOutputDimension() const
  {
    return function_.getOutputDimension();
  }

protected:
  NumericalPoint x_;
  NumericalMathFunction function_;
};


/* Evaluation */
NumericalPoint IndividualChanceMeasure::operator()(const NumericalPoint & inP) const
{
  NumericalMathFunction function(getFunction());
  NumericalPoint parameter(function.getParameter());
  const UnsignedInteger outputDimension = function.getOutputDimension();
  NumericalPoint outP(outputDimension);
  if (getDistribution().isContinuous())
  {
    GaussKronrod gkr;
    gkr.setRule(GaussKronrodRule::G1K3);
    IteratedQuadrature algo(gkr);
    Pointer<NumericalMathFunctionImplementation> p_wrapper(new IndividualChanceMeasureParametricFunctionWrapper(inP, function));
    NumericalMathFunction G(p_wrapper);
    outP = algo.integrate(G, getDistribution().getRange()) * 1.0 / getDistribution().getRange().getVolume();
  }
  else
  {
    NumericalSample support(getDistribution().getSupport());
    const UnsignedInteger size = support.getSize();
    for (UnsignedInteger i = 0; i < size; ++ i)
    {
      outP += 1.0 / size * function(inP, support[i]);
    }
  }
  NumericalScalar p = 1.0;
  for (UnsignedInteger j = 0; j < outputDimension; ++ j)
  {
    p *= outP[j] > alpha_ ? 1.0 : 0.0;
  }
  function.setParameter(parameter);
  return NumericalPoint(1, -p);
}

/* Alpha coefficient accessor */
void IndividualChanceMeasure::setAlpha(const NumericalScalar alpha)
{
  if (!(alpha >= 0.0) || !(alpha <= 1.0))
    throw InvalidArgumentException(HERE) << "Alpha should be in (0, 1)";
  alpha_ = alpha;
}

NumericalScalar IndividualChanceMeasure::getAlpha() const
{
  return alpha_;
}

UnsignedInteger IndividualChanceMeasure::getOutputDimension() const
{
  return 1;
}

/* String converter */
String IndividualChanceMeasure::__repr__() const
{
  OSS oss;
  oss << "class=" << IndividualChanceMeasure::GetClassName()
      << " alpha=" << alpha_;
  return oss;
}

/* Method save() stores the object through the StorageManager */
void IndividualChanceMeasure::save(Advocate & adv) const
{
  MeasureFunctionImplementation::save(adv);
  adv.saveAttribute("alpha_", alpha_);
}

/* Method load() reloads the object from the StorageManager */
void IndividualChanceMeasure::load(Advocate & adv)
{
  MeasureFunctionImplementation::load(adv);
  adv.loadAttribute("alpha_", alpha_);
}


} /* namespace OTROBOPT */