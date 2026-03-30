import threading
import time
from collections import deque
from datetime import datetime
from typing import Any, Dict, List

import rclpy
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node

from robot_platform_interfaces.msg import RobotStatus, TaskState
from robot_platform_interfaces.srv import SubmitTask


class DashboardRosBridge(Node):
    def __init__(self):
        super().__init__('dashboard_ros_bridge')

        self._lock = threading.RLock()

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

        self.max_task_history = 50
        self.task_history: deque = deque()
        self.task_index: Dict[str, Dict[str, Any]] = {}

        self.alert_history: deque = deque(maxlen=100)
        self.active_alert_key = ''

        self.status_sub = self.create_subscription(
            RobotStatus,
            '/robot_1/status',
            self.status_callback,
            10,
        )

        self.task_state_sub = self.create_subscription(
            TaskState,
            '/robot_1/task_state',
            self.task_state_callback,
            10,
        )

        self.task_client = self.create_client(SubmitTask, '/submit_task')

    def _now_str(self) -> str:
        return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    def _trim_task_history(self) -> None:
        while len(self.task_history) > self.max_task_history:
            removed = self.task_history.pop()
            task_id = removed.get('task_id', '')
            if task_id:
                self.task_index.pop(task_id, None)

    def _append_task_history_item(
        self,
        *,
        task_id: str,
        robot_id: str,
        task_type: str,
        target_x: float,
        target_y: float,
        status: str,
        message: str,
    ) -> None:
        now = self._now_str()

        item = {
            'task_id': task_id,
            'robot_id': robot_id,
            'task_type': task_type,
            'target_x': float(target_x),
            'target_y': float(target_y),
            'target': f'({target_x:.2f}, {target_y:.2f})',
            'status': status,
            'message': message,
            'created_at': now,
            'updated_at': now,
            'current_x': None,
            'current_y': None,
            'distance_remaining': None,
        }

        self.task_history.appendleft(item)
        if task_id:
            self.task_index[task_id] = item
        self._trim_task_history()

    def _upsert_task_state_item(self, msg: TaskState) -> None:
        now = self._now_str()
        item = self.task_index.get(msg.task_id)

        if item is None:
            item = {
                'task_id': msg.task_id,
                'robot_id': msg.robot_id,
                'task_type': msg.task_type,
                'target_x': float(msg.target_x),
                'target_y': float(msg.target_y),
                'target': f'({msg.target_x:.2f}, {msg.target_y:.2f})',
                'status': msg.state,
                'message': msg.message,
                'created_at': now,
                'updated_at': now,
                'current_x': float(msg.current_x),
                'current_y': float(msg.current_y),
                'distance_remaining': float(msg.distance_remaining),
            }
            self.task_history.appendleft(item)
            self.task_index[msg.task_id] = item
            self._trim_task_history()
        else:
            item['robot_id'] = msg.robot_id
            item['task_type'] = msg.task_type
            item['target_x'] = float(msg.target_x)
            item['target_y'] = float(msg.target_y)
            item['target'] = f'({msg.target_x:.2f}, {msg.target_y:.2f})'
            item['status'] = msg.state
            item['message'] = msg.message
            item['updated_at'] = now
            item['current_x'] = float(msg.current_x)
            item['current_y'] = float(msg.current_y)
            item['distance_remaining'] = float(msg.distance_remaining)

        if msg.state == 'running' and 'started_at' not in item:
            item['started_at'] = now

        if msg.state in ('succeeded', 'failed') and 'finished_at' not in item:
            item['finished_at'] = now

    def status_callback(self, msg: RobotStatus):
        now = time.monotonic()
        dt = 0.0 if self.last_status_monotonic == 0.0 else max(0.0, now - self.last_status_monotonic)

        with self._lock:
            self.last_status_monotonic = now
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
            self._update_alert_history(self.latest_status, dt)

    def task_state_callback(self, msg: TaskState):
        with self._lock:
            self._upsert_task_state_item(msg)

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
        with self._lock:
            snapshot = dict(self.latest_status)

            if self.last_status_time == 0.0 or (time.monotonic() - self.last_status_time) > 3.5:
                snapshot['is_online'] = False
                snapshot['mode'] = 'offline'
                snapshot['alert_text'] = 'ROS status timeout'
                self.stuck_seconds = 0.0
                self._push_alert('critical', 'offline', 'ROS status timeout', snapshot['robot_id'])

            return snapshot

    def get_recent_tasks(self) -> List[Dict[str, Any]]:
        with self._lock:
            return [dict(item) for item in self.task_history]

    def get_recent_alerts(self) -> List[Dict[str, Any]]:
        with self._lock:
            return [dict(item) for item in self.alert_history]

    def _state_rank(self, state: str) -> int:
        order = {
            'accepted': 0,
            'pending': 1,
            'running': 2,
            'succeeded': 3,
            'failed': 3,
            'rejected': 3,
            'service_unavailable': 3,
            'timeout': 3,
        }
        return order.get(state or '', 0)

    def _upsert_submit_result_item(
        self,
        *,
        task_id: str,
        robot_id: str,
        task_type: str,
        target_x: float,
        target_y: float,
        accepted: bool,
        message: str,
    ) -> None:
        now = self._now_str()
        new_status = 'accepted' if accepted else 'rejected'

        item = self.task_index.get(task_id) if task_id else None
        if item is None:
            item = {
                'task_id': task_id,
                'robot_id': robot_id,
                'task_type': task_type,
                'target_x': float(target_x),
                'target_y': float(target_y),
                'target': f'({target_x:.2f}, {target_y:.2f})',
                'status': new_status,
                'message': message,
                'created_at': now,
                'updated_at': now,
                'current_x': None,
                'current_y': None,
                'distance_remaining': None,
            }
            self.task_history.appendleft(item)
            if task_id:
                self.task_index[task_id] = item
            self._trim_task_history()
            return

        item['robot_id'] = robot_id
        item['task_type'] = task_type
        item['target_x'] = float(target_x)
        item['target_y'] = float(target_y)
        item['target'] = f'({target_x:.2f}, {target_y:.2f})'
        item['updated_at'] = now

        # 不要把 pending/running/succeeded 又倒退回 accepted
        if self._state_rank(new_status) >= self._state_rank(item.get('status', '')):
            item['status'] = new_status
            item['message'] = message
        else:
            item['submit_message'] = message

    def submit_task(self, robot_id: str, target_x: float, target_y: float, task_type: str) -> Dict[str, Any]:
        if not self.task_client.wait_for_service(timeout_sec=2.0):
            with self._lock:
                self._append_task_history_item(
                    task_id='',
                    robot_id=robot_id,
                    task_type=task_type,
                    target_x=target_x,
                    target_y=target_y,
                    status='service_unavailable',
                    message='submit_task service not available',
                )
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
            with self._lock:
                self._append_task_history_item(
                    task_id='',
                    robot_id=robot_id,
                    task_type=task_type,
                    target_x=target_x,
                    target_y=target_y,
                    status='timeout',
                    message='service call timeout',
                )
            return {
                'accepted': False,
                'task_id': '',
                'message': 'service call timeout',
            }

        res = result_holder['result']
        if res is None:
            with self._lock:
                self._append_task_history_item(
                    task_id='',
                    robot_id=robot_id,
                    task_type=task_type,
                    target_x=target_x,
                    target_y=target_y,
                    status='failed',
                    message='service call failed',
                )
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

        with self._lock:
            self._upsert_submit_result_item(
                task_id=res.task_id,
                robot_id=robot_id,
                task_type=task_type,
                target_x=target_x,
                target_y=target_y,
                accepted=bool(res.accepted),
                message=res.message,
            )

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