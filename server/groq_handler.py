from groq import Groq
import time 

class groqhandler():
    def __init__(self,api_key,model= "llama3-8b-8192",max_tokens = 1024, stop=1 ):
        self.api_key = api_key 
        self.model = model 
        self.max_tokens = max_tokens
    def inference(self , text):
        client = Groq(api_key=self.api_key,)

        chat_completion = client.chat.completions.create(
        messages=[
            {
                "role": "user",
                "content": text,
            }],
        model=self.model,
        max_tokens=self.max_tokens, 
        )
        #print(help(client.chat.completions.create))
        return chat_completion.choices[0].message.content

