<?xml version="1.0" ?>
@{
from ignition.math import Cylinderd
from ignition.math import MassMatrix3d
from ignition.math import Material
import math

##############
# Parameters #
##############

# Piston
piston_length = 5.08
piston_z_offset = -3.50066

# Tether
tether_radius = 0.009525 # Nominal O.D. 0.75 in
tether_density = 3350 # kg/m^3
tether_length = 20.3

num_tether_top_links = 4
tether_top_length = 3.0

num_tether_bottom_links = 5

###################
# Computed values #
###################

# Top tether
tether_top_link_length = tether_top_length / num_tether_top_links

tether_top_link_cylinder = Cylinderd(tether_top_link_length, tether_radius)
tether_top_link_cylinder.set_mat(Material(tether_density))
tether_top_link_mm = MassMatrix3d()
tether_top_link_cylinder.mass_matrix(tether_top_link_mm)

# Bottom tether
tether_bottom_length = tether_length - tether_top_length
tether_bottom_link_length = tether_bottom_length / num_tether_bottom_links

tether_bottom_link_cylinder = Cylinderd(tether_bottom_link_length, tether_radius)
tether_bottom_link_cylinder.set_mat(Material(tether_density))
tether_bottom_link_mm = MassMatrix3d()
tether_bottom_link_cylinder.mass_matrix(tether_bottom_link_mm)

def tether_joint_dynamics():
    """ Prints the <dynamics> block for tether joints. """
    print("""
        <dynamics>
          <damping>10000.0</damping>
          <friction>1000</friction>
        </dynamics>
    """)
}@
<sdf version="1.8">
  <model name="MBARI_WEC">
    <self_collide>true</self_collide>
    <link name="Buoy">
      <pose relative_to="__model__">0 0 0 0 0 0</pose>
      <inertial>
        <mass>1080</mass>
        <inertia>
          <ixx>10000</ixx>
          <ixy>0</ixy>
          <ixz>0</ixz>
          <iyy>10000</iyy>
          <iyz>0</iyz>
          <izz>1000</izz>
        </inertia>
      </inertial>
      <visual name="visual">
        <geometry>
          <mesh>
            <uri>meshes/buoy_float.stl</uri>
          </mesh>
        </geometry>
        <!--color-->
        <material>
          <ambient>1.0 1.0 0.0 1</ambient>
          <diffuse>1.0 1.0 0.0 1</diffuse>
          <specular>1.0 1.0 0.0 1</specular>
        </material>
      </visual>
    </link>

    <link name="PTO">
      <pose relative_to="Buoy">0 0 0 0 0 0</pose>
      <inertial>
        <mass>605</mass>
        <inertia>
          <ixx>10000</ixx>
          <ixy>0</ixy>
          <ixz>0</ixz>
          <iyy>10000</iyy>
          <iyz>0</iyz>
          <izz>1000</izz>
        </inertia>
      </inertial>
      <visual name="visual">
        <geometry>
          <mesh>
            <uri>meshes/pto.stl</uri>
          </mesh>
        </geometry>
        <!--color-->
        <material>
          <ambient>1 1 1 0.9</ambient>
          <diffuse>.2 .2 1 0.9</diffuse>
          <specular>1 1 1 1</specular>
        </material>
      </visual>
      <collision name="collision">
        <geometry>
          <mesh>
            <uri>meshes/pto_collision.stl</uri>
          </mesh>
        </geometry>
        <surface>
          <contact>
            <!-- Collide with top tether -->
            <!--collide_bitmask>0x01</collide_bitmask-->
          </contact>
        </surface>
      </collision>
    </link>

    <link name="Piston">
      <pose relative_to="PTO">0 0 @(piston_z_offset) 0 0 0</pose>
      <inertial>
        <mass>48</mass>
        <inertia>
          <ixx>10000</ixx>
          <ixy>0</ixy>
          <ixz>0</ixz>
          <iyy>10000</iyy>
          <iyz>0</iyz>
          <izz>1000</izz>
        </inertia>
      </inertial>
      <visual name="visual">
        <geometry>
          <mesh>
            <uri>meshes/rod_and_piston.stl</uri>
          </mesh>
        </geometry>
        <!--color-->
        <material>
          <ambient>0.0 0.0 1.0 1</ambient>
          <diffuse>0.0 0.0 1.0 1</diffuse>
          <specular>0.0 0.0 1.0 1</specular>
        </material>
      </visual>
    </link>
    <frame name="PistonBottom" attached_to="Piston">
      <pose>0 0 -@(piston_length) 0 0 0</pose>
    </frame>

    <!-- start tether top -->
