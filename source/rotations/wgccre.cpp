/*
 * This file is part of Cosmonium.
 *
 * Copyright (C) 2018-2020 Laurent Deru.
 *
 * Cosmonium is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cosmonium is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cosmonium.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wgccre.h"
#include "frames.h"
#include "math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double const deg_to_rad = M_PI / 180;
static double const century = 36525.0;
static double const j2000_epoch = 2451545.0;

TypeHandle WGCCRESimpleRotation::_type_handle;

WGCCRESimpleRotation::WGCCRESimpleRotation(double a0, double d0, double prime,
    double rate, double epoch) :
    RotationBase(new J2000EquatorialReferenceFrame()),
    meridian_angle(prime * deg_to_rad), mean_motion(rate * deg_to_rad), epoch(epoch)
{
  flipped = rate < 0;
  orientation = calc_orientation(a0, d0, flipped);
}

WGCCRESimpleRotation::WGCCRESimpleRotation(WGCCRESimpleRotation const &other) :
    RotationBase(other.frame->make_copy()),
    meridian_angle(other.meridian_angle),
    mean_motion(other.mean_motion),
    epoch(other.epoch),
    flipped(other.flipped),
    orientation(other.orientation)
{
}

PT(RotationBase)
WGCCRESimpleRotation::make_copy(void) const
{
  return new WGCCRESimpleRotation(*this);
}

LQuaterniond WGCCRESimpleRotation::get_frame_equatorial_orientation_at(
    double time)
{
  return orientation;
}

LQuaterniond WGCCRESimpleRotation::get_frame_rotation_at(double time)
{
  double angle = (time - epoch) * mean_motion + meridian_angle;
  LQuaterniond local;
  if (flipped) {
    angle = -angle;
  }
  local.set_from_axis_angle_rad(angle, LVector3d::unit_z());
  LQuaterniond rotation = local * orientation;
  return rotation;
}

bool WGCCRESimpleRotation::is_flipped(void) const
{
  return flipped;
}

TypeHandle WGCCRESimplePrecessingRotation::_type_handle;

WGCCRESimplePrecessingRotation::WGCCRESimplePrecessingRotation(double a0,
    double a0_rate, double d0, double d0_rate, double prime, double rate,
    double epoch, double validity) :
    RotationBase(new J2000EquatorialReferenceFrame()),
    a0(a0), a0_rate(a0_rate), d0(d0), d0_rate(d0_rate), meridian_angle(prime * deg_to_rad), mean_motion(
        rate * deg_to_rad), epoch(epoch), validity(validity / century)
{
  flipped = rate < 0;
}

WGCCRESimplePrecessingRotation::WGCCRESimplePrecessingRotation(WGCCRESimplePrecessingRotation const &other) :
      RotationBase(other.frame->make_copy()),
      a0(other.a0), a0_rate(other.a0_rate), d0(other.d0), d0_rate(other.d0_rate),
      meridian_angle(other.meridian_angle), mean_motion(other.mean_motion),
      epoch(other.epoch), validity(other.validity),
      flipped(other.flipped)
{
}

PT(RotationBase)
WGCCRESimplePrecessingRotation::make_copy(void) const
{
  return new WGCCRESimplePrecessingRotation(*this);
}

double WGCCRESimplePrecessingRotation::get_T(double jd) const
{
  double T = (jd - epoch) / century;
  if (T < -validity) {
    T = -validity;
  } else if(T > validity) {
    T = validity;
  }
  return T;
}

LQuaterniond WGCCRESimplePrecessingRotation::get_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);
  double a0p = a0 + a0_rate * T;
  double d0p = d0 + d0_rate * T;
  return calc_orientation(a0p, d0p, flipped);
}

LQuaterniond WGCCRESimplePrecessingRotation::get_frame_rotation_at(double time)
{
  double angle = (time - epoch) * mean_motion + meridian_angle;
  LQuaterniond local;
  if (flipped) {
    angle = -angle;
  }
  local.set_from_axis_angle_rad(angle, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

bool WGCCRESimplePrecessingRotation::is_flipped(void) const
{
  return flipped;
}

TypeHandle WGCCREComplexRotation::_type_handle;

WGCCREComplexRotation::WGCCREComplexRotation(double epoch, double validity) :
    CachedRotationBase(new J2000EquatorialReferenceFrame()),
    epoch(epoch),
    validity(validity / century)
{

}

double WGCCREComplexRotation::get_T(double jd) const
{
  double T = (jd - epoch) / century;
  if (T < -validity) {
    T = -validity;
  } else if(T > validity) {
    T = validity;
  }
  return T;
}

PT(RotationBase) WGCCREMercuryRotation::make_copy(void) const
{
  return new WGCCREMercuryRotation(*this);
}

LQuaterniond WGCCREMercuryRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);
  double a0 = 281.0103 - 0.0328 * T;
  double d0 = 61.4155 - 0.0049 * T;
  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREMercuryRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double M1 = 174.7910857 * deg_to_rad + 4.092335 * deg_to_rad * d;
  double M2 = 349.5821714 * deg_to_rad + 8.184670 * deg_to_rad * d;
  double M3 = 164.3732571 * deg_to_rad + 12.277005 * deg_to_rad * d;
  double M4 = 339.1643429 * deg_to_rad + 16.369340 * deg_to_rad * d;
  double M5 = 153.9554286 * deg_to_rad + 20.461675 * deg_to_rad * d;

  double W = 329.5988 + 6.1385108 * d + 0.01067257 * sin(M1)
      - 0.00112309 * sin(M2) - 0.00011040 * sin(M3) - 0.00002539 * sin(M4)
      - 0.00000571 * sin(M5);
  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREMarsRotation::make_copy(void) const
{
  return new WGCCREMarsRotation(*this);
}

LQuaterniond WGCCREMarsRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);
  double a0 = 317.269202 - 0.10927547 * T
      + 0.000068 * sin(198.991226 * deg_to_rad + 19139.4819985 * deg_to_rad * T)
      + 0.000238 * sin(226.292679 * deg_to_rad + 38280.8511281 * deg_to_rad * T)
      + 0.000052 * sin(249.663391 * deg_to_rad + 57420.7251593 * deg_to_rad * T)
      + 0.000009 * sin(266.183510 * deg_to_rad + 76560.6367950 * deg_to_rad * T)
      + 0.419057 * sin(79.398797 * deg_to_rad + 0.5042615 * deg_to_rad * T);

  double d0 = 54.432516 - 0.05827105 * T
      + 0.000051 * cos(122.433576 * deg_to_rad + 19139.9407476 * deg_to_rad * T)
      + 0.000141 * cos(43.058401 * deg_to_rad + 38280.8753272 * deg_to_rad * T)
      + 0.000031 * cos(57.663379 * deg_to_rad + 57420.7517205 * deg_to_rad * T)
      + 0.000005 * cos(79.476401 * deg_to_rad + 76560.6495004 * deg_to_rad * T)
      + 1.591274 * cos(166.325722 * deg_to_rad + 0.5042615 * deg_to_rad * T);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREMarsRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double W = 176.049863 + 350.891982443297 * d
      + 0.000145 * sin(129.071773 * deg_to_rad + 19140.0328244 * deg_to_rad * T)
      + 0.000157 * sin(36.352167 * deg_to_rad + 38281.0473591 * deg_to_rad * T)
      + 0.000040 * sin(56.668646 * deg_to_rad + 57420.9295360 * deg_to_rad * T)
      + 0.000001 * sin(67.364003 * deg_to_rad + 76560.2552215 * deg_to_rad * T)
      + 0.000001 * sin(104.792680 * deg_to_rad + 95700.4387578 * deg_to_rad * T)
      + 0.584542 * sin(95.391654 * deg_to_rad + 0.5042615 * deg_to_rad * T);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREJupiterRotation::make_copy(void) const
{
  return new WGCCREJupiterRotation(*this);
}

LQuaterniond WGCCREJupiterRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double Ja = 99.360714 * deg_to_rad + 4850.4046 * deg_to_rad * T;
  double Jb = 175.895369 * deg_to_rad + 1191.9605 * deg_to_rad * T;
  double Jc = 300.323162 * deg_to_rad + 262.5475 * deg_to_rad * T;
  double Jd = 114.012305 * deg_to_rad + 6070.2476 * deg_to_rad * T;
  double Je = 49.511251 * deg_to_rad + 64.3000 * deg_to_rad * T;

  double a0 = 268.056595 - 0.006499 * T
      + 0.000117 * sin(Ja)
      + 0.000938 * sin(Jb)
      + 0.001432 * sin(Jc)
      + 0.000030 * sin(Jd)
      + 0.002150 * sin(Je);
  double d0 = 64.495303 + 0.002413 * T
      + 0.000050 * cos(Ja)
      + 0.000404 * cos(Jb)
      + 0.000617 * cos(Jc)
      - 0.000013 * cos(Jd)
      + 0.000926 * cos(Je);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREJupiterRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;

  double W = 284.95 + 870.5360000 * d;

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRENeptuneRotation::make_copy(void) const
{
  return new WGCCRENeptuneRotation(*this);
}

LQuaterniond WGCCRENeptuneRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;

  double a0 = 299.36 + 0.70 * sin(N);
  double d0 = 43.46 - 0.51 * cos(N);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRENeptuneRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double W = 249.978 + 541.1397757 * d - 0.48 * sin(N);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRE9MoonRotation::make_copy(void) const
{
  return new WGCCRE9MoonRotation(*this);
}

LQuaterniond WGCCRE9MoonRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double E1 = 125.045 * deg_to_rad - 0.0529921 * deg_to_rad * d;
  double E2 = 250.089 * deg_to_rad - 0.1059842 * deg_to_rad * d;
  double E3 = 260.008 * deg_to_rad + 13.0120009 * deg_to_rad * d;
  double E4 = 176.625 * deg_to_rad + 13.3407154 * deg_to_rad * d;
  double E6 = 311.589 * deg_to_rad + 26.4057084 * deg_to_rad * d;
  double E7 = 134.963 * deg_to_rad + 13.0649930 * deg_to_rad * d;
  double E10 = 15.134 * deg_to_rad - 0.1589763 * deg_to_rad * d;
  double E13 = 25.053 * deg_to_rad + 12.9590088 * deg_to_rad * d;

  double a0 = 269.9949 + 0.0031 * T
      -3.8787 * sin(E1)
      -0.1204 * sin(E2)
      +0.0700 * sin(E3)
      -0.0172 * sin(E4)
      +0.0072 * sin(E6)
      -0.0052 * sin(E10)
      +0.0043 * sin(E13);

  double d0 = 66.5392 + 0.0130 * T
      +1.5419 * cos(E1)
      +0.0239 * cos(E2)
      -0.0278 * cos(E3)
      +0.0068 * cos(E4)
      -0.0029 * cos(E6)
      +0.0009 * cos(E7)
      +0.0008 * cos(E10)
      -0.0009 * cos(E13);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRE9MoonRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;

  double E1 = 125.045 * deg_to_rad - 0.0529921 * deg_to_rad * d;
  double E2 = 250.089 * deg_to_rad - 0.1059842 * deg_to_rad * d;
  double E3 = 260.008 * deg_to_rad + 13.0120009 * deg_to_rad * d;
  double E4 = 176.625 * deg_to_rad + 13.3407154 * deg_to_rad * d;
  double E5 = 357.529 * deg_to_rad + 0.9856003 * deg_to_rad * d;
  double E6 = 311.589 * deg_to_rad + 26.4057084 * deg_to_rad * d;
  double E7 = 134.963 * deg_to_rad + 13.0649930 * deg_to_rad * d;
  double E8 = 276.617 * deg_to_rad + 0.3287146 * deg_to_rad * d;
  double E9 = 34.226 * deg_to_rad + 1.7484877 * deg_to_rad * d;
  double E10 = 15.134 * deg_to_rad - 0.1589763 * deg_to_rad * d;
  double E11 = 119.743 * deg_to_rad + 0.0036096 * deg_to_rad * d;
  double E12 = 239.961 * deg_to_rad + 0.1643573 * deg_to_rad * d;
  double E13 = 25.053 * deg_to_rad + 12.9590088 * deg_to_rad * d;

  double W = 38.3213 + 13.17635815 * d -1.410E-12 * d * d
      +3.5610 * sin(E1)
      +0.1208 * sin(E2)
      -0.0642 * sin(E3)
      +0.0158 * sin(E4)
      +0.0252 * sin(E5)
      -0.0066 * sin(E6)
      -0.0047 * sin(E7)
      -0.0046 * sin(E8)
      +0.0028 * sin(E9)
      +0.0052 * sin(E10)
      +0.0040 * sin(E11)
      +0.0019 * sin(E12)
      -0.0044 * sin(E13);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREPhobosRotation::make_copy(void) const
{
  return new WGCCREPhobosRotation(*this);
}

LQuaterniond WGCCREPhobosRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double M1 = 190.72646643 * deg_to_rad + 15917.10818695 * deg_to_rad * T;
  double M2 = 21.46892470 * deg_to_rad + 31834.27934054 * deg_to_rad * T;
  double M3 = 332.86082793 * deg_to_rad + 19139.89694742 * deg_to_rad * T;
  double M4 = 394.93256437 * deg_to_rad + 38280.79631835 * deg_to_rad * T;

  double a0 = 317.67071657 - 0.10844326 * T
      - 1.78428399 * sin(M1)
      + 0.02212824 * sin(M2)
      - 0.01028251 * sin(M3)
      - 0.00475595 * sin(M4);

  double d0 = 52.88627266 - 0.06134706 * T
      - 1.07516537 * cos(M1)
      + 0.00668626 * cos(M2)
      - 0.00648740 * cos(M3)
      + 0.00281576 * cos(M4);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREPhobosRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double M1 = 190.72646643 * deg_to_rad + 15917.10818695 * deg_to_rad * T;
  double M2 = 21.46892470 * deg_to_rad + 31834.27934054 * deg_to_rad * T;
  double M3 = 332.86082793 * deg_to_rad + 19139.89694742 * deg_to_rad * T;
  double M4 = 394.93256437 * deg_to_rad + 38280.79631835 * deg_to_rad * T;
  double M5 = 189.63271560 * deg_to_rad + 41215158.18420050 * deg_to_rad * T + 12.71192322 * deg_to_rad * T * T;

  double W = 35.18774440 + 1128.84475928 * d + 12.72192797 * T
      + 1.42421769 * sin(M1)
      - 0.02273783 * sin(M2)
      + 0.00410711 * sin(M3)
      + 0.00631964 * sin(M4)
      - 1.143 * sin(M5);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREDeimosRotation::make_copy(void) const
{
  return new WGCCREDeimosRotation(*this);
}

LQuaterniond WGCCREDeimosRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double M6 = 121.46893664 * deg_to_rad + 660.22803474 * deg_to_rad * T;
  double M7 = 231.05028581 * deg_to_rad + 660.99123540 * deg_to_rad * T;
  double M8 = 251.37314025 * deg_to_rad + 1320.50145245 * deg_to_rad * T;
  double M9 = 217.98635955 * deg_to_rad + 38279.96125550 * deg_to_rad * T;
  double M10 = 196.19729402 * deg_to_rad + 19139.83628608 * deg_to_rad * T;

  double a0 = 316.65705808 - 0.10518014 * T
      + 3.09217726 * sin(M6)
      + 0.22980637 * sin(M7)
      + 0.06418655 * sin(M8)
      + 0.02533537 * sin(M9)
      + 0.00778695 * sin(M10);

  double d0 = 53.50992033 - 0.05979094 * T
      + 1.83936004 * cos(M6)
      + 0.14325320 * cos(M7)
      + 0.01911409 * cos(M8)
      - 0.01482590 * cos(M9)
      + 0.00192430 * cos(M10);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREDeimosRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double M6 = 121.46893664 * deg_to_rad + 660.22803474 * deg_to_rad * T;
  double M7 = 231.05028581 * deg_to_rad + 660.99123540 * deg_to_rad * T;
  double M8 = 251.37314025 * deg_to_rad + 1320.50145245 * deg_to_rad * T;
  double M9 = 217.98635955 * deg_to_rad + 38279.96125550 * deg_to_rad * T;
  double M10 = 196.19729402 * deg_to_rad + 19139.83628608 * deg_to_rad * T;

  double W = 79.39932954 + 285.16188899 * d
      - 2.73954829 * sin(M6)
      - 0.39968606 * sin(M7)
      - 0.06563259 * sin(M8)
      - 0.02912940 * sin(M9)
      + 0.01699160 * sin(M10);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREAmaltheaRotation::make_copy(void) const
{
  return new WGCCREAmaltheaRotation(*this);
}

LQuaterniond WGCCREAmaltheaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);
  double J1 = 73.32 * deg_to_rad + 91472.9 * deg_to_rad * T;

  double a0 = 268.05 - 0.009 * T
      - 0.84 * sin(J1)
      + 0.01 * sin(2 * J1);
  double d0 = 64.49 + 0.003 * T
      - 0.36 * cos(J1);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREAmaltheaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double J1 = 73.32 * deg_to_rad + 91472.9 * deg_to_rad * T;
  double W = 231.67 + 722.6314560 * d
      + 0.76 * sin(J1)
      - 0.01 * sin(2 * J1);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREThebeRotation::make_copy(void) const
{
  return new WGCCREThebeRotation(*this);
}

LQuaterniond WGCCREThebeRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double J2 = 24.62 * deg_to_rad + 45137.2 * deg_to_rad * T;

  double a0 = 268.05 - 0.009 * T
      - 2.11 * sin(J2)
      + 0.04 * sin(2 * J2);

  double d0 = 64.49 + 0.003 * T
      - 0.91 * cos(J2)
      + 0.01 * cos(2 * J2);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREThebeRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double J2 = 24.62 * deg_to_rad + 45137.2 * deg_to_rad * T;

  double W = 8.56 + 533.7004100 * d
      + 1.91 * sin(J2)
      - 0.04 * sin(2 * J2);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREIoRotation::make_copy(void) const
{
  return new WGCCREIoRotation(*this);
}

LQuaterniond WGCCREIoRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double J3 = 283.90 * deg_to_rad + 4850.7 * deg_to_rad * T;
  double J4 = 355.80 * deg_to_rad + 1191.3 * deg_to_rad * T;

  double a0 = 268.05 - 0.009 * T
      + 0.094 * sin(J3)
      + 0.024 * sin(J4);

  double d0 = 64.50 + 0.003 * T
      + 0.040 * cos(J3)
      + 0.011 * cos(J4);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREIoRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double J3 = 283.90 * deg_to_rad + 4850.7 * deg_to_rad * T;
  double J4 = 355.80 * deg_to_rad + 1191.3 * deg_to_rad * T;

  double W = 200.39 + 203.4889538 * d
      - 0.085 * sin(J3)
      - 0.022 * sin(J4);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREEuropaRotation::make_copy(void) const
{
  return new WGCCREEuropaRotation(*this);
}

LQuaterniond WGCCREEuropaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double J4 = 355.80 * deg_to_rad + 1191.3 * deg_to_rad * T;
  double J5 = 119.90 * deg_to_rad + 262.1 * deg_to_rad * T;
  double J6 = 229.80 * deg_to_rad + 64.3 * deg_to_rad * T;
  double J7 = 352.25 * deg_to_rad + 2382.6 * deg_to_rad * T;

  double a0 = 268.08 - 0.009 * T
      + 1.086 * sin(J4)
      + 0.060 * sin(J5)
      + 0.015 * sin(J6)
      + 0.009 * sin(J7);

  double d0 = 64.51 + 0.003 * T
      + 0.468 * cos(J4)
      + 0.026 * cos(J5)
      + 0.007 * cos(J6)
      + 0.002 * cos(J7);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREEuropaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double J4 = 355.80 * deg_to_rad + 1191.3 * deg_to_rad * T;
  double J5 = 119.90 * deg_to_rad + 262.1 * deg_to_rad * T;
  double J6 = 229.80 * deg_to_rad + 64.3 * deg_to_rad * T;
  double J7 = 352.25 * deg_to_rad + 2382.6 * deg_to_rad * T;

  double W = 36.022 + 101.3747235 * d
      - 0.980 * sin(J4)
      - 0.054 * sin(J5)
      - 0.014 * sin(J6)
      - 0.008 * sin(J7);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREGanymedeRotation::make_copy(void) const
{
  return new WGCCREGanymedeRotation(*this);
}

LQuaterniond WGCCREGanymedeRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double J4 = 355.80 * deg_to_rad + 1191.3 * deg_to_rad * T;
  double J5 = 119.90 * deg_to_rad + 262.1 * deg_to_rad * T;
  double J6 = 229.80 * deg_to_rad + 64.3 * deg_to_rad * T;

  double a0 = 268.20 - 0.009 * T
      - 0.037 * sin(J4)
      + 0.431 * sin(J5)
      + 0.091 * sin(J6);

  double d0 = 64.57 + 0.003 * T
      - 0.016 * cos(J4)
      + 0.186 * cos(J5)
      + 0.039 * cos(J6);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREGanymedeRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double J4 = 355.80 * deg_to_rad + 1191.3 * deg_to_rad * T;
  double J5 = 119.90 * deg_to_rad + 262.1 * deg_to_rad * T;
  double J6 = 229.80 * deg_to_rad + 64.3 * deg_to_rad * T;

  double W = 44.064 + 50.3176081 * d
      + 0.033 * sin(J4)
      - 0.389 * sin(J5)
      - 0.082 * sin(J6);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRECallistoRotation::make_copy(void) const
{
  return new WGCCRECallistoRotation(*this);
}

LQuaterniond WGCCRECallistoRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double J5 = 119.90 * deg_to_rad + 262.1 * deg_to_rad * T;
  double J6 = 229.80 * deg_to_rad + 64.3 * deg_to_rad * T;
  double J8 = 113.35 * deg_to_rad + 6070.0 * deg_to_rad * T;

  double a0 = 268.72 - 0.009 * T
      - 0.068 * sin(J5)
      + 0.590 * sin(J6)
      + 0.010 * sin(J8);

  double d0 = 64.83 + 0.003 * T
      - 0.029 * cos(J5)
      + 0.254 * cos(J6)
      - 0.004 * cos(J8);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRECallistoRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double J5 = 119.90 * deg_to_rad + 262.1 * deg_to_rad * T;
  double J6 = 229.80 * deg_to_rad + 64.3 * deg_to_rad * T;
  double J8 = 113.35 * deg_to_rad + 6070.0 * deg_to_rad * T;

  double W = 259.51 + 21.5710715 * d
      + 0.061 * sin(J5)
      - 0.533 * sin(J6)
      - 0.009 * sin(J8);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREEpimetheusRotation::make_copy(void) const
{
  return new WGCCREEpimetheusRotation(*this);
}

LQuaterniond WGCCREEpimetheusRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double S1 = 353.32 * deg_to_rad + 75706.7 * deg_to_rad * T;

  double a0 = 40.58 - 0.036 * T
      - 3.153 * sin(S1)
      + 0.086 * sin(2 * S1);
  double d0 = 83.52 - 0.004 * T
      - 0.356 * cos(S1)
      + 0.005 * cos(2 * S1);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREEpimetheusRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double S1 = 353.32 * deg_to_rad + 75706.7 * deg_to_rad * T;

  double W = 293.87 + 518.4907239 * d
      + 3.133 * sin(S1)
      - 0.086 * sin(2 * S1);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREJanusRotation::make_copy(void) const
{
  return new WGCCREJanusRotation(*this);
}

LQuaterniond WGCCREJanusRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double S2 = 28.72 * deg_to_rad + 75706.7 * deg_to_rad * T;

  double a0 = 40.58 - 0.036 * T
      - 1.623 * sin(S2)
      + 0.023 * sin(2 * S2);
  double d0 = 83.52 - 0.004 * T
      - 0.183 * cos(S2)
      + 0.001 * cos(2 * S2);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREJanusRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double S2 = 28.72 * deg_to_rad + 75706.7 * deg_to_rad * T;

  double W = 58.83 + 518.2359876 * d
      + 1.613 * sin(S2)
      - 0.023 * sin(2 * S2);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREMimasRotation::make_copy(void) const
{
  return new WGCCREMimasRotation(*this);
}

LQuaterniond WGCCREMimasRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double S3 = 177.40 * deg_to_rad - 36505.5 * deg_to_rad * T;

  double a0 = 40.66 - 0.036 * T
      + 13.56 * sin(S3);
  double d0 = 83.52 - 0.004 * T
      - 1.53 * cos(S3);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREMimasRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double S3 = 177.40 * deg_to_rad - 36505.5 * deg_to_rad * T;
  double S5 = 316.45 * deg_to_rad + 506.2 * deg_to_rad * T;

  double W = 333.46 + 381.9945550 * d
      - 13.48 * sin(S3)
      - 44.85 * sin(S5);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRETethysRotation::make_copy(void) const
{
  return new WGCCRETethysRotation(*this);
}

LQuaterniond WGCCRETethysRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double S4 = 300.00 * deg_to_rad - 7225.9 * deg_to_rad * T;

  double a0 = 40.66 - 0.036 * T
      + 9.66 * sin(S4);
  double d0 = 83.52 - 0.004 * T
      - 1.09 * cos(S4);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRETethysRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double S4 = 300.00 * deg_to_rad - 7225.9 * deg_to_rad * T;
  double S5 = 316.45 * deg_to_rad + 506.2 * deg_to_rad * T;

  double W = 8.95 + 190.6979085 * d
      - 9.60 * sin(S4)
      + 2.23 * sin(S5);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRERheaRotation::make_copy(void) const
{
  return new WGCCRERheaRotation(*this);
}

LQuaterniond WGCCRERheaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double S6 = 345.20 * deg_to_rad - 1016.3 * deg_to_rad * T;

  double a0 = 40.38 - 0.036 * T
      + 3.10 * sin(S6);

  double d0 = 83.55 - 0.004 * T
      - 0.35 * cos(S6);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRERheaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double S6 = 345.20 * deg_to_rad - 1016.3 * deg_to_rad * T;

  double W = 235.16 + 79.6900478 * d
      - 3.08 * sin(S6);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRECordeliaRotation::make_copy(void) const
{
  return new WGCCRECordeliaRotation(*this);
}

LQuaterniond WGCCRECordeliaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U1 = 115.75 * deg_to_rad + 54991.87 * deg_to_rad * T;

  double a0 = 257.31 - 0.15 * sin(U1);
  double d0 = - 15.18 + 0.14 * cos(U1);
  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRECordeliaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U1 = 115.75 * deg_to_rad + 54991.87 * deg_to_rad * T;
  double W = 127.69 - 1074.5205730 * d - 0.04 * sin(U1);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREOpheliaRotation::make_copy(void) const
{
  return new WGCCREOpheliaRotation(*this);
}

LQuaterniond WGCCREOpheliaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U2 = 141.69 * deg_to_rad + 41887.66 * deg_to_rad * T;

  double a0 = 257.31 - 0.09 * sin(U2);
  double d0 = - 15.18 + 0.09 * cos(U2);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREOpheliaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U2 = 141.69 * deg_to_rad + 41887.66 * deg_to_rad * T;

  double W = 130.35 - 956.4068150 * d - 0.03 * sin(U2);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREBiancaRotation::make_copy(void) const
{
  return new WGCCREBiancaRotation(*this);
}

LQuaterniond WGCCREBiancaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U3 = 135.03 * deg_to_rad + 29927.35 * deg_to_rad * T;

  double a0 = 257.31 - 0.16 * sin(U3);
  double d0 = - 15.18 + 0.16 * cos(U3);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREBiancaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U3 = 135.03 * deg_to_rad + 29927.35 * deg_to_rad * T;

  double W = 105.46 - 828.3914760 * d - 0.04 * sin(U3);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRECressidaRotation::make_copy(void) const
{
  return new WGCCRECressidaRotation(*this);
}

LQuaterniond WGCCRECressidaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U4 = 61.77 * deg_to_rad + 25733.59 * deg_to_rad * T;

  double a0 = 257.31 - 0.04 * sin(U4);
  double d0 = - 15.18 + 0.04 * cos(U4);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRECressidaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U4 = 61.77 * deg_to_rad + 25733.59 * deg_to_rad * T;

  double W = 59.16 - 776.5816320 * d - 0.01 * sin(U4);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREDesdemonaRotation::make_copy(void) const
{
  return new WGCCREDesdemonaRotation(*this);
}

LQuaterniond WGCCREDesdemonaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U5 = 249.32 * deg_to_rad + 24471.46 * deg_to_rad * T;

  double a0 = 257.31 - 0.17 * sin(U5);
  double d0 = - 15.18 + 0.16 * cos(U5);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREDesdemonaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U5 = 249.32 * deg_to_rad + 24471.46 * deg_to_rad * T;

  double W = 95.08 - 760.0531690 * d - 0.04 * sin(U5);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREJulietRotation::make_copy(void) const
{
  return new WGCCREJulietRotation(*this);
}

LQuaterniond WGCCREJulietRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U6 = 43.86 * deg_to_rad + 22278.41 * deg_to_rad * T;

  double a0 = 257.31 - 0.06 * sin(U6);
  double d0 = - 15.18 + 0.06 * cos(U6);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREJulietRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U6 = 43.86 * deg_to_rad + 22278.41 * deg_to_rad * T;

  double W = 302.56 - 730.1253660 * d - 0.02 * sin(U6);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREPortiaRotation::make_copy(void) const
{
  return new WGCCREPortiaRotation(*this);
}

LQuaterniond WGCCREPortiaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U7 = 77.66 * deg_to_rad + 20289.42 * deg_to_rad * T;

  double a0 = 257.31 - 0.09 * sin(U7);
  double d0 = - 15.18 + 0.09 * cos(U7);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREPortiaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U7 = 77.66 * deg_to_rad + 20289.42 * deg_to_rad * T;

  double W = 25.03 - 701.4865870 * d - 0.02 * sin(U7);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRERosalindRotation::make_copy(void) const
{
  return new WGCCRERosalindRotation(*this);
}

LQuaterniond WGCCRERosalindRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U8 = 157.36 * deg_to_rad + 16652.76 * deg_to_rad * T;

  double a0 = 257.31 - 0.29 * sin(U8);
  double d0 = - 15.18 + 0.28 * cos(U8);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRERosalindRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U8 = 157.36 * deg_to_rad + 16652.76 * deg_to_rad * T;

  double W = 314.90 - 644.6311260 * d - 0.08 * sin(U8);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREBelindaRotation::make_copy(void) const
{
  return new WGCCREBelindaRotation(*this);
}

LQuaterniond WGCCREBelindaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U9 = 101.81 * deg_to_rad + 12872.63 * deg_to_rad * T;

  double a0 = 257.31 - 0.03 * sin(U9);
  double d0 = - 15.18 + 0.03 * cos(U9);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREBelindaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U9 = 101.81 * deg_to_rad + 12872.63 * deg_to_rad * T;

  double W = 297.46 - 577.3628170 * d - 0.01 * sin(U9);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREPuckRotation::make_copy(void) const
{
  return new WGCCREPuckRotation(*this);
}

LQuaterniond WGCCREPuckRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U10 = 138.64 * deg_to_rad + 8061.81 * deg_to_rad * T;

  double a0 = 257.31 - 0.33 * sin(U10);
  double d0 = - 15.18 + 0.31 * cos(U10);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREPuckRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U10 = 138.64 * deg_to_rad + 8061.81 * deg_to_rad * T;

  double W = 91.24 - 472.5450690 * d - 0.09 * sin(U10);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREMirandaRotation::make_copy(void) const
{
  return new WGCCREMirandaRotation(*this);
}

LQuaterniond WGCCREMirandaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U11 = 102.23 * deg_to_rad - 2024.22 * deg_to_rad * T;

  double a0 = 257.43 + 4.41 * sin(U11) - 0.04 * sin(2 * U11);
  double d0 = - 15.08 + 4.25 * cos(U11) - 0.02 * cos(2 * U11);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREMirandaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U11 = 102.23 * deg_to_rad - 2024.22 * deg_to_rad * T;
  double U12 = 316.41 * deg_to_rad + 2863.96 * deg_to_rad * T;

  double W = 30.70 - 254.6906892 * d
      - 1.27 * sin(U12)
      + 0.15 * sin(2 * U12)
      + 1.15 * sin(U11)
      - 0.09 * sin(2 * U11);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREArielRotation::make_copy(void) const
{
  return new WGCCREArielRotation(*this);
}

LQuaterniond WGCCREArielRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U13 = 304.01 * deg_to_rad - 51.94 * deg_to_rad * T;

  double a0 = 257.43 + 0.29 * sin(U13);
  double d0 = - 15.10 + 0.28 * cos(U13);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREArielRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U12 = 316.41 * deg_to_rad + 2863.96 * deg_to_rad * T;
  double U13 = 304.01 * deg_to_rad - 51.94 * deg_to_rad * T;

  double W = 156.22 - 142.8356681 * d + 0.05 * sin(U12) + 0.08 * sin(U13);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREUmbrielRotation::make_copy(void) const
{
  return new WGCCREUmbrielRotation(*this);
}

LQuaterniond WGCCREUmbrielRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U14 = 308.71 * deg_to_rad - 93.17 * deg_to_rad * T;

  double a0 = 257.43 + 0.21 * sin(U14);
  double d0 = - 15.10 + 0.2 * cos(U14);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREUmbrielRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U12 = 316.41 * deg_to_rad + 2863.96 * deg_to_rad * T;
  double U14 = 308.71 * deg_to_rad - 93.17 * deg_to_rad * T;

  double W = 108.05 - 86.8688923 * d - 0.09 * sin(U12) + 0.06 * sin(U14);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRETitaniaRotation::make_copy(void) const
{
  return new WGCCRETitaniaRotation(*this);
}

LQuaterniond WGCCRETitaniaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U15 = 340.82 * deg_to_rad - 75.32 * deg_to_rad * T;

  double a0 = 257.43 + 0.29 * sin(U15);
  double d0 = - 15.10 + 0.28 * cos(U15);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRETitaniaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U15 = 340.82 * deg_to_rad - 75.32 * deg_to_rad * T;

  double W = 77.74 - 41.3514316 * d + 0.08 * sin(U15);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREOberonRotation::make_copy(void) const
{
  return new WGCCREOberonRotation(*this);
}

LQuaterniond WGCCREOberonRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double U16 = 259.14 * deg_to_rad - 504.81 * deg_to_rad * T;

  double a0 = 257.43 + 0.16 * sin(U16);
  double d0 = - 15.10 + 0.16 * cos(U16);

 return calc_orientation(a0, d0);
}

LQuaterniond WGCCREOberonRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double U16 = 259.14 * deg_to_rad - 504.81 * deg_to_rad * T;

  double W = 6.77 - 26.7394932 * d + 0.04 * sin(U16);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRENaiadRotation::make_copy(void) const
{
  return new WGCCRENaiadRotation(*this);
}

LQuaterniond WGCCRENaiadRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N1 = 323.92 * deg_to_rad + 62606.6 * deg_to_rad * T;

  double a0 = 299.36 + 0.70 * sin(N) - 6.49 * sin(N1) + 0.25 * sin(2 * N1);
  double d0 = 43.36 - 0.51 * cos(N) - 4.75 * cos(N1) + 0.09 * cos(2 * N1);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRENaiadRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N1 = 323.92 * deg_to_rad + 62606.6 * deg_to_rad * T;

  double W = 254.06 + 1222.8441209 * d - 0.48 * sin(N) + 4.40 * sin(N1) - 0.27 * sin(2 * N1);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREThalassaRotation::make_copy(void) const
{
  return new WGCCREThalassaRotation(*this);
}

LQuaterniond WGCCREThalassaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N2 = 220.51 * deg_to_rad + 55064.2 * deg_to_rad * T;

  double a0 = 299.36 + 0.70 * sin(N) - 0.28 * sin(N2);
  double d0 = 43.45 - 0.51 * cos(N) - 0.21 * cos(N2);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREThalassaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N2 = 220.51 * deg_to_rad + 55064.2 * deg_to_rad * T;

  double W = 102.06 + 1155.7555612 * d - 0.48 * sin(N) + 0.19 * sin(N2);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREDespinaRotation::make_copy(void) const
{
  return new WGCCREDespinaRotation(*this);
}

LQuaterniond WGCCREDespinaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N3 = 354.27 * deg_to_rad + 46564.5 * deg_to_rad * T;

  double a0 = 299.36 + 0.70 * sin(N) - 0.09 * sin(N3);
  double d0 = 43.45 - 0.51 * cos(N) - 0.07 * cos(N3);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREDespinaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N3 = 354.27 * deg_to_rad + 46564.5 * deg_to_rad * T;

  double W = 306.51 + 1075.7341562 * d - 0.49 * sin(N) + 0.06 * sin(N3);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREGalateaRotation::make_copy(void) const
{
  return new WGCCREGalateaRotation(*this);
}

LQuaterniond WGCCREGalateaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N4 = 75.31 * deg_to_rad + 26109.4 * deg_to_rad * T;

  double a0 = 299.36 + 0.70 * sin(N) - 0.07 * sin(N4);
  double d0 = 43.43 - 0.51 * cos(N) - 0.05 * cos(N4);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREGalateaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N4 = 75.31 * deg_to_rad + 26109.4 * deg_to_rad * T;

  double W = 258.09 + 839.6597686 * d - 0.48 * sin(N) + 0.05 * sin(N4);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRELarissaRotation::make_copy(void) const
{
  return new WGCCRELarissaRotation(*this);
}

LQuaterniond WGCCRELarissaRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N5 = 35.36 * deg_to_rad + 14325.4 * deg_to_rad * T;

  double a0 = 299.36 + 0.70 * sin(N) - 0.27 * sin(N5);
  double d0 = 43.41 - 0.51 * cos(N) - 0.20 * cos(N5);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRELarissaRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N5 = 35.36 * deg_to_rad + 14325.4 * deg_to_rad * T;

  double W = 179.41 + 649.0534470 * d - 0.48 * sin(N) + 0.19 * sin(N5);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCREProteusRotation::make_copy(void) const
{
  return new WGCCREProteusRotation(*this);
}

LQuaterniond WGCCREProteusRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N6 = 142.61 * deg_to_rad + 2824.6 * deg_to_rad * T;

  double a0 = 299.27 + 0.70 * sin(N) - 0.05 * sin(N6);
  double d0 = 42.91 - 0.51 * cos(N) - 0.04 * cos(N6);

  return calc_orientation(a0, d0);
}

LQuaterniond WGCCREProteusRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N = 357.85 * deg_to_rad + 52.316 * deg_to_rad * T;
  double N6 = 142.61 * deg_to_rad + 2824.6 * deg_to_rad * T;

  double W = 93.38 + 320.7654228 * d - 0.48 * sin(N) + 0.04 * sin(N6);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}

PT(RotationBase) WGCCRETritonRotation::make_copy(void) const
{
  return new WGCCRETritonRotation(*this);
}

LQuaterniond WGCCRETritonRotation::calc_frame_equatorial_orientation_at(
    double time)
{
  double T = get_T(time);

  double N7 = 177.85 * deg_to_rad + 52.316 * deg_to_rad * T;

  double a0 = 299.36 - 32.35 * sin(N7)
      - 6.28 * sin(2 * N7) - 2.08 * sin(3 * N7)
      - 0.74 * sin(4 * N7) - 0.28 * sin(5 * N7)
      - 0.11 * sin(6 * N7) - 0.07 * sin(7 * N7)
      - 0.02 * sin(8 * N7) - 0.01 * sin(9 * N7);
  double d0 = 41.17 + 22.55 * cos(N7)
      + 2.10 * cos(2 * N7) + 0.55 * cos(3 * N7)
      + 0.16 * cos(4 * N7) + 0.05 * cos(5 * N7)
      + 0.02 * cos(6 * N7) + 0.01 * cos(7 * N7);
  return calc_orientation(a0, d0);
}

LQuaterniond WGCCRETritonRotation::calc_frame_rotation_at(double time)
{
  double d = time - j2000_epoch;
  double T = get_T(time);

  double N7 = 177.85 * deg_to_rad + 52.316 * deg_to_rad * T;

  double W = 296.53 - 61.2572637 * d + 22.25 * sin(N7)
      + 6.73 * sin(2 * N7) + 2.05 * sin(3 * N7)
      + 0.74 * sin(4 * N7) + 0.28 * sin(5 * N7)
      + 0.11 * sin(6 * N7) + 0.05 * sin(7 * N7)
      + 0.02 * sin(8 * N7) + 0.01 * sin(9 * N7);

  LQuaterniond local;
  local.set_from_axis_angle_rad(W * deg_to_rad, LVector3d::unit_z());
  LQuaterniond rotation = local * get_frame_equatorial_orientation_at(time);
  return rotation;
}
