# Copyright 2022 Open Source Robotics Foundation, Inc. and Monterey Bay Aquarium Research Institute
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Launch Gazebo world with a buoy."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration


def generate_launch_description():

    pkg_ros_ign_gazebo = get_package_share_directory('ros_ign_gazebo')
    pkg_buoy_gazebo = get_package_share_directory('buoy_gazebo')
    pkg_buoy_description = get_package_share_directory('buoy_description')
    sdf_file = os.path.join(pkg_buoy_description, 'models', 'mbari_wec', 'model.sdf')

    with open(sdf_file, 'r') as infp:
        robot_desc = infp.read()

    gazebo_world_launch_arg = DeclareLaunchArgument('ign_args',
        default_value=[os.path.join(pkg_buoy_gazebo, 'worlds', 'mbari_wec.sdf'), ''],
        description='Ignition Gazebo arguments'
    )

    rviz_launch_arg = DeclareLaunchArgument('rviz',
        default_value='false',
        description='Open RViz.'
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_ign_gazebo, 'launch', 'ign_gazebo.launch.py'),
        )
    )

    # Spawn model
    ros_ign_gazebo = Node(
        package='ros_ign_gazebo',
        executable='create',
        arguments=[
            '-name', 'mbari_wec',
            '-topic', 'robot_description',
        ],
        output='screen'
    )

    # Bridge to forward tf and joint states to ros2
    bridge = Node(
        package='ros_ign_bridge',
        executable='parameter_bridge',
        arguments=[
            # Clock (IGN -> ROS2)
            '/clock@rosgraph_msgs/msg/Clock[ignition.msgs.Clock',
            # Joint states (IGN -> ROS2)
            '/world/empty/model/test_mbari_wec_model/joint_state@sensor_msgs/msg/JointState[ignition.msgs.Model',
        ],
        remappings=[
            ('/world/empty/model/test_mbari_wec_model/joint_state', 'joint_states'),
        ],
        output='screen'
    )

    # Get the parser plugin convert sdf to urdf using robot_description topic
    # Provides joint states and tf2
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='both',
        parameters=[
            {'use_sim_time': False},
            {'robot_description': robot_desc},
        ]
    )

    # Launch rviz
    rviz = Node(
        package='rviz2',
        executable='rviz2',
        arguments=['-d', os.path.join(pkg_buoy_gazebo, 'rviz', 'mbari_wec.rviz')],
        condition=IfCondition(LaunchConfiguration('rviz')),
    )

    bridge = Node(package='ros_ign_bridge',
                  executable='parameter_bridge',
                  arguments=['/clock@rosgraph_msgs/msg/Clock[ignition.msgs.Clock'],
                  output='screen')

    return LaunchDescription([
        gazebo_world_launch_arg,
        rviz_launch_arg,
        gazebo,
        ros_ign_gazebo,
        bridge,
        robot_state_publisher,
        rviz
    ])
