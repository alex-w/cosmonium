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


from direct.task.Task import gather

from . import settings


class DataSourceTasksTree:
    def __init__(self, sources):
        self.sources = sources
        self.named_tasks = {source.name: None for source in sources}
        self.tasks = []

    def add_task_for(self, source, coro):
        task = taskMgr.add(coro, sort=settings.shape_jobs_task_sort)
        self.named_tasks[source.name] = task
        self.tasks.append(task)

    def collect_tasks(self, shape, owner):
        for source in self.sources:
            source.create_load_task(self, shape, owner)

    async def run_tasks(self):
        await gather(*self.tasks)
        self.named_tasks = {}
        self.tasks = []


class DataSource:
    def __init__(self, name):
        self.name = name
        self._usage = 0

    def use(self, count=1):
        self._usage += count

    def release(self, count=1):
        if self._usage >= count:
            self._usage -= count
            if self._usage == 0:
                self.clear_all()
        else:
            print("Release already released data source", count, self)

    def create(self, shape):
        pass

    def create_load_task(self, tasks_tree, shape, owner):
        pass

    async def load(self, shape, owner):
        pass

    def early_apply(self, shape, instance):
        self.apply(shape, instance)

    def apply(self, shape, instance):
        pass

    def update(self, shape, instance, camera_pos, camera_rot):
        pass

    def clear(self, shape, instance):
        pass

    def clear_all(self):
        pass

    def get_user_parameters(self):
        return None

    def update_user_parameters(self):
        pass


class DataSourcesHandler(DataSource):
    def __init__(self, name):
        DataSource.__init__(self, name)
        self.sources = []

    def get_source(self, name):
        source = None
        for source in self.sources:
            if source.name == name:
                break
        else:
            print(f"Source {name} not found")
        return source

    def add_source(self, source):
        if source is not None:
            if source not in self.sources:
                self.sources.append(source)
                if self._usage > 0:
                    source.use(self._usage)
            else:
                print("Adding already added source", source)

    def remove_source(self, source):
        if source is not None:
            self.sources.remove(source)
            if self._usage > 0:
                source.release(self._usage)

    def remove_source_by_name(self, name):
        source = None
        for source in self.sources:
            if source.name == name:
                self.sources.remove(source)
                if self._usage > 0:
                    source.release(self._usage)
                break

    def use(self, count=1):
        DataSource.use(self, count)
        for source in self.sources:
            source.use(count)

    def release(self, count=1):
        DataSource.release(self, count)
        for source in self.sources:
            source.release(count)

    def create(self, shape):
        for source in self.sources:
            source.create(shape)

    def early_apply(self, shape):
        for source in self.sources:
            source.early_apply(shape, shape.instance)

    async def load(self, shape):
        for source in self.sources:
            source.create(shape)
        tasks_tree = DataSourceTasksTree(self.sources)
        tasks_tree.collect_tasks(shape, self)
        await tasks_tree.run_tasks()

    def apply(self, shape):
        for source in self.sources:
            source.apply(shape, shape.instance)

    def update(self, shape, camera_pos, camera_rot):
        for source in self.sources:
            source.update(shape, shape.instance, camera_pos, camera_rot)

    def clear(self, shape, instance):
        for source in self.sources:
            source.clear(shape, shape.instance)
