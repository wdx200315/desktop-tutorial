# Qt Creator 项目设置说明

## 如何在Qt Creator中打开项目

### 方法1: 直接打开CMakeLists.txt（推荐）

1. 打开Qt Creator
2. 菜单: `文件` → `打开文件或项目...`
3. 浏览到 `SmartSched-HIS` 文件夹
4. 选择 `CMakeLists.txt` 文件
5. 点击"配置项目"

### 方法2: 使用Qt Creator向导

1. Qt Creator → `文件` → `新建文件或项目`
2. 选择 `Import Project` → `Clone Repository`（或直接打开现有项目）
3. 选择项目文件夹

### 方法3: 打开 CMakeLists_qtcreator.txt

如果主CMakeLists.txt有问题，可使用简化版本：
1. 将 `CMakeLists_qtcreator.txt` 重命名为 `CMakeLists.txt`
2. 然后用Qt Creator打开

---

## Kit配置

1. ** Kits选择**: 选择 `Desktop Qt 6.11.1 MinGW 64-bit`
2. ** CMake**: Qt Creator会自动检测
3. ** Build目录**: 建议使用 `build-qtcreator` 文件夹

---

## 首次配置步骤

1. 打开项目后，Qt Creator会提示配置Kits
2. 选择 `Desktop Qt 6.11.1 MinGW 64-bit`
3. 点击 `Configure Project`

---

## 常见问题

### 问题1: CMake配置失败

**解决**: 
- 确保Qt 6.11.1已正确安装
- 检查CMake版本（需要3.16+）
- 清理构建目录后重新配置

### 问题2: 找不到OpenSSL

**解决**: 
- 可忽略警告，项目会使用备用方案
- 或安装OpenSSL: https://slproweb.com/products/Win32OpenSSL.html

### 问题3: 找不到MySQL

**解决**:
- 可忽略，项目会禁用数据库功能
- 或安装MySQL并设置环境变量

---

## 构建

配置成功后：
- 点击左下角 `▶` 按钮构建项目
- 或使用快捷键 `Ctrl+B`

## 运行

构建成功后，可运行各个客户端：
- `SmartSchedPatient` - 患者挂号终端
- `SmartSchedDoctor` - 医生工作站
- `SmartSchedDisplay` - 排队看板
- `SmartSchedAdmin` - 管理后台
- `SmartSchedServer` - 服务端

---
