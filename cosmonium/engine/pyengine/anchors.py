#
#This file is part of Cosmonium.
#
#Copyright (C) 2018-2019 Laurent Deru.
#
#Cosmonium is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.
#
#Cosmonium is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Cosmonium.  If not, see <https://www.gnu.org/licenses/>.
#


from panda3d.core import LPoint3d, LVector3d, LQuaterniond, LColor

from ..octree import OctreeNode
from ...astro import units
from ...astro.astro import abs_to_app_mag, app_to_abs_mag, abs_mag_to_lum, lum_to_abs_mag
from ...astro.frame import AbsoluteReferenceFrame
from ... import utils

from ... import settings

from math import sqrt, asin, pi
from time import time

class AnchorBase():
    def __init__(self, anchor_class, body):
        self.content = anchor_class
        self.body = body
        self.parent = None
        self.rebuild_needed = False
        #Flags
        self.was_visible = False
        self.visible = False
        self.visibility_override = False
        self.was_resolved = False
        self.resolved = False
        self.update_id = 0
        self.update_frozen = False
        self.force_update = False
        #Cached values
        self._position = LPoint3d()
        self._global_position = LPoint3d()
        self._local_position = LPoint3d()
        self._orientation = LQuaterniond()
        self.bounding_radius = 0.0
        self._height_under = 0.0
        #Scene parameters
        self.rel_position = LPoint3d()
        self.distance_to_obs = 0
        self.vector_to_obs = LVector3d()
        self.visible_size = 0.0
        self.z_distance = 0.0

    def set_rebuild_needed(self):
        self.rebuild_needed = True
        if self.parent is not None:
            self.parent.set_rebuild_needed()

    def rebuild(self):
        pass

    def traverse(self, visitor):
        visitor.traverse_anchor(self)

    def get_bounding_radius(self):
        return self.bounding_radius

    def set_bounding_radius(self, bounding_radius):
        self.bounding_radius = bounding_radius

    def get_apparent_radius(self):
        return self.get_bounding_radius()

    def calc_absolute_relative_position_to(self, position):
        return (self.get_absolute_reference_point() - position) + self.get_local_position()

    def calc_absolute_relative_position(self, anchor):
        reference_point_delta = anchor.get_absolute_reference_point() - self.get_absolute_reference_point()
        local_delta = anchor.get_local_position() - self.get_local_position()
        delta = reference_point_delta + local_delta
        return delta

    def calc_local_distance_to(self, anchor):
        local_delta = anchor.get_local_position() - self.get_local_position()
        length = local_delta.length()
        return (local_delta / length, length)

    def update(self, time, update_id):
        pass

    def update_observer(self, observer, update_id):
        if self.update_id == update_id: return
        global_delta = self._global_position - observer._global_position
        local_delta = self._local_position - observer._local_position
        rel_position = global_delta + local_delta
        distance_to_obs = rel_position.length()
        vector_to_obs = -rel_position / distance_to_obs
        if distance_to_obs > 0.0:
            vector_to_obs = -rel_position / distance_to_obs
            visible_size = self.bounding_radius / (distance_to_obs * observer.pixel_size)
            coef = -vector_to_obs.dot(observer.camera_vector)
            self.z_distance = distance_to_obs * coef
        else:
            vector_to_obs = LVector3d()
            visible_size = 0.0
            self.z_distance = 0.0
        radius = self.bounding_radius
        if distance_to_obs > radius:
            in_view = observer.rel_frustum.is_sphere_in(rel_position, radius)
            resolved = visible_size > settings.min_body_size
            visible = in_view# and (visible_size > 1.0 or self._app_magnitude < settings.lowest_app_magnitude)
        else:
            #We are in the object
            resolved = True
            visible = True
        self.rel_position = rel_position
        self.vector_to_obs = vector_to_obs
        self.distance_to_obs = distance_to_obs
        self.was_visible = self.visible
        self.was_resolved = self.resolved
        self.visible = visible
        self.resolved = resolved
        self.visible_size = visible_size

    def update_and_update_observer(self, time, observer, update_id):
        self.update(time, update_id)
        self.update_observer(observer, update_id)

    def update_app_magnitude(self, star):
        pass