@[for link_index in range(num_tether_top_links)]@
    <link name="tether_top_@(link_index)">
      <pose relative_to="PistonBottom">0 0 -@((link_index + 0.5) * tether_top_link_length) 0 0 0</pose>
      <inertial>
        <mass>@(tether_top_link_mm.mass())</mass>
        <inertia>
          <ixx>@(tether_top_link_mm.ixx())</ixx>
          <iyy>@(tether_top_link_mm.iyy())</iyy>
          <izz>@(tether_top_link_mm.izz())</izz>
        </inertia>
      </inertial>
      <visual name="visual">
        <geometry>
          <cylinder>
            <radius>@(tether_radius)</radius>
            <length>@(tether_top_link_length)</length>
          </cylinder>
        </geometry>
        <material>
          <ambient>0.1 0.1 .1 1</ambient>
          <diffuse>0.1 0.1 .1 1</diffuse>
          <specular>0.1 0.1 .1 1</specular>
        </material>
      </visual>
      <collision name="collision">
        <geometry>
          <cylinder>
            <radius>@(tether_radius)</radius>
            <length>@(tether_top_link_length)</length>
          </cylinder>
        </geometry>
        <surface>
          <contact>
            <!-- Collide with PTO -->
            <!--collide_bitmask>0x01</collide_bitmask-->
          </contact>
        </surface>
      </collision>
    </link>

    <joint name="tether_top_joint_@(link_index)" type="universal">
      <pose>0 0 @(tether_top_link_length * 0.5) 0 0 0</pose>
@[if link_index == 0]@
      <parent>Piston</parent>
@[else]@
      <parent>tether_top_@(link_index-1)</parent>
@[end if]@
      <child>tether_top_@(link_index)</child>
      <axis>
        <xyz>1 0 0</xyz>
        @(tether_joint_dynamics())
      </axis>
      <axis2>
        <xyz>0 1 0</xyz>
        @(tether_joint_dynamics())
      </axis2>
    </joint>
@[end for]@
    <!-- end tether top -->

    <!-- start tether bottom -->
@[for link_index in range(num_tether_bottom_links)]@
    <link name="tether_bottom_@(link_index)">
      <pose relative_to="PistonBottom">0 0 -@((link_index + 0.5) * tether_bottom_link_length + tether_top_length) 0 0 0</pose>
      <inertial>
        <mass>@(tether_bottom_link_mm.mass())</mass>
        <inertia>
          <ixx>@(tether_bottom_link_mm.ixx())</ixx>
          <iyy>@(tether_bottom_link_mm.iyy())</iyy>
          <izz>@(tether_bottom_link_mm.izz())</izz>
        </inertia>
      </inertial>
      <visual name="visual">
        <geometry>
          <cylinder>
            <radius>@(tether_radius)</radius>
            <length>@(tether_bottom_link_length)</length>
          </cylinder>
        </geometry>
        <material>
          <ambient>0.1 1 .1 1</ambient>
          <diffuse>0.1 1 .1 1</diffuse>
          <specular>0.1 1 .1 1</specular>
        </material>
      </visual>
      <collision name="collision">
        <geometry>
          <cylinder>
            <radius>@(tether_radius)</radius>
            <length>@(tether_bottom_link_length)</length>
          </cylinder>
        </geometry>
      </collision>
    </link>

    <joint name="tether_bottom_joint_@(link_index)" type="universal">
      <pose>0 0 @(tether_bottom_link_length * 0.5) 0 0 0</pose>
@[if link_index == 0]@
      <parent>tether_top_@(num_tether_top_links-1)</parent>
@[else]@
      <parent>tether_bottom_@(link_index-1)</parent>
