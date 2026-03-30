import math

import rclpy
from rclpy.node import Node

from robot_platform_interfaces.msg import RobotStatus, TaskCommand, TaskState


class FakeRobotNode(Node):
    def __init__(self):
        super().__init__('fake_robot_node')

        self.robot_id = 'robot_1'

        self.x = 0.0
        self.y = 0.0
        self.yaw = 0.0
        self.linear_vel = 0.0
        self.angular_vel = 0.0
        self.battery_pct = 100.0
        self.mode = 'idle'
        self.task_id = ''
        self.task_type = ''
        self.alert_text = ''

        self.target_x = None
        self.target_y = None

        self.status_pub = self.create_publisher(
            RobotStatus,
            '/robot_1/status',
            10
        )

        self.task_state_pub = self.create_publisher(
            TaskState,
            '/robot_1/task_state',
            10
        )

        self.task_sub = self.create_subscription(
            TaskCommand,
            '/robot_1/task_cmd',
            self.task_callback,
            10
        )

        self.timer = self.create_timer(0.1, self.on_timer)

        self.get_logger().info(f'Fake robot node started. robot_id={self.robot_id}')

    def publish_task_state(self, state: str, message: str, distance_remaining: float):
        if not self.task_id:
            return

        msg = TaskState()
        msg.stamp = self.get_clock().now().to_msg()
        msg.robot_id = self.robot_id
        msg.task_id = self.task_id
        msg.state = state
        msg.task_type = self.task_type
        msg.target_x = float(self.target_x if self.target_x is not None else self.x)
        msg.target_y = float(self.target_y if self.target_y is not None else self.y)
        msg.current_x = float(self.x)
        msg.current_y = float(self.y)
        msg.distance_remaining = float(distance_remaining)
        msg.message = message

        self.task_state_pub.publish(msg)

    def task_callback(self, msg: TaskCommand):
        self.target_x = float(msg.target_x)
        self.target_y = float(msg.target_y)
        self.task_id = msg.task_id
        self.task_type = msg.task_type if msg.task_type else 'goto'
        self.mode = 'moving'

        distance = math.hypot(self.target_x - self.x, self.target_y - self.y)
        self.publish_task_state('pending', 'task accepted by fake robot', distance)

        self.get_logger().info(
            f'Received task: id={self.task_id}, type={self.task_type}, '
            f'goal=({self.target_x:.2f}, {self.target_y:.2f})'
        )

    def on_timer(self):
        dt = 0.1
        speed = 0.5

        if self.target_x is not None and self.target_y is not None:
            dx = self.target_x - self.x
            dy = self.target_y - self.y
            dist = math.hypot(dx, dy)

            if dist > 0.05:
                self.yaw = math.atan2(dy, dx)
                step = min(speed * dt, dist)

                self.x += step * math.cos(self.yaw)
                self.y += step * math.sin(self.yaw)

                self.linear_vel = speed
                self.angular_vel = 0.0
                self.mode = 'moving'

                remain = math.hypot(self.target_x - self.x, self.target_y - self.y)
                self.publish_task_state('running', 'moving to target', remain)
            else:
                self.x = self.target_x
                self.y = self.target_y
                self.linear_vel = 0.0
                self.angular_vel = 0.0
                self.mode = 'idle'

                self.publish_task_state('succeeded', 'goal reached', 0.0)
                self.get_logger().info(f'Goal reached: task={self.task_id}')

                self.target_x = None
                self.target_y = None
                self.task_id = ''
                self.task_type = ''
        else:
            self.linear_vel = 0.0
            self.angular_vel = 0.0
            self.mode = 'idle'

        self.battery_pct = max(0.0, self.battery_pct - 0.001)

        status = RobotStatus()
        status.stamp = self.get_clock().now().to_msg()
        status.robot_id = self.robot_id
        status.x = float(self.x)
        status.y = float(self.y)
        status.yaw = float(self.yaw)
        status.linear_vel = float(self.linear_vel)
        status.angular_vel = float(self.angular_vel)
        status.battery_pct = float(self.battery_pct)
        status.mode = self.mode
        status.task_id = self.task_id
        status.is_online = True
        status.alert_text = self.alert_text

        self.status_pub.publish(status)


def main(args=None):
    rclpy.init(args=args)
    node = FakeRobotNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()