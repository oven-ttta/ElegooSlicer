# VSCode/Cursor 开发环境配置 (Windows)

## 1. clangd 配置

### 为什么使用 clangd？

在 Windows 下使用 C/C++ IntelliSense 开发 ElegooSlicer 时，**定位变量、跳转定义等操作非常慢**，影响开发效率。

### 相关文件

- `clangd` - clangd 配置文件
- `settings.json` - 禁用 C/C++ IntelliSense，启用 clangd
- `extensions.json` - 推荐安装 clangd 扩展

### 使用方法

运行脚本自动配置：

```bash
generate_clangd_config.bat
```

脚本会自动：
1. 生成 `compile_commands.json`
2. 拷贝配置文件到项目根目录
3. 智能合并已存在的配置

然后重新加载窗口 (`Ctrl+Shift+P` > "Reload Window") 即可生效。

## 2. CMake 配置

### 相关文件

- `CMakePresets.json` - VSCode CMake 扩展的预设配置

### 使用方法

手动拷贝到项目根目录：

```bash
copy doc\vscode_settings\CMakePresets.json CMakePresets.json
```
