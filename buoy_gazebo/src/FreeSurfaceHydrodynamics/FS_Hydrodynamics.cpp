// Copyright 2022 Open Source Robotics Foundation, Inc. and Monterey Bay
// Aquarium Research Institute
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fstream>
#include <iostream>
#include <vector>

#include "FS_Hydrodynamics.hpp"
#include "mlinterp.hpp"
#include <Eigen/Dense>

#include "IncidentWave.hpp"

#include <cmath>
//#include <boost/tuple/tuple.hpp>
#include "gnuplot-iostream.h"

using namespace Eigen;
using namespace mlinterp;

int CountLines(std::string filenm)
{
  int count = 0;
  std::string line;
  std::ifstream ifile(filenm);

  if (ifile.is_open())
{  
    while (ifile.peek() != EOF)
    {
      std::getline(ifile, line);
      count++;
    }
    ifile.close();
  }
  else
    std::cout << "ERROR:  " << filenm << " can't be opened" << std::endl;

  return count;
}

FS_HydroDynamics::FS_HydroDynamics(IncidentWave& IncWave): _IncWave(IncWave), m_L{1.0}, m_grav{9.81}, m_rho{1025} { }

FS_HydroDynamics::FS_HydroDynamics(IncidentWave& IncWave, double L, double g, double rho) :
                                                       _IncWave(IncWave), m_L{L}, m_grav{g}, m_rho{rho} { }

/// \brief  Read frequency domain coefficients from WAMIT.
///
///  .1 files include added mass and damping coefficients
///
///  .3 files include wave-exciting force coefficients
void FS_HydroDynamics::ReadWAMITData_FD(std::string filenm)
{
  int n_lines1 = CountLines(filenm + ".1");

  std::ifstream ifile1(filenm + ".1");
  m_fd_filename = (filenm + ".1 and .3 not found");
  if (ifile1.is_open())
  {
    m_fd_filename = (filenm + ".3 not found");
    MatrixXd s1(n_lines1, 5);
    for (int n = 0; n < n_lines1; n++)
      ifile1 >> s1(n, 0) >> s1(n, 1) >> s1(n, 2) >> s1(n, 3) >> s1(n, 4);
    ifile1.close();

    // Determine number of frequencies by looking for first repeat of i,j
    int n_freqs = 0;
    int n = 0;
    while (n < n_lines1)
    {
      if ((s1(n, 1) == s1(0, 1)) && (s1(n, 2) == s1(0, 2)))
        n_freqs++;
      n++;
    }
    int coeffPerFreqs = n_lines1 / n_freqs;

    this->fd_am_dmp_tps = VectorXd(n_freqs-1);  //Infinite Frequency coefficents stored independelty, assumpting here is all .1 files have an infinite frequency field, may or may not be true...
    this->fd_am_dmp_omega = VectorXd(n_freqs-1);
    int nn = 0;
    for (n = 0; n < n_freqs; n++)
    {
      if(s1(n*coeffPerFreqs, 0) == 0.0) //Infinite frequency case
      {
      for (int k = 0; k < coeffPerFreqs; k++)
        {
        int i = n * coeffPerFreqs + k;
        this->fd_X_inf_freq(s1(i, 1) - 1, s1(i, 2) - 1) = m_rho * s1(i, 3);
        this->fd_Y_inf_freq(s1(i, 1) - 1, s1(i, 2) - 1) = 0; //m_rho * fd_am_dmp_omega(n) * s1(i, 4);
        }
      }
      else
      {
      this->fd_am_dmp_tps(nn) = s1(n * coeffPerFreqs, 0);
      if(this->fd_am_dmp_tps(nn) < 0) //Negative period indicates zero frequency
        this->fd_am_dmp_omega(nn) = 0.0;
      else
        this->fd_am_dmp_omega(nn) = M_PI * 2 / this->fd_am_dmp_tps(nn);
      Eigen::Matrix<double, 6, 6> X;
      Eigen::Matrix<double, 6, 6> Y;
      X.Constant(0.0);
      Y.Constant(0.0);
      for (int k = 0; k < coeffPerFreqs; k++)
      {
        int i = n * coeffPerFreqs + k;
        X(s1(i, 1) - 1, s1(i, 2) - 1) = m_rho * s1(i, 3);
        Y(s1(i, 1) - 1, s1(i, 2) - 1) = m_rho * fd_am_dmp_omega(nn) * s1(i, 4);
      }
      this->fd_X.push_back(X);
      this->fd_Y.push_back(Y);
       nn++;
      }
    }
  }
  else
    std::cout << "ERROR:  " << filenm << ".1 can't be opened" << std::endl;

  // Read Wave Exciting Forces
  int n_lines3 = CountLines(filenm + ".3");
  std::ifstream ifile3(filenm + ".3");
  if (ifile3.is_open())
  {
    m_fd_filename = filenm;
    MatrixXd s3(n_lines3, 7);
    for (int n = 0; n < n_lines3; n++)
      ifile3 >> s3(n, 0) >> s3(n, 1) >> s3(n, 2) >> s3(n, 3) >> s3(n, 4) >>
          s3(n, 5) >> s3(n, 6);
    ifile3.close();

    // Determine number of frequencies by looking for number of i repeats
    int n_freqs = 0;
    int n = 0;
    while (n < n_lines3)
    {
      if (s3(n, 2) == s3(0, 2))
        n_freqs++;
      n++;
    }
    int coeffPerFreq = n_lines3 / n_freqs;
    this->fd_ext_tps = VectorXd(n_freqs);
    this->fd_ext_omega = VectorXd(n_freqs);
    this->fd_ext_beta = VectorXd(n_freqs);
    auto Mod_Xi = VectorXd(n_freqs);
    auto Pha_Xi = VectorXd(n_freqs);
    auto Re_Xi = VectorXd(n_freqs);
    auto Im_Xi = VectorXd(n_freqs);

    this->fd_Mod_Xi.resize(6);

    for (int k = 0; k < coeffPerFreq; k++) // Collect all values for each mode
    {
      for (n = 0; n < n_freqs; n++)
      {
        int i = n * coeffPerFreq + k;
        Mod_Xi(n) = m_rho * m_grav * s3(i, 3);
        Pha_Xi(n) = s3(i, 4);
        Re_Xi(n) = m_rho * m_grav * m_grav * s3(i, 5);
        Im_Xi(n) = m_rho * m_grav * s3(i, 6);
        if (k == 0) // Collect tps and beta just once
        {
          this->fd_ext_tps(n) = s3(i, 0);
          this->fd_ext_omega(n) = M_PI * 2 / this->fd_ext_tps(n);
          this->fd_ext_beta(n) = s3(i, 1);
        }
      }
      this->fd_Mod_Xi.push_back(Mod_Xi); // Relies on mode ordering being 1->6
      this->fd_Pha_Xi.push_back(Pha_Xi);
      this->fd_Re_Xi.push_back(Re_Xi);
      this->fd_Im_Xi.push_back(Im_Xi);
    }
  }
  else
    std::cout << "ERROR:  " << filenm << ".3 can't be opened" << std::endl;
}

