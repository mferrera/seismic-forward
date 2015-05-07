#include <seismic_output.hpp>

#include <utils/storm_writer.hpp>
#include "utils/segy_writer.hpp"
#include "seismic_parameters.hpp"
#include "seismic_geometry.hpp"

SeismicOutput::SeismicOutput(ModelSettings *model_settings) {
  top_time_window_  = model_settings->GetTopTimeWindow();
  bot_time_window_  = model_settings->GetBotTimeWindow();
  time_window_      = model_settings->GetTimeWindowSpecified();
  depth_window_     = model_settings->GetDepthWindowSpecified();
  top_depth_window_ = model_settings->GetTopDepthWindow();
  bot_depth_window_ = model_settings->GetBotDepthWindow();

  prefix_ = "";
  if (model_settings->GetPrefix() != "") {
    prefix_ = model_settings->GetPrefix() + "_";
  }

  suffix_ = "";
  if (model_settings->GetSuffix() != "") {
    suffix_ = "_" + model_settings->GetSuffix();
  }

  extra_parameter_names_ = model_settings->GetExtraParameterNames();

  //-----------------Get segy indexes for print----------------------------
  inline_start_     = model_settings->GetSegyInlineStart();
  xline_start_      = model_settings->GetSegyXlineStart();
  inline_direction_ = model_settings->GetSegyInlineDirection();

  //-----------------UTM precision in segy header--------------------------
  scalco_ = model_settings->GetUtmPrecision();


  xline_x_axis_ = true;
  if (NRLib::Uppercase(inline_direction_) == "X") {
    xline_x_axis_ = false;
  } else if (NRLib::Uppercase(inline_direction_) == "Y") {
    xline_x_axis_ = true;
  }
  inline_step_ = model_settings->GetSegyInlineStep();
  xline_step_  = model_settings->GetSegyXlineStep();
}

void SeismicOutput::writeDepthSurfaces(const NRLib::RegularSurface<double> &top_eclipse, const NRLib::RegularSurface<double> &bottom_eclipse) {
  printf("Write depth surfaces on Storm format\n");
  std::string filename = prefix_ + "topeclipse" + suffix_ + ".storm";
  top_eclipse.WriteToFile(filename);
  filename = prefix_ + "boteclipse" + suffix_ + ".storm";
  bottom_eclipse.WriteToFile(filename);
}

void SeismicOutput::writeReflections(SeismicParameters &seismic_parameters, bool noise_added) {
  std::vector<NRLib::StormContGrid> &rgridvec = seismic_parameters.rGrids();
  double theta_0                              = seismic_parameters.theta0();
  double dtheta                               = seismic_parameters.dTheta();
  size_t ntheta                               = seismic_parameters.nTheta();

  std::string reflection_string = "reflections_";
  if (noise_added) {
    printf("Write reflections with noise on Storm format");
    reflection_string = reflection_string + "noise_";
  } else {
    printf("Write reflections on Storm format");
  }

  double theta = theta_0;
  for (size_t i = 0; i < ntheta; i++) {
    std::string filename = prefix_ + reflection_string + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".storm";
    rgridvec[i].WriteToFile(filename);
    theta += dtheta;
  }
}

void SeismicOutput::writeNMOReflections(SeismicParameters &seismic_parameters, double offset, bool noise_added) {
  std::vector<NRLib::StormContGrid> &rgridvec = seismic_parameters.rGrids();

  std::string reflection_string = "reflections_";
  if (noise_added) {
    printf("Write reflections with noise on Storm format");
    reflection_string = reflection_string + "noise_";
  } 
  else {
    printf("Write reflections on Storm format");
  }
  std::string filename = prefix_ + reflection_string + NRLib::ToString(offset) + suffix_ + ".storm";
  rgridvec[0].WriteToFile(filename);  
}

