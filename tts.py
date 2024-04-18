import asyncio  
import random  
  
import edge_tts  
from edge_tts import VoicesManager  
  
TEXT = "中文语音测试"  
OUTPUT_FILE ="china.mp3"  
  
  
async def _main() -> None:  
    voices = await VoicesManager.create()  
    voice = voices.find(Gender="Female", Language="zh")  
  
    communicate = edge_tts.Communicate(TEXT, random.choice(voice)["Name"])  
    await communicate.save(OUTPUT_FILE)  
  
  
if __name__ == "__main__":  
    asyncio.run(_main())
