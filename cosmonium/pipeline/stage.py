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


class PipelineStage():
    def __init__(self,name):
        self.name = name
        self.win = None
        self.engine = None
        self.targets = []
        self.sources = {}
        self.scene_manager = None

    def requires(self):
        return []

    def provides(self):
        return {}

    def min_version(self):
        return 330

    def add_source(self, source_name, stage, output_name):
        self.sources[source_name] = (stage, output_name)

    def get_output(self, target_name):
        texture_name = self.provides().get(target_name)
        return self.targets[-1].texture_targets[texture_name].texture

    def set_scene(self, scene_manager):
        self.scene_manager = scene_manager

    def set_win(self, win):
        self.win = win

    def set_engine(self, engine):
        self.engine = engine

    def update_win_size(self, size):
        for target in self.targets:
            target.update_win_size(size)

    def add_target(self, target):
        self.targets.append(target)
        target.set_win(self.win)
        target.set_engine(self.engine)

    def create(self, pipeline):
        raise NotImplementedError()

    def clear(self):
        for target in self.targets:
            target.clear()

    def remove(self):
        for target in self.targets:
            target.remove()


class ProcessStage(PipelineStage):
    def prepare(self, prepare_data):
        for target in self.targets:
            target.prepare(prepare_data)

    def trigger(self):
        for target in self.targets:
            target.trigger()

    def update_shader_data(self, shader_data):
        for target in self.targets:
            target.update_shader_data(shader_data)

    def gather(self, result):
        data = {}
        for name, texture_target in self.targets[-1].texture_targets.items():
            data[name] = texture_target.texture
        result[self.name] = data


class SceneStage(PipelineStage):
    def __init__(self, name):
        PipelineStage.__init__(self, name)
        self.scene_stage = False
        self.screen_stage = False

    def set_scene_stage(self, scene_stage):
        self.scene_stage = scene_stage

    def set_screen_stage(self, screen_stage):
        self.screen_stage = screen_stage

    def set_scene_manager(self, scene_manager):
        scene_manager.set_target(self.targets[0].target)

    def can_render_scene(self):
        return False

    def can_render_to_screen(self):
        return True
