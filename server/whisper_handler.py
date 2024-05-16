import subprocess
class WhisperHandler:
    def __init__(self, model_path):
        self.model_path = model_path
    def run_whisper(self, wav_file):
        try:
            args = ['../whisper.cpp/main', '-m', self.model_path, wav_file, '-otxt', '1'  ]
            process = subprocess.run(args)
            return process
        except Exception as e:
            print(f"Error: {e}")
            return None
            