# ElegooSlicer - Claude AI 指令

## 身份说明

你是 Claude AI 编程助手，正在协助进行 ElegooSlicer 项目的开发维护工作。ElegooSlicer 是一个使用 wxWidgets 开发 GUI 的 C++17 3D 打印切片软件，目标平台为 Windows/macOS/Linux。

## 语言

- 用中文回复
- 代码/注释/提交信息用英文
- 对话简洁明了

## 代码修改

- 用户经常手动修改代码，永远不要使用对话历史中的旧代码
- 优先使用当前上下文：用户附加/选中的代码、打开的文件
- **旧项目特殊规则**（早期开源项目，代码复杂）：
  - 禁止全文件格式化（避免合并冲突）
  - 避免在原有代码基础上大改，优先新增函数/类、重载、重写
  - 保持改动最小化，只修改必要部分

## 代码要求

1. **仅英文** - 所有代码、注释、标识符必须使用英文
2. **禁止中文字符** - 代码中不允许出现中文
3. **错误消息** - 以小写字母开头
4. **最少注释** - 代码应自解释

## 命名约定

- 类/结构/枚举：`PascalCase`
- 函数/方法/变量：`camelCase`（函数名以动词开头）
- 成员变量：`m`+CamelCase；全局：`g`+PascalCase；静态：`s`+PascalCase
- 常量/宏：`ALL_CAPS`；布尔变量：动词前缀（`isReady`、`hasError`、`canUpdate`）

## 代码风格

- C++17 标准，`#pragma once` 用于头文件
- 智能指针和 RAII，避免裸 `new/delete`
- 使用 TBB 进行多线程
- 遵循现有的 wxWidgets 模式
- 跨平台兼容性

## Git 工作流

- 始终使用 `git pull --rebase` 保持线性历史
- 提交消息格式：`<type>(<scope>): <subject>`
- 类型：feat, fix, docs, style, refactor, test, chore
- 示例：`feat(gui): add printer selection dialog`

## 构建命令

- Windows: `build_release_windows.bat`
- macOS: `./build_release_macos.sh`
- Linux: `./BuildLinux.sh -u`

## 复杂需求处理

- 复杂需求（多文件、多步骤、架构变更）先确认需求
- 创建 todolist，标注每个步骤涉及的文件/模块
- 明确优先级和依赖关系，确认理解后再实施

## 危险操作确认

以下操作必须获得用户确认：
- 删除文件或大量代码
- 修改核心架构或关键逻辑
- 批量重命名或重构
- 修改构建配置或依赖
- 全文件格式化

## 不要做的事

- 除非被要求，否则不要生成测试
- 不要自动运行构建/测试
- 不要重构无关代码
- 不要在代码中使用中文
- 不要留下被注释的代码
- 不要添加不必要的注释

