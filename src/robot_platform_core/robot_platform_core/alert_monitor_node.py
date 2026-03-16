import rclpy
from rclpy.node import Node
from robot_platform_interfaces.msg import RobotStatus

# 检查1.低电量告警 2.机器人阻塞
class AlertMonitorNode():
    def __init__(self):
        super().__init__('alert_monitor_node')

        self.last_status = None
        # 机器人卡柱的时间（按道理每0.1秒fake_robot_node.py节点就会发布状态消息）
        self.stuck_seconds = 0.0

        self.sub = self.create_subscription(
            RobotStatus, '/robot_1/status', self.status_callback, 10
        )
        # 定时检查状态是否告警
        self.timer = self.create_timer(1.0, self.check_alerts)
        self.get_logger().info('Alert monitor node started.')

    def status_callback(self, msg: RobotStatus):
        self.last_status = msg

    def check_alerts(self):
        if self.last_status is None:
            return
        
        msg = self.last_status
        if msg.battery_pct < 20.0:
            self.get_logger().warn(
                f'Low battery alert: {msg.battery_pct:.1f}%'
            )
        
        if msg.task_id and abs(msg.linear_vel) < 0.01:
            self.stuck_seconds += 1.0
        else:
            self.stuck_seconds = 0.0

        if self.stuck_seconds >= 5.0:
            self.get_logger().warn(
                f'Possible block alert: robot={msg.robot_id}, '
                f'task={msg.task_id}, stuck={self.stuck_seconds:.0f}s'
            )

def main(args=None):
    rclpy.init(args=args)
    node = AlertMonitorNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