/// \brief  Read time-domain coefficients from WAMIT.
///
///  _IR.1 files include radiation force impulse response functions
///
///  _IR.3 files include wave-exciting force impulse response functions
void FS_HydroDynamics::ReadWAMITData_TD(std::string filenm)
{
  int n_lines1 = CountLines(filenm + "_IR.1");

  std::ifstream ifile1(filenm + "_IR.1");
  m_td_filename = (filenm + "_IR.1 and _IR.3 not found");
  if (ifile1.is_open())
  {
    m_td_filename = (filenm + "_IR.3 not found");
    MatrixXd s1(n_lines1, 5);
    for (int n = 0; n < n_lines1; n++)
      ifile1 >> s1(n, 0) >> s1(n, 1) >> s1(n, 2) >> s1(n, 3) >> s1(n, 4);
    ifile1.close();

    // Determine number of timesteps by looking for first repeat of i,j
    int n_timesteps = 0;
    for (int n = 0; n < n_lines1; n++)
      if ((s1(n, 1) == s1(0, 1)) && (s1(n, 2) == s1(0, 2)))
        n_timesteps++;

    m_tau_rad.resize(n_timesteps);
    for (int n = 0; n < n_timesteps; n++)
      m_tau_rad(n) = s1(n * n_lines1 / n_timesteps, 0);

    for (int n = 0, k = 0; n < n_lines1; n++)
    {
      if (m_IR_cosint(s1(n, 1) - 1, s1(n, 2) - 1).size() == 0)
      {
        m_IR_cosint(s1(n, 1) - 1, s1(n, 2) - 1)
            .resize(n_timesteps); // Set vector size once
        m_IR_sinint(s1(n, 1) - 1, s1(n, 2) - 1)
            .resize(n_timesteps); // Set vector size once
      }
      if ((s1(n, 1) == s1(0, 1)) && (s1(n, 2) == s1(0, 2)))
        k++;

      m_IR_cosint(s1(n, 1) - 1, s1(n, 2) - 1)(k - 1) =
          m_rho * s1(n, 3); // Fill in dataFill in data
      m_IR_sinint(s1(n, 1) - 1, s1(n, 2) - 1)(k - 1) =
          m_rho * s1(n, 4); // Fill in dataFill in data
    }
  }
  else
    std::cout << "ERROR:  " << filenm << "_IR.1 can't be opened" << std::endl;

  int n_lines3 = CountLines(filenm + "_JR.3");

  std::ifstream ifile3(filenm + "_JR.3");
  m_td_filename = (filenm + "_IR.3 not found");
  if (ifile3.is_open())
  {
    m_td_filename = (filenm + "_IR.3 not found");
    MatrixXd s3(n_lines3, 7);
    for (int n = 0; n < n_lines3; n++)
      ifile3 >> s3(n, 0) >> s3(n, 1) >> s3(n, 2) >> s3(n, 3) >> s3(n, 4) >>
          s3(n, 5) >> s3(n, 6);
    ifile3.close();

    m_tau_exc.resize(n_lines3);
    for (int k = 0; k < n_lines3; k++)
      m_tau_exc(k) = s3(k, 0);

     m_dtau_exc = m_tau_exc(1)-m_tau_exc(0);

    for (int j = 0; j < 6; j++)
    {
      m_IR_exc(j).resize(n_lines3);
      for (int k = 0; k < n_lines3; k++)
        m_IR_exc(j)(k) = m_rho * m_grav * s3(k, j + 1);
    }
  }
  else
    std::cout << "ERROR:  " << filenm << "_JR.3 can't be opened" << std::endl;
}

