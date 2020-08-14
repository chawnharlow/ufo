/*
 * (C) Crown copyright 2020, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ufo/profile/ProfileCheckSamePDiffT.h"
#include "ufo/profile/VariableNames.h"

namespace ufo {

  static ProfileCheckMaker<ProfileCheckSamePDiffT> makerProfileCheckSamePDiffT_("SamePDiffT");

  ProfileCheckSamePDiffT::ProfileCheckSamePDiffT(const ProfileConsistencyCheckParameters &options,
                                                 const ProfileIndices &profileIndices,
                                                 ProfileDataHandler &profileDataHandler,
                                                 ProfileCheckValidator &profileCheckValidator)
    : ProfileCheckBase(options, profileIndices, profileDataHandler, profileCheckValidator)
  {}

  void ProfileCheckSamePDiffT::runCheck()
  {
    oops::Log::debug() << " Test for same pressure and different temperature" << std::endl;
    int jlevprev = -1;
    int NumErrors = 0;

    const int numLevelsToCheck = profileIndices_.getNumLevelsToCheck();

    const std::vector <float> &pressures =
      profileDataHandler_.get<float>(ufo::VariableNames::name_air_pressure);
    const std::vector <float> &tObs =
      profileDataHandler_.get<float>(ufo::VariableNames::name_obs_air_temperature);
    const std::vector <float> &tBkg =
      profileDataHandler_.get<float>(ufo::VariableNames::name_hofx_air_temperature);
    std::vector <int> &tFlags =
      profileDataHandler_.get<int>(ufo::VariableNames::name_qc_tFlags);
    std::vector <int> &NumAnyErrors =
      profileDataHandler_.get<int>(ufo::VariableNames::name_counter_NumAnyErrors);
    std::vector <int> &NumSamePErrObs =
      profileDataHandler_.get<int>(ufo::VariableNames::name_counter_NumSamePErrObs);
    const std::vector <float> &tObsCorrection =
      profileDataHandler_.get<float>(ufo::VariableNames::name_tObsCorrection);

    if (oops::anyVectorEmpty(pressures, tObs, tBkg, tFlags, tObsCorrection)) {
      oops::Log::warning() << "At least one vector is empty. "
                           << "Check will not be performed." << std::endl;
      return;
    }
    if (!oops::allVectorsSameSize(pressures, tObs, tBkg, tFlags, tObsCorrection)) {
      oops::Log::warning() << "Not all vectors have the same size. "
                           << "Check will not be performed." << std::endl;
      return;
    }

    std::vector <float> tObsFinal;
    correctVector(tObs, tObsCorrection, tObsFinal);

    for (int jlev = 0; jlev < numLevelsToCheck; ++jlev) {
      if (tObs[jlev] == missingValueFloat) continue;

      if (jlevprev == -1) {
        jlevprev = jlev;
        continue;
      }

      if (pressures[jlev] == pressures[jlevprev]) {
        int jlevuse = jlevprev;
        if (std::abs(tObsFinal[jlev] - tObsFinal[jlevprev]) > options_.SPDTCheck_TThresh.value()) {
          NumErrors++;
          NumAnyErrors[0]++;

          // Choose which level to flag
          if (std::abs(tObsFinal[jlev] - tBkg[jlev]) <=
              std::abs(tObsFinal[jlevprev] - tBkg[jlevprev])) {
            tFlags[jlevprev] |= ufo::FlagsElem::FinalRejectFlag;
            tFlags[jlev]     |= ufo::FlagsProfile::InterpolationFlag;
            jlevuse = jlev;
          } else {
            tFlags[jlevprev] |= ufo::FlagsProfile::InterpolationFlag;
            tFlags[jlev]     |= ufo::FlagsElem::FinalRejectFlag;
          }

          oops::Log::debug() << " -> Failed same P/different T check for levels "
                             << jlevprev << " and " << jlev << std::endl;
          oops::Log::debug() << " -> Level " << jlevprev << ": "
                             << "P = " << pressures[jlevprev] * 0.01 << "hPa, tObs = "
                             << tObsFinal[jlevprev] - ufo::Constants::t0c << "C, "
                             << "tBkg = " << tBkg[jlevprev] - ufo::Constants::t0c
                             << "C" << std::endl;
          oops::Log::debug() << " -> Level " << jlev << ": "
                             << "P = " << pressures[jlev] * 0.01 << "hPa, tObs = "
                             << tObsFinal[jlev] - ufo::Constants::t0c << "C, "
                             << "tBkg = " << tBkg[jlev] - ufo::Constants::t0c
                             << "C" << std::endl;
          oops::Log::debug() << " -> tObs difference: " << tObsFinal[jlev] - tObsFinal[jlevprev]
                             << std::endl;
          oops::Log::debug() << " -> Use level " << jlevuse << std::endl;
        }
        jlevprev = jlevuse;
      } else {  // Distinct pressures
        jlevprev = jlev;
      }
    }
    if (NumErrors > 0) NumSamePErrObs[0]++;
  }
}  // namespace ufo
