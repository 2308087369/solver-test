import numpy as np
from mindoptpy import *

foods = ["BEEF", "CHK", "FISH", "HAM", "MCH", "MTL", "SPG", "TUR"]
costs = [3.19, 2.59, 2.29, 2.89, 1.89, 1.99, 1.99, 2.49]
lbs = [0, 0, 0, 0, 0, 0, 0, 0]
ubs = [100, 100, 100, 100, 100, 100, 100, 100]

nutri = ["A", "B1", "B2", "C"]
f_min = [700, 700, 700, 700]
f_max = [10000, 10000, 10000, 10000]

foods_nutri = np.array([
    [60, 20, 10, 15],
    [8, 0, 20, 20],
    [8, 10, 15, 10],
    [0, 40, 35, 10],
    [15, 35, 0, 15],
    [70, 30, 15, 0],
    [0, 50, 25, 15],
    [60, 0, 15, 0]
])

model = Model()
vars = model.addMVar((len(foods),), lb=lbs, ub=ubs, obj=costs, name="food")
print(nutri)
constrs = model.addConstr(vars @ foods_nutri <= 0, name="nutri")
constrs.lhs = f_min
constrs.rhs = f_max
model.modelsense = 1
model.optimize()

if model.status == MDO.OPTIMAL:
    print(f"Optimal objective value is: {model.objval}")

    print("Best solution found.")
    print("Minimum cost is: {}.".format(model.objval))
    print("Recipe:")
    x = vars.X
    for i in range(len(foods)):
        if x[i] != 0:
            print("{} = {}".format(foods[i], x[i]))
else:
    print("No feasible solution.")
