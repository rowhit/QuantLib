
/*
 * Copyright (C) 2000-2001 QuantLib Group
 *
 * This file is part of QuantLib.
 * QuantLib is a C++ open source library for financial quantitative
 * analysts and developers --- http://quantlib.sourceforge.net/
 *
 * QuantLib is free software and you are allowed to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of it under the conditions stated
 * in the QuantLib License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You should have received a copy of the license along with this file;
 * if not, contact ferdinando@ametrano.net
 * The license is also available at http://quantlib.sourceforge.net/LICENSE.TXT
 *
 * The members of the QuantLib Group are listed in the Authors.txt file, also
 * available at http://quantlib.sourceforge.net/Authors.txt
*/

/*! \file dividendoption.cpp
    \brief base class for options with dividends

    $Id$
*/

// $Source$
// $Log$
// Revision 1.16  2001/05/25 09:29:40  nando
// smoothing #include xx.hpp and cutting old Log messages
//
// Revision 1.15  2001/05/24 15:40:10  nando
// smoothing #include xx.hpp and cutting old Log messages
//

#include "ql/Pricers/dividendoption.hpp"
#include "ql/Math/cubicspline.hpp"
#include "ql/Pricers/dividendeuropeanoption.hpp"
#include "ql/FiniteDifferences/valueatcenter.hpp"
#include <vector>

namespace QuantLib {

    namespace Pricers {

        using Math::CubicSpline;
        using FiniteDifferences::valueAtCenter;

        DividendOption::DividendOption(Type type, double underlying,
            double strike, Rate dividendYield, Rate riskFreeRate,
            Time residualTime, double volatility,
            const std::vector<double>& dividends,
            const std::vector<Time>& exdivdates, int timeSteps, int gridPoints)
        : dividends_(dividends),
          MultiPeriodOption(type, underlying - addElements(dividends),
          strike, dividendYield, riskFreeRate, residualTime, volatility,
          exdivdates, timeSteps, gridPoints) {

            QL_REQUIRE(dateNumber_ == dividends_.size(),
                       "the number of dividends(" +
                       IntegerFormatter::toString(dividends_.size()) +
                       ") is different from the number of dates(" +
                       IntegerFormatter::toString(dateNumber_) +
                       ")");

            QL_REQUIRE(underlying > addElements(dividends),
                       "Dividends(" +
                       DoubleFormatter::toString(underlying - underlying_) +
                       ") cannot exceed underlying(" +
                       DoubleFormatter::toString(underlying) +
                       ")");
        }

        void DividendOption::initializeControlVariate() const{
            analytic_ = Handle<BSMOption> (new
                            DividendEuropeanOption (type_,
                                underlying_ + addElements(dividends_),
                                strike_,
                                dividendYield_,
                                riskFreeRate_,
                                residualTime_,
                                volatility_,
                                dividends_,
                                dates_));
        }

        void DividendOption::executeIntermediateStep(int step) const{

            double newSMin = sMin_ + dividends_[step];
            double newSMax = sMax_ + dividends_[step];

            setGridLimits(center_ + dividends_[step], dates_[step]);
            if (sMin_ < newSMin){
                sMin_ = newSMin;
                sMax_ = center_/(sMin_/center_);
            }
            if (sMax_ > newSMax){
                sMax_ = newSMax;
                sMin_ = center_/(sMax_/center_);
            }
            Array oldGrid = grid_ + dividends_[step];

            initializeGrid();
            initializeInitialCondition();
            // This operation was faster than the obvious:
            //     movePricesBeforeExDiv(initialPrices_, grid_, oldGrid);

            movePricesBeforeExDiv(prices_,        grid_, oldGrid);
            movePricesBeforeExDiv(controlPrices_, grid_, oldGrid);
            initializeOperator();
            initializeModel();
            initializeStepCondition();

            stepCondition_ -> applyTo(prices_, dates_[step]);

           }

        void DividendOption::movePricesBeforeExDiv(
                Array& prices, const Array& newGrid,
                               const Array& oldGrid) const {

            int j, gridSize = oldGrid.size();

            std::vector<double> logOldGrid(0);
            std::vector<double> tmpPrices(0);

            for(j = 0; j<gridSize; j++){
                double p = prices[j];
                double g = oldGrid[j];
                if (g > 0){
                    logOldGrid.push_back(QL_LOG(g));
                    tmpPrices.push_back(p);
                }
            }

            CubicSpline<Array::iterator, Array::iterator> priceSpline(
                logOldGrid.begin(), logOldGrid.end(), tmpPrices.begin());

            for (j = 0; j < gridSize; j++)
                prices[j] = priceSpline(QL_LOG(newGrid[j]));

        }

    }

}
