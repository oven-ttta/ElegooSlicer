# AI 辅助开发规则文件

本目录包含针对不同 AI IDE 的开发规则文件，用于指导 AI 更好地理解和协助 ElegooSlicer 项目开发。

## 目录结构

规则文件按语言和 IDE 类型组织：

```
doc/rules/
├── zh-CN/          # 中文版本
│   ├── cursor/     # Cursor AI 规则
│   ├── vscode/     # VSCode Copilot 规则
│   └── claude/     # Claude AI 规则
└── en/             # English version
    ├── cursor/     # Cursor AI rules
    ├── vscode/     # VSCode Copilot rules
    └── claude/     # Claude AI rules
```

## 使用方法

在项目根目录运行对应的脚本：

```bash
# 使用英文规则（默认）
python scripts\setup_rules.py cursor
python scripts\setup_rules.py vscode
python scripts\setup_rules.py claude

# 使用中文规则
python scripts\setup_rules.py cursor cn
python scripts\setup_rules.py vscode cn
python scripts\setup_rules.py claude cn
```

## 文件拷贝位置

### Cursor
所有规则文件拷贝到：`.cursor/rules/`

### VSCode (Copilot)
- `copilot-instructions.md` → `.github/copilot-instructions.md`（Copilot 主配置）
- 其他参考文档 → `.vscode/rules/`（开发参考）

### Claude
所有规则文件拷贝到：`claude-docs/`（项目文档）