@[end if]@
      <child>tether_bottom_@(link_index)</child>
      <axis>
        <xyz>1 0 0</xyz>
        @(tether_joint_dynamics())
      </axis>
      <axis2>
        <xyz>0 1 0</xyz>
        @(tether_joint_dynamics())
      </axis2>
    </joint>
@[end for]@

    <joint name="TetherToHeaveCone" type="fixed">
      <parent>tether_bottom_@(num_tether_bottom_links-1)</parent>
      <child>HeaveCone</child>
    </joint>
    <!-- end tether bottom -->

    <link name="HeaveCone">
      <pose relative_to="PistonBottom">0 0 -@(tether_length) 0 0 0</pose>
      <inertial>
        <mass>1000</mass>
        <inertia>
          <ixx>10000</ixx>
          <ixy>0</ixy>
          <ixz>0</ixz>
          <iyy>10000</iyy>
          <iyz>0</iyz>
          <izz>1000</izz>
        </inertia>
      </inertial>
      <visual name="visual">
        <geometry>
          <mesh>
            <uri>meshes/heave_cone.stl</uri>
          </mesh>
        </geometry>
        <!--color-->
        <material>
          <ambient>0.1 0.1 .1 1</ambient>
          <diffuse>0.1 0.1 .1 1</diffuse>
          <specular>0.1 0.1 .1 1</specular>
        </material>
      </visual>
    </link>

    <link name="Trefoil">
      <pose relative_to="HeaveCone">0 0 0 0 0 0</pose>
      <inertial>
        <mass>10</mass>
        <inertia>
          <ixx>10000</ixx>
          <ixy>0</ixy>
          <ixz>0</ixz>
          <iyy>10000</iyy>
          <iyz>0</iyz>
          <izz>1000</izz>
        </inertia>
      </inertial>
      <visual name="visual">
        <geometry>
          <mesh>
            <uri>meshes/trefoil.stl</uri>
          </mesh>
        </geometry>
        <!--color-->
        <material>
          <ambient>0.1 0.1 .1 1</ambient>
          <diffuse>0.1 0.1 .1 1</diffuse>
          <specular>0.1 0.1 .1 1</specular>
        </material>
      </visual>
    </link>

    <!-- TODO(chapulina) Remove once buoyancy is in action -->
    <joint name="WaterPlaneArea" type="revolute">
      <parent>world</parent>
      <child>Buoy</child>
      <pose>0 0 2 0 0 0</pose>
      <axis>
        <limit>
          <lower>-2.0</lower>
          <upper>2.0</upper>
        </limit>
        <xyz>1.0 0.0 0.0</xyz>
        <!--dynamics>
          <spring_stiffness>55000</spring_stiffness>
          <spring_reference>0.0</spring_reference>
          <damping>1000.0</damping>
          <friction>0.0</friction>
        </dynamics-->
      </axis>
    </joint>

    <joint name="Universal" type="universal">
      <parent>Buoy</parent>
      <child>PTO</child>
      <provide_feedback>1</provide_feedback>
      <pose>0.0 0.0 0.0 0 0 0</pose>
      <axis>
        <xyz>1 0 0</xyz>
        @(tether_joint_dynamics())
      </axis>
      <axis2>
        <xyz>0 1 0</xyz>
        @(tether_joint_dynamics())
      </axis2>
    </joint>

    <joint name="HydraulicRam" type="prismatic">
      <parent>PTO</parent>
      <child>Piston</child>
      <pose>0.0 0.0 0.0 0 0 0</pose>
      <axis>
        <limit>
          <lower>0.0</lower>
          <upper>2.03</upper>
        </limit>
        <xyz>0.0 0.0 1.0</xyz>
      </axis>
    </joint>

    <joint name="TrefoilDoors" type="revolute">
      <parent>HeaveCone</parent>
      <child>Trefoil</child>
      <provide_feedback>1</provide_feedback>
      <pose>0.0 0.0 0.0 0 0 0</pose>
      <axis>
        <limit>
          <lower>0.0</lower>
          <upper>1.047</upper>
        </limit>
        <xyz>0.0 0.0 1.0</xyz>
      </axis>
    </joint>
  </model>
</sdf>