class CartesianAnchor(AnchorBase):
    def __init__(self, anchor_class, body, frame):
        AnchorBase.__init__(self, anchor_class, body)
        self.frame = frame
        self._frame_position = LPoint3d()
        self._frame_orientation = LQuaterniond()

    def copy(self, other):
        self.frame = other.get_frame()
        self._global_position = other.get_absolute_reference_point()
        self._frame_position = other.get_frame_position()
        self._frame_orientation = other.get_frame_orientation()

    def get_frame(self):
        return self.frame

    def set_frame(self, frame):
        #Get position and rotation in the absolute reference frame
        pos = self.get_local_position()
        rot = self.get_absolute_orientation()
        #Update reference frame
        self.frame = frame
        #Set back the position to calculate the position in the new reference frame
        self.set_local_position(pos)
        self.set_absolute_orientation(rot)

    def do_update(self):
        #TODO: _position should be global + local !
        self._position = self.get_local_position()
        self._local_position = self.get_local_position()
        self._orientation = self.get_absolute_orientation()

    def update(self, time, dt):
        self.do_update()

    def get_position_bounding_radius(self):
        return 0.0

    def set_absolute_reference_point(self, new_reference_point):
        old_local = self.frame.get_local_position(self._frame_position)
        new_local = (self._global_position - new_reference_point) + old_local
        self._global_position = new_reference_point
        self._frame_position = self.frame.get_frame_position(new_local)
        self.do_update()

    def set_frame_position(self, position):
        self._frame_position = position

    def get_frame_position(self):
        return self._frame_position

    def set_frame_orientation(self, rotation):
        self._frame_orientation = rotation

    def get_frame_orientation(self):
        return self._frame_orientation

    def get_local_position(self):
        return self.frame.get_local_position(self._frame_position)

    def set_local_position(self, position):
        self._frame_position = self.frame.get_frame_position(position)

    def get_absolute_reference_point(self):
        return self._global_position

    def get_absolute_position(self):
        return self._global_position + self.get_local_position()

    def set_absolute_position(self, position):
        position -= self._global_position
        self._frame_position = self.frame.get_frame_position(position)

    def get_absolute_orientation(self):
        return self.frame.get_absolute_orientation(self._frame_orientation)

    def set_absolute_orientation(self, orientation):
        self._frame_orientation = self.frame.get_frame_orientation(orientation)

    def calc_absolute_position_of(self, frame_position):
        return self._global_position + self.frame.get_local_position(frame_position)

    def calc_relative_position_to(self, position):
        return (self._global_position - position) + self.get_local_position()

    def calc_frame_position_of_absolute(self, position):
        return self.frame.get_frame_position(position - self._global_position)

    def calc_frame_position_of_local(self, position):
        return self.frame.get_frame_position(position)

    def calc_frame_orientation_of(self, orientation):
        return self.frame.get_frame_orientation(orientation)

    def calc_look_at2(self, target, rel=True, position=None):
        if not rel:
            if position is None:
                position = self.get_pos()
            direction = LVector3d(target - position)
        else:
            direction = LVector3d(target)
        direction.normalize()
        local_direction = self.get_absolute_orientation().conjugate().xform(direction)
        angle = LVector3d.forward().angleRad(local_direction)
        axis = LVector3d.forward().cross(local_direction)
        if axis.length() > 0.0:
            new_rot = utils.relative_rotation(self.get_absolute_orientation(), axis, angle)
#         new_rot=LQuaterniond()
#         lookAt(new_rot, direction, LVector3d.up())
        else:
            new_rot = self.get_absolute_orientation()
        return new_rot, angle


class CameraAnchor(CartesianAnchor):
    def __init__(self, body, frame):
        CartesianAnchor.__init__(self, 0, body, frame)
        self.camera_vector = LVector3d()
        self.frustum = None
        self.rel_frustum = None
        self.pixel_size = 0.0

    def do_update(self):
        CartesianAnchor.do_update(self)
        self.camera_vector = self.get_absolute_orientation().xform(LVector3d.forward())


class OriginAnchor(CartesianAnchor):
    def __init__(self, anchor_class, body):
        CartesianAnchor.__init__(self, anchor_class, body, AbsoluteReferenceFrame())

