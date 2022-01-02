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


from ..elementsdb import orbit_elements_db

try:
    from cosmonium_engine import DourneauOrbit
    loaded = True
except ImportError as e:
    print("WARNING: Could not load Dourneau C implementation")
    print("\t", e)
    loaded = False

orbit_elements_db.register_category('dourneau', 100)
if loaded:
    orbit_elements_db.register_element('dourneau', 'mimas',     DourneauOrbit(0,   0.942,  185539, 0.0196))
    orbit_elements_db.register_element('dourneau', 'enceladus', DourneauOrbit(1,   1.370,  238042, 0.0000))
    orbit_elements_db.register_element('dourneau', 'tethys',    DourneauOrbit(2,   1.888,  294672, 0.0001))
    orbit_elements_db.register_element('dourneau', 'dione',     DourneauOrbit(3,   2.737,  377415, 0.0022))
    orbit_elements_db.register_element('dourneau', 'rhea',      DourneauOrbit(4,   4.518,  527068, 0.0002))
    orbit_elements_db.register_element('dourneau', 'titan',     DourneauOrbit(5,  15.95,  1221865, 0.0288))
    orbit_elements_db.register_element('dourneau', 'hyperion',  DourneauOrbit(6,  21.28,  1500933, 0.0232))
    orbit_elements_db.register_element('dourneau', 'iapetus',   DourneauOrbit(7,  79.33,  3560854, 0.0293))
    orbit_elements_db.register_element('dourneau', 'phoebe',    DourneauOrbit(8, 548.02, 12947918, 0.1634))
