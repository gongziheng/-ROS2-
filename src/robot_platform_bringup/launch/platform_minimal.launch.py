from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='robot_platform_core',
            executable='fake_robot_node',
            name='fake_robot_node',
            output='screen'
        ),
        Node(
            package='robot_platform_core',
            executable='task_manager_node',
            name='task_manager_node',
            output='screen'
        ),
        Node(
            package='robot_platform_core',
            executable='alert_monitor_node',
            name='alert_monitor_node',
            output='screen'
        ),
    ])
