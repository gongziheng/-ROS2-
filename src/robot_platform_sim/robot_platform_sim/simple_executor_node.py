import math
from typing import Optional

import rclpy
from rclpy.node import Node

from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from robot_platform_interfaces.msg import TaskCommand, TaskState


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


def normalize_angle(angle: float) -> float:
    while angle > math.pi:
        angle -= 2.0 * math.pi
    while angle < -math.pi:
        angle += 2.0 * math.pi
    return angle


def yaw_from_quaternion(x: float, y: float, z: float, w: float) -> float:
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    return math.atan2(siny_cosp, cosy_cosp)


class SimpleExecutorNode(Node):
    def __init__(self):
        super().__init__('simple_executor_node')

        self.declare_parameter('robot_id', 'robot_1')
        self.declare_parameter('odom_topic', '/odom')
        self.declare_parameter('cmd_vel_topic', '/cmd_vel')
        self.declare_parameter('task_cmd_topic', '/robot_1/task_cmd')
        self.declare_parameter('task_state_topic', '/robot_1/task_state')
        self.declare_parameter('goal_tolerance', 0.15)
        self.declare_parameter('max_linear_speed', 0.30)
        self.declare_parameter('max_angular_speed', 1.20)

        self.robot_id = self.get_parameter('robot_id').value
        self.odom_topic = self.get_parameter('odom_topic').value
        self.cmd_vel_topic = self.get_parameter('cmd_vel_topic').value
        self.task_cmd_topic = self.get_parameter('task_cmd_topic').value
        self.task_state_topic = self.get_parameter('task_state_topic').value

        self.goal_tolerance = float(self.get_parameter('goal_tolerance').value)
        self.max_linear_speed = float(self.get_parameter('max_linear_speed').value)
        self.max_angular_speed = float(self.get_parameter('max_angular_speed').value)

        self.cmd_pub = self.create_publisher(Twist, self.cmd_vel_topic, 10)
        self.task_state_pub = self.create_publisher(TaskState, self.task_state_topic, 10)

        self.odom_sub = self.create_subscription(
            Odometry, self.odom_topic, self.odom_callback, 10
        )
        self.task_sub = self.create_subscription(
            TaskCommand, self.task_cmd_topic, self.task_callback, 10
        )

        self.timer = self.create_timer(0.1, self.on_timer)

        self.current_x = 0.0
        self.current_y = 0.0
        self.current_yaw = 0.0
        self.has_odom = False

        self.active_task: Optional[TaskCommand] = None
        self.last_state = ''

        self.get_logger().info(
            f'simple_executor_node started, odom={self.odom_topic}, cmd_vel={self.cmd_vel_topic}'
        )

    def odom_callback(self, msg: Odometry):
        self.current_x = float(msg.pose.pose.position.x)
        self.current_y = float(msg.pose.pose.position.y)

        q = msg.pose.pose.orientation
        self.current_yaw = yaw_from_quaternion(q.x, q.y, q.z, q.w)
        self.has_odom = True

    def task_callback(self, msg: TaskCommand):
        if msg.robot_id != self.robot_id:
            return

        self.active_task = msg
        self.last_state = ''
        self.publish_task_state('pending', 'task accepted by executor')

        self.get_logger().info(
            f'receive task: id={msg.task_id}, type={msg.task_type}, '
            f'goal=({msg.target_x:.2f}, {msg.target_y:.2f})'
        )

    def publish_task_state(self, state: str, message: str):
        if self.active_task is None:
            return

        dx = float(self.active_task.target_x) - self.current_x
        dy = float(self.active_task.target_y) - self.current_y
        distance = math.hypot(dx, dy)

        msg = TaskState()
        msg.stamp = self.get_clock().now().to_msg()
        msg.robot_id = self.robot_id
        msg.task_id = self.active_task.task_id
        msg.state = state
        msg.task_type = self.active_task.task_type
        msg.target_x = float(self.active_task.target_x)
        msg.target_y = float(self.active_task.target_y)
        msg.current_x = float(self.current_x)
        msg.current_y = float(self.current_y)
        msg.distance_remaining = float(distance)
        msg.message = message

        self.task_state_pub.publish(msg)
        self.last_state = state

    def stop_robot(self):
        self.cmd_pub.publish(Twist())

    def on_timer(self):
        if not self.has_odom or self.active_task is None:
            return

        dx = float(self.active_task.target_x) - self.current_x
        dy = float(self.active_task.target_y) - self.current_y
        distance = math.hypot(dx, dy)

        if distance <= self.goal_tolerance:
            self.stop_robot()
            if self.last_state != 'succeeded':
                self.publish_task_state('succeeded', 'goal reached')
                self.get_logger().info(f'task succeeded: {self.active_task.task_id}')
            self.active_task = None
            return

        target_yaw = math.atan2(dy, dx)
        yaw_error = normalize_angle(target_yaw - self.current_yaw)

        twist = Twist()
        twist.angular.z = clamp(1.5 * yaw_error, -self.max_angular_speed, self.max_angular_speed)

        if abs(yaw_error) < 0.35:
            twist.linear.x = clamp(0.6 * distance, 0.0, self.max_linear_speed)
        else:
            twist.linear.x = 0.0

        self.cmd_pub.publish(twist)

        if self.last_state != 'running':
            self.publish_task_state('running', 'moving to target')
        else:
            self.publish_task_state('running', 'moving to target')


def main(args=None):
    rclpy.init(args=args)
    node = SimpleExecutorNode()
    try:
        rclpy.spin(node)
    finally:
        node.stop_robot()
        node.destroy_node()
        rclpy.shutdown()