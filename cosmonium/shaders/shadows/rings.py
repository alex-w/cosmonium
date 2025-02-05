#
#This file is part of Cosmonium.
#
#Copyright (C) 2018-2022 Laurent Deru.
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


from .base import ShaderShadow

class ShaderRingsShadow(ShaderShadow):
    use_vertex = True
    use_vertex_frag = True
    world_vertex = True
    model_vertex = True
    def get_id(self):
        return 'rs'

    def fragment_uniforms(self, code):
        code.append("uniform sampler2D shadow_ring_tex;")
        code.append("uniform vec3 ring_normal;")
        code.append("uniform float ring_inner_radius;")
        code.append("uniform float ring_outer_radius;")
        code.append("uniform vec3 body_center;")

    def fragment_shader(self, code):
        #Simple line-plane intersection:
        #line is surface of the planet to the center of the light source
        #plane is the plane of the rings system
        code.append("vec3 new_pos = world_vertex - body_center;")
        code.append("float ring_intersection_param = -dot(new_pos, ring_normal.xyz) / dot(light_dir, ring_normal.xyz);")
        code.append("if (ring_intersection_param > 0.0) {")
        code.append("  vec3 ring_intersection = new_pos + light_dir * ring_intersection_param;")
        code.append('  float ring_shadow_local = (length(ring_intersection) - ring_inner_radius) / (ring_outer_radius - ring_inner_radius);')
        code.append("  shadow *= 1.0 - texture2D(shadow_ring_tex, vec2(ring_shadow_local, 0.0)).a;")
        code.append("} else {")
        code.append("  //Not in shadow")
        code.append("}")
