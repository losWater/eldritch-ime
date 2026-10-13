# 复现与核验

## 精确核验保存的第一版

需要 Python 3，所有 Python 脚本仅使用标准库。在仓库根目录运行：

```bash
python3 tools/evaluate.py --check
```

核验从冻结的根键布局和数据重新生成全部15844个全码，检查：

- 覆盖15496字，所有全码唯一。
- 字族容量不超限，第三、第四码异手。
- 固定一二简保留，原组成员按本版归并同键，虫与鸟分键。
- 单字表内容与发布文件完全一致。
- 两段实际当量、形码平均、四码字频、常用字词碰撞和全词码碰撞与冻结指标一致。
- 二维字频 × 词频避重表与保存结果一致。

重新生成第一版单字表与二维数据：

```bash
python3 tools/evaluate.py --export
python3 tools/evaluate.py --check
```

## 重新运行落根搜索

需要 C++17 编译器 g++ 或 clang++，以及 Python 3：

```bash
python3 tools/search.py
python3 tools/evaluate.py --layout build/searched-layout.json
```

默认参数：seed 617、900000次迭代、第二→第三码权重1、四码项惩罚0.003、换键惩罚0.001。搜索输入和初始布局为本次试验的冻结文件。

更换编译器示例：

```bash
python3 tools/search.py --compiler clang++
```

搜索编译和输入解压在临时目录完成，结果默认写入被git忽略的build目录。不同编译器和标准库的随机分布实现可能产生不同候选，因此不承诺重新搜索必然逐字复现第一版；精确复核应使用保存的data/layout.json。