class FlatSurfaceAnchor(OriginAnchor):
    def __init__(self, anchor_class, body, surface):
        OriginAnchor.__init__(self, anchor_class, body)
        self.surface = surface

    def set_surface(self, surface):
        self.surface = surface

    def update_observer(self, observer, update_id):
        if self.update_id == update_id: return
        self.vector_to_obs = LPoint3d(observer.get_local_position())
        self.vector_to_obs.normalize()
        observer_local_position = observer.get_local_position()
        self.distance_to_obs = observer_local_position.get_z()# - self.get_height(self.observer._local_position)
        self._height_under = self.surface.get_height_at(observer_local_position[0], observer_local_position[1])
        self.rel_position = self._local_position - observer_local_position
        self.was_visible = self.visible
        self.was_resolved = self.resolved
        self.visible_size = 0.0
        self.z_distance = 0.0
        self.visible = True
        self.resolved = True

class ObserverAnchor(CartesianAnchor):
    def __init__(self, anchor_class, body):
        CartesianAnchor.__init__(self, anchor_class, body, AbsoluteReferenceFrame())

    def update(self, time, update_id):
        #TODO: This anchor should be updated by the Observer Class, now only the ObserverSceneAnchor is valid
        pass

    def update_observer(self, observer, update_id):
        if self.update_id == update_id: return
        self.copy(observer)
        self.was_visible = self.visible
        self.was_resolved = self.resolved
        self.rel_position = LPoint3d()
        self.distance_to_obs = 0
        self.vector_to_obs = LVector3d()
        self.visible_size = 0.0
        self.z_distance = 0.0
        self.visible = True
        self.resolved = True


class ControlledCartesianAnchor(CartesianAnchor):
    def update(self, time, update_id):
        pass


class StellarAnchor(AnchorBase):
    Emissive   = 1
    Reflective = 2
    System     = 4
    def __init__(self, anchor_class, body, orbit, rotation, point_color):
        AnchorBase.__init__(self, anchor_class, body)
        #TODO: To remove
        if point_color is None:
            point_color = LColor(1.0, 1.0, 1.0, 1.0)
        self.point_color = point_color
        self.orbit = orbit
        self.rotation = rotation
        self._abs_magnitude = 1000.0
        self._app_magnitude = 1000.0
        self._equatorial = LQuaterniond()
        self._albedo = 0.5
        #TODO: Should be done properly
        #orbit.body = body
        #rotation.body = body

    def get_position_bounding_radius(self):
        return self.orbit.get_bounding_radius()

    def get_absolute_reference_point(self):
        return self._global_position

    def get_absolute_position(self):
        return self._global_position + self._local_position

    def get_local_position(self):
        return self._local_position

    def get_absolute_orientation(self):
        return self._orientation

    def get_equatorial_rotation(self):
        return self._equatorial

    def get_sync_rotation(self):
        return self._orientation

    def get_absolute_magnitude(self):
        return self._abs_magnitude

    def get_apparent_magnitude(self):
        return self._app_magnitude

    def update(self, time, update_id):
        if self.update_id == update_id: return
        self._orientation = self.rotation.get_absolute_rotation_at(time)
        self._equatorial = self.rotation.get_equatorial_orientation_at(time)
        self._local_position = self.orbit.get_local_position_at(time)
        self._global_position = self.orbit.get_absolute_reference_point_at(time)
        self._position = self._global_position + self._local_position

    def get_luminosity(self, star):
        vector_to_star = self.calc_absolute_relative_position(star)
        distance_to_star = vector_to_star.length()
        vector_to_star /= distance_to_star
        star_power = abs_mag_to_lum(star._abs_magnitude)
        area = 4 * pi * distance_to_star * distance_to_star * 1000 * 1000 # Units are in km
        if area > 0.0:
            irradiance = star_power / area
            surface = pi * self.bounding_radius * self.bounding_radius * 1000 * 1000 # Units are in km
            received_energy = irradiance * surface
            reflected_energy = received_energy * self._albedo
            phase_angle = self.vector_to_obs.dot(vector_to_star)
            fraction = (1.0 + phase_angle) / 2.0
            return reflected_energy * fraction
        else:
            print("No area", self.body.get_name())
            return 0.0

    def update_app_magnitude(self, star):
        #TODO: Should be done by inheritance !
        if self.distance_to_obs == 0:
            self._app_magnitude = 1000.0
            return
        if hasattr(self, 'primary') and self.primary is not None:
            self.primary.update_app_magnitude(star)
            self._app_magnitude = self.primary._app_magnitude
            self._abs_magnitude = self.primary._abs_magnitude
        elif self.content & self.Emissive != 0:
            self._app_magnitude = abs_to_app_mag(self._abs_magnitude, self.distance_to_obs)
        elif self.content & self.Reflective != 0:
            if star is not None:
                reflected = self.get_luminosity(star)
                if reflected > 0:
                    self._app_magnitude = abs_to_app_mag(lum_to_abs_mag(reflected), self.distance_to_obs)
                else:
                    self._app_magnitude = 1000.0
            else:
                self._app_magnitude = 1000.0
        else:
            self._app_magnitude = abs_to_app_mag(self._abs_magnitude, self.distance_to_obs)

