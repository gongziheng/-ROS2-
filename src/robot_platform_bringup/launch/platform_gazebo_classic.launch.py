import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    desc_share = get_package_share_directory('robot_platform_description')
    gazebo_ros_share = get_package_share_directory('gazebo_ros')

    xacro_file = os.path.join(desc_share, 'urdf', 'platform_robot.urdf.xacro')
    world_file = os.path.join(desc_share, 'worlds', 'platform_demo.world')

    robot_description = ParameterValue(
        Command(['xacro', ' ', xacro_file]),
        value_type=str
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gazebo_ros_share, 'launch', 'gazebo.launch.py')
        ),
        launch_arguments={
            'world': world_file,
            'verbose': 'true',
        }.items()
    )

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[
            {
                'use_sim_time': True,
                'robot_description': robot_description,
            }
        ]
    )

    spawn_robot = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        name='spawn_platform_robot',
        output='screen',
        arguments=[
            '-topic', 'robot_description',
            '-entity', 'platform_robot',
            '-x', '0.0',
            '-y', '0.0',
            '-z', '0.15',
        ]
    )

    task_manager = Node(
        package='robot_platform_core',
        executable='task_manager_node',
        name='task_manager_node',
        output='screen',
    )

    alert_monitor = Node(
        package='robot_platform_core',
        executable='alert_monitor_node',
        name='alert_monitor_node',
        output='screen',
    )

    simple_executor = Node(
        package='robot_platform_sim',
        executable='simple_executor_node',
        name='simple_executor_node',
        output='screen',
        parameters=[
            {
                'use_sim_time': True,
                'robot_id': 'robot_1',
                'odom_topic': '/odom',
                'cmd_vel_topic': '/cmd_vel',
                'task_cmd_topic': '/robot_1/task_cmd',
                'task_state_topic': '/robot_1/task_state',
            }
        ]
    )

    status_adapter = Node(
        package='robot_platform_sim',
        executable='status_adapter_node',
        name='status_adapter_node',
        output='screen',
        parameters=[
            {
                'use_sim_time': True,
                'robot_id': 'robot_1',
                'odom_topic': '/odom',
                'task_state_topic': '/robot_1/task_state',
                'status_topic': '/robot_1/status',
            }
        ]
    )

    return LaunchDescription([
        gazebo,
        robot_state_publisher,
        spawn_robot,
        task_manager,
        alert_monitor,
        simple_executor,
        status_adapter,
    ])