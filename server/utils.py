def get_string_inputs(wav):
    with open('{}.txt'.format(wav), 'r') as f:
        text = f.read()
    text = text.replace('\n', '')
    text += '\n'
    return text

def write_wav_file(wav_file, audio_data, frame_rate, num_channels):
    pass
    # ...

def parse_json_data(json_data):
    pass
    # ...

def send_output_back_to_client(output, client_socket):
    
    # TO DO: implement this function to send the output back to the client
    pass