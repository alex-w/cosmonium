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


from panda3d.core import LPoint3d, LVector3d, LVector3, LColor

from .foundation import CompositeObject, ObjectLabel
from .namedobject import NamedObject
from .annotations import ReferenceAxis, RotationAxis, Orbit
from .anchors import FixedStellarAnchor, DynamicStellarAnchor
from .sceneanchor import SceneAnchor
from .astro.frame import SynchroneReferenceFrame
from .astro.orbits import FixedPosition
from .astro.astro import abs_to_app_mag
from .bodyclass import bodyClasses
from .catalogs import objectsDB
from .parameters import ParametersGroup
from . import settings
from .utils import srgb_to_linear

from math import pi, asin, atan2, sin, cos

class StellarBodyLabel(ObjectLabel):
    def create_instance(self):
        ObjectLabel.create_instance(self)
        if settings.color_picking and self.label_source.oid_color is not None:
            self.instance.set_shader_input("color_picking", self.label_source.oid_color)

    def check_visibility(self, frustum, pixel_size):
        if hasattr(self.label_source, "primary") and self.label_source.anchor.resolved:
            self.visible = False
            return
        if self.label_source.system is not None:
            body = self.label_source.system
        else:
            body = self.label_source
        if self.label_source.anchor.visible and self.label_source.anchor.resolved:
            self.visible = True
            self.fade = 1.0
        else:
            if body.anchor.distance_to_obs > 0.0:
                size = body.anchor.orbit.get_apparent_radius() / (body.anchor.distance_to_obs * pixel_size)
                self.visible = size > settings.label_fade
                self.fade = min(1.0, max(0.0, (size - settings.orbit_fade) / settings.orbit_fade))
        self.fade = clamp(self.fade, 0.0, 1.0)

    def update_instance(self, camera_pos, camera_rot):
        body = self.label_source
        if body.is_emissive() and (not body.anchor.resolved or body.background):
            if body.scene_anchor.scene_position != None:
                self.instance.setPos(*body.scene_anchor.scene_position)
                scale = abs(self.context.observer.pixel_size * body.get_label_size() * body.scene_anchor.scene_distance)
            else:
                scale = 0.0
        else:
            offset = body.get_apparent_radius() * 1.01
            rel_front_pos = body.anchor.rel_position - camera_rot.xform(LPoint3d(0, offset, 0))
            vector_to_obs = LVector3d(-rel_front_pos)
            distance_to_obs = vector_to_obs.length()
            vector_to_obs /= distance_to_obs
            position, distance, scale_factor = self.label_source.scene_anchor.calc_scene_params(rel_front_pos, rel_front_pos, distance_to_obs, vector_to_obs)
            self.instance.setPos(*position)
            scale = abs(self.context.observer.pixel_size * body.get_label_size() * distance)
        self.look_at.set_pos(LVector3(*(camera_rot.xform(LVector3d.forward()))))
        self.label_instance.look_at(self.look_at, LVector3(), LVector3(*(camera_rot.xform(LVector3d.up()))))
        self.instance.set_color_scale(LColor(self.fade, self.fade, self.fade, 1.0))
        if scale < 1e-7:
            print("Label too far", self.get_name(), scale)
            scale = 1e-7
        self.instance.setScale(scale)

class FixedOrbitLabel(StellarBodyLabel):
    def check_visibility(self, frustum, pixel_size):
        #TODO: Should be refactored !
        if hasattr(self.label_source, "primary") and self.label_source.anchor.resolved and (self.label_source.primary is None or (self.label_source.primary.label is not None and self.label_source.primary.label.visible)):
            self.visible = False
            return
        self.visible = self.label_source.anchor._app_magnitude < settings.label_lowest_app_magnitude
        self.fade = 0.2 + (settings.label_lowest_app_magnitude - self.label_source.anchor._app_magnitude) / (settings.label_lowest_app_magnitude - settings.max_app_magnitude)
        self.fade = clamp(self.fade, 0.0, 1.0)

