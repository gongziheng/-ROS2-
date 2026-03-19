import threading
# 类型注释
from typing import Dict, Any

import rclpy
from rclpy.node import Node

from robot_platform_interfaces.msg import RobotStatus
from robot_platform_interfaces.srv import SubmitTask

class DashboardRosBridge(Node):
    def __init__(self):
        super().__init__('dashboard_ros_bridge')

        self.latest_status: Dict[str, Any] = {
            'robot_id': 'robot_1',
            'x': 0.0,
            'y': 0.0,
            'yaw': 0.0,
            'linear_vel': 0.0,
            'angular_vel': 0.0,
            'battery_pct': 0.0,
            'mode': 'unknown',
            'task_id': '',
            'is_online': False,
            'alert_text': ''
        }

        # 订阅机器人状态
        self.status_sub = self.create_subscription(
            RobotStatus,
            '/robot_1/status',
            self.status_callback,
            10
        )

        # 创建客户端：和机器人管理系统通信
        self.task_client = self.create_client(
            SubmitTask,
            '/submit_task'
        )

    def status_callback(self, msg: RobotStatus):
        self.latest_status = {
            'robot_id': msg.robot_id,
            'x': float(msg.x),
            'y': float(msg.y),
            'yaw': float(msg.yaw),
            'linear_vel': float(msg.linear_vel),
            'angular_vel': float(msg.angular_vel),
            'battery_pct': float(msg.battery_pct),
            'mode': msg.mode,
            'task_id': msg.task_id,
            'is_online': bool(msg.is_online),
            'alert_text': msg.alert_text
        }

    # 给API 调用
    def submit_task(self, robot_id: str, target_x: float, target_y: float, task_type: str):
        # 判断服务器是否运行（2s超时）
        if not self.task_client.wait_for_service(timeout_sec=2.0):
            return {
                'accepted': False,
                'task_id': '',
                'message': 'submit_task service not avaliable'
            }

        req = SubmitTask.Request()
        req.robot_id = robot_id
        req.target_x = float(target_x)
        req.target_y = float(target_y)
        req.task_type = task_type

        future = self.task_client.call_async(req)

        # 阻塞到获取结果
        # rclpy.spin_until_future_complete(self, future, timeout_sec=3.0)

        # 非阻塞
        def request_callback(result_future):
            if result_future.result() is None:
                return {
                    'accepted': False,
                    'task_id': '',
                    'message': 'service call failed or timed out'
                }
            response = result_future.result()
            return {
                'accepted': True,
                'task_id': resp.task_id,
                'message': resp.message
            }

        future.add_done_callback(request_callback)

    
_ros_node = None
_ros_thread = None

def start_ros_bridge():
    global _ros_node, _ros_thread

    if _ros_node is not None:
        return _ros_node

    if not rclpy.ok():
        rclpy.init()

    _ros_node = DashboardRosBridge()

    def spin():
        rclpy.spin(_ros_node)

    _ros_thread = threading.Thread(target=spin, daemon=True)
    _ros_thread.start()
    return _ros_node

def get_ros_bridge():
    return _ros_node