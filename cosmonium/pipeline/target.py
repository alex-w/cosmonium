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


from panda3d.core import Camera, OrthographicLens, CardMaker, GraphicsOutput, Texture, NodePath
from panda3d.core import WindowProperties, FrameBufferProperties, GraphicsPipe

from ..textures import TextureConfiguration


class TextureTarget:
    def __init__(self, to_ram, render_target, config):
        self.texture = None
        self.to_ram = to_ram
        self.render_target = render_target
        self.config = config

    def create(self):
        self.texture = Texture()
        if self.config is not None:
            self.config.apply(self.texture)
        return self.texture

    def clear(self):
        self.texture = None

    def get_mode(self):
        if self.to_ram:
            mode = GraphicsOutput.RTM_copy_ram
        else:
            mode = GraphicsOutput.RTM_bind_or_copy
        return mode

    def get_render_target(self):
        return self.render_target


class RenderTarget:
    def __init__(self, name):
        self.name = name
        self.target = None
        self.sort = 0
        self.win = None
        self.graphics_engine = None
        self.dr = None

    def set_win(self, win):
        self.win = win

    def set_engine(self, engine):
        self.graphics_engine = engine

    def create_display_region(self):
        #Create the display region and attach the camera
        self.dr = self.target.make_display_region((0, 1, 0, 1))
        self.dr.disable_clears()
        self.dr.set_scissor_enabled(False)


class BufferMixin:
    def __init__(self):
        self.requested_size = (1, 1)
        self.size = (0, 0)
        self.fixed_size = False
        self.color_bits = None
        self.srgb_colors = None
        self.depth_bits = None
        self.float_depth = False
        self.aux_bits = None
        self.nb_aux = 0
        self.multisamples = 0
        self.texture_targets = {}
        self.sort = 0
        self.active = True
        self.one_shot = False

    def set_one_shot(self, one_shot):
        self.one_shot = one_shot

    def set_fixed_size(self, size):
        self.requested_size = size
        self.size = size
        self.fixed_size = True
        if self.target is not None:
            self.target.set_size(*self.size)

    def set_relative_size(self, size):
        self.requested_size = size
        self.fixed_size = False
        if self.win is not None:
            self.update_win_size((self.win.get_x_size(), self.win_get_y_size()))

    def update_win_size(self, size):
        if self.fixed_size: return
        new_size = (size[0] * self.requested_size[0], size[1] * self.requested_size[1])
        if new_size != self.size:
            self.size = new_size
            if self.target is not None:
                self.target.set_size(*self.size)

    def get_attachment(self, name):
        return self.texture_targets[name].texture

    def add_color_target(self, color_bits, srgb_colors=True, name='color', to_ram=False, config=TextureConfiguration()):
        self.color_bits = color_bits
        self.srgb_colors = srgb_colors
        texture_target = TextureTarget(to_ram, GraphicsOutput.RTP_color, config)
        self.texture_targets[name] = texture_target

    def add_depth_target(self, depth_bits, stencil_bits=0, float_depth=False, name='depth', to_ram=False, config=TextureConfiguration()):
        self.depth_bits = depth_bits
        self.float_depth = float_depth
        self.stencil_bits = stencil_bits
        if depth_bits < 32:
            rtp = GraphicsOutput.RTP_depth_stencil
        else:
            rtp = GraphicsOutput.RTP_depth
        texture_target = TextureTarget(to_ram, rtp, config)
        self.texture_targets[name] = texture_target

    def add_depth(self, depth_bits, stencil_bits=0, float_depth=False, config=TextureConfiguration()):
        self.depth_bits = depth_bits
        self.float_depth = float_depth
        self.stencil_bits = stencil_bits

    def add_aux_target(self, aux_bits, name=None, to_ram=False, config=TextureConfiguration()):
        if self.aux_bits is None:
            self.aux_bits = aux_bits
        elif self.aux_bits != aux_bits:
            print("ERROR: aux bits must all the the same")
        #TODO: Map to int, hfloat, float
        texture_target = TextureTarget(to_ram, GraphicsOutput.RTP_aux, config)
        self.texture_targets[name] = texture_target

    def set_multisamples(self, multisamples):
        self.multisamples = multisamples

    def create_textures(self):
        for texture_target in self.texture_targets.values():
            texture = texture_target.create()
            mode = texture_target.get_mode()
            render_target = texture_target.get_render_target()
            self.target.add_render_texture(texture, mode, render_target)

    def make_fbprops(self):
        fbprops = FrameBufferProperties()
        if self.color_bits is not None:
            fbprops.set_rgba_bits(*self.color_bits)
            if max(*self.color_bits)  > 8:
                fbprops.set_float_color(True)
            fbprops.set_srgb_color(self.srgb_colors)
        if self.depth_bits is not None:
            fbprops.set_depth_bits(self.depth_bits)
            fbprops.set_float_depth(self.float_depth)
        if self.multisamples:
            fbprops.set_multisamples(self.multisamples)
        return fbprops

    def make_winprops(self):
        winprops = WindowProperties()
        winprops.set_size(*self.size)
        return winprops

    def make_buffer_options(self):
        buffer_options = GraphicsPipe.BF_refuse_window
        if self.requested_size < (0, 0):
            buffer_options |= GraphicsPipe.BF_resizeable
        return buffer_options

    def create_buffer(self):
        fbprops = self.make_fbprops()
        winprops = self.make_winprops()
        buffer_options = self.make_buffer_options()
        self.target = self.graphics_engine.make_output(self.win.get_pipe(), self.name, -1,
            fbprops, winprops,  buffer_options, self.win.get_gsg(), self.win)
        print("New buffer", self.target.get_fb_properties())

    def create_target(self, pipeline):
        if self.target is not None:
            print("ERROR: create_target() called on an initialized target")
            return
        self.create_buffer()
        self.active = True
        self.target.disable_clears()
        slot = pipeline.request_slot()
        self.target.set_sort(slot)
        if self.one_shot:
            self.target.set_active(False)
        else:
            self.target.set_active(True)

    def prepare(self, prepare_data):
        if not self.one_shot:
            print("Can't call prepare on non one-shot target")
            return
        self.clear()
        self.create_textures()
        if prepare_data is not None:
            for texture_name, texture_target in self.texture_targets.items():
                config = prepare_data.get(texture_name)
                if config is not None:
                    config.apply(texture_target.texture)

    def trigger(self):
        if not self.one_shot:
            print("Can't call trigger on non one-shot target")
            return
        self.target.set_one_shot(True)
        self.target.set_active(True)

    def clear(self):
        self.target.clear_render_textures()
        for texture_target in self.texture_targets.values():
            texture_target.clear()
        #TODO: Do we need to call release_all() on all textures ?

    def remove(self):
        if self.target is not None:
            self.clear()
            self.target.set_active(False)
            self.active = False
            self.graphics_engine.remove_window(self.target)
            self.target = None


