import math
from typing import Optional

import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from robot_platform_interfaces.msg import RobotStatus, TaskState


def yaw_from_quaternion(x: float, y: float, z: float, w: float) -> float:
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    return math.atan2(siny_cosp, cosy_cosp)


class StatusAdapterNode(Node):
    def __init__(self):
        super().__init__('status_adapter_node')

        self.declare_parameter('robot_id', 'robot_1')
        self.declare_parameter('odom_topic', '/odom')
        self.declare_parameter('task_state_topic', '/robot_1/task_state')
        self.declare_parameter('status_topic', '/robot_1/status')
        self.declare_parameter('offline_timeout_sec', 1.0)

        self.robot_id = str(self.get_parameter('robot_id').value)
        self.odom_topic = str(self.get_parameter('odom_topic').value)
        self.task_state_topic = str(self.get_parameter('task_state_topic').value)
        self.status_topic = str(self.get_parameter('status_topic').value)
        self.offline_timeout_sec = float(self.get_parameter('offline_timeout_sec').value)

        self.status_pub = self.create_publisher(RobotStatus, self.status_topic, 10)
        self.odom_sub = self.create_subscription(Odometry, self.odom_topic, self.on_odom, 10)
        self.task_state_sub = self.create_subscription(
            TaskState, self.task_state_topic, self.on_task_state, 10
        )
        self.timer = self.create_timer(0.2, self.on_timer)

        self.current_x = 0.0
        self.current_y = 0.0
        self.current_yaw = 0.0
        self.linear_vel = 0.0
        self.angular_vel = 0.0

        self.current_task_id = ''
        self.current_task_state = ''
        self.current_task_message = ''

        self.battery_pct = 95.0
        self.last_odom_time: Optional[rclpy.time.Time] = None

        self.get_logger().info(
            f'status_adapter_node started, odom={self.odom_topic}, status={self.status_topic}'
        )

    def on_odom(self, msg: Odometry):
        self.current_x = float(msg.pose.pose.position.x)
        self.current_y = float(msg.pose.pose.position.y)

        q = msg.pose.pose.orientation
        self.current_yaw = yaw_from_quaternion(q.x, q.y, q.z, q.w)

        self.linear_vel = float(msg.twist.twist.linear.x)
        self.angular_vel = float(msg.twist.twist.angular.z)

        self.last_odom_time = self.get_clock().now()

    def on_task_state(self, msg: TaskState):
        if msg.robot_id != self.robot_id:
            return

        self.current_task_id = msg.task_id
        self.current_task_state = msg.state
        self.current_task_message = msg.message

        if msg.state in ('succeeded', 'failed'):
            # 保留最后一个任务号一会儿也可以，这里先直接保留 task_id
            pass

    def calc_online(self) -> bool:
        if self.last_odom_time is None:
            return False
        dt = (self.get_clock().now() - self.last_odom_time).nanoseconds / 1e9
        return dt <= self.offline_timeout_sec

    def update_battery(self):
        moving = abs(self.linear_vel) > 0.02 or abs(self.angular_vel) > 0.02
        busy = self.current_task_state in ('pending', 'running')

        if moving or busy:
            self.battery_pct = max(5.0, self.battery_pct - 0.03)
        else:
            self.battery_pct = min(100.0, self.battery_pct + 0.01)

    def on_timer(self):
        online = self.calc_online()
        self.update_battery()

        moving = abs(self.linear_vel) > 0.02 or abs(self.angular_vel) > 0.02
        busy = self.current_task_state in ('pending', 'running')

        mode = 'idle'
        if moving:
            mode = 'moving'
        elif busy:
            mode = 'busy'

        alert_text = ''
        if not online:
            alert_text = 'robot offline'
        elif self.battery_pct < 20.0:
            alert_text = 'low battery'

        msg = RobotStatus()
        msg.robot_id = self.robot_id
        msg.x = float(self.current_x)
        msg.y = float(self.current_y)
        msg.yaw = float(self.current_yaw)
        msg.linear_vel = float(self.linear_vel)
        msg.angular_vel = float(self.angular_vel)
        msg.battery_pct = float(self.battery_pct)
        msg.mode = mode
        msg.task_id = self.current_task_id
        msg.is_online = bool(online)
        msg.alert_text = alert_text

        self.status_pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = StatusAdapterNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()