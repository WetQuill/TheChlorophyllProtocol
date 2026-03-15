# 单机确定性开发清单（联机前置）

本清单用于指导项目先完成“单机确定性 + 可回放”，再进入 ENet 联机阶段。

## 0. 总体目标与闸门
- [ ] 目标：同一输入命令流在同一版本下可 100% 复现结果。
- [ ] 闸门 G1：逻辑层（`src/logic`）禁止 `float/double`。
- [ ] 闸门 G2：固定 Tick（建议 30 TPS）稳定运行，渲染不影响逻辑结果。
- [ ] 闸门 G3：回放 100 次状态哈希一致后，才允许进入联机开发。

## 1. 项目脚手架与工程配置（P0）
- [x] 建立目录：`src/app`、`src/logic`、`src/render`、`src/data`、`src/tests`、`assets`、`tools`。
- [x] 增加 CMake：根 `CMakeLists.txt` + 测试 target。
- [x] 增加基础依赖接入：SFML、EnTT、GoogleTest（可选开关集成）。
- [x] 定义编译选项：C++20、警告级别、Debug/Release 配置。
- [x] 在 `AGENTS.md` 中更新实际可执行 build/test/lint 命令。

验收标准：
- [x] `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` 可成功配置（本机环境验证）。
- [x] `cmake --build build --config Debug -j` 可成功构建（本机环境验证）。
- [x] `ctest --test-dir build --output-on-failure` 可执行并产出结果（`DeterminismSmoke` 已通过）。

## 2. 确定性基础设施（P0）
- [x] 实现 `FixedPoint`（`int32_t` + 固定缩放，当前为 1000）。
- [x] 实现确定性 PRNG（固定 seed，可由 `matchSeed + tick` 派生）。
- [x] 提供 `DeterminismConfig`（tickrate、尺度、随机种子）。
- [x] 增加静态检查策略：禁止逻辑层浮点类型（CTest + Python 脚本检查）。
- [x] 约束容器/遍历：提供稳定排序辅助工具（`StableOrder`）。

建议文件：
- `src/logic/math/FixedPoint.h`
- `src/logic/math/FixedPoint.cpp`
- `src/logic/core/DeterministicRng.h`
- `src/logic/core/DeterminismConfig.h`
- `src/logic/core/StableOrder.h`
- `tools/check_logic_no_float.py`

验收标准：
- [x] `FixedPoint` 的加减乘除、比较、溢出边界测试已实现（`FixedPointMath`）。
- [x] 同 seed + 同调用序列下 PRNG 输出一致测试已实现（`DeterministicRngSequence`）。

待本机执行验证命令：
- [ ] `ctest --test-dir build -C Debug -R "FixedPointMath|DeterministicRngSequence|LogicNoFloatCheck" --output-on-failure`

## 3. 主循环与时间系统（P0）
- [x] 实现 App 主循环：渲染帧率可变，逻辑帧固定步长（`GameLoop` + `Main` 示例驱动）。
- [x] 逻辑调度器按固定顺序执行系统（`TickScheduler` 在 Tick 边界回调逻辑更新）。
- [x] 加入插值结构（仅渲染使用，不回写逻辑状态，使用 `interpolationPermille`）。
- [x] 增加慢帧保护策略（最大补帧数上限，超限后丢弃积压整 Tick 并保留余量）。

建议文件：
- `src/app/Main.cpp`
- `src/app/GameLoop.h`
- `src/logic/core/TickScheduler.h`
- `src/logic/core/TickScheduler.cpp`
- `src/tests/TickSchedulerTest.cpp`

验收标准：
- [x] 在不同渲染帧率下，给定同命令流，逻辑结果一致（测试用例 `TickSchedulerDeterminism` 已实现）。

待本机执行验证命令：
- [ ] `ctest --test-dir build -C Debug -R "TickSchedulerDeterminism" --output-on-failure`

## 4. ECS 基础模型（P0）
- [x] 定义核心组件：`Transform`、`Velocity`、`Health`、`Team`、`Identity`。
- [x] 定义玩法组件：`Production`、`Weapon`、`Vision`、`PowerConsumer`。
- [x] 定义命令缓冲组件：`CommandBuffer`。
- [x] 明确系统执行序：输入 -> 生产 -> 寻路 -> 移动 -> 战斗 -> 资源 -> 清理。

