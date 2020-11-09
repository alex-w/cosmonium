/*
 * This file is part of Cosmonium.
 *
 * Copyright (C) 2018-2019 Laurent Deru.
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

#ifndef OCTREE_RENDERER_H
#define OCTREE_RENDERER_H

#include "pandabase.h"
#include "luse.h"
#include "referenceCount.h"

class OctreeRenderer : public ReferenceCount
{
PUBLISHED:
  OctreeRenderer();
  ~OctreeRenderer(void);
};

#endif //OCTREE_RENDERER_H