class FixedStellarAnchor(StellarAnchor):
    def __init__(self, body, orbit, rotation, point_color):
        StellarAnchor.__init__(self, body, orbit, rotation, point_color)
        #self.update_frozen = True
        #self.update(0)

class DynamicStellarAnchor(StellarAnchor):
    pass

class SystemAnchor(DynamicStellarAnchor):
    def __init__(self, body, orbit, rotation, point_color):
        DynamicStellarAnchor.__init__(self, self.System, body, orbit, rotation, point_color)
        self.primary = None
        self.children = []

    def set_primary(self, primary):
        self.primary = primary

    def add_child(self, child):
        #Primary is still managed by StellarSystem
        self.children.append(child)
        child.parent = self
        if not self.rebuild_needed:
            self.set_rebuild_needed()

    def remove_child(self, child):
        try:
            self.children.remove(child)
            child.parent = None
        except ValueError:
            pass
        if not self.rebuild_needed:
            self.set_rebuild_needed()

    def rebuild(self):
        content = self.System
        bounding_radius = 0
        for child in self.children:
            if child.rebuild_needed:
                child.rebuild()
            content |= child.content
            farthest_distance = child.get_position_bounding_radius() + child.bounding_radius
            if farthest_distance > bounding_radius:
                bounding_radius = farthest_distance
        self.content = content
        self.bounding_radius = bounding_radius
        luminosity = 0.0
        if self.primary is None:
            for child in self.children:
                #TODO: We should instead check if the child is emissive or not
                if child._abs_magnitude is not None:
                    luminosity += abs_mag_to_lum(child._abs_magnitude)
            if luminosity > 0.0:
                self._abs_magnitude = lum_to_abs_mag(luminosity)
            else:
                self._abs_magnitude = 1000.0
        else:
            self._abs_magnitude = self.primary._abs_magnitude
        self.rebuild_needed = False

    def traverse(self, visitor):
        if visitor.enter_system(self):
            visitor.traverse_system(self)

class OctreeAnchor(SystemAnchor):
    def __init__(self, body, orbit, rotation, point_color):
        SystemAnchor.__init__(self, body, orbit, rotation, point_color)
        #TODO: Turn this into a parameter or infer it from the children
        self.bounding_radius = 100000.0 * units.Ly
        #TODO: Should be configurable
        abs_mag = app_to_abs_mag(6.0, self.bounding_radius * sqrt(3))
        #TODO: position should be extracted from orbit
        self.octree = OctreeNode(0, self,
                             LPoint3d(10 * units.Ly, 10 * units.Ly, 10 * units.Ly),
                             self.bounding_radius,
                             abs_mag)
        #TODO: Right now an octree contains anything
        self.content = ~0
        self.recreate_octree = True

    def rebuild(self):
        if self.recreate_octree:
            self.create_octree()
        if self.octree.rebuild_needed:
            self.octree.rebuild()
        self.rebuild_needed = False

    def traverse(self, visitor):
        if visitor.enter_octree_node(self.octree):
            self.octree.traverse(visitor)

    def create_octree(self):
        print("Creating octree...")
        start = time()
        for child in self.children:
            #TODO: this should be done properly at anchor creation
            child.update(0, None)
            child.rebuild()
            self.octree.add(child)
        end = time()
        print("Creation time:", end - start)

class UniverseAnchor(OctreeAnchor):
    def __init__(self, body, orbit, rotation, point_color):
        OctreeAnchor.__init__(self, body, orbit, rotation, point_color)
        self.visible = True
        self.resolved = True

    def traverse(self, visitor):
        self.octree.traverse(visitor)