class StellarObject(NamedObject):
    context = None
    anchor_class = 0
    has_rotation_axis = False
    has_reference_axis = False
    has_orbit = True
    has_halo = False
    has_resolved_halo = False
    virtual_object = False
    support_offset_body_center = True
    background = False
    nb_update = 0
    nb_obs = 0
    nb_visibility = 0
    nb_instance = 0

    def __init__(self, names, source_names, orbit=None, rotation=None, body_class=None, point_color=None, description=''):
        NamedObject.__init__(self, names, source_names, description)
        self.system = None
        self.body_class = body_class
        if point_color is None:
            point_color = LColor(1.0, 1.0, 1.0, 1.0)
        point_color = srgb_to_linear(point_color)
        #if not (orbit.dynamic or rotation.dynamic):
        #    self.anchor = FixedStellarAnchor(self, orbit, rotation, point_color)
        #else:
        self.anchor = self.create_anchor(self.anchor_class, orbit, rotation, point_color)
        self.scene_anchor = SceneAnchor(self.anchor, self.support_offset_body_center)
        self.abs_magnitude = 99.0
        self.oid = None
        self.oid_color = None
        #Flags
        self.selected = False
        #Scene parameters
        self.light_color = (1.0, 1.0, 1.0, 1.0)
        #Components
        self.orbit_object = None
        self.rotation_axis = None
        self.reference_axis = None
        self.init_components = False
        objectsDB.add(self)

        self.shown = True
        self.visible = False
        self.parent = None
        self.lights = None

        self.components = CompositeObject(self.get_ascii_name())

    def set_parent(self, parent):
        self.parent = parent

    def set_lights(self, lights):
        if self.lights is not None:
            self.lights.remove_all()
        self.lights = lights
        self.components.set_lights(lights)

    def create_anchor(self, anchor_class, orbit, rotation, point_color):
        return DynamicStellarAnchor(anchor_class, self, orbit, rotation, point_color)

    def is_system(self):
        return False

    def check_settings(self):
        self.components.check_settings()
        if self.label is not None:
            self.label.check_settings()
        if self.body_class is None:
            print("No class for", self.get_name())
            return
        self.set_shown(bodyClasses.get_show(self.body_class))

    def set_shown(self, new_shown_status):
        if new_shown_status != self.shown:
            if new_shown_status:
                self.show()
            else:
                self.hide()

    def show(self):
        self.shown = True

    def hide(self):
        self.shown = False

    def get_user_parameters(self):
        group = ParametersGroup(self.get_name())
        parameters = []
        if isinstance(self.orbit, FixedPosition) and self.system is not None:
            orbit = self.system.orbit
        else:
            orbit = self.orbit
        general_group = ParametersGroup(_('General'))
        general_group.add_parameter(orbit.get_user_parameters())
        general_group.add_parameter(self.rotation.get_user_parameters())
        group.add_parameter(general_group)
        for component in self.components:
            component_group = component.get_user_parameters()
            if component_group is not None:
                group.add_parameter(component_group)
        return group

    def update_user_parameters(self):
        self.components.update_user_parameters()
        if isinstance(self.orbit, FixedPosition) and self.system is not None:
            self.system.orbit.update_user_parameters()
            if self.system.orbit_object is not None:
                self.system.orbit_object.update_user_parameters()
        else:
            self.orbit.update_user_parameters()
            if self.orbit_object is not None:
                self.orbit_object.update_user_parameters()
        self.rotation.update_user_parameters()

    def get_fullname(self, separator='/'):
        if hasattr(self, "primary") and self.primary is not None:
            name = self.primary.get_friendly_name()
        else:
            name = self.get_friendly_name()
        if not hasattr(self.parent, "primary") or self.parent.primary is not self:
            fullname = self.parent.get_fullname(separator)
        else:
            fullname = self.parent.parent.get_fullname(separator)
        if fullname != '':
            return fullname + separator + name
        else:
            return name

    def create_label_instance(self):
        if isinstance(self.anchor.orbit, FixedPosition):
            return FixedOrbitLabel(self.get_ascii_name() + '-label', self)
        else:
            return StellarBodyLabel(self.get_ascii_name() + '-label', self)

    def get_description(self):
        return self.description

    def create_components(self):
        if self.has_rotation_axis:
            self.rotation_axis = RotationAxis(self)
            self.components.add_component(self.rotation_axis)
        if self.has_reference_axis:
            self.reference_axis = ReferenceAxis(self)
            self.components.add_component(self.reference_axis)
        self.init_components = True

    def update_components(self, camera_pos):
        pass

    def remove_components(self):
        self.components.remove_component(self.rotation_axis)
        self.rotation_axis = None
        self.components.remove_component(self.reference_axis)
        self.reference_axis = None
        self.init_components = False

    def set_system(self, system):
        self.system = system

    def set_body_class(self, body_class):
        self.body_class = body_class

    def create_orbit_object(self):
        if self.orbit_object is None and not isinstance(self.anchor.orbit, FixedPosition):
            self.orbit_object = Orbit(self)
            self.components.add_component(self.orbit_object)
            self.orbit_object.check_settings()

    def set_orbit(self, orbit):
        if self.orbit_object is not None:
            self.orbit_object.remove_instance()
            self.orbit_object = None
            self.components.remove_component(self.orbit_object)
        self.anchor.orbit = orbit
        self.create_orbit_object()

    def set_rotation(self, rotation):
        self.anchor.rotation = rotation

    def find_by_name(self, name, name_up=None):
        if self.is_named(name, name_up):
            return self
        else:
            return None

    def is_named(self, name, name_up=None):
        if name_up is None:
            name_up = name.upper()
        for name in self.names:
            if name.upper() == name_up:
                return True
        for name in self.source_names:
            if name.upper() == name_up:
                return True
        return False

    def set_selected(self, selected):
        self.selected = selected
        if self.orbit_object:
            self.orbit_object.set_selected(selected)
        else:
            if self.parent:
                self.parent.set_selected(selected)

    def is_emissive(self):
        return False

    def get_label_color(self):
        return bodyClasses.get_label_color(self.body_class)

    def get_label_size(self):
        return settings.label_size

    def get_label_text(self):
        return self.get_name()

    def get_orbit_color(self):
        return bodyClasses.get_orbit_color(self.body_class)

    def get_apparent_radius(self):
        return 0

    def get_extend(self):
        return self.get_apparent_radius()

    def get_abs_magnitude(self):
        return self.anchor.get_absolute_magnitude()

    def get_app_magnitude(self):
        return self.anchor.get_apparent_magnitude()

    def get_global_position(self):
        return self.anchor._global_position

    def get_local_position(self):
        return self.anchor._local_position

    def get_position(self):
        return self.get_global_position() + self.get_local_position()

    def get_rel_position_to(self, position):
        return (self.get_global_position() - position) + self.get_local_position()

    def get_abs_rotation(self):
        return self.anchor._orientation

    def get_equatorial_rotation(self):
        return self.anchor._equatorial

    def get_sync_rotation(self):
        return self.anchor._orientation

    def frame_cartesian_to_spherical(self, position):
        distance = position.length()
        if distance > 0:
            theta = asin(position[2] / distance)
            if position[0] != 0.0:
                phi = atan2(position[1], position[0])
                #Offset phi by 180 deg with proper wrap around
                #phi = (phi + pi + pi) % (2 * pi) - pi
            else:
                phi = 0.0
        else:
            phi = 0.0
            theta = 0.0
        return (phi, theta, distance)

    def cartesian_to_spherical(self, position):
        sync_frame = SynchroneReferenceFrame(self.anchor)
        rel_position = sync_frame.get_frame_position(position)
        return self.frame_cartesian_to_spherical(rel_position)

    def spherical_to_frame_cartesian(self, position):
        (phi, theta, distance) = position
        #Offset phi by 180 deg with proper wrap around
        #phi = (phi + pi + pi) % (2 * pi) - pi
        rel_position = LPoint3d(cos(theta) * cos(phi), cos(theta) * sin(phi), sin(theta))
        rel_position *= distance
        return rel_position

    def spherical_to_cartesian(self, position):
        rel_position = self.spherical_to_frame_cartesian(position)
        sync_frame = SynchroneReferenceFrame(self.anchor)
        position = sync_frame.get_local_position(rel_position)
        return position

    def spherical_to_xy(self, position):
        (phi, theta, distance) = position
        x = phi / pi / 2 + 0.5
        y = theta / pi + 0.5
        return (x, y, distance)

    def xy_to_spherical(self, position):
        (x, y, distance) = position
        phi = (x - 0.5) * pi / 2
        tetha = (y - 0.5) * pi
        return (phi, tetha, distance)

    def calc_global_distance_to(self, position):
        direction = self.get_position() - position
        length = direction.length()
        return (direction / length, length)

    def calc_local_distance_to(self, position):
        direction = position - self.get_local_position()
        length = direction.length()
        return (direction / length, length)

    def get_height_under(self, position):
        return self.get_apparent_radius()

    def set_visibility_override(self, override):
        if override == self.anchor.visibility_override: return
        if override:
            self.anchor.visibility_override = True
            if self.system is not None:
                self.system.set_visibility_override(override)
        else:
            self.anchor.visibility_override = False
            if self.system is not None:
                self.system.set_visibility_override(override)
            #Force recheck of visibility or the object will be instanciated in create_or_update_instance()
            self.check_visibility(self.context.observer.frustum, self.context.observer.pixel_size)

    def update_obs(self, observer):
        self.components.update_obs(observer)

    def check_visibility(self, frustum, pixel_size):
        self.components.check_visibility(frustum, pixel_size)

    def check_and_update_instance(self, camera_pos, camera_rot):
        StellarObject.nb_instance += 1
        if not self.init_components:
            self.scene_anchor.create_instance()
            self.scene_anchor.update()
            self.create_components()
            self.components.visible = True
            self.check_settings()
        if self.lights is not None:
            self.lights.update_instances(camera_pos)
        self.update_components(camera_pos)
        self.components.check_and_update_instance(camera_pos, camera_rot)

    def remove_instance(self):
        self.components.remove_instance()
        self.scene_anchor.remove_instance()
        if self.init_components:
            self.remove_components()
        self.remove_label()

    def show_rotation_axis(self):
        if self.rotation_axis:
            self.rotation_axis.show()

    def hide_rotation_axis(self):
        if self.rotation_axis:
            self.rotation_axis.hide()

    def toggle_rotation_axis(self):
        if self.rotation_axis:
            self.rotation_axis.toggle_shown()

    def show_reference_axis(self):
        if self.reference_axis:
            self.reference_axis.show()

    def hide_reference_axis(self):
        if self.reference_axis:
            self.reference_axis.hide()

    def toggle_reference_axis(self):
        if self.reference_axis:
            self.reference_axis.toggle_shown()

    def show_orbit(self):
        if self.orbit_object:
            self.orbit_object.show()

    def hide_orbit(self):
        if self.orbit_object:
            self.orbit_object.hide()

    def toggle_orbit(self):
        if self.orbit_object:
            self.orbit_object.toggle_shown()
