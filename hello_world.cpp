#include <iostream>
#include <vector>
#include <string>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include <random>
#include <fstream>

// 参数初始化
const int clear_interval = 30; // 分钟
const int clear_period = 24 * 60; // 优化周期，分钟
const int ctrl_interval = 1; // 控制变量步长，分钟
const double charge_eff = 0.91;
const double discharge_eff = 0.95;
const double nominal_power = 0.8;
const double SOC_ub = 1;
const double SOC_lb = 0;
const double SOC0 = 0.5;
const double Ckwh = 1; // 电池容量 (kWh)

int main()
{
    // 初始化 SCIP
    SCIP* scip = nullptr;
    SCIP_CALL(SCIPcreate(&scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    SCIP_CALL(SCIPcreateProb(scip, "Electricity Optimization", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
    SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE));

    // 生成随机的用户负荷和电价
    std::vector<double> user_loads, user_powers, elec_price;
    std::default_random_engine generator(1234);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    for (int i = 0; i < clear_period / clear_interval; ++i)
    {
        double load = distribution(generator);
        double power = distribution(generator);
        for (int j = 0; j < clear_interval / ctrl_interval; ++j){
            user_loads.push_back(load);
            user_powers.push_back(power);
        }
    }
    for (int i = 0; i < clear_period / 240; ++i)
    {
        double price = distribution(generator) / 5 + 0.4;
        for (int j = 0; j < 240; ++j){
            elec_price.push_back(price); // 保存电价
        }
    }

    // 初始化决策变量
    std::vector<SCIP_VAR*> x_vars, y_vars, soc_vars, z_vars;
    for (int i = 0; i < clear_period / ctrl_interval; ++i)
    {
        SCIP_VAR* x;
        SCIP_VAR* y;
        SCIP_VAR* soc;
        SCIP_VAR* z;

        // 创建变量
        SCIP_CALL(SCIPcreateVarBasic(scip, &x, ("x_" + std::to_string(i)).c_str(), 0.0, nominal_power, 0.0, SCIP_VARTYPE_CONTINUOUS));
        SCIP_CALL(SCIPcreateVarBasic(scip, &y, ("y_" + std::to_string(i)).c_str(), -nominal_power, 0.0, 0.0, SCIP_VARTYPE_CONTINUOUS));
        SCIP_CALL(SCIPcreateVarBasic(scip, &soc, ("soc_" + std::to_string(i)).c_str(), SOC_lb, SOC_ub, 0.0, SCIP_VARTYPE_CONTINUOUS));
        SCIP_CALL(SCIPcreateVarBasic(scip, &z, ("z_" + std::to_string(i)).c_str(), 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY));

        // 添加变量到模型
        SCIP_CALL(SCIPaddVar(scip, x));
        SCIP_CALL(SCIPaddVar(scip, y));
        SCIP_CALL(SCIPaddVar(scip, soc));
        SCIP_CALL(SCIPaddVar(scip, z));

        x_vars.push_back(x);
        y_vars.push_back(y);
        soc_vars.push_back(soc);
        z_vars.push_back(z);
    }

    // 目标函数
    SCIP_Real obj_coeffs[clear_period / ctrl_interval];
    SCIP_VAR* obj_vars[clear_period / ctrl_interval];

    for (int i = 0; i < clear_period / ctrl_interval; ++i)
    {
        double user_load = user_loads[i];
        double user_power = user_powers[i];
        double price = elec_price[i];

        // 目标函数的系数
        obj_coeffs[i] = (user_load - user_power) * price;
        obj_vars[i] = x_vars[i]; // 目标变量可以是x或y，根据需求调整
    }

    // 修正这里，使用有效的约束名称
    SCIP_CALL(SCIPcreateConsBasicLinear(scip, nullptr,"约束名臣", obj_vars, obj_coeffs, 0.0, SCIPinfinity(scip)));
    SCIP_CALL(SCIPaddCons(scip, nullptr)); // 添加目标函数约束

    // 添加初始SOC约束
    SCIP_CONS* cons; // 声明 cons 变量
    SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cons, "soc_initial", 1, &soc_vars[0], (SCIP_Real[]){1.0}, SOC0, SOC0));
    SCIP_CALL(SCIPaddCons(scip, cons));

    // 更新SOC约束
    for (int i = 0; i < clear_period / ctrl_interval - 1; ++i)
    {
        SCIP_VAR* vars[] = {soc_vars[i], soc_vars[i + 1], x_vars[i], y_vars[i]};
        SCIP_Real coeffs[] = {1.0, -1.0, charge_eff / (60.0 * Ckwh) * ctrl_interval, -1.0 / discharge_eff / (60.0 * Ckwh) * ctrl_interval};

        SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cons, ("soc_update_" + std::to_string(i)).c_str(), 4, vars, coeffs, 0.0, SCIPinfinity(scip)));
        SCIP_CALL(SCIPaddCons(scip, cons));
    }

    // 添加充电/放电互斥约束
    for (int i = 0; i < clear_period / ctrl_interval; ++i)
    {
        SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cons, ("charge_exclusivity_" + std::to_string(i)).c_str(), 2, &z_vars[i], nullptr, 0.0, nominal_power));
        SCIP_CALL(SCIPaddCons(scip, cons));

        SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cons, ("discharge_exclusivity_" + std::to_string(i)).c_str(), 2, &z_vars[i], nullptr, -nominal_power, 0.0));
        SCIP_CALL(SCIPaddCons(scip, cons));
    }

    // 求解模型
    SCIP_CALL(SCIPsolve(scip));
    SCIP_SOL* solution = SCIPgetBestSol(scip);

    if (solution != nullptr)
    {
        // 显示优化后的目标函数值
        SCIP_Real objective_value = SCIPgetSolOrigObj(scip, solution);
        std::cout << "Optimized objective function value: " << objective_value << std::endl;

        std::ofstream result_file("optimization_results.csv");
        result_file << "Time,User Load,User Power,Charge,Discharge,SOC\n";

        for (int i = 0; i < clear_period / ctrl_interval; ++i)
        {
            double x_val = SCIPgetSolVal(scip, solution, x_vars[i]);
            double y_val = SCIPgetSolVal(scip, solution, y_vars[i]);
            double soc_val = SCIPgetSolVal(scip, solution, soc_vars[i]);

            result_file << i << "," << user_loads[i] << "," << user_powers[i] << "," << x_val << "," << y_val << "," << soc_val << "\n";
        }
        result_file.close();
        std::cout << "Results saved to 'optimization_results.csv'\n";
    }
    else
    {
        std::cout << "No feasible solution found.\n";
    }

    // 释放所有决策变量
    for (SCIP_VAR* var : x_vars) {
        SCIP_CALL(SCIPreleaseVar(scip, &var));
    }
    for (SCIP_VAR* var : y_vars) {
        SCIP_CALL(SCIPreleaseVar(scip, &var));
    }
    for (SCIP_VAR* var : soc_vars) {
        SCIP_CALL(SCIPreleaseVar(scip, &var));
    }
    for (SCIP_VAR* var : z_vars) {
        SCIP_CALL(SCIPreleaseVar(scip, &var));
    }

    // 然后释放 SCIP 问题
    SCIP_CALL(SCIPfree(&scip));

    return 0;
}
