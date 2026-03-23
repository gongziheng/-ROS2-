
# 为当前项目创建一个独立的虚拟环境，不污染系统Python
# python3 -m venv .venv

# 激活这个虚拟环境
# source .venv/bin/activate

# 安装需要的库
# fastapi：负责“你怎么写接口”
# uvicorn：负责“把这个接口真正跑起来，对外提供 HTTP / WebSocket 服务”
# pip install fastapi uvicorn PyYAML numpy

# 把当前虚拟环境里已经安装的所有 Python 依赖及版本导出到 requirements.txt 文件中。
# pip freeze > requirements.txt