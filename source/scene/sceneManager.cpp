/*
 * This file is part of Cosmonium.
 *
 * Copyright (C) 2018-2022 Laurent Deru.
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


#include "sceneManager.h"


TypeHandle SceneManager::_type_handle;


bool SceneManager::inverse_z = false;
double SceneManager::default_near_plane = 1.0;
bool SceneManager::infinite_far_plane = true;
double SceneManager::default_far_plane = 30000.0;
double SceneManager::infinite_plane = 100000.0;
bool SceneManager::auto_infinite_plane = false;
double SceneManager::lens_far_limit = 1e-12;


SceneManager::SceneManager(void) :
    scale(1.0),
    mid_plane(0.0)
{
}


SceneManager::~SceneManager(void)
{
}

double
SceneManager::get_scale(void)
{
  return scale;
}

void
SceneManager::set_scale(double scale)
{
  this->scale = scale;
}

double
SceneManager::get_mid_plane(void)
{
  return mid_plane;
}

void
SceneManager::set_mid_plane(double scale)
{
  this->mid_plane = mid_plane;
}
