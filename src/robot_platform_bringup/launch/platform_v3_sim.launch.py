from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='robot_platform_core',
            executable='task_manager_node',
            name='task_manager_node',
            output='screen',
        ),
        Node(
            package='robot_platform_core',
            executable='alert_monitor_node',
            name='alert_monitor_node',
            output='screen',
        ),
        Node(
            package='robot_platform_sim',
            executable='simple_executor_node',
            name='simple_executor_node',
            output='screen',
            parameters=[{
                'robot_id': 'robot_1',
                'odom_topic': '/odom',
                'cmd_vel_topic': '/cmd_vel',
                'task_cmd_topic': '/robot_1/task_cmd',
                'task_state_topic': '/robot_1/task_state',
            }],
        ),
        Node(
            package='robot_platform_sim',
            executable='status_adapter_node',
            name='status_adapter_node',
            output='screen',
            parameters=[{
                'robot_id': 'robot_1',
                'odom_topic': '/odom',
                'task_state_topic': '/robot_1/task_state',
                'status_topic': '/robot_1/status',
            }],
        ),
    ])