void FS_HydroDynamics::Plot_FD_Coeffs()
{
  std::vector<double> pts_omega;
  for(int k = 0;k<fd_am_dmp_omega.size();k++){ //k = 0 is zero freq, k = 1 is inf freq
      pts_omega.push_back(fd_am_dmp_omega(k));
  }
 
 for(int i = 0; i<6; i++) 
   for(int j = i; j<6; j++) 
   {
  std::vector<double> pts_am,pts_am_sym;
  std::vector<double> pts_dmp,pts_dmp_sym;
  double am_norm = 0;
  double dmp_norm = 0;
  for(int k = 0;k<fd_am_dmp_tps.size();k++)
  {
    pts_am.push_back(fd_X[k](i,j));
    pts_am_sym.push_back(fd_X[k](j,i));
    am_norm += fabs(fd_X[k](i,j))+fabs(fd_X[k](j,i));
    pts_dmp.push_back(fd_Y[k](i,j));
    pts_dmp_sym.push_back(fd_Y[k](j,i));
    dmp_norm += fabs(fd_Y[k](i,j))+fabs(fd_Y[k](j,i));
     }
   if((am_norm > 1e-6) || (dmp_norm > 1e-6))
   {
   Gnuplot am_gp;
   Gnuplot dmp_gp; 
   Gnuplot gp;

   if(i==j) 
   {
     gp << "set term X11 title 'Added Mass and Damping (" << i+1 << "," << j+1 << ")\n";
     gp << "set multiplot layout 2,1 rowsfirst \n";
     gp << "set grid\n";
     gp << "plot '-' u 1:2 with lines title 'am(" << i+1 << "," << j+1 << ") inf\\_freq\\_am = " << std::fixed << std::setprecision(2) << this->fd_X_inf_freq(i,j)  <<  "'\n";
     gp.send1d(boost::make_tuple(pts_omega, pts_am));
     gp << "set xlabel 'rad/sec'\n";
     gp << "set ylabel 'kg'\n";

     gp << "plot '-' u 1:2 with lines title 'dmp(" << i+1 << "," << j+1 << ") inf\\_freq\\_dmp = " << std::fixed << std::setprecision(2) << this->fd_Y_inf_freq(i,j)  <<  "'\n";
     gp.send1d(boost::make_tuple(pts_omega, pts_dmp));
     gp << "set xlabel 'rad/sec'\n";
     gp << "set ylabel 'kg'\n";
   }
else{
     gp << "set term X11 title 'Added Mass and Damping (" << i+1 << "," << j+1 << ") and (" << j+1 << "," << i+1 <<")\n";
     gp << "set multiplot layout 2,1 rowsfirst \n";
     gp << "set grid\n";
     gp << "plot '-' w l title 'am(" << i+1 << "," << j+1 << ") inf\\_freq\\_am = " << std::fixed << std::setprecision(2) << this->fd_X_inf_freq(i,j)  <<
     "','-' w l title 'am(" << j+1 << "," << i+1 << ") inf\\_freq\\_am = " << std::fixed << std::setprecision(2) << this->fd_X_inf_freq(j,i)  <<  "'\n";
     gp.send1d(boost::make_tuple(pts_omega, pts_am));
     gp.send1d(boost::make_tuple(pts_omega, pts_am_sym));
     gp << "set xlabel 'rad/sec'\n";
     gp << "set ylabel 'kg'\n";

     gp << "plot '-' w l title 'dmp(" << i+1 << "," << j+1 << ") inf\\_freq\\_dmp = " << std::fixed << std::setprecision(2) << this->fd_Y_inf_freq(i,j)  <<
     "','-' w l title 'dmp(" << j+1 << "," << i+1 << ") inf\\_freq\\_dmp = " << std::fixed << std::setprecision(2) << this->fd_Y_inf_freq(j,i)  <<  "'\n";
     gp.send1d(boost::make_tuple(pts_omega, pts_dmp));
     gp.send1d(boost::make_tuple(pts_omega, pts_dmp_sym));
     gp << "set xlabel 'rad/sec'\n";
     gp << "set ylabel 'kg'\n";
}
   }
}
}



