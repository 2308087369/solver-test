import random
import pandas as pd
import numpy as np
import pulp
from pulp import GLPK_CMD
import tempfile
import time

# 设置随机种子
random.seed(1234)

# 参数设置
clear_interval = 30
clear_period = 24 * 600
ctrl_interval = 1

if clear_period % clear_interval != 0 or clear_interval % ctrl_interval != 0:
    raise ValueError("clear_period 必须是 clear_interval 的整数倍")

n_clearance = clear_period // clear_interval
n_ctrl = clear_period // ctrl_interval

# 生成随机数据
user_loads = [round(random.uniform(0, 1), 2) for _ in range(n_clearance)]
user_powers = [round(random.uniform(0, 1), 2) for _ in range(n_clearance)]
user_loads = np.repeat(user_loads, clear_interval // ctrl_interval)
user_powers = np.repeat(user_powers, clear_interval // ctrl_interval)
netload = [user_loads[i] - user_powers[i] for i in range(n_ctrl)]
print("净负荷：", netload)

elec_price = []
for i in range(0, n_clearance, 12):
    price = round(0.5 + random.uniform(-0.2, 0.2), 2)
    elec_price.extend([price] * 12)
elec_price = np.repeat(elec_price, clear_interval // ctrl_interval)
print('电价：', elec_price)

# 储能系统参数
charge_eff = 0.91
discharge_eff = 0.95
nominal_power = 0.8
SOC_ub = 1
SOC_lb = 0
SOC0 = 0.5
Ckwh = 1

# 创建Pulp模型
model = pulp.LpProblem("Electricity_Optimization", pulp.LpMinimize)

# 变量定义
x = [pulp.LpVariable(f"x_{i}", lowBound=0, upBound=nominal_power) for i in range(n_ctrl)]
y = [pulp.LpVariable(f"y_{i}", lowBound=-nominal_power, upBound=0) for i in range(n_ctrl)]
soc = [pulp.LpVariable(f"soc_{i}", lowBound=SOC_lb, upBound=SOC_ub) for i in range(n_ctrl)]
z = [pulp.LpVariable(f"z_{i}", cat="Binary") for i in range(n_ctrl)]

# 无储能下的花费
cost_base = sum(((user_loads[i] - user_powers[i]) * elec_price[i]) for i in range(n_ctrl))
print("无储能下的花费：", cost_base)

# 目标函数
total_cost = pulp.lpSum(
    (user_loads[i] - user_powers[i] + x[i] + y[i]) * elec_price[i] for i in range(n_ctrl)
)
model += total_cost

# 初始 SOC 约束
model += soc[0] == SOC0

# 添加 SOC 更新和充放电限制约束
for i in range(n_ctrl - 1):
    model += soc[i + 1] == soc[i] + x[i] * (charge_eff * ctrl_interval / 60 / Ckwh) + y[i] * (ctrl_interval / discharge_eff / 60 / Ckwh)
    model += x[i + 1] <= x[i] + 0.01
    model += x[i + 1] >= x[i] - 0.01
    model += y[i + 1] <= y[i] + 0.01
    model += y[i + 1] >= y[i] - 0.01

for i in range(n_ctrl):
    model += x[i] <= z[i] * nominal_power
    model += -y[i] <= (1 - z[i]) * nominal_power

# 使用 GLPK 进行求解
solution = model.solve(GLPK_CMD(msg=True))  # msg=True 用于显示 GLPK 输出日志

# 检查解并输出结果
if pulp.LpStatus[model.status] == 'Optimal':
    print("优化后的总花费：", pulp.value(model.objective))
    
    charge_values = [pulp.value(x[i]) for i in range(n_ctrl)]
    discharge_values = [pulp.value(y[i]) for i in range(n_ctrl)]
    soc_values = [pulp.value(soc[i]) for i in range(n_ctrl)]
    
    results = pd.DataFrame({
        '时间段': range(n_ctrl),
        '用户负荷': user_loads,
        '用户功率': user_powers,
        '电价': elec_price,
        '充电功率': charge_values,
        '放电功率': discharge_values,
        'SOC': soc_values
    })
    
    results.to_excel('optimization_results.xlsx', index=False)
    print("优化结果已保存为 'optimization_results.xlsx'")
else:
    print("未找到可行解")
