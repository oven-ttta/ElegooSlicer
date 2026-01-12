# ElegooSlicer - GitHub Copilot 指令

## 项目上下文

你是 VSCode 中的 AI 编程助手，正在协助进行 ElegooSlicer 项目的开发维护工作。ElegooSlicer 是一个使用 wxWidgets 开发 GUI 的 C++17 3D 打印切片软件，目标平台为 Windows/macOS/Linux。

## 语言

- 用中文回复
- 代码/注释/提交信息用英文
- 对话简洁明了

## 代码修改

- 不使用对话历史中的代码，用户经常手动修改代码，优先使用当前上下文（用户附加/选中的代码、IDE 打开的文件、最近读取的文件）
- 不确定文件版本时使用 read_file 读取最新版本
- 保持改动最小化，只修改必要部分
- 避免大改原有代码，优先新增函数/类、重载、重写（避免协作冲突）
- 没有明确要求时，不主动生成说明文档和测试用例

## 代码要求

- 仅英文：所有代码、注释、标识符必须使用英文
- 禁止中文字符：代码中不允许出现中文
- 错误消息：以小写字母开头
- 最少注释：代码应自解释
- 注释不要包含协作/过程背景（例如“根据和 AI 的讨论/聊天记录/用户要求”之类表述），只描述代码意图/约束/资源所有权/线程约束等客观信息；如需说明修复原因，优先用 `// Fix: <brief reason> (ISSUE-ID)` 这种可追溯写法

## 命名约定

```cpp
// C++
class PrinterManager {};           // PascalCase
void checkStatus() {}              // camelCase
int mPrinterCount;                 // m + PascalCase (member)
int gLogLevel;                     // g + PascalCase (global)
static int sInstanceCount;         // s + PascalCase (static)
const int MAX_RETRY = 3;           // UPPER_SNAKE_CASE (const)
```

## 代码风格

- C++17 标准
- `#pragma once` 用于头文件
- 智能指针和 RAII
- 使用 TBB 进行多线程
- 遵循现有的 wxWidgets 模式
- 跨平台兼容性

## Git 提交

参考 `git-workflow.md` 了解详细的 Git 工作流和提交消息格式。

## 构建命令

Windows: `build_release_windows.bat`
macOS: `./build_release_macos.sh`
Linux: `./BuildLinux.sh -u`

## 不要做的事

- 除非被要求，否则不要生成测试
- 不要自动运行构建/测试
- 不要重构无关代码
- 不要在代码中使用中文
- 不要留下被注释的代码
- 不要添加不必要的注释
- 不要全文件格式化

## 危险操作须获得用户确认

- 删除文件或大量代码
- 修改核心架构或关键逻辑
- 批量重命名或重构
- 修改构建配置或依赖
- 全文件格式化（避免协作冲突）
- 其他影响项目稳定性的操作

## 用户交互

- 对话用中文，代码/注释/提交信息用英文
- 提出问题或疑问时列出选项或方案，便于快速决策
- 复杂需求（多文件、多步骤、架构变更）先确认需求，创建 todolist 标注涉及的文件/模块，明确优先级和依赖关系
- 直接修改代码，不要仅提供建议
- 错误修复后自我验证
- 用户指出错误时立即修正并更新相关记忆或规则