void FS_HydroDynamics::Plot_TD_Coeffs()
{
 for(int i = 0; i<6; i++) 
   for(int j = i; j<6; j++) 
   {
  std::vector<double> pts_L,pts_L_sym;
  std::vector<double> pts_tau;
  if (m_L_rad(i, j).size() > 0)  //Check to see if impulse response functoin is nonzero.
  {
  for(int k = 0;k<m_L_rad(i,j).size();k++)
  {
    pts_tau.push_back(m_dt*k);
    pts_L.push_back(m_L_rad(i,j)[k]);
    if(i !=j )
      pts_L_sym.push_back(m_L_rad(j,i)[k]);
   }
   
   Gnuplot gp;
   if(i==j) 
   {
     gp << "set term X11 title 'Radiation IRF (" << i+1 << "," << j+1 << ")\n";
     gp << "set grid\n";
     gp << "plot '-' u 1:2 with lines title 'IRF(" << i+1 << "," << j+1 << ")'\n";
     gp.send1d(boost::make_tuple(pts_tau, pts_L));
     gp << "set xlabel 'sec'\n";
     gp << "set ylabel '-'\n";
   }
else{
     gp << "set term X11 title 'Radiation IRF (" << i+1 << "," << j+1 << ") and (" << j+1 << "," << i+1 <<")\n";
     gp << "set grid\n";
     gp << "plot '-' w l title 'IRF(" << i+1 << "," << j+1 << ")"   << "','-' w l title 'IRF(" << j+1 << "," << i+1 << ")'\n";
     gp.send1d(boost::make_tuple(pts_tau, pts_L));
     gp.send1d(boost::make_tuple(pts_tau, pts_L_sym));
     gp << "set xlabel 'sec'\n";
     gp << "set ylabel '-'\n";
}
   }
}
}

void FS_HydroDynamics::SetTimestepSize(double dt)
{

  m_dt = dt;

  _n_rad_intpts = (m_tau_rad(m_tau_rad.size() - 1) - m_tau_rad(0)) / dt + 1;
  double x_rad[_n_rad_intpts];

  for (int k = 0; k < _n_rad_intpts; k++)
    x_rad[k] = k * dt;

  for (int i = 0; i < 6; i++)
    for (int j = 0; j < 6; j++)
    {
      int nd = m_IR_sinint(i, j).size();
      if (nd > 0)
      {
        m_L_rad(i, j).resize(_n_rad_intpts);
        interp(&nd, _n_rad_intpts,
               m_IR_sinint(i, j).data(), m_L_rad(i, j).data(),
               m_tau_rad.data(), x_rad);
      }
    }

  for (int i = 0; i < 6; i++)
    m_xddot(i).resize(STORAGE_MULTIPLIER * _n_rad_intpts); // Create storage for 5 times the length of the Impulse
                                     // response function,  code will shift data when this fills.


  _rad_tstep_index = 0;


  _n_exc_intpts = (m_tau_exc(m_tau_exc.size() - 1) - m_tau_exc(0)) / dt + 1;
  double x_exc[_n_exc_intpts];

  for (int k = 0; k < _n_exc_intpts; k++)
    x_exc[k] = m_tau_exc(0) + k * dt;


  for (int j = 0; j < 6; j++)
    if (m_IR_exc(j).size() > 0)
    {
      m_L_exc(j).resize(_n_exc_intpts);
      int nd = m_IR_exc(j).size();
      interp(&nd, _n_exc_intpts, m_IR_exc(j).data(), m_L_exc(j).data(),
             m_tau_exc.data(), x_exc);
    }
  _eta0.resize(STORAGE_MULTIPLIER * _n_exc_intpts); // Create storage for 5 times the length of the Impulse
                               // response function,  code will shift data when this fills.

 
  _exc_tstep_index = 0;
}