class TargetShaderMixin():
    def __init__(self):
        self.root = None
        self.camera = None
        self.shader = None

    def create_target_geom(self):
        #Create the plane with the texture
        cm = CardMaker("plane")
        cm.set_frame_fullscreen_quad()
        #TODO: This should either be done inside the passthrough vertex shader
        # Or in the heightmap sampler.
        #x_margin = 1.0 / self.width / 2.0
        #y_margin = 1.0 / self.height / 2.0
        #cm.set_uv_range((-x_margin, -y_margin), (1 + x_margin, 1 + y_margin))
        self.root = NodePath(cm.generate())
        self.root.set_depth_test(False)
        self.root.set_depth_write(False)

    def create_target_cam(self):
        #Create the camera for the buffer
        cam = Camera("buffer-cam")
        lens = OrthographicLens()
        lens.set_film_size(2, 2)
        lens.set_near_far(-1000, 1000)
        cam.set_lens(lens)
        self.camera = self.root.attach_new_node(cam)

    def create_infra(self):
        self.create_target_geom()
        self.create_target_cam()
        self.dr.set_camera(self.camera)

    def set_shader(self, shader):
        self.shader = shader
        shader.apply(self.root)

    def update_shader_data(self, shader_data):
        self.shader.update(self.root, **shader_data)


class ProcessTarget(RenderTarget, BufferMixin, TargetShaderMixin):
    def __init__(self, name):
        RenderTarget.__init__(self, name)
        BufferMixin.__init__(self)
        TargetShaderMixin.__init__(self)

    def create(self, pipeline):
        self.create_target(pipeline)
        self.create_display_region()
        self.create_infra()


class SceneTarget(RenderTarget, BufferMixin):
    def __init__(self, name):
        RenderTarget.__init__(self, name)
        BufferMixin.__init__(self)

    def create(self, pipeline):
        self.create_target(pipeline)
        self.create_textures()
        self.create_display_region()

    def attach_scene_manager(self, scene_manager):
        scene_manager.set_target(self.target)


class ScreenTarget(RenderTarget, TargetShaderMixin):
    def __init__(self, name):
        RenderTarget.__init__(self, name)
        TargetShaderMixin.__init__(self)

    def set_win(self, win):
        RenderTarget.set_win(self, win)
        self.target = win

    def update_win_size(self, size):
        self.root.set_shader_input("screen_size", size)

    def create(self, pipeline):
        self.create_display_region()
        self.create_infra()


class PasstroughTarget(RenderTarget):
    def __init__(self, name):
        RenderTarget.__init__(self, name)

    def update_win_size(self, size):
        pass

    def create(self, pipeline):
        self.target = self.win
        self.create_display_region()
