# 异步编程
import asyncio
# 上下文管理库，它常见于 FastAPI 里做应用启动/关闭资源管理，用来写FastAPI的生命周期：服务启动时做什么，服务关闭做什么
from contextlib import asynccontextmanager
# 第三方库 fastAPI  Python Web 框架，用来写接口服务
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Response
from fastapi.responses import HTMLResponse
# 第三方库 数据模型 定义请求体的数据结构
from pydantic import BaseModel

from ros_bridge import start_ros_bridge, stop_ros_bridge, get_ros_bridge

# 请求体模型 BaseModel用来（1）自动解析json （2）检查字段是否齐全 （3）检查类型是否合理
# 定义提交任务接口的输入格式
class SubmitTaskBody(BaseModel):
    robot_id: str
    target_x: float
    target_y: float
    task_type: str

# FastAPI的生命周期管理函数：当FastAPI服务启动时，执行start_ros_bridge把ROS bridge也启动起来
# yield 前面：服务启动时执行
# yield 后面：服务关闭时执行
@asynccontextmanager
async def lifespan(app: FastAPI):
    start_ros_bridge()
    try:
        yield
    finally:
        stop_ros_bridge()

# 创建FastAPI对象（服务名字， 生命周期函数）
app = FastAPI(title='Robot Platform Dashboard API', lifespan=lifespan)

# 定义根路径
@app.get("/", response_class=HTMLResponse)
def index():
    return """
    <html>
      <head><title>Robot Platform Dashboard API</title></head>
      <body>
        <h1>Robot Platform Dashboard API</h1>
        <p>API is running.</p>
        <ul>
          <li><a href="/docs">Swagger Docs</a></li>
          <li><a href="/redoc">ReDoc</a></li>
          <li><a href="/api/health">/api/health</a></li>
          <li><a href="/api/status">/api/status</a></li>
        </ul>
      </body>
    </html>
    """

@app.get("/favicon.ico", include_in_schema=False)
def favicon():
    return Response(status_code=204)

# 健康检查接口
@app.get('/api/health')
def health():
    return {'ok': True}

# 状态检查接口
@app.get('/api/status')
def get_status():
    ros_bridge = get_ros_bridge()
    if ros_bridge is None:
        return {'ok': False, 'message': 'ROS bridge not ready'}
    return {'ok': True, 'data': ros_bridge.get_status_snapshot()}

@app.get('/api/tasks/recent')
def get_recent_tasks():
    ros_bridge = get_ros_bridge()
    if ros_bridge is None:
        return {'ok': False, 'data': []}
    return {'ok': True, 'data': ros_bridge.get_recent_tasks()}


@app.get('/api/alerts/recent')
def get_recent_alerts():
    ros_bridge = get_ros_bridge()
    if ros_bridge is None:
        return {'ok': False, 'data': []}
    return {'ok': True, 'data': ros_bridge.get_recent_alerts()}

# 提交任务接口
@app.post('/api/tasks')
def submit_task(body: SubmitTaskBody):
    ros_bridge = get_ros_bridge()
    if ros_bridge is None:
        return {'ok': False, 'message': 'ROS bridge not ready'}

    result = ros_bridge.submit_task(
        # 这个位置增加请求id呢
        robot_id=body.robot_id,
        target_x=body.target_x,
        target_y=body.target_y,
        task_type=body.task_type
    )
    return {'ok': True, 'data': result}

# websocket 实时状态推送
@app.websocket('/ws/status')
async def websocket_status(websocket: WebSocket):
    await websocket.accept()
    try:
        while True:
            ros_bridge = get_ros_bridge()
            if ros_bridge is not None:
                await websocket.send_json(ros_bridge.get_status_snapshot())
            await asyncio.sleep(0.5)
    except WebSocketDisconnect:
        pass