Eigen::VectorXd FS_HydroDynamics::ExcitingForce()
{
  Eigen::VectorXd ExctForces(6);

  if(_exc_tstep_index == 0)  //Fill in initial values
    for(; _exc_tstep_index <_n_exc_intpts;_exc_tstep_index++)
       _eta0(_exc_tstep_index) = _IncWave.eta(0,0,_exc_tstep_index*m_dt+m_tau_exc(0));
  else
    _eta0(_exc_tstep_index) = _IncWave.eta(0,0,_exc_tstep_index*m_dt+m_tau_exc(0)); //Compute next needed wave-elevation


  for (int i = 0; i < 6; i++) // Compute convolution integrals, no need to adjust ends in trap rule since integrand there is zero.
    ExctForces(i) = m_L_exc(i).dot(_eta0.segment(_exc_tstep_index - _n_exc_intpts, _n_exc_intpts).reverse());
  
  // Increment timestep index and shift stored xddot data if needed.
  _exc_tstep_index++;
  if (_exc_tstep_index == STORAGE_MULTIPLIER*_n_exc_intpts) // At end of allocated storage,
  {
    _eta0.head(_n_exc_intpts) = _eta0.tail(_n_exc_intpts);
    _exc_tstep_index = _n_exc_intpts;
  }

  ExctForces *= m_dt;
  return ExctForces;
}

Eigen::VectorXd FS_HydroDynamics::RadiationForce(Eigen::VectorXd last_xddot)
{
  Eigen::VectorXd RadiationForces(6);



  for (int j = 0; j < 6; j++)
   {
    m_xddot(j)(_rad_tstep_index) = 0; //This is unknown at this timestep, but can be set to zero since L_rad(tau = 0) = 0 always.
    if(_rad_tstep_index > 0)
      m_xddot(j)(_rad_tstep_index-1) = last_xddot(j);  //Store xddot from last timestep
   }

  for (int i = 0; i < 6; i++) 
  {
    RadiationForces(i) = 0;     // Initialize to zero.
    for (int j = 0; j < 6; j++) // Sum up convolution integrals, no need to adjust ends in trap rule since integrand there is zero.
    {
      if (m_L_rad(i, j).size() > 0)
      {
        if (_rad_tstep_index < _n_rad_intpts) // Limit IRF used early in the simulation
        {
          RadiationForces(i) +=
              m_L_rad(i, j)
                  .head(_rad_tstep_index)
                  .dot(m_xddot(j).segment(0, _rad_tstep_index).reverse());
        }
        else
        {
          RadiationForces(i) += m_L_rad(i, j).dot(m_xddot(j).segment(_rad_tstep_index - _n_rad_intpts, _n_rad_intpts) .reverse());
        }
      }
    }
  }

  // Increment timestep index and shift stored xddot data if needed.
  _rad_tstep_index++;
  if (_rad_tstep_index == STORAGE_MULTIPLIER*_n_rad_intpts) // At end of allocated storage,
  {
    for (int j = 0; j < 6; j++) // Copy most recent xdddot data to start of vector.
      m_xddot(j).head(_n_rad_intpts) = m_xddot(j).tail(_n_rad_intpts);
    _rad_tstep_index = _n_rad_intpts;
  }

  RadiationForces *= m_dt;

  return RadiationForces;
}

std::ostream &operator<<(std::ostream &out, const FS_HydroDynamics &f)
{

  std::cout << "# Freq Domain File = " << f.m_fd_filename << ".1" << std::endl;
  std::cout << "# L = " << f.m_L << std::endl;
  std::cout << "# gravity = " << f.m_grav << std::endl;
  std::cout << "# rho = " << f.m_rho << std::endl;
  std::cout << std::endl;
  std::cout << "#tps = " << f.fd_am_dmp_tps.transpose() << std::endl;
  std::cout << "#omega = " << f.fd_am_dmp_omega.transpose() << std::endl;
  std::cout << "#inf freq fd_X = " << std::endl
            << f.fd_X[0] << std::endl
            << std::endl;
  std::cout << "#inf freq fd_Y = " << std::endl
            << f.fd_Y[0] << std::endl;

  return out; // return std::ostream so we can chain calls to operator<<
}