建议文件：
- `src/logic/ecs/components/Components.h`
- `src/logic/ecs/systems/SystemPipeline.h`
- `src/logic/ecs/World.h`
- `src/logic/ecs/World.cpp`
- `src/logic/ecs/systems/BuiltInSystems.h`
- `src/logic/ecs/systems/BuiltInSystems.cpp`
- `src/tests/EcsWorldTest.cpp`
- `src/tests/EcsCoreSystemsTest.cpp`

验收标准：
- [x] 空场景下 Tick 可稳定运行，无未定义行为（测试用例 `EcsWorldPipeline` 已实现）。

待本机执行验证命令：
- [ ] `ctest --test-dir build -C Debug -R "EcsWorldPipeline|EcsCoreSystems" --output-on-failure`

## 5. 最小玩法闭环（P0）
- [ ] 格点建造：基地附近可建区域 + 放置校验。
- [ ] 自由移动：单位右键移动，基础 A* 可达且不穿障碍。
- [ ] 资源：向日葵产阳光，生产单位消耗阳光。
- [ ] 战斗：基础攻击/受伤/死亡。
- [ ] 胜负：最小胜负判定（例如主基地摧毁）。

验收标准：
- [ ] 单机可完成“建造 -> 生产 -> 移动 -> 战斗 -> 胜负”一局流程。

## 6. 命令流与回放系统（P0）
- [ ] 统一输入为 `PlayerCommand`，带 `tick/playerId/type/payload`。
- [ ] 命令应用仅在逻辑 Tick 边界生效。
- [ ] 命令录制器：保存全局命令流到文件。
- [ ] 回放器：离线读取命令流并重放。
- [ ] 状态哈希：每 N Tick 输出世界摘要哈希。

建议文件：
- `src/logic/commands/PlayerCommand.h`
- `src/logic/commands/CommandQueue.h`
- `src/logic/replay/ReplayRecorder.h`
- `src/logic/replay/ReplayPlayer.h`
- `src/logic/debug/StateHasher.h`

验收标准：
- [ ] 同一回放文件重复运行 100 次，哈希全一致。

## 7. 数据驱动配置（P1）
- [ ] 引入 JSON 单位/建筑定义（成本、血量、射程、攻速、移动方式等）。
- [ ] 配置加载校验（字段缺失、范围错误时 fail-fast）。
- [ ] 单位工厂 `UnitFactory`：根据配置实例化 ECS 组件。

建议文件：
- `src/data/UnitConfig.h`
- `src/data/UnitConfigLoader.cpp`
- `src/logic/factory/UnitFactory.cpp`
- `assets/data/units/*.json`

验收标准：
- [ ] 新增一个单位配置无需改逻辑代码即可生成实体。

## 8. 测试策略与清单（P0/P1）
- [ ] 单元测试：`FixedPoint`、`DeterministicRng`、资源结算、伤害计算。
- [ ] 系统测试：命令应用顺序、建造合法性、路径一致性。
- [ ] 集成测试：同命令流双实例执行后哈希一致。
- [ ] 回归测试：每个 bug 修复附最小复现回放。

建议文件：
- `src/tests/math/*`
- `src/tests/systems/*`
- `src/tests/integration/*`

验收标准：
- [ ] CI 或本地 `ctest` 稳定通过，无随机失败。

## 9. 性能与可观测性（P1）
- [ ] 热路径减少每 Tick 动态分配。
- [ ] 增加 Tick 时长、实体数、路径请求数统计。
- [ ] 增加 determinism 调试开关（输出关键系统摘要）。

验收标准：
- [ ] 目标场景下逻辑 30 TPS 稳定，无持续超时帧。

## 10. 进入联机阶段前检查单（Gate）
- [ ] 已有回放工具，且可离线复盘问题。
- [ ] 哈希一致性达到目标（建议 100 次一致）。
- [ ] 关键玩法闭环完整且测试覆盖核心规则。
- [ ] `AGENTS.md` 中命令与流程已更新为真实可执行版本。

---

## 执行节奏建议（两周样例）
- Week 1：完成 1~4（脚手架、定点数、循环、ECS 骨架）。
- Week 2：完成 5~8（玩法闭环、回放哈希、测试体系）。
- 进入联机前：完成 9~10（性能与闸门检查）。
