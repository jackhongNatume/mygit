import socket
import json
import time 

# 设置服务器的IP地址和端口号
SERVER_HOST = '162.105.183.51'  # 服务器的IP地址
SERVER_PORT = 12347


import wave
import numpy as np

# 读取 WAV 文件
with wave.open('part1.wav', 'rb') as wav_file:
    # 获取音频文件的参数
    num_channels = wav_file.getnchannels()  # 声道数
    sample_width = wav_file.getsampwidth()   # 采样宽度（字节）
    frame_rate = wav_file.getframerate()     # 采样率
    num_frames = wav_file.getnframes()       # 总帧数
    
    # 读取音频数据
    raw_audio = wav_file.readframes(num_frames)

# 将二进制音频数据转换成 numpy 数组
data = np.frombuffer(raw_audio, dtype=np.int16)
if(len(data)%1024==0):
        slices = len(data)//1024
else:
        slices = len(data)//1024+1

for  i in range (slices):
    if(1024*i+1024>len(data)):
        data1 = data.tolist()[1024*i:]
    else:
        data1 = data.tolist()[1024*i:1024*i+1024]
    if(i==slices-1):
        state=0
    else:
        state = 10
    encoding = 'utf-8'
# 将数据打包成JSON
    json_data = json.dumps({'data': data1, 'state': state, 'encoding': encoding})
# 创建一个TCP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# 连接到服务器
    client_socket.connect((SERVER_HOST, SERVER_PORT))
# 发送数据到服务器
    client_socket.sendall(json_data.encode())

# 关闭连接
    client_socket.close()
