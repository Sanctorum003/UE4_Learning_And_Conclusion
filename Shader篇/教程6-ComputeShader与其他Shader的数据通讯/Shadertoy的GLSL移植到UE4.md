# 未解决问题
1.如何传入图片

# 坑
https://blog.csdn.net/u013384019/article/details/70313264
https://blog.csdn.net/u013384019/article/details/70313446

## 相关函数的转换
http://gad.qq.com/program/translateview/7168849

## float[N]
* 在glsl中经常使用vec[N](n),需要转换成如float3(n,n,n)而不能float(n)

## 矩阵 / 矩阵
* 在glsl中矩阵/矩阵是逐分量的，在hlsl中需要手动设置

## 矩阵乘法
* glsl 里是 A*B
* HLSL 里是 mul(A,B)
