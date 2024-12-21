/*
 * This file is part of Cosmonium.
 *
 * Copyright (C) 2018-2024 Laurent Deru.
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

#ifndef PATCH_BOUNDING_BOX_H
#define PATCH_BOUNDING_BOX_H

#include "pandabase.h"
#include "referenceCount.h"
#include "pta_LVecBase3.h"

class BoundingBox;


class PatchBoundingBox : public ReferenceCount
{
PUBLISHED:
    PatchBoundingBox(const PTA_LVecBase3d &points);
    virtual ~PatchBoundingBox(void);

    BoundingBox *create_bounding_volume(LQuaterniond rot, LVector3d offset);
    void xform(LMatrix3d mat);

protected:
    PTA_LVecBase3d points;
};

#endif //PATCH_BOUNDING_BOX_H
