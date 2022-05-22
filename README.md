http://www.ffmpeg.org/ 下载FFmpeg编译好的Windows版本
或
链接：https://pan.baidu.com/s/1F0hZh1s91P_QdzyS-uHGTQ 提取码：iv3v

1.项目名称->属性->配置属性->c/c++->附加包含的目录：
加入以下：增加 include
..\..\ffmpeg-x.x.x\include
 

2.项目名称->属性->配置属性->链接器->常规->附加库目录:
加入
..\..\ffmpeg-x.x.x\lib

3.项目名称->属性->配置属性->链接器->输入->附加依赖项:
加入
avcodec.lib
avdevice.lib
avfilter.lib
avformat.lib
avutil.lib
postproc.lib
swresample.lib
swscale.lib

将..\..\ffmpeg-x.x.x\bin 下的dll拷贝到解决方案目录下(..\bin)

如果debug是x86的话，改成x64，否则会调试不过。
