import time

import rclpy
from rclpy.node import Node

from robot_platform_interfaces.msg import TaskCommand, TaskState
from robot_platform_interfaces.srv import SubmitTask


class TaskManagerNode(Node):
    def __init__(self):
        super().__init__('task_manager_node')

        self.cmd_pub = self.create_publisher(
            TaskCommand,
            '/robot_1/task_cmd',
            10
        )

        self.task_state_sub = self.create_subscription(
            TaskState,
            '/robot_1/task_state',
            self.task_state_callback,
            10
        )

        self.srv = self.create_service(
            SubmitTask,
            '/submit_task',
            self.handle_submit_task
        )

        self.current_task_id = ''
        self.current_robot_id = ''
        self.current_task_state = ''
        self.task_state_map = {}

        self.get_logger().info('Task manager node started.')

    def is_robot_busy(self, robot_id: str) -> bool:
        return (
            robot_id == self.current_robot_id and
            bool(self.current_task_id) and
            self.current_task_state in ('pending', 'running')
        )

    def handle_submit_task(self, request, response):
        robot_id = request.robot_id.strip()
        task_type = request.task_type.strip() if request.task_type.strip() else 'goto'

        if not robot_id:
            response.accepted = False
            response.task_id = ''
            response.message = 'robot_id is required'
            return response

        if self.is_robot_busy(robot_id):
            response.accepted = False
            response.task_id = self.current_task_id
            response.message = (
                f'robot {robot_id} is busy, current task={self.current_task_id}, '
                f'state={self.current_task_state}'
            )
            self.get_logger().warn(response.message)
            return response

        task_id = f'{task_type}_{int(time.time())}'

        self.current_task_id = task_id
        self.current_robot_id = robot_id
        self.current_task_state = 'pending'

        cmd = TaskCommand()
        cmd.stamp = self.get_clock().now().to_msg()
        cmd.robot_id = robot_id
        cmd.task_id = task_id
        cmd.task_type = task_type
        cmd.target_x = float(request.target_x)
        cmd.target_y = float(request.target_y)

        self.cmd_pub.publish(cmd)

        self.task_state_map[task_id] = {
            'robot_id': robot_id,
            'state': 'pending',
            'task_type': task_type,
            'target_x': float(request.target_x),
            'target_y': float(request.target_y),
            'message': 'task submitted',
        }

        response.accepted = True
        response.task_id = task_id
        response.message = (
            f'Task accepted for {robot_id}: '
            f'({request.target_x:.2f}, {request.target_y:.2f})'
        )

        self.get_logger().info(
            f'Accepted task={task_id} robot={robot_id} '
            f'goal=({request.target_x:.2f}, {request.target_y:.2f})'
        )
        return response

    def task_state_callback(self, msg: TaskState):
        old_state = ''
        if msg.task_id in self.task_state_map:
            old_state = self.task_state_map[msg.task_id].get('state', '')

        self.task_state_map[msg.task_id] = {
            'robot_id': msg.robot_id,
            'state': msg.state,
            'task_type': msg.task_type,
            'target_x': float(msg.target_x),
            'target_y': float(msg.target_y),
            'current_x': float(msg.current_x),
            'current_y': float(msg.current_y),
            'distance_remaining': float(msg.distance_remaining),
            'message': msg.message,
        }

        if msg.task_id == self.current_task_id:
            self.current_task_state = msg.state

            if msg.state in ('succeeded', 'failed'):
                self.current_task_id = ''
                self.current_robot_id = ''
                self.current_task_state = ''

        if msg.state != old_state:
            self.get_logger().info(
                f'Task state update: task={msg.task_id}, robot={msg.robot_id}, '
                f'state={msg.state}, remain={msg.distance_remaining:.2f}, '
                f'message={msg.message}'
            )


def main(args=None):
    rclpy.init(args=args)
    node = TaskManagerNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()