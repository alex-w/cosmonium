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


from ..astro import units
from .. import settings

from .yamlparser import YamlModuleParser
from ..procedural.populator import RandomObjectPlacer
from cosmonium.procedural.populator import CpuTerrainPopulator, GpuTerrainPopulator
from cosmonium.parsers.shapesparser import ShapeYamlParser
from cosmonium.parsers.appearancesparser import AppearanceYamlParser
from cosmonium.shaders import BasicShader
from cosmonium.parsers.shadersparser import VertexControlYamlParser
from cosmonium.shapes import ShapeObject, MeshShape

class PlacerYamlParser(YamlModuleParser):
    @classmethod
    def decode(self, data, default='random'):
        placer = None
        extra = {}
        (placer_type, placer_data) = self.get_type_and_data(data, default)
        if placer_type == 'random':
            placer = RandomObjectPlacer()
        else:
            print("Unknown placer", placer_type)
        return placer

class PopulatorYamlParser(YamlModuleParser):
    @classmethod
    def decode(cls, data):
        if settings.hardware_instancing:
            default = 'gpu'
        else:
            default = 'cpu'
        (populator_type, populator_data) = cls.get_type_and_data(data, default)
        populator = None
        density = data.get('density', 250)
        density /= 1000000.0
        max_instances = data.get('max-instances', 1000)
        min_lod = data.get('min-lod', 0)
        shape, extra = ShapeYamlParser.decode(populator_data.get('shape', None))
        appearance = populator_data.get('appearance', None)
        if appearance is None:
            if isinstance(shape, MeshShape):
                appearance = 'model'
            else:
                appearance = 'textures'
        appearance = AppearanceYamlParser.decode(appearance)
        vertex_control = VertexControlYamlParser.decode(populator_data.get('vertex', None))
        shader = BasicShader(#lighting_model=lighting_model,
                             #scattering=scattering,
                             vertex_control=vertex_control,
                             use_model_texcoord=not extra.get('create-uv', False))
        object_template = ShapeObject('template', shape=shape, appearance=appearance, shader=shader)
        placer = PlacerYamlParser.decode(populator_data.get('placer', None))
        if populator_type == 'cpu':
            populator = CpuTerrainPopulator(object_template, density, max_instances, placer, min_lod)
        elif populator_type == 'gpu':
            populator = GpuTerrainPopulator(object_template, density, max_instances, placer, min_lod)
        else:
            print("Unknown populator", populator_type, populator_data)
        return populator
