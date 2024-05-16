from groq_handler import groqhandler
from groq import Groq
from faster_whisper import WhisperModel
from whisper_handler import WhisperHandler
from socket_handler import socketcommunication
import json
import socket
import wave
import numpy as np
from utils import * 

HOST = '0.0.0.0'
PORT = 12345
model_size = "tiny"
model = WhisperModel(model_size, device="cpu", compute_type="int8")

groq_api = "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"
model_path = "/Users/mr.meeseeks/Peking_univ/coding/python/elder_talk/whisper.cpp/models/ggml-base.bin"


t =0 
while(True):
    t += 1 
    #initialization 
    groq= groqhandler(api_key=groq_api)
    testing = socketcommunication(host=HOST, port=PORT)
    whisper=WhisperHandler(model_path)


    wav,flag = testing.recieve()
    if(flag == 1):
        text = get_string_inputs(wav)
        print(text)
        sending = "{}".format(groq.inference(text))
        testing.send(sending)
        testing.close()
    else:
        sending = "communication turned down"
        testing.send(sending)
        break
