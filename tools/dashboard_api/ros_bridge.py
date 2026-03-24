import threading
import time
from collections import deque
from datetime import datetime
from typing import Any, Dict, List

import rclpy
from rclpy.executors import SingleThreadedExecutor
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
            'mode': 'offline',
            'task_id': '',
            'is_online': False,
            'alert_text': 'ROS status timeout',
        }

        self.last_status_time = 0.0
        self.last_status_monotonic = 0.0
        self.stuck_seconds = 0.0

        self.task_history: deque = deque(maxlen=50)
        self.alert_history: deque = deque(maxlen=100)
        self.active_alert_key = ''

        self.status_sub = self.create_subscription(
            RobotStatus,
            '/robot_1/status',
            self.status_callback,
            10,
        )
        self.task_client = self.create_client(SubmitTask, '/submit_task')

    def _now_str(self) -> str:
        return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    def status_callback(self, msg: RobotStatus):
        now = time.monotonic()
        dt = 0.0 if self.last_status_monotonic == 0.0 else max(0.0, now - self.last_status_monotonic)
        self.last_status_monotonic = now

        prev_task_id = self.latest_status.get('task_id', '')
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
            'alert_text': msg.alert_text,
        }
        self.last_status_time = now

        self._update_task_history(prev_task_id, self.latest_status)
        self._update_alert_history(self.latest_status, dt)

    def _update_task_history(self, prev_task_id: str, status: Dict[str, Any]):
        current_task_id = status.get('task_id', '')
        current_mode = status.get('mode', '')

        if current_task_id:
            for item in self.task_history:
                if item['task_id'] == current_task_id:
                    item['status'] = 'running' if current_mode == 'moving' else current_mode
                    item['updated_at'] = self._now_str()
                    return

        if prev_task_id and not current_task_id:
            for item in self.task_history:
                if item['task_id'] == prev_task_id and item['status'] not in ('finished', 'failed'):
                    item['status'] = 'finished'
                    item['finished_at'] = self._now_str()
                    return

    def _push_alert(self, level: str, code: str, message: str, robot_id: str):
        key = f'{robot_id}:{code}:{message}'
        if self.active_alert_key == key:
            return

        self._resolve_alert()
        self.active_alert_key = key
        self.alert_history.appendleft({
            'key': key,
            'time': self._now_str(),
            'robot_id': robot_id,
            'level': level,
            'code': code,
            'message': message,
            'active': True,
        })

    def _resolve_alert(self):
        if not self.active_alert_key:
            return

        for item in self.alert_history:
            if item['key'] == self.active_alert_key and item['active']:
                item['active'] = False
                item['recovered_at'] = self._now_str()
                break

        self.active_alert_key = ''

    def _update_alert_history(self, status: Dict[str, Any], dt: float):
        robot_id = status.get('robot_id', 'robot_1')
        text = (status.get('alert_text') or '').strip()
        task_id = status.get('task_id', '')
        linear_vel = abs(float(status.get('linear_vel', 0.0)))

        if not status.get('is_online', True):
            self.stuck_seconds = 0.0
            self._push_alert('critical', 'offline', text or 'ROS status timeout', robot_id)
            return

        battery = float(status.get('battery_pct', 100.0))
        if battery < 20.0:
            self.stuck_seconds = 0.0
            self._push_alert('warning', 'low_battery', f'Low battery: {battery:.1f}%', robot_id)
            return

        if task_id and linear_vel < 0.01:
            self.stuck_seconds += dt
        else:
            self.stuck_seconds = 0.0

        if task_id and self.stuck_seconds >= 5.0:
            self._push_alert('warning', 'blocked', f'Robot blocked for {self.stuck_seconds:.0f}s', robot_id)
            return

        if text:
            self._push_alert('warning', 'robot_alert', text, robot_id)
            return

        self._resolve_alert()

    def get_status_snapshot(self) -> Dict[str, Any]:
        snapshot = dict(self.latest_status)

        if self.last_status_time == 0.0 or (time.monotonic() - self.last_status_time) > 1.5:
            snapshot['is_online'] = False
            snapshot['mode'] = 'offline'
            snapshot['alert_text'] = 'ROS status timeout'
            self.stuck_seconds = 0.0
            self._push_alert('critical', 'offline', 'ROS status timeout', snapshot['robot_id'])

        return snapshot

    def get_recent_tasks(self) -> List[Dict[str, Any]]:
        return list(self.task_history)

    def get_recent_alerts(self) -> List[Dict[str, Any]]:
        return list(self.alert_history)

def submit_task(self, robot_id: str, target_x: float, target_y: float, task_type: str) -> Dict[str, Any]:
    if not self.task_client.wait_for_service(timeout_sec=2.0):
        return {
            'accepted': False,
            'task_id': '',
            'message': 'submit_task service not available',
        }

    req = SubmitTask.Request()
    req.robot_id = robot_id
    req.target_x = float(target_x)
    req.target_y = float(target_y)
    req.task_type = task_type

    future = self.task_client.call_async(req)

    done_event = threading.Event()
    result_holder = {'result': None}

    def _done_callback(fut):
        try:
            result_holder['result'] = fut.result()
        except Exception:
            result_holder['result'] = None
        finally:
            done_event.set()

    future.add_done_callback(_done_callback)

    if not done_event.wait(timeout=3.0):
        return {
            'accepted': False,
            'task_id': '',
            'message': 'service call timeout',
        }

    res = result_holder['result']
    if res is None:
        return {
            'accepted': False,
            'task_id': '',
            'message': 'service call failed',
        }

    result = {
        'accepted': bool(res.accepted),
        'task_id': res.task_id,
        'message': res.message,
    }

    if result['accepted']:
        self.task_history.appendleft({
            'task_id': res.task_id,
            'robot_id': robot_id,
            'task_type': task_type,
            'target_x': float(target_x),
            'target_y': float(target_y),
            'target': f'({target_x:.2f}, {target_y:.2f})',
            'status': 'accepted',
            'created_at': self._now_str(),
        })

    return result


_ros_node = None
_ros_thread = None
_executor = None


def start_ros_bridge():
    global _ros_node, _ros_thread, _executor

    if _ros_node is not None:
        return _ros_node

    if not rclpy.ok():
        rclpy.init()

    _ros_node = DashboardRosBridge()
    _executor = SingleThreadedExecutor()
    _executor.add_node(_ros_node)

    def spin():
        _executor.spin()

    _ros_thread = threading.Thread(target=spin, daemon=True)
    _ros_thread.start()
    return _ros_node


def stop_ros_bridge():
    global _ros_node, _ros_thread, _executor

    if _executor is not None:
        _executor.shutdown()
        _executor = None

    if _ros_node is not None:
        _ros_node.destroy_node()
        _ros_node = None

    if rclpy.ok():
        rclpy.shutdown()

    _ros_thread = None


def get_ros_bridge():
    return _ros_node