# Cubemap
这是一个用glfw重写SB5第7章的cubemap

主要修改
1、GLTool glew部分。
  GLTool里有一份完整的glew.c文件，这个文件已经不适用mac mojave版本，会有一些gl函数无法正确hook。所以要把glew.c文件删除。
2、GTool头文件
  头文件里的一些自定义gl函数的宏要删掉，使用最新的glew库的宏。
