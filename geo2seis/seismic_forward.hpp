#ifndef SEISMIC_FORWARD_HPP
#define SEISMIC_FORWARD_HPP

#include <stdio.h>
#include <string>
#include <vector>
#include "modelsettings.hpp"
#include <seismic_parameters.hpp>

#include <nrlib/stormgrid/stormcontgrid.hpp>
#include <nrlib/surface/regularsurface.hpp>


class SeismicForward {
  public:
    static void seismicForward(SeismicParameters &seismic_parameters);

  private:
    static void generateSeismicOLD(std::vector<NRLib::StormContGrid> &rgridvec,
                                NRLib::StormContGrid &twtgrid,
                                NRLib::StormContGrid &zgrid,
                                NRLib::StormContGrid &twt_timeshift,
                                std::vector<NRLib::StormContGrid> &timegridvec,
                                std::vector<NRLib::StormContGrid> &depthgridvec,
                                std::vector<NRLib::StormContGrid> &timeshiftgridvec,
                                Wavelet *wavelet,
                                double dt,
                                NRLib::RegularSurface<double> &bot,
                                NRLib::RegularSurface<double> &toptime,
                                double t0, double dz, double z0,
                                std::vector<double> &constvp,
                                double waveletScale,
                                bool time_output,
                                bool depth_output,
                                bool timeshift_output);
    
    static void generateSeismic(std::vector<NRLib::StormContGrid> &timegridvec,
                                std::vector<NRLib::StormContGrid> &rgridvec,
                                NRLib::StormContGrid              &twtgrid,
                                NRLib::StormContGrid              &zgrid,                                     
                                NRLib::RegularSurface<double>     &toptime,
                                Wavelet *wavelet,
                                double waveletScale,
                                double t0,
                                double dt);
    static void generateSeismicPos(std::vector<std::vector<double> > timegrid_pos,
                                   std::vector<std::vector<double> > refl_pos,
                                   std::vector<std::vector<double> > twtx_pos,
                                   NRLib::StormContGrid              &zgrid,
                                   NRLib::RegularSurface<double>     &toptime,
                                   Wavelet                           *wavelet,
                                   double                            waveletScale,
                                   std::vector<double>               offset,
                                   double                            t0,
                                   double                            dt,
                                   size_t                            i,
                                   size_t                            j);

    static void generateSeismicOnFile(std::vector<NRLib::StormContGrid> &rgridvec,
                                      NRLib::StormContGrid &twtgrid,
                                      NRLib::StormContGrid &zgrid,
                                      NRLib::StormContGrid &twt_timeshift,
                                      Wavelet *wavelet,
                                      double dt,
                                      int nt, int nz, int nx, int ny,
                                      NRLib::RegularSurface<double> &bot,
                                      NRLib::RegularSurface<double> &toptime,
                                      double t0, double dz, double z0,
                                      std::vector<double> &constvp,
                                      double waveletScale,
                                      bool time_output,
                                      bool depth_output,
                                      bool timeshift_output);

    static double findTFromZ(double z, std::vector<double> &zvec, std::vector<double> &tvec);
    //static void   findRegularVrms(SeismicParameters &seismic_parameters, NRLib::StormContGrid &vrms_reg, std::vector<double> t0);
    static void   regSamplInterpol1(NRLib::StormContGrid &t_in, NRLib::StormContGrid &data_in, std::vector<double> t_out, NRLib::StormContGrid &data_out);
    static void   nmoCorrInterpol1(std::vector<double> &t_in, NRLib::StormContGrid &data_in, NRLib::StormContGrid t_out, NRLib::StormContGrid &data_out);
    static void   nmoCorrInterpol1Pos(std::vector<double> &t_in, std::vector<std::vector<double> > data_in, std::vector<std::vector<double> > t_out, std::vector<std::vector<double> > data_out);
    //static void   convertSeis(NRLib::StormContGrid &t_in, NRLib::StormContGrid &data_in, NRLib::StormContGrid t_out, NRLib::StormContGrid &data_out);
    static std::vector<double> interpol1(std::vector<double> t_in, std::vector<double> data_in, std::vector<double> t_out);
    static size_t findNearestNeighbourIndex(double x, std::vector<double> x_in);
    static void   findReflections(SeismicParameters &seismic_parameters);
    static void   findReflectionsPos(SeismicParameters &seismic_parameters,  std::vector<std::vector<double> > r_vec, std::vector<std::vector<double> > theta_vec, std::vector<double> offset, size_t i, size_t j);
    static void   findTheta(SeismicParameters &seismic_parameters, double offset);
    static void   findThetaPos(std::vector<std::vector<double> > thetagrid, std::vector<double> twt_vec, std::vector<double> vrms_vec, std::vector<double> offset);
    static void   findRegTWTx(NRLib::StormContGrid &twtxgrid, NRLib::StormContGrid &vrmsgrid, std::vector<double> twt, double offset);
    static void   findRegTWTxPos(std::vector<std::vector<double> > twtx_pos_reg, std::vector<double> twt_vec, std::vector<double> vrms_vec, std::vector<double> offset);
    static void   findTWTxPos(std::vector<std::vector<double> > twtx_grid,  std::vector<double> twt_vec, std::vector<double> vrms_vec, std::vector<double> offset);
    static void   findTWTx(NRLib::StormContGrid &twtxgrid,  NRLib::StormContGrid &vrmsgrid, NRLib::StormContGrid &twtgrid, double offset);
    
};

#endif