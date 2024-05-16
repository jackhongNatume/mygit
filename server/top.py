from groq_handler import groqhandler
from groq import Groq
from faster_whisper import WhisperModel

from socket_handler import socketcommunication
import json
import socket
import wave
import numpy as np

HOST = '0.0.0.0'
PORT = 12345
model_size = "tiny"
model = WhisperModel(model_size, device="cpu", compute_type="int8")
groq_api = "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"

while(True):
    testing = socketcommunication(host=HOST, port=PORT)
    groq= groqhandler(api_key=groq_api)
    wav = testing.recieve()
    segments, info = model.transcribe(wav, beam_size=10)
    print("Detected language '%s' with probability %f" % (info.language, info.language_probability))
    inputs = " "
    for segment in segments:
         inputs += segment.text
    sending = "{}".format(groq.inference(inputs))
    testing.send(sending)
    break
