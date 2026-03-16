import time
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Point
from robot_platform_interfaces.srv import SubmitTask

# 机器人任务管理器
class TaskManagerNode(node):
    def __init__(self):
        super().__init__('task_manager_node')

        # 发布话题：发布机器人的目标位置
        self.goal_pub = self.create_publisher(Point, '/robot1/goal', 10)
        # 创建服务的服务端：平台（客户端）向机器人任务管理器（服务端）发送任务请求
        self.srv = self.create_service(
            SubmitTask, '/submit_task', self.handle_submit_task
        )

        self.current_task_id = ''
        self.current_robot_id = ''
        self.get_logger().info('Task manager node started.')

    def handle_submit_task(task, request, response):
        # time.time()获取当前时间的时间戳(python)
        task_id = f'{request.task_type}_{int(time.time())}'
        self.current_task_id = task_id
        self.current_robot_id = request.robot_id

        goal = Point()
        goal.x = request.target_x
        goal.y = request.target_y
        goal.z = 0.0
        self.goal_pub.publish(goal)

        response.accepted = True
        response.task_id = task_id
        response.message = (
            f'Task accepted for {request.robot_id}: '
            f'({request.target_x:.2f}, {request.target_y:.2f})'
        )

        self.get_logger().info(
            f'Accepted task{task_id} for {request.robot_id}'
        )
        return response

    def main(args=None):
        rclpy.init(args=args)
        node = TaskManagerNode()
        rclpy.spin(node)
        node.destroy_node()
        rclpy.shutdown()