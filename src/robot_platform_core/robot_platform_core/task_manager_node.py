import time
import rclpy
from rclpy.node import Node
# from geometry_msgs.msg import Point
from robot_platform_interfaces.msg import TaskCommand
from robot_platform_interfaces.srv import SubmitTask

# 机器人任务管理器
class TaskManagerNode(Node):
    def __init__(self):
        super().__init__('task_manager_node')

        # 发布话题：发布机器人的目标位置
        # self.goal_pub = self.create_publisher(Point, '/robot_1/goal', 10)
        self.cmd_pub = self.create_publisher(TaskCommand, '/robot_1/task_cmd', 10)
        # 创建服务的服务端：平台（客户端）向机器人任务管理器（服务端）发送任务请求
        self.srv = self.create_service(
            SubmitTask, '/submit_task', self.handle_submit_task
        )

        self.current_task_id = ''
        self.current_robot_id = ''
        self.get_logger().info('Task manager node started.')

    def handle_submit_task(self, request, response):
        # 判断机器人Id是否为空
        if not request.robot_id.strip():
            response.accepted = False
            response.task_id = ''
            response.message = 'robot_id is required'
            return response

        # time.time()获取当前时间的时间戳(python)
        # 创建一个任务ID
        task_id = f'{request.task_type}_{int(time.time())}'
        self.current_task_id = task_id
        self.current_robot_id = request.robot_id

        cmd = TaskCommand()
        cmd.stamp = self.get_clock().now().to_msg()
        cmd.robot_id = request.robot_id
        cmd.task_id = task_id
        cmd.task_type = request.task_type
        cmd.target_x = float(request.target_x)
        cmd.target_y = float(request.target_y)

        self.cmd_pub.publish(cmd)

        response.accepted = True
        response.task_id = task_id
        response.message = (
            f'Task accepted for {request.robot_id}: '
            f'({request.target_x:.2f}, {request.target_y:.2f})'
        )

        self.get_logger().info(
            f'Accepted task={task_id} robot={request.robot_id} '
            f'goal=({request.target_x:.2f}, {request.target_y:.2f})'
        )
        return response

def main(args=None):
    rclpy.init(args=args)
    node = TaskManagerNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()