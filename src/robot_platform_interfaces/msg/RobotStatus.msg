# 话题：机器人向上位机发送当前状态
# 机器人状态消息
# 用于机器人周期性向平台发布“当前状态快照”

# 这条状态消息的时间戳
# 用于轨迹回放、离线检测、日志排序等
builtin_interfaces/Time stamp

# 机器人唯一标识
# 第一版虽然只有 robot_1，但后续多机器人必须保留
string robot_id

# 当前二维平面位置
# 单位默认可理解为米（前后代码里保持一致即可）
float32 x
float32 y

# 当前朝向角（平面旋转角）
# 一般单位为弧度
float32 yaw

# 当前线速度
# 一般单位为 m/s
float32 linear_vel

# 当前角速度
# 一般单位为 rad/s
float32 angular_vel

# 当前电量百分比
# 取值示例：100.0 / 56.3 / 18.5
float32 battery_pct

# 当前工作模式
# 示例：idle / moving / charging / error
string mode

# 当前正在执行的任务 ID
# 如果当前没有任务，可以为空字符串
string task_id

# 机器人是否在线
# true 表示在线，false 表示离线
bool is_online

# 当前告警文本
# 第一版先直接放在状态里，便于页面显示和调试
# 没有告警时可为空字符串
string alert_text