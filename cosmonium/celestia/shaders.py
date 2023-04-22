#
#This file is part of Cosmonium.
#
#Copyright (C) 2018-2023 Laurent Deru.
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


from ..shaders.lighting.base import LightingModelBase


class LunarLambertLightingModel(LightingModelBase):

    fragment_requires = {'world_vertex', 'world_normal'}

    def get_id(self):
        return "lunar"

    def fragment_uniforms(self, code):
        LightingModelBase.fragment_uniforms(self, code)
        code.append("uniform float ambient_coef;")
        code.append("uniform vec3 light_dir;")
        code.append("uniform vec4 ambient_color;")
        code.append("uniform vec4 light_color;")

    def fragment_shader(self, code):
        code.append("vec4 ambient = ambient_color * ambient_coef;")
        code.append("float light_angle = dot(normal, light_dir);")
        code.append("vec4 diffuse = vec4(0.0, 0.0, 0.0, 1.0);")
        code.append("float diffuse_coef = 0.0;")
        code.append("if (light_angle > 0.0) {")
        code.append("  float view_angle = dot(normal, normalize(-world_vertex));")
        code.append("  diffuse_coef = clamp(light_angle / (max(view_angle, 0.001) + light_angle), 0.0, 1.0);")
        code.append("  diffuse = light_color * shadow * diffuse_coef;")
        code.append("}")
        code.append("vec4 total_light = diffuse + ambient;")
        code.append("total_light.a = 1.0;")
        code.append("total_diffuse_color = surface_color * total_light;")
        self.apply_emission(code, 'light_angle')