void SeismicOutput::writeElasticParametersTimeSegy(SeismicParameters &seismic_parameters) {
  size_t nx = seismic_parameters.seismicGeometry()->nx();
  size_t ny = seismic_parameters.seismicGeometry()->ny();
  size_t nt = seismic_parameters.seismicGeometry()->nt();
  double dt = seismic_parameters.seismicGeometry()->dt();

  NRLib::RegularSurface<double> &toptime = seismic_parameters.topTime();
  NRLib::SegyGeometry *segy_geometry     = seismic_parameters.segyGeometry();

  NRLib::Volume volume_time = seismic_parameters.seismicGeometry()->createTimeVolume();
  NRLib::StormContGrid vp_time_grid(volume_time, nx, ny, nt);
  NRLib::StormContGrid vs_time_grid(volume_time, nx, ny, nt);
  NRLib::StormContGrid rho_time_grid(volume_time, nx, ny, nt);

  NRLib::StormContGrid &vpgrid  = seismic_parameters.vpGrid();
  NRLib::StormContGrid &vsgrid  = seismic_parameters.vsGrid();
  NRLib::StormContGrid &rhogrid = seismic_parameters.rhoGrid();
  NRLib::StormContGrid &twtgrid = seismic_parameters.twtGrid();

  double t_min = toptime.Min();
  generateParameterGridForOutput(vpgrid, twtgrid, vp_time_grid, dt, t_min, toptime);
  generateParameterGridForOutput(vsgrid, twtgrid, vs_time_grid, dt, t_min, toptime);
  generateParameterGridForOutput(rhogrid, twtgrid, rho_time_grid, dt, t_min, toptime);

  printf("Write vp in time on Segy format\n");
  SEGY::writeSegy(vp_time_grid, prefix_ + "vp_time" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);

  printf("Write vs in time on Segy format\n");
  SEGY::writeSegy(vs_time_grid, prefix_ + "vs_time" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);

  printf("Write rho in time on Segy format\n");
  SEGY::writeSegy(rho_time_grid, prefix_ + "rho_time" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);

}

