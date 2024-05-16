import json
import socket
import wave
import numpy as np
class socketcommunication:
    def __init__(self, host='0.0.0.0', port=12348):
        self.host = host
        self.port = port
        self.received_arrays = []
        self.resend_port = None
        self.resend_addr = None
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print(f"Socket server initiallized" )
        
    def recieve(self):
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(1)
        print(f"Socket server listening on {self.host}:{self.port}")
        while True:
            client_socket, client_address = self.server_socket.accept()
            self.resend_addr , _ = client_address
            print(f"Connection from {client_address}")
            received_data = b'' 
            while True:
                chunk = client_socket.recv(4096)
                if not chunk:
                    break
                received_data += chunk
            
            try:
                received_json = json.loads(received_data.decode('utf-8'))
            except json.JSONDecodeError:
                print('JSON 解码错误')
                continue 
            received_array = received_json['data']
            self.received_arrays.append(received_array)
            received_state = received_json['state']
            received_encoding = received_json['encoding']
            self.resend_port = received_json['port']
            flag = received_json['flag']
            print('received state:', received_state)
            print('data encoding:', received_encoding)
            client_socket.close()
            if(received_state==0):
                print("finished")
                self.return_socket =client_socket
                break
            
        wav_file = wave.open('input.wav', 'w')
        wav_file.setnchannels(1)  # 单声道
        wav_file.setsampwidth(2)  # 16 位采样
        wav_file.setframerate(16000)  # 16 kHz 采样率
        combined_array = np.concatenate(self.received_arrays, axis=0,dtype=np.int16)
        print(len(combined_array ))
        wav_file.writeframes(combined_array.tobytes())
        wav_file.close()
        #self.send(client_socket.close(),)
        self.server_socket.close()
        return 'input.wav',flag 
    
    def send(self,text):
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((self.resend_addr, self.resend_port))
        client_socket.sendall(text.encode())
        client_socket.close()
    
    def close(self):
        self.server_socket.close()