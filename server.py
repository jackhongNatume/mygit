import socket
import json
import wave
import numpy as np

# 设置服务器的IP地址和端口号
HOST = '192.168.59.37'
PORT = 12347

received_arrays =[]
# 创建一个TCP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 绑定IP地址和端口号
server_socket.bind((HOST, PORT))
Flag = 1 

# 开始监听传入的连接
server_socket.listen(1)
print('等待客户端连接...')
while True:

# 接受客户端的连接
    client_socket, client_address = server_socket.accept()
    print('连接来自:', client_address)

# 接收客户端发送的数据
    received_data = b'' 
    while True:
        chunk = client_socket.recv(4096)
        if not chunk:
            break
        received_data += chunk

# 解析接收到的JSON数据
    try:
        received_json = json.loads(received_data.decode('utf-8'))
    except json.JSONDecodeError:
        print('JSON 解码错误')
        continue

# 从JSON中提取数据
    received_array = received_json['data']
    received_arrays.append(received_array)
    received_state = received_json['state']
    received_encoding = received_json['encoding']

# 输出接收到的数据
    print('接收到的数组:', received_array)
    print('接收到的数字:', received_state)
    print('接收到的字符串编码:', received_encoding)

    if(received_state==0):
        print("finished")
        break
    
# 关闭连接
    client_socket.close()



# 创建一个新的 WAV 文件
wav_file = wave.open('output.wav', 'w')

# 设置 WAV 文件的参数
wav_file.setnchannels(1)  # 单声道
wav_file.setsampwidth(2)  # 16 位采样
wav_file.setframerate(16000)  # 44.1 kHz 采样率

# 将切片数组合并成一个完整的 NumPy 数组
combined_array = np.concatenate(received_arrays, axis=0,dtype=np.int16)
print(len(combined_array ))
# 将 NumPy 数组写入 WAV 文件
wav_file.writeframes(combined_array.tobytes())

# 关闭 WAV 文件
wav_file.close()

server_socket.close()
