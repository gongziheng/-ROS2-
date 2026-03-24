import math
import rclpy
from rclpy.node import Node
# from geometry_msgs.msg import Point
from robot_platform_interfaces.msg import TaskCommand
from robot_platform_interfaces.msg import RobotStatus

# 机器人根据机器人任务管理器发布的位置信息移动，并定时发布当前的位置信息
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
        # 发布当前机器人的状态
        self.status_pub = self.create_publisher(
            RobotStatus, '/robot_1/status', 10
        )
        # 订阅robot_1/goal，获取机器人的目标位置
        self.task_sub = self.create_subscription(
            TaskCommand, '/robot_1/task_cmd', self.task_callback, 10
        )
        # 定时器：定时发布机器人的状态
        self.timer = self.create_timer(0.1, self.on_timer)
        self.get_logger().info(f'Fake robot node started. {self.robot_id}')

    def task_callback(self, msg: TaskCommand):
        self.target_x = float(msg.target_x)
        self.target_y = float(msg.target_y)
        self.task_id = msg.task_id
        self.task_type = msg.task_type
        self.mode = 'moving'

        self.get_logger().info(
            f'Received task: id={self.task_id}, type={self.task_type}, '
            f'goal=({self.target_x:.2f}, {self.target_y:.2f})'
        )

    def on_timer(self):
        dt = 0.1
        speed = 0.5

        if self.target_x is not None and self.target_y is not None:
        # if None not in(self.target_x, self.target_y):
            dx = self.target_x - self.x
            dy = self.target_y - self.y
            dist = math.hypot(dx, dy)   # 等价于 sqrt(x*x + y*y)

            if dist > 0.05:
                self.yaw = math.atan2(dy, dx)
                step = min(speed * dt, dist)
                self.x += step * math.cos(self.yaw)
                self.y += step * math.sin(self.yaw)
                self.linear_vel = speed
                self.angular_vel = 0.0
                self.mode = 'moving'
            else:
                self.x = self.target_x
                self.y = self.target_y
                self.target_x = None
                self.target_y = None
                self.linear_vel = 0.0
                self.angular_vel = 0.0
                self.mode = 'idle'
                self.get_logger().info(f'Goal reached: task={self.task_id}')
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
    rclpy.spin(node)
    # node.destroy_node()
    rclpy.shutdown()