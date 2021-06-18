+--------------------+
 ffeq 音频均衡器算法
+--------------------+


ffeq 是一个 c 语言实现的数字音频均衡器的算法库

将音频信号用 fft 或 stft 变换到频域，然后对频域数据进行处理，实现了音频均衡器的功能

采用 fft 进行变换因为存在频谱泄漏问题，实测在加了 EQ 后音频会存在杂音问题
使用 stft 变换后的 EQ 处理，解决了频谱泄漏问题，因此几乎听不到杂音


编译和测试
----------

目前该程序在 windows + msys2 + gcc 的环境下编译通过

执行：
./build.sh

即可编译生成 ffeq.exe 程序

./ffeq.exe --source=mic --samprate=8000 --eqfile=eq.txt

可测试效果

命令行参数：
--source=   用于指定音频输入源，mic 表示使用 mic 输入，也可以支持 pcm 音频文件
--samprate= 用于指定音频采样率
--eqfile=   用于指定 EQ 文件，文件格式参考 heavybass.txt


heavybass.txt 是一个重低音效果的 EQ 文件，44100 采样率下，对 0 - 600Hz 音频做了增益
使用这个测试可以感受到低音的效果


rockcarry
2021-6-18