void SeismicOutput::writeExtraParametersTimeSegy(SeismicParameters &seismic_parameters) {
  printf("Write extra parameters in time on Segy format\n");
  size_t nx = seismic_parameters.seismicGeometry()->nx();
  size_t ny = seismic_parameters.seismicGeometry()->ny();
  size_t nt = seismic_parameters.seismicGeometry()->nt();

  double dt = seismic_parameters.seismicGeometry()->dt();

  NRLib::RegularSurface<double> &toptime = seismic_parameters.topTime();
  NRLib::SegyGeometry *segy_geometry     = seismic_parameters.segyGeometry();

  NRLib::Volume volume_time                               = seismic_parameters.seismicGeometry()->createTimeVolume();
  NRLib::StormContGrid &twtgrid                           = seismic_parameters.twtGrid();
  std::vector<NRLib::StormContGrid> &extra_parameter_grid = seismic_parameters.extraParametersGrids();

  double tmin = toptime.Min();
  std::vector<NRLib::StormContGrid> extra_parameter_time_grid;
  for (size_t i = 0; i < extra_parameter_names_.size(); ++i) {
    NRLib::StormContGrid extra_parameter_time_grid_temp(volume_time, nx, ny, nt);
    generateParameterGridForOutput((extra_parameter_grid)[i], twtgrid, extra_parameter_time_grid_temp, dt, tmin, toptime);
    extra_parameter_time_grid.push_back(extra_parameter_time_grid_temp);
  }
  for (size_t i = 0; i < extra_parameter_names_.size(); ++i) {
    SEGY::writeSegy(extra_parameter_time_grid[i], prefix_ + extra_parameter_names_[i] + "_time" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
  }
}

void SeismicOutput::writeTimeSurfaces(SeismicParameters &seismic_parameters) {
  NRLib::RegularSurface<double> &toptime = seismic_parameters.topTime();
  NRLib::RegularSurface<double> &bottime = seismic_parameters.bottomTime();

  printf("Write time surfaces on Storm format\n");
  bottime.WriteToFile(prefix_ + "bottime" + suffix_ + ".storm");
  toptime.WriteToFile(prefix_ + "toptime" + suffix_ + ".storm");
}

void SeismicOutput::writeElasticParametersDepthSegy(SeismicParameters &seismic_parameters) {
  size_t nx = seismic_parameters.seismicGeometry()->nx();
  size_t ny = seismic_parameters.seismicGeometry()->ny();
  size_t nz = seismic_parameters.seismicGeometry()->nz();
  double dz = seismic_parameters.seismicGeometry()->dz();
  double z0 = seismic_parameters.seismicGeometry()->z0();

  NRLib::RegularSurface<double> &toptime = seismic_parameters.topTime();
  NRLib::SegyGeometry *segy_geometry     = seismic_parameters.segyGeometry();

  NRLib::Volume volume = seismic_parameters.seismicGeometry()->createDepthVolume();

  NRLib::StormContGrid &vpgrid  = seismic_parameters.vpGrid();
  NRLib::StormContGrid &vsgrid  = seismic_parameters.vsGrid();
  NRLib::StormContGrid &rhogrid = seismic_parameters.rhoGrid();
  NRLib::StormContGrid &zgrid   = seismic_parameters.zGrid();

  NRLib::StormContGrid vp_depth_grid(volume, nx, ny, nz);
  NRLib::StormContGrid vs_depth_grid(volume, nx, ny, nz);
  NRLib::StormContGrid rho_depth_grid(volume, nx, ny, nz);

  generateParameterGridForOutput(vpgrid, zgrid, vp_depth_grid, dz, z0, toptime);
  generateParameterGridForOutput(vsgrid, zgrid, vs_depth_grid, dz, z0, toptime);
  generateParameterGridForOutput(rhogrid, zgrid, rho_depth_grid, dz, z0, toptime);
  printf("Write vp in depth on Segy format\n");
  SEGY::writeSegy(vp_depth_grid, prefix_ + "vp_depth" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);

  printf("Write vs in depth on Segy format\n");
  SEGY::writeSegy(vs_depth_grid, prefix_ + "vs_depth" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);

  printf("Write rho in depth on Segy format\n");
  SEGY::writeSegy(rho_depth_grid, prefix_ + "rho_depth" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);
}

void SeismicOutput::writeExtraParametersDepthSegy(SeismicParameters &seismic_parameters) {
  printf("Write extra parameters in depth on Segy format\n");
  size_t nx                              = seismic_parameters.seismicGeometry()->nx();
  size_t ny                              = seismic_parameters.seismicGeometry()->ny();
  size_t nz                              = seismic_parameters.seismicGeometry()->nz();
  double dz                              = seismic_parameters.seismicGeometry()->dz();
  NRLib::RegularSurface<double> &toptime = seismic_parameters.topTime();

  NRLib::Volume        volume        = seismic_parameters.seismicGeometry()->createDepthVolume();
  NRLib::StormContGrid &zgrid        = seismic_parameters.zGrid();
  NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();

  std::vector<NRLib::StormContGrid> &extra_parameter_grid = seismic_parameters.extraParametersGrids();
  std::vector<NRLib::StormContGrid> extra_parameter_depth_grid;

  for (size_t i = 0; i < extra_parameter_names_.size(); ++i) {
    NRLib::StormContGrid extra_parameter_depth_grid_temp(volume, nx, ny, nz);
    generateParameterGridForOutput((extra_parameter_grid)[i], zgrid, extra_parameter_depth_grid_temp, dz, toptime.Min(), toptime);
    extra_parameter_depth_grid.push_back(extra_parameter_depth_grid_temp);
  }
  for (size_t i = 0; i < extra_parameter_names_.size(); ++i) {
    SEGY::writeSegy(extra_parameter_depth_grid[i], prefix_ + extra_parameter_names_[i] + "_depth" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);
  }
}

void SeismicOutput::writeVpVsRho(SeismicParameters &seismic_parameters) {
  NRLib::StormContGrid &vpgrid  = seismic_parameters.vpGrid();
  NRLib::StormContGrid &vsgrid  = seismic_parameters.vsGrid();
  NRLib::StormContGrid &rhogrid = seismic_parameters.rhoGrid();

  printf("Write elastic parameters on Storm format\n");
  std::string filename = prefix_ + "vp" + suffix_ + ".storm";
  vpgrid.WriteToFile(filename);
  filename = prefix_ + "vs" + suffix_ + ".storm";
  vsgrid.WriteToFile(filename);
  filename = prefix_ + "rho" + suffix_ + ".storm";
  rhogrid.WriteToFile(filename);
}

void SeismicOutput::writeZValues(SeismicParameters &seismic_parameters) {
  NRLib::StormContGrid &zgrid = seismic_parameters.zGrid();
  std::string filename = prefix_ + "zgrid" + suffix_ + ".storm";

  printf("Write zvalues on Storm format\n");
  zgrid.WriteToFile(filename);
}

void SeismicOutput::writeTwt(SeismicParameters &seismic_parameters) {
  NRLib::StormContGrid &twtgrid = seismic_parameters.twtGrid();
  std::string filename = prefix_ + "twt" + suffix_ + ".storm";

  printf("Write two way time on Storm format\n");
  twtgrid.WriteToFile(filename);
}


void SeismicOutput::generateParameterGridForOutput(NRLib::StormContGrid &input_grid, NRLib::StormContGrid &time_or_depth_grid, NRLib::StormContGrid &output_grid, double delta_time_or_depth, double zero_time_or_depth, NRLib::RegularSurface<double> &toptime) {
  for (size_t i = 0; i < output_grid.GetNI(); i++) {
    for (size_t j = 0; j < output_grid.GetNJ(); j++) {
      double x, y, z;
      input_grid.FindCenterOfCell(i, j, 0, x, y, z);

      double topt = toptime.GetZ(x, y);
      if (!toptime.IsMissing(topt)) { //check whether there are values in input_grid in this pillar - if not, cells in output_grid will be zero
        double location = zero_time_or_depth + 0.5 * delta_time_or_depth;
        for (size_t k = 0; k < output_grid.GetNK(); k++) {
          //find cell index in time or depth grid
          size_t location_index = findCellIndex(i, j, location, time_or_depth_grid);
          if (location_index == 999999) {          //if location is above all values in pillar of time_or_depth_grid,
            location_index = input_grid.GetNK() - 1;    //output_grid is given the value of the bottom cell of input_grid
          }
          output_grid(i, j, k) = input_grid(i, j, location_index);
          location += delta_time_or_depth;
        }
      } else {
        for (size_t k = 0; k < output_grid.GetNK(); k++) {
          output_grid(i, j, k) = 0.0;
        }
      }
    }
  }
}

size_t SeismicOutput::findCellIndex(size_t i, size_t j, double target_k, NRLib::StormContGrid &grid) {
  size_t found_k = 999999;
  size_t nz = grid.GetNK();
  for (size_t k = 0; k < nz; k++) {
    if (grid(i, j, k) > target_k) {
      found_k = k;
      break;
    }
  }
  return found_k;
}

void SeismicOutput::writeNMOSeismicTimeSegy(SeismicParameters &seismic_parameters, NRLib::StormContGrid &timegrid, double offset) {
  printf("Write NMO corrected seismic in time on Segy format\n");
  NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();
  SEGY::writeSegy(timegrid, prefix_ + "NMO_seismic_time_" + NRLib::ToString(offset) + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);  
}

void SeismicOutput::writeNMOSeismicTimeStorm(SeismicParameters &seismic_parameters, NRLib::StormContGrid &timegrid, double offset, bool is_stack) {
  printf("Write seismic in time on Storm format\n");
  ModelSettings *model_settings = seismic_parameters.modelSettings();
  std::string filename = prefix_ + "NMO_seismic_time_" + NRLib::ToString(offset) + suffix_ + ".storm";
  if (is_stack == false && (model_settings->GetOutputSeismicStackTimeStorm() || model_settings->GetOutputSeismicStackTimeSegy())) {
    STORM::writeStorm(timegrid, filename, top_time_window_, bot_time_window_, time_window_, true);
  } else {
    STORM::writeStorm(timegrid, filename, top_time_window_, bot_time_window_, time_window_);
  }  
}


void SeismicOutput::writeSeismicTimeSegy(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &timegridvec) {
  printf("Write seismic in time on Segy format\n");
  NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();
  double theta                       = seismic_parameters.theta0();
  double ntheta                      = seismic_parameters.nTheta();
  double dtheta                      = seismic_parameters.dTheta();
  for (size_t i = 0; i < ntheta; i++) {
    SEGY::writeSegy(timegridvec[i], prefix_ + "seismic_time_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
    theta = theta + dtheta;
  }
}

void SeismicOutput::writeSeismicTimeStorm(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &timegridvec) {
  printf("Write seismic in time on Storm format\n");
  ModelSettings *model_settings = seismic_parameters.modelSettings();
  double theta                  = seismic_parameters.theta0();
  double ntheta                 = seismic_parameters.nTheta();
  double dtheta                 = seismic_parameters.dTheta();
  for (size_t i = 0; i < ntheta; i++) {
    std::string filename = prefix_ + "seismic_time_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".storm";
    if (model_settings->GetOutputSeismicStackTimeStorm() || model_settings->GetOutputSeismicStackTimeSegy()) {
      STORM::writeStorm(timegridvec[i], filename, top_time_window_, bot_time_window_, time_window_, true);
    } else {
      STORM::writeStorm(timegridvec[i], filename, top_time_window_, bot_time_window_, time_window_);
    }
    theta = theta + dtheta;
  }
}

void SeismicOutput::writeSeismicTimeshiftSegy(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &timeshiftgridvec) {
  printf("Write seismic shifted in time on Segy format\n");
  double theta                       = seismic_parameters.theta0();
  double ntheta                      = seismic_parameters.nTheta();
  double dtheta                      = seismic_parameters.dTheta();
  NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();
  for (size_t i = 0; i < ntheta; i++) {
    SEGY::writeSegy(timeshiftgridvec[i], prefix_ + "seismic_timeshift_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
    theta = theta + dtheta;
  }
}

void SeismicOutput::writeSeismicTimeshiftStorm(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &timeshiftgridvec) {
  printf("Write seismic shifted in time on Storm format\n");
  ModelSettings *model_settings = seismic_parameters.modelSettings();
  double theta                  = seismic_parameters.theta0();
  double ntheta                 = seismic_parameters.nTheta();
  double dtheta                 = seismic_parameters.dTheta();
  for (size_t i = 0; i < ntheta; i++) {
    std::string filename = prefix_ + "seismic_timeshift_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".storm";
    if (model_settings->GetOutputSeismicStackTimeShiftStorm() || model_settings->GetOutputSeismicStackTimeShiftSegy()) {
      STORM::writeStorm(timeshiftgridvec[i], filename, top_time_window_, bot_time_window_, time_window_, true);
    } else {
      STORM::writeStorm(timeshiftgridvec[i], filename, top_time_window_, bot_time_window_, time_window_);
    }
    theta = theta + dtheta;
  }
}

void SeismicOutput::writeSeismicDepthSegy(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &depthgridvec) {
  printf("Write seismic in depth on Segy format\n");
  double theta                       = seismic_parameters.theta0();
  double ntheta                      = seismic_parameters.nTheta();
  double dtheta                      = seismic_parameters.dTheta();
  NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();
  for (size_t i = 0; i < ntheta; i++) {
    SEGY::writeSegy(depthgridvec[i], prefix_ + "seismic_depth_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);
    theta = theta + dtheta;
  }
}

void SeismicOutput::writeSeismicDepthStorm(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &depthgridvec) {
  printf("Write seismic in depth on Storm format\n");
  ModelSettings *model_settings = seismic_parameters.modelSettings();
  double theta  = seismic_parameters.theta0();
  double ntheta = seismic_parameters.nTheta();
  double dtheta = seismic_parameters.dTheta();
  for (size_t i = 0; i < ntheta; i++) {
    std::string filename = prefix_ + "seismic_depth_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".storm";
    if (model_settings->GetOutputSeismicStackDepthStorm() || model_settings->GetOutputSeismicStackDepthSegy()) {
      STORM::writeStorm(depthgridvec[i], filename, top_depth_window_, bot_depth_window_, depth_window_, true);
    } else {
      STORM::writeStorm(depthgridvec[i], filename, top_depth_window_, bot_depth_window_, depth_window_);
    }
    theta = theta + dtheta;
  }
}

void SeismicOutput::writeSeismicStackTime(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &timegridvec) {
  ModelSettings *model_settings = seismic_parameters.modelSettings();
  size_t nx                     = seismic_parameters.seismicGeometry()->nx();
  size_t ny                     = seismic_parameters.seismicGeometry()->ny();
  size_t nt                     = seismic_parameters.seismicGeometry()->nt();
  NRLib::Volume volume_t        = seismic_parameters.seismicGeometry()->createTimeVolume();
  size_t ntheta                 = seismic_parameters.nTheta();

  NRLib::StormContGrid stack_timegridvec(volume_t, nx, ny, nt);
  float ntheta_inv = static_cast<float>(1.0 / ntheta);
  for (size_t angle = 0; angle < ntheta; ++angle) {
    for (size_t i = 0; i < nx; ++i) {
      for (size_t j = 0; j < ny; ++j) {
        for (size_t k = 0; k < nt; ++k) {
          stack_timegridvec(i, j, k) += ntheta_inv * timegridvec[angle](i, j, k);
        }
      }
    }
    timegridvec[angle] = NRLib::StormContGrid(0, 0, 0);
  }

  //print out seismic stack in time, in storm and segy
  if (model_settings->GetOutputSeismicStackTimeSegy()) {
    printf("Write seismic stack in time on Segy format\n");
    NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();
    std::string filename = prefix_ + "seismic_time_stack" + suffix_ + ".segy";
    SEGY::writeSegy(stack_timegridvec, filename, inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
  }
  if (model_settings->GetOutputSeismicStackTimeStorm()) {
    printf("Write seismic stack in time on Storm format\n");
    std::string filename = prefix_ + "seismic_time_stack" + suffix_ + ".storm";
    STORM::writeStorm(stack_timegridvec, filename, top_time_window_, bot_time_window_, time_window_);
  }
}

void SeismicOutput::writeSeismicStackTimeshift(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &timeshiftgridvec) {
  ModelSettings *model_settings = seismic_parameters.modelSettings();
  size_t nx                     = seismic_parameters.seismicGeometry()->nx();
  size_t ny                     = seismic_parameters.seismicGeometry()->ny();
  size_t nt                     = seismic_parameters.seismicGeometry()->nt();
  NRLib::Volume volume_t        = seismic_parameters.seismicGeometry()->createTimeVolume();
  size_t ntheta                 = seismic_parameters.nTheta();

  NRLib::StormContGrid stack_timeshiftgridvec(volume_t, nx, ny, nt);
  float ntheta_inv = static_cast<float>(1.0 / ntheta);
  for (size_t angle = 0; angle < ntheta; ++angle) {
    for (size_t i = 0; i < nx; ++i) {
      for (size_t j = 0; j < ny; ++j) {
        for (size_t k = 0; k < nt; ++k) {
          stack_timeshiftgridvec(i, j, k) += ntheta_inv * timeshiftgridvec[angle](i, j, k);
        }
      }
    }
    timeshiftgridvec[angle] = NRLib::StormContGrid(0, 0, 0);
  }

  //print out seismic stack shifted in time, in storm and segy
  if (model_settings->GetOutputSeismicStackTimeShiftSegy()) {
    printf("Write seismic stack shifted in time on Segy format\n");
    NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();
    std::string filename = prefix_ + "seismic_timeshift_stack" + suffix_ + ".segy";
    SEGY::writeSegy(stack_timeshiftgridvec, filename, inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
  }
  if (model_settings->GetOutputSeismicStackTimeShiftStorm()) {
    printf("Write seismic stack shifted in time on Storm format\n");
    std::string filename = prefix_ + "seismic_timeshift_stack" + suffix_ + ".storm";
    STORM::writeStorm(stack_timeshiftgridvec, filename, top_time_window_, bot_time_window_, time_window_);
  }
}

void SeismicOutput::writeSeismicStackDepth(SeismicParameters &seismic_parameters, std::vector<NRLib::StormContGrid> &depthgridvec) {
  ModelSettings *model_settings = seismic_parameters.modelSettings();
  size_t nx                     = seismic_parameters.seismicGeometry()->nx();
  size_t ny                     = seismic_parameters.seismicGeometry()->ny();
  size_t nz                     = seismic_parameters.seismicGeometry()->nz();
  NRLib::Volume volume          = seismic_parameters.seismicGeometry()->createDepthVolume();
  size_t ntheta                 = seismic_parameters.nTheta();

  NRLib::StormContGrid stack_depthgrid(volume, nx, ny, nz);
  float ntheta_inv = static_cast<float>(1.0 / ntheta);
  for (size_t angle = 0; angle < ntheta; ++angle) {
    for (size_t i = 0; i < nx; ++i) {
      for (size_t j = 0; j < ny; ++j) {
        for (size_t k = 0; k < nz; ++k) {
          stack_depthgrid(i, j, k) += ntheta_inv * depthgridvec[angle](i, j, k);
        }
      }
    }
    depthgridvec[angle] = NRLib::StormContGrid(0, 0, 0);
  }

  //print out seismic stack in depth, in storm and segy
  if (model_settings->GetOutputSeismicStackDepthSegy()) {
    printf("Write seismic stack in depth on Segy format\n");
    NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();
    std::string filename = prefix_ + "seismic_depth_stack" + suffix_ + ".segy";
    SEGY::writeSegy(stack_depthgrid, filename, inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);
  }
  if (model_settings->GetOutputSeismicStackDepthStorm()) {
    printf("Write seismic stack in depth on Storm format\n");
    std::string filename = prefix_ + "seismic_depth_stack" + suffix_ + ".storm";
    STORM::writeStorm(stack_depthgrid, filename, top_depth_window_, bot_depth_window_, depth_window_);
  }
}


void SeismicOutput::writeSeismicTimeSeismicOnFile(SeismicParameters &seismic_parameters, bool time_output) {
  // seismic time (storm and segy) and seismic stack - time (storm and segy)
  NRLib::StormContGrid *timegrid       = NULL;
  NRLib::StormContGrid *stack_timegrid = NULL;

  ModelSettings *model_settings      = seismic_parameters.modelSettings();
  NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();

  double theta_0    = seismic_parameters.theta0();
  double ntheta     = seismic_parameters.nTheta();
  double dtheta     = seismic_parameters.dTheta();
  float  ntheta_inv = static_cast<float>(1.0 / ntheta);

  size_t nx              = seismic_parameters.seismicGeometry()->nx();
  size_t ny              = seismic_parameters.seismicGeometry()->ny();
  size_t nt              = seismic_parameters.seismicGeometry()->nt();
  NRLib::Volume volume_t = seismic_parameters.seismicGeometry()->createTimeVolume();

  if (model_settings->GetOutputSeismicStackTimeStorm() || model_settings->GetOutputSeismicStackTimeSegy()) {
    stack_timegrid = new NRLib::StormContGrid(volume_t, nx, ny, nt);
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = 0; j < ny; j++) {
        for (size_t k = 0; k < nt; k++) {
          (*stack_timegrid)(i, j, k) = 0.0;
        }
      }
    }
  }
  if (time_output == true) {
    timegrid = new NRLib::StormContGrid(volume_t, nx, ny, nt);
    for (size_t l = 0; l < ntheta; l++) {
      std::ifstream file;
      std::string filename = "time_" + NRLib::ToString(l);
      NRLib::OpenRead(file, filename, std::ios::in | std::ios::binary);
      for (size_t i = 0; i < nx; i++) {
        for (size_t j = 0; j < ny; j++) {
          for (size_t k = 0; k < nt; k++) {
            float value = NRLib::ReadBinaryFloat(file);
            (*timegrid)(i, j, k) = value;
            if (model_settings->GetOutputSeismicStackTimeStorm() || model_settings->GetOutputSeismicStackTimeSegy()) {
              (*stack_timegrid)(i, j, k) += ntheta_inv * value;
            }
          }
        }
      }
      file.close();
      NRLib::RemoveFile(filename);
      if (model_settings->GetOutputTimeSegy()) {
        printf("Write seismic in time on Segy format\n");
        double theta = theta_0 + l * dtheta;
        SEGY::writeSegy(*timegrid, prefix_ + "seismic_time_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
      }
      if (model_settings->GetOutputSeismicTime()) {
        printf("Write seismic in time on Storm format\n");
        double theta = theta_0 + l * dtheta;
        std::string filename_out = prefix_ + "seismic_time_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".storm";
        if (model_settings->GetOutputSeismicTimeshift() || model_settings->GetOutputTimeshiftSegy()) {
          STORM::writeStorm(*timegrid, filename_out, top_time_window_, bot_time_window_, time_window_, true);
        } else {
          STORM::writeStorm(*timegrid, filename_out, top_time_window_, bot_time_window_, time_window_);
        }
      }
    }
    if (model_settings->GetOutputSeismicStackTimeSegy()) {
      printf("Write seismic stack in time on Segy format\n");
      SEGY::writeSegy(*stack_timegrid, prefix_ + "seismic_time_stack" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
    }
    if (model_settings->GetOutputSeismicStackTimeStorm()) {
      printf("Write seismic stack in time on Storm format\n");
      std::string filename_out = prefix_ + "seismic_time_stack" + suffix_ + ".storm";
      if (model_settings->GetOutputSeismicStackTimeShiftStorm() || model_settings->GetOutputSeismicStackTimeShiftSegy()) {
        STORM::writeStorm(*stack_timegrid, filename_out, top_time_window_, bot_time_window_, time_window_, true);
      } else {
        STORM::writeStorm(*stack_timegrid, filename_out, top_time_window_, bot_time_window_, time_window_);
      }
    }

  }


  // seismic shifted time (storm and segy) and seismic stack - shifted time (storm and segy)
  if (model_settings->GetOutputSeismicStackTimeShiftStorm() || model_settings->GetOutputSeismicStackTimeShiftSegy()) {
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = 0; j < ny; j++) {
        for (size_t k = 0; k < nt; k++) {
          (*stack_timegrid)(i, j, k) = 0.0;
        }
      }
    }
  }
  if (model_settings->GetOutputSeismicTimeshift() || model_settings->GetOutputTimeshiftSegy()) {
    for (size_t l = 0; l < ntheta; l++) {
      std::ifstream file;
      std::string filename = "timeshift_" + NRLib::ToString(l);
      NRLib::OpenRead(file, filename, std::ios::in | std::ios::binary);
      for (size_t i = 0; i < nx; i++) {
        for (size_t j = 0; j < ny; j++) {
          for (size_t k = 0; k < nt; k++) {
            float value = NRLib::ReadBinaryFloat(file);
            (*timegrid)(i, j, k) = value;
            if (model_settings->GetOutputSeismicStackTimeShiftStorm() || model_settings->GetOutputSeismicStackTimeShiftSegy()) {
              (*stack_timegrid)(i, j, k) += ntheta_inv * value;
            }
          }
        }
      }
      file.close();
      NRLib::RemoveFile(filename);
      if (model_settings->GetOutputTimeSegy()) {
        printf("Write seismic shifted in time on Segy format\n");
        double theta = theta_0 + l * dtheta;
        SEGY::writeSegy(*timegrid, prefix_ + "seismic_timeshift_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
      }
      if (model_settings->GetOutputSeismicTime()) {
        printf("Write seismic shifted in time on Storm format\n");
        double theta = theta_0 + l * dtheta;
        STORM::writeStorm(*timegrid, prefix_ + "seismic_timeshift_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".storm", top_time_window_, bot_time_window_, time_window_);
      }
    }
    if (model_settings->GetOutputSeismicStackTimeShiftSegy()) {
      printf("Write seismic stack shifted in time on Segy format\n");
      SEGY::writeSegy(*stack_timegrid, prefix_ + "seismic_timeshift_stack" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_time_window_, bot_time_window_, time_window_);
    }
    if (model_settings->GetOutputSeismicStackTimeShiftStorm() == true) {
      printf("Write seismic stack shifted in time on Storm format\n");
      STORM::writeStorm(*stack_timegrid, prefix_ + "seismic_timeshift_stack" + suffix_ + ".storm", top_time_window_, bot_time_window_, time_window_);
    }
  }

  if (timegrid != NULL) {
    delete timegrid;
  }
  if (stack_timegrid != NULL) {
    delete stack_timegrid;
  }

}


void SeismicOutput::writeSeismicDepthSeismicOnFile(SeismicParameters &seismic_parameters, bool depth_output) {
  // seismic depth (storm and segy) and seismic stack - depth (storm and segy)
  NRLib::StormContGrid *depthgrid = NULL;
  NRLib::StormContGrid *stack_depthgrid = NULL;

  ModelSettings *model_settings      = seismic_parameters.modelSettings();
  NRLib::SegyGeometry *segy_geometry = seismic_parameters.segyGeometry();

  double theta_0    = seismic_parameters.theta0();
  double ntheta     = seismic_parameters.nTheta();
  double dtheta     = seismic_parameters.dTheta();
  float  ntheta_inv = static_cast<float>(1.0 / ntheta);

  size_t nx              = seismic_parameters.seismicGeometry()->nx();
  size_t ny              = seismic_parameters.seismicGeometry()->ny();
  size_t nz              = seismic_parameters.seismicGeometry()->nz();
  NRLib::Volume volume   = seismic_parameters.seismicGeometry()->createDepthVolume();

  if (model_settings->GetOutputSeismicStackDepthStorm() || model_settings->GetOutputSeismicStackDepthSegy()) {
    stack_depthgrid = new NRLib::StormContGrid(volume, nx, ny, nz);
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = 0; j < ny; j++) {
        for (size_t k = 0; k < nz; k++) {
          (*stack_depthgrid)(i, j, k) = 0.0;
        }
      }
    }
  }
  if (depth_output == true) {
    depthgrid = new NRLib::StormContGrid(volume, nx, ny, nz);
    for (size_t l = 0; l < ntheta; l++) {
      std::ifstream file;
      std::string filename = "depth_" + NRLib::ToString(l);
      NRLib::OpenRead(file, filename, std::ios::in | std::ios::binary);
      for (size_t i = 0; i < nx; i++) {
        for (size_t j = 0; j < ny; j++) {
          for (size_t k = 0; k < nz; k++) {
            float value = NRLib::ReadBinaryFloat(file);
            (*depthgrid)(i, j, k) = value;
            if (model_settings->GetOutputSeismicStackDepthStorm() || model_settings->GetOutputSeismicStackDepthSegy()) {
              (*stack_depthgrid)(i, j, k) += ntheta_inv * value;
            }
          }
        }
      }
      file.close();
      NRLib::RemoveFile(filename);
      if (model_settings->GetOutputDepthSegy() == true) {
        printf("Write seismic in depth on Segy format\n");
        double theta = theta_0 + l * dtheta;
        SEGY::writeSegy(*depthgrid, prefix_ + "seismic_depth_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);
      }
      if (model_settings->GetOutputSeismicDepth() == true) {
        printf("Write seismic in depth on Storm format\n");
        double theta = theta_0 + l * dtheta;
        std::string filename_out = prefix_ + "seismic_depth_" + NRLib::ToString(NRLib::Radian * theta) + suffix_ + ".storm";
        STORM::writeStorm(*depthgrid, filename_out, top_depth_window_, bot_depth_window_, depth_window_);
      }
    }
    if (model_settings->GetOutputSeismicStackDepthSegy() == true) {
      printf("Write seismic stack in depth on Segy format\n");
      SEGY::writeSegy(*stack_depthgrid, prefix_ + "seismic_depth_stack" + suffix_ + ".segy", inline_start_, xline_start_, xline_x_axis_, inline_step_, xline_step_, segy_geometry, scalco_, top_depth_window_, bot_depth_window_, depth_window_);
    }
    if (model_settings->GetOutputSeismicStackDepthStorm() == true) {
      printf("Write seismic stack in depth on Storm format\n");
      std::string filename_out = prefix_ + "seismic_depth_stack" + suffix_ + ".storm";
      STORM::writeStorm(*stack_depthgrid, filename_out, top_depth_window_, bot_depth_window_, depth_window_);
    }
  }

  if (depthgrid != NULL) {
    delete depthgrid;
  }
  if (stack_depthgrid != NULL) {
    delete stack_depthgrid;
  }
}